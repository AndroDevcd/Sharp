package sharp.ide.run

import com.intellij.openapi.progress.EmptyProgressIndicator
import com.intellij.openapi.progress.ProgressIndicator
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.openapi.vfs.VfsUtil
import com.intellij.openapi.application.runWriteAction
import com.intellij.testFramework.PlatformTestUtil
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.index.CompilerOutputFixture
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpDefaults
import sharp.ide.settings.SharpProjectSettings
import sharp.ide.settings.TranslateMode
import java.nio.file.Files
import java.nio.file.Path
import java.util.concurrent.atomic.AtomicInteger

/**
 * THE WHOLE PIPELINE WITH A FAKE COMPILER
 *
 * A real (temporary) project folder with a standard library and a source file, the real
 * AnalysisService, ProgramFiles, CompilerRunner, DiagnosticsReader and annotator; only the process
 * launcher is replaced by one that, instead of starting a compiler, writes REAL compiler output
 * (the fixtures) into the run folder the plugin gave it. The fake reads the same files.txt the real
 * compiler would, so the test also proves what the plugin hands over.
 */
class AnalysisServiceTest : BasePlatformTestCase() {
    private lateinit var root: Path
    private lateinit var sourcePath: String
    private lateinit var fixture: CompilerOutputFixture
    private lateinit var service: AnalysisService
    private lateinit var savedApp: SharpApplicationSettings.State
    private lateinit var savedProject: SharpProjectSettings.State
    private val failures = ArrayList<RunFailure>()
    private val infos = ArrayList<String>()

    /** What the fake compiler saw. */
    private val launches = AtomicInteger()
    private var listedFiles: List<String> = emptyList()
    private var bodiesFor: List<String> = emptyList()

    /** What the fake does when launched; replaced by a test. Default: act like a compiler that found the fixture's errors. */
    private var behaviour: (command: List<String>, folder: Path) -> LaunchResult = { _, folder ->
        fixture.writeRunFolder(folder, sourcePath)
        LaunchResult.Finished(0, "2 error(s)")
    }

    override fun setUp() {
        super.setUp()
        root = Files.createTempDirectory("sharp-pipeline").toRealPath()
        fixture = CompilerOutputFixture("analysis_errors")
        Files.createDirectories(root.resolve("lib/std"))
        Files.write(root.resolve("lib/std/core.sharp"), "mod std;\n".toByteArray())
        Files.createDirectories(root.resolve("app"))
        Files.write(root.resolve("app/main.sharp"), fixture.sourceBytes)
        sourcePath = ProgramFiles.normalise(root.resolve("app/main.sharp").toString())

        val app = SharpApplicationSettings.getInstance().state
        val proj = project.getService(SharpProjectSettings::class.java).state
        savedApp = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(app, it) }
        savedProject = SharpProjectSettings.State().also { XmlSerializerUtil.copyBean(proj, it) }
        app.launcher = SharpDefaults.LAUNCHER_NATIVE
        app.translateMode = TranslateMode.NEVER.name
        app.debounceMillis = 20
        proj.enabled = true
        proj.sourceRoots = mutableListOf(ProgramFiles.normalise(root.resolve("app").toString()))
        proj.libraryPath = ProgramFiles.normalise(root.resolve("lib").toString())

        service = AnalysisService.getInstance(project)
        service.runBaseOverride = root.resolve("runs")
        service.failureSink = { _: Project, failure -> failures.add(failure) }
        service.infoSink = { _: Project, text -> infos.add(text) }
        service.launcher = object : ProcessLauncher {
            override fun launch(command: List<String>, timeoutMillis: Long, indicator: ProgressIndicator): LaunchResult {
                launches.incrementAndGet()
                val folder = Path.of(command.last()).parent                       // run.sh sits in the run folder
                listedFiles = Files.readAllLines(folder.resolve("files.txt"))
                bodiesFor = Files.readAllLines(folder.resolve("bodies_for.txt"))
                return behaviour(command, folder)
            }
        }
    }

    override fun tearDown() {
        try {
            XmlSerializerUtil.copyBean(savedApp, SharpApplicationSettings.getInstance().state)
            XmlSerializerUtil.copyBean(savedProject, project.getService(SharpProjectSettings::class.java).state)
            root.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    fun testARunHandsOverTheLibraryFirstThenTheSourcesAndPublishesTheErrors() {
        val report = service.runOnce(EmptyProgressIndicator())
        assertTrue(report.toString(), report is RunReport.Done)
        assertEquals(
            listOf(ProgramFiles.normalise(root.resolve("lib/std/core.sharp").toString()), sourcePath),
            listedFiles,
        )
        val latest = service.latest!!
        assertEquals(3, latest.forFile(sourcePath).size)
        assertTrue(failures.isEmpty())
    }

    fun testByDefaultEveryFileOfTheProgramIsListedForBodies() {
        // nothing is open: a library file reached later by Ctrl+click must work without another run
        service.runOnce(EmptyProgressIndicator())
        assertEquals(listedFiles, bodiesFor)
        assertTrue(bodiesFor.any { it.contains("/lib/") })
        assertTrue(bodiesFor.contains(sourcePath))
    }

    fun testTheOpenFilesGetASnapshotAndWithTheSwitchOffOnlyTheyAreListedForBodies() {
        project.getService(SharpProjectSettings::class.java).state.bodiesForEveryFile = false
        val vf = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve("app/main.sharp"))!!
        myFixture.openFileInEditor(vf)
        service.runOnce(EmptyProgressIndicator())
        assertEquals(listOf(sourcePath), bodiesFor)
        val snapshot = service.snapshots.get(service.latest!!.run, sourcePath)
        assertNotNull(snapshot)
        assertEquals(SavedText.decode(fixture.sourceBytes), snapshot!!.text)
    }

    fun testASnapshotOfAFileSavedAgainDuringTheRunIsDropped() {
        val vf = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve("app/main.sharp"))!!
        myFixture.openFileInEditor(vf)
        behaviour = { _, folder ->
            // the compiler read other bytes than the ones the snapshot has: its hash in the manifest differs
            val changed = fixture.manifestText.replace(fixture.originalPath, sourcePath)
                .replace(Regex("(\"path\":\"" + Regex.escape(sourcePath) + "\"[^}]*?\"hash\":\")[0-9a-f]+"), "$1deadbeef")
            Files.write(folder.resolve("ide.json"), changed.toByteArray())
            Files.write(folder.resolve("diagnostics.json"), fixture.diagnosticsText.replace(fixture.originalPath, sourcePath).toByteArray())
            LaunchResult.Finished(0, "")
        }
        service.runOnce(EmptyProgressIndicator())
        assertNull(service.snapshots.get(service.latest!!.run, sourcePath))
    }

    fun testACompilerThatCannotBeStartedIsReportedOnceAndTheOldErrorsStay() {
        service.runOnce(EmptyProgressIndicator())
        val before = service.latest
        behaviour = { _, _ -> LaunchResult.CouldNotStart("wsl.exe is not installed") }
        repeat(3) { service.runOnce(EmptyProgressIndicator()) }
        assertEquals(1, failures.size)
        assertTrue(failures[0].reason, failures[0].reason.contains("wsl.exe is not installed"))
        assertSame(before, service.latest)
    }

    fun testASuccessfulRunMakesTheNextFailureNewsAgain() {
        behaviour = { _, _ -> LaunchResult.TimedOut("") }
        service.runOnce(EmptyProgressIndicator()); service.runOnce(EmptyProgressIndicator())
        assertEquals(1, failures.size)
        behaviour = { _, folder -> fixture.writeRunFolder(folder, sourcePath); LaunchResult.Finished(0, "") }
        service.runOnce(EmptyProgressIndicator())
        behaviour = { _, _ -> LaunchResult.TimedOut("") }
        service.runOnce(EmptyProgressIndicator())
        assertEquals(2, failures.size)
    }

    fun testARunThatWroteNoAnswerIsAFailureWithTheCommandAndTheOutput() {
        behaviour = { _, _ -> LaunchResult.Finished(1, "invalid option -ide") }
        val report = service.runOnce(EmptyProgressIndicator())
        assertTrue(report is RunReport.Failed)
        assertEquals(1, failures.size)
        assertTrue(failures[0].reason, failures[0].reason.contains("no ide.json"))
        assertEquals("invalid option -ide", failures[0].outputTail)
        assertTrue(failures[0].command.startsWith("bash "))
    }

    fun testNoLibraryAtAllIsReportedAndNothingIsRun() {
        // an empty library path means lib/support/0.4.0 below the project folder, which the test project has not
        project.getService(SharpProjectSettings::class.java).state.libraryPath = ""
        val report = service.runOnce(EmptyProgressIndicator())
        assertTrue(report.toString(), report is RunReport.Failed)
        assertEquals(0, launches.get())
        assertEquals(1, failures.size)
        assertTrue(failures[0].reason, failures[0].reason.contains("standard library"))
    }

    fun testSwitchedOffMeansNoRunExceptWhenAskedForByHand() {
        project.getService(SharpProjectSettings::class.java).state.enabled = false
        assertTrue(service.runOnce(EmptyProgressIndicator()) is RunReport.Skipped)
        assertEquals(0, launches.get())
        assertFalse(service.wantsRunFor(sourcePath))
        service.requestRun(RunReason.MANUAL)                                     // sets the manual flag...
        assertTrue(service.runOnce(EmptyProgressIndicator()) is RunReport.Done)   // ...which the next run honours
        assertEquals(1, infos.size)
        assertTrue(infos[0], infos[0].contains("2 error(s), 1 warning(s)"))
    }

    fun testOnlyFilesOfTheProgramWantARun() {
        assertTrue(service.wantsRunFor(sourcePath))
        assertTrue(service.wantsRunFor(ProgramFiles.normalise(root.resolve("lib/std/core.sharp").toString())))
        assertFalse(service.wantsRunFor(ProgramFiles.normalise(root.resolve("elsewhere/x.sharp").toString())))
        assertFalse(service.wantsRunFor(sourcePath.removeSuffix(".sharp") + ".txt"))
    }

    fun testOldRunFoldersAreDeletedKeepingTheNewestTwo() {
        repeat(4) { service.runOnce(EmptyProgressIndicator()) }
        val kept = RunFolders.numberedFolders(root.resolve("runs")).map { it.first }
        assertEquals(listOf(3L, 4L), kept)
    }

    /** A compiler that fails on every save must not leave a folder per save behind. */
    fun testRunFoldersOfFailedRunsAreDeletedTooKeepingTheNewestTwo() {
        behaviour = { _, _ -> LaunchResult.CouldNotStart("wsl.exe is not installed") }
        repeat(3) { service.runOnce(EmptyProgressIndicator()) }
        behaviour = { _, _ -> LaunchResult.Finished(1, "invalid option -ide") }           // ended, but wrote no answer
        repeat(2) { service.runOnce(EmptyProgressIndicator()) }
        val kept = RunFolders.numberedFolders(root.resolve("runs")).map { it.first }
        assertEquals(listOf(4L, 5L), kept)
    }

    /** Saving a file of the program (a real change in the virtual file system) is what starts a run, after the debounce. */
    fun testSavingASharpFileOfTheProgramStartsARun() {
        val vf = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve("app/main.sharp"))!!
        runWriteAction { VfsUtil.saveText(vf, String(fixture.sourceBytes) + "// changed\n") }
        PlatformTestUtil.waitWithEventsDispatching("a run after the save", { launches.get() >= 1 && service.latest != null }, 20)
        assertEquals(sourcePath, listedFiles.last())
    }

    fun testSavingSomethingElseStartsNothing() {
        Files.write(root.resolve("notes.txt"), byteArrayOf(1))
        val vf = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve("notes.txt"))!!
        runWriteAction { VfsUtil.saveText(vf, "changed") }
        Thread.sleep(300)
        PlatformTestUtil.dispatchAllEventsInIdeEventQueue()
        assertEquals(0, launches.get())
    }

    fun testTheRunActionIsRegistered() {
        val action = com.intellij.openapi.actionSystem.ActionManager.getInstance().getAction("Sharp.RunCheckNow")
        assertNotNull(action)
        assertTrue(action is RunSharpCheckNowAction)
    }
}
