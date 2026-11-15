/*
 * CompilerRunner - prepares a run folder and starts the compiler on it.
 *
 * WHAT THIS IS FOR
 *   One run, from "here are the files" to "the process ended". It does NOT read the
 *   compiler's answer (that is DiagnosticsReader's job) and never touches the editor:
 *   it only deals with files and a process.
 *
 *   Steps (doc/ide.md section 3.3):
 *     1. into the run folder write
 *          files.txt       one source path per line, in the compiler's form
 *          bodies_for.txt  the Sharp files open in editors (the compiler writes
 *                          per-function information for these only)
 *          run.sh          the script with its placeholders filled in
 *     2. build the command from the launcher setting, `{script}` being run.sh
 *     3. start it (ProcessLauncher), with a time limit and cancellation
 *     4. say how it ended. A process that did not start, ran too long, or was cancelled
 *        is reported here; whether the compiler's files are there and valid is decided
 *        later by looking at the folder.
 *
 *   The compiler runs in WSL on this machine, so paths are translated Windows -> WSL
 *   when the launcher starts wsl.exe (see PathTranslator).
 */
package sharp.ide.run

import com.intellij.openapi.progress.ProgressIndicator
import sharp.ide.paths.PathTranslator
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path

/** Everything one run needs. All paths in `files` and `openFiles` are in the EDITOR's form (`C:/x/y`). */
class RunRequest(
    val folder: Path,
    val files: List<String>,
    val openFiles: List<String>,
    val launcher: String,
    val script: String,
    val vm: String,
    val compiler: String,
    val extraArguments: List<String>,
    val timeoutSeconds: Int,
    /** Null: paths are passed as they are (the compiler runs where the IDE runs). */
    val translator: PathTranslator?,
)

/** What went wrong with a run, in words for the owner: shown once as a quiet notification. */
class RunFailure(val reason: String, val command: String, val outputTail: String)

sealed class RunOutcome {
    /** The process ended; `exitCode` is only logged (the compiler exits 0 even for a program with errors). */
    class Finished(val exitCode: Int, val outputTail: String, val command: String) : RunOutcome()
    class Failed(val failure: RunFailure) : RunOutcome()
    object Cancelled : RunOutcome()
}

class CompilerRunner(private val launcher: ProcessLauncher) {

    /** Seconds the process may run beyond the script's own `timeout`, so the script's kill of the compiler happens first. */
    private val graceSeconds = 10L

    fun run(request: RunRequest, indicator: ProgressIndicator): RunOutcome {
        val toCompiler = { path: String -> request.translator?.toWsl(path) ?: path }
        val folder = request.folder
        val scriptFile = folder.resolve("run.sh")
        val filesList = folder.resolve("files.txt")
        val command: List<String>
        try {
            Files.createDirectories(folder)
            writeLines(filesList, request.files.map(toCompiler))
            writeLines(folder.resolve("bodies_for.txt"), request.openFiles.map(toCompiler))
            val script = CommandTemplate.fillScript(
                request.script,
                ScriptValues(
                    vm = request.vm,
                    compiler = request.compiler,
                    filesList = toCompiler(filesList.toString().replace('\\', '/')),
                    ideDirectory = toCompiler(folder.toString().replace('\\', '/')),
                    extraArguments = request.extraArguments,
                    timeoutSeconds = request.timeoutSeconds,
                ),
            )
            Files.write(scriptFile, script.toByteArray(StandardCharsets.UTF_8))     // `\n` line ends, no BOM
            command = CommandTemplate.launcherArguments(request.launcher, toCompiler(scriptFile.toString().replace('\\', '/')))
        } catch (e: java.io.IOException) {
            return RunOutcome.Failed(RunFailure("could not prepare the run folder $folder: ${e.message}", "", ""))
        }
        val shown = command.joinToString(" ") { if (it.contains(' ')) "\"$it\"" else it }
        val timeoutMillis = (request.timeoutSeconds + graceSeconds) * 1000
        return when (val result = launcher.launch(command, timeoutMillis, indicator)) {
            is LaunchResult.CouldNotStart ->
                RunOutcome.Failed(RunFailure("the compiler could not be started: ${result.message}", shown, ""))
            is LaunchResult.TimedOut ->
                RunOutcome.Failed(RunFailure("the compiler did not finish within ${request.timeoutSeconds} seconds", shown, tail(result.output)))
            is LaunchResult.Cancelled -> RunOutcome.Cancelled
            is LaunchResult.Finished ->
                // 124 is what `timeout` exits with when it ended the compiler: the script's own limit was reached.
                if (result.exitCode == 124) {
                    RunOutcome.Failed(RunFailure("the compiler did not finish within ${request.timeoutSeconds} seconds", shown, tail(result.output)))
                } else {
                    RunOutcome.Finished(result.exitCode, tail(result.output), shown)
                }
        }
    }

    private fun writeLines(file: Path, lines: List<String>) {
        val text = if (lines.isEmpty()) "" else lines.joinToString("\n") + "\n"
        Files.write(file, text.toByteArray(StandardCharsets.UTF_8))
    }

    /** The last 20 lines of the output: what a failure message shows. */
    private fun tail(output: String): String = output.lines().takeLast(20).joinToString("\n")
}
