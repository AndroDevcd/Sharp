/*
 * AnalysisService - the owner of everything that changes while the IDE is open.
 *
 * WHAT THIS IS FOR
 *   One per project. It ties the pieces together (doc/ide.md sections 3 and 6.5):
 *
 *     a save --> FileChangeListener --> requestRun --> RunScheduler (debounce, one at a time)
 *       --> a background task --> runOnce:
 *            list the program's files (ProgramFiles)
 *            snapshot the open files (Snapshots)
 *            make a fresh run folder, start the compiler (CompilerRunner)
 *            read its answer (DiagnosticsReader), keep it as `latest`
 *            ask the editor to highlight again (DaemonCodeAnalyzer.restart)
 *
 *   `latest` is the newest run's diagnostics. It is an immutable object replaced by one
 *   assignment, so the annotator can read it on any thread without a lock.
 *
 *   When the project opens, `loadLastRunFromDisk` reads the last session's run folders back,
 *   so errors and completion do not have to wait for the first run.
 *
 *   `runOnce` is a plain synchronous function (the background task only calls it), so a
 *   test can drive the whole pipeline with a fake ProcessLauncher.
 */
package sharp.ide.run

import com.intellij.codeInsight.daemon.DaemonCodeAnalyzer
import com.intellij.openapi.Disposable
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.application.PathManager
import com.intellij.openapi.application.ReadAction
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.service
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.openapi.progress.ProgressIndicator
import com.intellij.openapi.progress.ProgressManager
import com.intellij.openapi.progress.Task
import com.intellij.openapi.project.Project
import com.intellij.util.concurrency.AppExecutorUtil
import sharp.ide.index.DiagnosticsReader
import sharp.ide.index.DiagnosticsRun
import sharp.ide.index.IndexLoader
import sharp.ide.index.RunIndex
import sharp.ide.index.RunReadException
import sharp.ide.paths.PathTranslator
import sharp.ide.toolchain.SharpToolchains
import sharp.ide.paths.ProgramFiles
import sharp.ide.paths.ProgramLayout
import sharp.ide.paths.ProgramSettings
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpProjectSettings
import sharp.ide.settings.TranslateMode
import java.nio.file.Path
import java.util.concurrent.atomic.AtomicBoolean

/** Why a run was asked for. */
enum class RunReason { SAVE, MANUAL, SETTINGS, STARTUP, EDITOR_OPENED }

/** How a call of `runOnce` ended; tests look at it, the task ignores it. */
sealed class RunReport {
    class Done(val run: DiagnosticsRun) : RunReport()
    class Skipped(val why: String) : RunReport()
    class Failed(val failure: RunFailure) : RunReport()
    object Cancelled : RunReport()
}

@Service(Service.Level.PROJECT)
class AnalysisService(private val project: Project) : Disposable {

    /** The newest run's diagnostics; null before the first successful run. */
    @Volatile
    var latest: DiagnosticsRun? = null
        private set

    /**
     * The index of the newest run that parsed (level `declarations` or `full`), the WHOLE of that one
     * run (doc/ide.md 3.5). A run that stopped at syntax errors leaves it as it was: completion keeps
     * using the last good index. Null before the first good run.
     */
    @Volatile
    var index: RunIndex? = null
        private set

    val snapshots = Snapshots()
    val failures = FailureTracker()

    /** The process starter; a test replaces it with a fake. */
    @Volatile
    var launcher: ProcessLauncher = PlatformProcessLauncher()

    /** Where failures go; a test replaces it. */
    @Volatile
    var failureSink: (Project, RunFailure) -> Unit = FailureNotifier::report

    /** Where the short note after a manual run goes; a test replaces it. */
    @Volatile
    var infoSink: (Project, String) -> Unit = FailureNotifier::info

    /** Replaces the folder runs are made in (the IDE's system folder by default); for tests. */
    @Volatile
    var runBaseOverride: Path? = null

    /** The run folder the current index was read from; pruning never deletes it (see `pruneKeepingNewest`). */
    @Volatile
    private var indexFolder: Path? = null

    private val manualRequested = AtomicBoolean(false)
    private val scheduler = RunScheduler(
        AppExecutorUtil.getAppScheduledExecutorService(),
        { SharpApplicationSettings.getInstance().state.debounceMillis.toLong().coerceAtLeast(0) },
        ::startInBackground,
    )

    @Volatile
    private var cachedLayoutFor: ProgramSettings? = null
    @Volatile
    private var cachedLayout: ProgramLayout? = null

    // ---------------------------------------------------------------- asking for a run

    /** A run is wanted. Returns at once; the run happens later, in the background. */
    /** Has the compiler answered for this project at least once (so: the project holds a Sharp program)? */
    fun hasSharpProgram(): Boolean = latest != null

    fun requestRun(reason: RunReason) {
        if (reason == RunReason.MANUAL) manualRequested.set(true)
        scheduler.request(immediate = reason == RunReason.MANUAL)
    }

    /** Is this path a file of the program, and is checking on save switched on? A string test only: safe on the UI thread. */
    fun wantsRunFor(editorPath: String): Boolean {
        if (!project.service<SharpProjectSettings>().state.enabled) return false
        return ProgramFiles.contains(layout(), editorPath)
    }

    private fun layout(): ProgramLayout {
        val settings = project.service<SharpProjectSettings>().programSettings()
        val known = cachedLayout
        if (known != null && cachedLayoutFor == settings) return known
        val fresh = ProgramFiles.layoutOf(settings)
        cachedLayout = fresh
        cachedLayoutFor = settings
        return fresh
    }


    // ---------------------------------------------------------------- the background task

    /**
     * The scheduler's `start`: `finished` MUST be called once for every call, or the scheduler waits
     * for this run for ever and no later save starts one.
     *
     * So the hop to the UI thread is NOT given "drop this when the project is gone" (the platform's
     * `expired` condition): a hop that is dropped runs nothing, also not `finished`. The project is
     * looked at inside instead, and a run that cannot be made says it is over. (A project counts as
     * gone for a moment between two tests of the plugin, too - that is how this was found: a run
     * asked for by one test, due between two tests, left every later test without runs.)
     */
    private fun startInBackground(finished: () -> Unit) {
        ApplicationManager.getApplication().invokeLater {
            if (project.isDisposed) {
                finished()
                return@invokeLater
            }
            val task = object : Task.Backgroundable(project, "Sharp: analysing", true) {
                override fun run(indicator: ProgressIndicator) {
                    try {
                        runOnce(indicator)
                    } finally {
                        finished()
                    }
                }
            }
            try {
                ProgressManager.getInstance().run(task)
            } catch (e: Throwable) {
                finished()                                    // (called twice does no harm: the scheduler takes the first)
                throw e
            }
        }
    }

    /**
     * `-testdir <folder>` when the program is a project with a tests folder: its tests are checked with
     * the program (SharpProjectSettings.testsFolderOfProgram). Left out for an installed Sharp that
     * does not know the option yet.
     */
    private fun testsFolderArguments(installedVersion: String?, translator: PathTranslator?): List<String> {
        if (installedVersion != null && SharpToolchains.compareVersions(installedVersion, SharpToolchains.FIRST_WITH_PROJECT_TESTS) < 0) return emptyList()
        val folder = project.service<SharpProjectSettings>().testsFolderOfProgram() ?: return emptyList()
        return listOf("-testdir", translator?.toWsl(folder) ?: folder)
    }

    // ---------------------------------------------------------------- one run, start to end

    /** Runs the compiler once and publishes what it said. Synchronous; call it on a background thread. */
    fun runOnce(indicator: ProgressIndicator): RunReport {
        val manual = manualRequested.getAndSet(false)
        val app = SharpApplicationSettings.getInstance().state
        val projectState = project.service<SharpProjectSettings>().state
        if (!manual && !projectState.enabled) return RunReport.Skipped("checking on save is switched off")

        val programSettings = project.service<SharpProjectSettings>().programSettings()
        val layout = ProgramFiles.layoutOf(programSettings)
        cachedLayout = layout
        cachedLayoutFor = programSettings
        if (layout.libraryRoot == null) {
            return fail(RunFailure("the Sharp standard library was not found: set its folder in Settings > Languages & Frameworks > Sharp", "", ""), manual)
        }
        val files = ProgramFiles.list(programSettings)
        if (files.none { !it.startsWith(layout.libraryRoot + "/") }) {
            return RunReport.Skipped("the project has no Sharp source files")
        }

        // the installed Sharp, started directly, when there is one; else the launcher and script of the settings
        val toolchain = SharpApplicationSettings.getInstance().toolchain()
        val translator = translatorFor(app, project.basePath)
        val folder = RunFolders.create(runBase())
        val taken = takeSnapshots(files.toHashSet())
        val request = RunRequest(
            folder = folder,
            files = files,
            // Which files get their function bodies analysed FOR THE EDITOR (hover and go-to inside
            // them). Every file by default: then a library file reached by Ctrl+click works at
            // once, and closing and reopening a file costs nothing. "Open files only" makes each
            // run about a quarter shorter, at the price of one more run whenever a file is opened.
            // (The name `openFiles` is the older, narrower meaning: the paths of `bodies_for.txt`.)
            openFiles = if (projectState.bodiesForEveryFile) files else taken.keys.sorted(),
            launcher = app.launcher,
            script = app.script,
            vm = toolchain?.vm ?: app.vm,
            compiler = toolchain?.compiler ?: app.compiler,
            extraArguments = CommandTemplate.splitExtraArguments(projectState.extraArguments) + testsFolderArguments(toolchain?.version, translator),
            timeoutSeconds = app.timeoutSeconds,
            translator = translator,
            direct = toolchain != null,
        )

        return when (val outcome = CompilerRunner(launcher).run(request, indicator)) {
            is RunOutcome.Cancelled -> {
                pruneKeepingNewest(folder)
                RunReport.Cancelled
            }
            is RunOutcome.Failed -> {
                pruneKeepingNewest(folder)
                fail(outcome.failure, manual)
            }
            is RunOutcome.Finished -> {
                val run = try {
                    DiagnosticsReader.readFolder(folder)
                } catch (e: RunReadException) {
                    pruneKeepingNewest(folder)
                    return fail(RunFailure(e.message ?: "the compiler's answer could not be read", outcome.command, outcome.outputTail), manual)
                }
                val compilerKeyed = taken.mapKeys { translator?.toWsl(it.key) ?: it.key }
                publish(run, compilerKeyed)
                loadIndex(run, folder)
                pruneKeepingNewest(folder)
                if (manual) infoSink(project, describe(run))
                RunReport.Done(run)
            }
        }
    }

    /**
     * Makes `run` the current diagnostics and asks the editor to highlight again. The snapshots are
     * stored BEFORE `latest` changes, so a reader that sees the new run always finds its snapshots.
     */
    fun publish(run: DiagnosticsRun, snapshotsByCompilerPath: Map<String, Snapshot>) {
        snapshots.keep(run.run, snapshotsByCompilerPath, run.manifest.hashByPath)
        latest = run
        failures.clear()
        restartHighlighting()
    }

    /**
     * Makes the index of this run the current one, when the run parsed. A run that stopped at syntax
     * errors keeps the old index; an index that cannot be read is dropped quietly (the diagnostics of
     * the run are shown all the same, and completion keeps the older index).
     */
    private fun loadIndex(run: DiagnosticsRun, folder: Path) {
        if (run.manifest.level == "syntax-errors") return
        val loaded = try {
            IndexLoader.load(folder, run.manifest)
        } catch (_: RunReadException) {
            return
        }
        publishIndex(loaded)
        indexFolder = folder
    }

    /**
     * Reads the last session's answer back from the run folders on disk (doc/ide.md 3.1), so that after
     * the IDE is restarted the errors and completion are there at once, not only after the first run:
     *
     *   - the diagnostics of the NEWEST folder that can be read, whatever its level;
     *   - the index of the newest folder that parsed: the same folder, or an older one when the
     *     newest run stopped at syntax errors.
     *
     * Only what is still empty is filled, so the result of a run that already finished is never
     * replaced by older files. The text of the last session is not kept, so there are no snapshots:
     * a file gets its marks and its body information only when the text on disk still has the hash
     * the run recorded (the annotator and FileViews "adopt" it then; doc/ide.md 3.6). A folder that
     * cannot be read (a run that was killed, files of another format) is skipped quietly.
     *
     * Reads files: call it on a background thread. StartupRun does, once, when the project opens.
     */
    fun loadLastRunFromDisk() {
        val folders = try {
            RunFolders.numberedFolders(runBase()).asReversed()
        } catch (_: java.io.IOException) {
            return                                             // cannot be listed: the first run fills everything
        }
        for ((_, folder) in folders) {
            if (latest != null && index != null) return
            val run = try {
                DiagnosticsReader.readFolder(folder)
            } catch (_: RunReadException) {
                continue
            }
            if (latest == null) {
                publish(run, emptyMap())
            } else if (index == null) {
                // An older run, read for its index only: give its (still empty) snapshots a place, so a file can be adopted.
                snapshots.keep(run.run, emptyMap(), run.manifest.hashByPath)
            }
            if (index == null) loadIndex(run, folder)
        }
    }

    /** Makes `loaded` the current index. Its snapshots must be stored already (`publish` does that). */
    fun publishIndex(loaded: RunIndex) {
        snapshots.pin(loaded.run)
        index = loaded
    }

    /** Forgets the index and the snapshots (a test of "no index yet"; the program's files changed completely). */
    fun clearIndex() {
        index = null
        indexFolder = null
        snapshots.pin(null)
        snapshots.clear()
    }

    /** Forgets the diagnostics as well: the state of a project that has just been opened (for tests). */
    fun forgetAllRuns() {
        latest = null
        clearIndex()
    }

    /**
     * Asks the platform to run its highlighting (and so our annotator) again for every open file.
     * `restart()` exists in every supported version. IntelliJ IDEA 2025.3 marks it (and the overload that
     * takes a file) deprecated, because it prefers a variant that takes a "reason"; that variant does not
     * exist in 2025.2, our minimum. It is a plain deprecation, not "for removal" (doc/ide.md 8.5).
     */
    fun restartHighlighting() {
        if (project.isDisposed) return
        DaemonCodeAnalyzer.getInstance(project).restart()
    }

    private fun fail(failure: RunFailure, manual: Boolean): RunReport {
        if (manual || failures.shouldReport(failure.reason)) failureSink(project, failure)
        return RunReport.Failed(failure)
    }

    private fun describe(run: DiagnosticsRun): String {
        val m = run.manifest
        return "${m.errors} error(s), ${m.warnings} warning(s)" + if (m.level == "syntax-errors") " (the program does not parse yet)" else ""
    }

    // ---------------------------------------------------------------- helpers

    /** The folder all of this project's runs are made in: `<IDE system folder>/sharp-ide/<project key>`. */
    fun runBase(): Path {
        runBaseOverride?.let { return it }
        val name = project.name.replace(Regex("[^A-Za-z0-9._-]"), "_")
        val key = name + "-" + Integer.toHexString((project.basePath ?: "").hashCode())
        return PathManager.getSystemDir().resolve("sharp-ide").resolve(key)
    }

    /**
     * Deletes the old run folders, keeping the two newest: the one of this run (what was run, what was written,
     * `failed.txt`: the place to look when something went wrong) and the one before it - and the folder the
     * current index was read from, however old: while the program does not parse, that folder is what
     * `loadLastRunFromDisk` gives completion back from after the IDE is restarted. Called after EVERY run,
     * also one that failed or was cancelled: a compiler that fails on each save must not fill the disk with
     * a folder per save (at most three folders are left at any time).
     */
    private fun pruneKeepingNewest(folder: Path) {
        val base = folder.parent
        try {
            val keep = RunFolders.numberedFolders(base).takeLast(2).map { it.second }.toMutableSet()
            indexFolder?.let { keep.add(it) }
            RunFolders.prune(base, keep)
        } catch (_: java.io.IOException) {
            // the folder cannot be listed right now; the next run's prune tries again
        }
    }

    /**
     * The saved text of every Sharp file of the program that is open in an editor, by EDITOR path. Reads
     * what is on disk through the virtual file system (not the unsaved editor text), because that is
     * what the compiler reads.
     */
    private fun takeSnapshots(programFiles: Set<String>): Map<String, Snapshot> {
        val taken = LinkedHashMap<String, Snapshot>()
        ReadAction.run<RuntimeException> {
            for (file in FileEditorManager.getInstance(project).openFiles) {
                if (!file.isValid || file.extension != "sharp" || file.path !in programFiles) continue
                val bytes = try { file.contentsToByteArray() } catch (_: java.io.IOException) { continue }
                taken[file.path] = Snapshot(SavedText.decode(bytes), SavedText.hash(bytes))
            }
        }
        return taken
    }

    override fun dispose() {
        scheduler.dispose()
    }

    companion object {
        fun getInstance(project: Project): AnalysisService = project.service()

        /** The translator for this setup, or null when paths go through untranslated. */
        fun translatorFor(app: SharpApplicationSettings.State, projectBase: String?): PathTranslator? {
            // the installed Sharp runs where the IDE runs: its paths are the IDE's paths - on Windows with
            // backslashes where the editor has forward slashes, which is all there is to translate
            if (SharpApplicationSettings.toolchainOf(app) != null) {
                return if (com.intellij.openapi.util.SystemInfo.isWindows) PathTranslator.forNativeWindows() else null
            }
            val mode = TranslateMode.entries.firstOrNull { it.name == app.translateMode } ?: TranslateMode.AUTOMATIC
            val wanted = when (mode) {
                TranslateMode.ALWAYS -> true
                TranslateMode.NEVER -> false
                TranslateMode.AUTOMATIC -> CommandTemplate.startsWsl(app.launcher)
            }
            if (!wanted) return null
            var translator = PathTranslator(app.mountRoot, app.wslDistro.trim())
            if (projectBase != null) translator = translator.withDistroFrom(projectBase)
            return translator
        }
    }
}
