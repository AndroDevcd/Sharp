/*
 * ProcessLauncher - starts an operating-system process and waits for it.
 *
 * WHAT THIS IS FOR
 *   Starting the compiler is the one place where the plugin leaves the JVM. It is put
 *   behind this small interface for two reasons: the tests substitute a FAKE launcher
 *   (which, instead of starting a compiler, writes a prepared answer into the run
 *   folder), and everything that can go wrong with a process - it does not start, it
 *   runs too long, the person cancels - is turned into one of four plain results here,
 *   so CompilerRunner never handles exceptions.
 *
 *   The real launcher uses the platform's own process classes (GeneralCommandLine,
 *   CapturingProcessHandler), which take care of quoting an argument with a space on
 *   Windows and of reading the output on separate threads (so a full pipe never
 *   blocks the compiler). It must be called on a BACKGROUND thread, never the UI thread.
 */
package sharp.ide.run

import com.intellij.execution.ExecutionException
import com.intellij.execution.configurations.GeneralCommandLine
import com.intellij.execution.process.CapturingProcessHandler
import com.intellij.openapi.progress.ProgressIndicator
import java.nio.charset.StandardCharsets

/** How a process ended. `output` is its standard output and error together. */
sealed class LaunchResult {
    class Finished(val exitCode: Int, val output: String) : LaunchResult()
    class TimedOut(val output: String) : LaunchResult()
    object Cancelled : LaunchResult()
    class CouldNotStart(val message: String) : LaunchResult()
}

interface ProcessLauncher {
    /** Runs `command` (program first, then its arguments, each one argument), killing it after `timeoutMillis` or when `indicator` is cancelled. */
    fun launch(command: List<String>, timeoutMillis: Long, indicator: ProgressIndicator): LaunchResult
}

class PlatformProcessLauncher : ProcessLauncher {

    override fun launch(command: List<String>, timeoutMillis: Long, indicator: ProgressIndicator): LaunchResult {
        if (command.isEmpty()) return LaunchResult.CouldNotStart("the launcher setting is empty")
        val handler = try {
            CapturingProcessHandler(GeneralCommandLine(command).withCharset(StandardCharsets.UTF_8))
        } catch (e: ExecutionException) {
            return LaunchResult.CouldNotStart(e.message ?: "the process could not be started")
        }
        val output = handler.runProcessWithProgressIndicator(indicator, timeoutMillis.coerceAtMost(Int.MAX_VALUE.toLong()).toInt(), true)
        val text = (output.stdout + output.stderr).trim()
        return when {
            output.isCancelled -> LaunchResult.Cancelled
            output.isTimeout -> LaunchResult.TimedOut(text)
            else -> LaunchResult.Finished(output.exitCode, text)
        }
    }
}
