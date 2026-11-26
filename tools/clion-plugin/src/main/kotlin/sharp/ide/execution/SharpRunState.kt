/*
 * ONE RUN OF A "SHARP PROGRAM" CONFIGURATION: from the Run button to a process in the console
 *
 * The platform asks a run configuration for a "state" and the state for a process; whatever that
 * process prints appears in the Run console, and what is typed there goes to it. Here:
 *
 *     1. every editor is saved (the compiler reads files from disk);
 *     2. the files the build reads are listed - the standard library, then the .sharp files of the
 *        program folder - into  <program folder>/build/.sharp-run/files.txt;
 *     3. the build's stamp is made and compared with the last good build's (RunStamp.kt): when
 *        nothing changed, the compiler will not be started;
 *     4. the build-then-run script is written beside it (RunScript.kt) and started.
 *
 * TWO WAYS TO BUILD, decided by the folder:
 *
 *     the folder has a settings.json   the compiler's PROJECT MODE:  sharpc -P <folder> [-L <library>]
 *     (sharp.ide.project)              The file says what the program is called, which folders are left
 *                                      out, which libraries are added, how it is optimized; the program
 *                                      is <folder>/build/outputs/<output>. `-L` is given only when the
 *                                      file names no library of its own.
 *     any other folder                 the files, listed:  sharpc -files <list> -o <folder>/build/<name>
 *                                      (the folders the plugin's settings exclude - tests, build - left out)
 *
 * Which standard library: the folder of the project's settings when one is set; the project's own
 * library when the project is the Sharp repository; else the one that was installed with Sharp.
 *
 * `plan` is separate from starting the process so that a test can ask "what would be run".
 */
package sharp.ide.execution

import com.intellij.execution.DefaultExecutionResult
import com.intellij.execution.ExecutionException
import com.intellij.execution.ExecutionResult
import com.intellij.execution.Executor
import com.intellij.execution.configurations.CommandLineState
import com.intellij.execution.configurations.GeneralCommandLine
import com.intellij.execution.process.ProcessHandler
import com.intellij.execution.process.ProcessHandlerFactory
import com.intellij.execution.process.ProcessTerminatedListener
import com.intellij.execution.runners.ExecutionEnvironment
import com.intellij.execution.runners.ProgramRunner
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.components.service
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.util.SystemInfo
import com.intellij.util.execution.ParametersListUtil
import sharp.ide.paths.ProgramFiles
import sharp.ide.paths.ProgramSettings
import sharp.ide.project.SharpProjectFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpProjectSettings
import sharp.ide.testing.SharpTestView
import sharp.ide.toolchain.SharpToolchain
import sharp.ide.toolchain.SharpToolchains
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/** What one run needs on disk and what it starts. */
class RunPlan(
    val runFolder: Path,
    val filesList: Path,
    /** Every file the build reads (what the stamp is made of; in file mode also what `-files` lists). */
    val files: List<String>,
    val scriptFile: Path,
    val scriptText: String,
    val command: List<String>,
    val workingDirectory: String,
    /** The stamp of this build (RunStamp.kt) and the file it is written to before the script starts. */
    val stampText: String,
    val stampFile: Path,
    /** true: nothing changed since the last good build - the script does not start the compiler. */
    val upToDate: Boolean,
    /** true: the folder has a settings.json and the build is the compiler's project mode (`-P`). */
    val projectMode: Boolean,
    /** Environment variables the run adds (SHARP_CXX: the machine's C++ compiler, when the settings name one). */
    val environment: Map<String, String> = emptyMap(),
)

class SharpRunState(environment: ExecutionEnvironment, private val configuration: SharpRunConfiguration) : CommandLineState(environment) {
    init {
        addConsoleFilters(SharpConsoleFilter(environment.project))
    }

    override fun startProcess(): ProcessHandler {
        val handler = ProcessHandlerFactory.getInstance().createColoredProcessHandler(prepare(debugPort = null))
        ProcessTerminatedListener.attach(handler)      // "Process finished with exit code N" at the end
        return handler
    }

    /** A test program's run is shown in the platform's test view (sharp.ide.testing); any other in the plain console. */
    override fun execute(executor: Executor, runner: ProgramRunner<*>): ExecutionResult {
        if (!showsTests()) return super.execute(executor, runner)
        val handler = startProcess()
        val console = SharpTestView.consoleFor(configuration, executor, handler)
        console.addMessageFilter(SharpConsoleFilter(environment.project))
        return DefaultExecutionResult(console, handler, *createActions(console, handler, executor))
    }

    /** A test program - or a project's tests - that is run (not only built) reports its tests for the test view. */
    private fun showsTests(): Boolean = (configuration.fields.testProgram || configuration.fields.projectTests) && !configuration.fields.buildOnly

    /**
     * Everything a run needs before its process is started: the editors saved, the plan made, its
     * files written. Gives the command to start.
     *
     * `debugPort`: a DEBUG run (sharp.ide.debug). The program is built with -dbg and started by sdb,
     * the debugger, which is told the port the IDE listens on. Null: a plain run.
     * Not to be called on the UI thread when it can be helped: it reads the folder's files.
     */
    fun prepare(debugPort: Int?): GeneralCommandLine {
        val project = environment.project
        // the compiler reads the files from disk: what is in the editors goes there first
        ApplicationManager.getApplication().invokeAndWait { FileDocumentManager.getInstance().saveAllDocuments() }

        val app = SharpApplicationSettings.getInstance().state
        val toolchain = SharpToolchains.find(app.sharpHome) ?: throw ExecutionException(SharpToolchains.notInstalledMessage(app.sharpHome))
        val projectSettings = project.service<SharpProjectSettings>().state
        val plan = plan(
            toolchain = toolchain,
            programFolder = configuration.programFolderPath(),
            projectBase = project.basePath ?: "",
            excludedFolders = projectSettings.excludedFolders.toList(),
            libraryPath = projectSettings.libraryPath,
            compilerOptions = ParametersListUtil.parse(configuration.fields.compilerOptions ?: ""),
            // (a test program is told to report for the test view; under the debugger its console is the plain one)
            programArguments = ParametersListUtil.parse(configuration.fields.programArguments ?: "") +
                (if (showsTests() && debugPort == null) listOf(SharpTestView.ARGUMENT) else emptyList()) +
                // (which of the project's tests: `-test-only <group>` or `-test-only "<group> > <test>"`, std.testing)
                (if (configuration.fields.projectTests) configuration.testFilters().flatMap { listOf(SharpTestView.ONLY_ARGUMENT, it) } else emptyList()),
            tests = configuration.fields.projectTests,
            workingDirectory = configuration.fields.workingDirectory ?: "",
            buildOnly = configuration.fields.buildOnly,
            unoptimizedCompiler = configuration.fields.unoptimizedCompiler,
            windows = SystemInfo.isWindows,
            alwaysRebuild = configuration.fields.alwaysRebuild,
            cxxCompiler = app.cxxCompiler,
            debugPort = debugPort,
        )
        write(plan)
        return GeneralCommandLine(plan.command)
            .withWorkDirectory(plan.workingDirectory)
            .withCharset(StandardCharsets.UTF_8)
            .withEnvironment(plan.environment)
    }

    companion object {
        /** The files a plan needs on disk before its command is started: the file list, the script, the new stamp. */
        fun write(plan: RunPlan) {
            try {
                Files.createDirectories(plan.runFolder)
                Files.write(plan.filesList, (plan.files.joinToString("\n") + "\n").toByteArray(StandardCharsets.UTF_8))
                Files.write(plan.scriptFile, plan.scriptText.toByteArray(StandardCharsets.UTF_8))
                Files.write(plan.stampFile, plan.stampText.toByteArray(StandardCharsets.UTF_8))
            } catch (e: java.io.IOException) {
                throw ExecutionException("The run could not be prepared in ${plan.runFolder}: ${e.message}")
            }
        }

        /**
         * Everything one run writes and starts, worked out from plain values. Throws ExecutionException
         * with a message for the user when the run cannot be made (no folder, no files, no library).
         */
        fun plan(
            toolchain: SharpToolchain,
            programFolder: String,
            projectBase: String,
            excludedFolders: List<String>,
            libraryPath: String,
            compilerOptions: List<String>,
            programArguments: List<String>,
            workingDirectory: String,
            buildOnly: Boolean,
            unoptimizedCompiler: Boolean,
            windows: Boolean,
            alwaysRebuild: Boolean = false,
            cxxCompiler: String = "",
            /** A debug run: the port the IDE listens on for sdb (sharp.ide.debug). Null: a plain run. */
            debugPort: Int? = null,
            /** Build and run the project's TESTS (`sharpc -P -tests`: its tests folder with the program), not the program. */
            tests: Boolean = false,
        ): RunPlan {
            // a debug run is built with -dbg (the names of variables and the source text the debugger
            // shows come from it) and is always run - by sdb, which is told where the IDE listens
            val debugger = if (debugPort == null) null else toolchain.debugger
                ?: throw ExecutionException("This Sharp installation has no debugger (looked for ${toolchain.home}/bin/sdb): install Sharp 0.5.6 or newer")
            @Suppress("NAME_SHADOWING")
            val compilerOptions = if (debugPort != null && "-dbg" !in compilerOptions && "-debug" !in compilerOptions) compilerOptions + "-dbg" else compilerOptions
            @Suppress("NAME_SHADOWING")
            val buildOnly = buildOnly && debugPort == null
            val folder = ProgramFiles.normalise(programFolder)
            if (folder.isEmpty()) throw ExecutionException("No program folder is set: choose it in Run > Edit Configurations")
            if (!Files.isDirectory(Paths.get(folder))) throw ExecutionException("The program folder does not exist: $folder")

            val project = SharpProjectFiles.read(folder)
            if (tests) {
                if (project == null) throw ExecutionException("The folder is not a Sharp project (it has no settings.json), so it has no tests folder: $folder")
                if (!project.hasTests()) throw ExecutionException("The project has no tests: there is no folder `${project.testsFolder}` in $folder")
                if (SharpToolchains.compareVersions(toolchain.version, SharpToolchains.FIRST_WITH_PROJECT_TESTS) < 0) {
                    throw ExecutionException("A project's tests need Sharp ${SharpToolchains.FIRST_WITH_PROJECT_TESTS} or newer; installed is ${toolchain.version}")
                }
            }
            // which files the build reads: in project mode settings.json decides what is left out and what is added
            val settings = if (project != null) {
                ProgramSettings(
                    baseDirectory = projectBase,
                    sourceRoots = listOf(folder) + project.externLibraryFolders(),
                    excludedFolders = listOf("build") + project.ignoredInsideProject(),
                    libraryPath = libraryPath,
                    installedLibrary = toolchain.library,
                )
            } else {
                ProgramSettings(
                    baseDirectory = projectBase,
                    sourceRoots = listOf(folder),
                    excludedFolders = excludedFolders,
                    libraryPath = libraryPath,
                    installedLibrary = toolchain.library,
                )
            }
            val library = ProgramFiles.layoutOf(settings).libraryRoot
                ?: throw ExecutionException("The Sharp standard library was not found (looked for the one installed in ${toolchain.home})")
            val files = ProgramFiles.list(settings)
            if (files.none { !it.startsWith("$library/") }) {
                throw ExecutionException("The program folder has no .sharp file: $folder")
            }

            val folderName = folder.substringAfterLast('/').ifEmpty { "program" }
            val name = (project?.name ?: folderName) + (if (tests) " tests" else "")
            // (the tests are a build of their own, with a stamp of their own: switching between the
            //  program and its tests builds neither again)
            val runFolder = Paths.get("$folder/build/.sharp-run" + (if (tests) "/tests" else ""))
            val filesList = runFolder.resolve("files.txt")
            val compiler = if (unoptimizedCompiler) {
                toolchain.unoptimizedCompiler
                    ?: throw ExecutionException("This Sharp installation has no unoptimized compiler (looked in ${toolchain.home}/compiler)")
            } else {
                toolchain.compiler
            }
            val output: String
            val compileArguments: List<CompileArgument>
            if (project != null) {
                output = if (tests) project.testsOutputPath else project.outputPath
                // a project that names its libraries itself gets no other; else the standard library is added
                compileArguments = listOf(CompileArgument.option("-P"), CompileArgument.path(folder)) +
                    (if (project.externLibraries.isEmpty()) listOf(CompileArgument.option("-L"), CompileArgument.path(library)) else emptyList()) +
                    (if (tests) listOf(CompileArgument.option("-tests")) else emptyList())
            } else {
                output = "$folder/build/$folderName"
                compileArguments = listOf(
                    CompileArgument.option("-files"), CompileArgument.path(filesList.toString()),
                    CompileArgument.option("-o"), CompileArgument.path(output),
                )
            }

            // ---- has anything the build depends on changed since the last good one?
            // (a project's C++ counts too: the compiler builds it into the native libraries, so a changed
            //  .cpp is a reason to build although no .sharp file changed - and so is another C++ compiler)
            val stampedFiles = if (project != null) files + "$folder/${SharpProjectFiles.FILE_NAME}" + project.nativeSourceFiles() else files
            val cxx = cxxCompiler.trim()
            val environment = if (cxx.isEmpty()) emptyMap() else mapOf("SHARP_CXX" to cxx)
            val stampText = RunStamp.of(compiler, compileArguments.map { it.text } + "|" + compilerOptions + "|cxx=$cxx", stampedFiles)
            val keptStamp = runFolder.resolve("stamp.txt")
            val newStamp = runFolder.resolve("stamp.new.txt")
            val upToDate = !alwaysRebuild && RunStamp.isUpToDate(stampText, keptStamp.toString(), output)

            val values = RunScriptValues(
                title = name,
                vm = toolchain.vm,
                compiler = compiler,
                compileArguments = compileArguments,
                output = output,
                compileLog = runFolder.resolve("compile.txt").toString(),
                compilerOptions = compilerOptions,
                programArguments = programArguments,
                workingDirectory = workingDirectory.trim().ifEmpty { folder },
                buildOnly = buildOnly,
                upToDate = upToDate,
                newStamp = newStamp.toString(),
                keptStamp = keptStamp.toString(),
                runner = if (debugger == null) emptyList() else listOf(debugger, "-machine", debugPort.toString()),
            )
            return if (windows) {
                val script = runFolder.resolve("run.cmd")
                RunPlan(runFolder, filesList, files, script, RunScript.windows(values), listOf("cmd.exe", "/d", "/c", script.toString()), folder,
                    stampText, newStamp, upToDate, project != null, environment)
            } else {
                val script = runFolder.resolve("run.sh")
                RunPlan(runFolder, filesList, files, script, RunScript.unix(values), listOf("sh", script.toString()), folder,
                    stampText, newStamp, upToDate, project != null, environment)
            }
        }
    }
}
