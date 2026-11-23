/*
 * THE DEBUG BUTTON, FOR A "SHARP PROGRAM" RUN CONFIGURATION
 *
 * The platform asks every program runner "can you run this configuration with this executor?";
 * this one says yes to the Debug executor and a Sharp program. (The plain Run needs no runner of its
 * own: the platform's default one starts the process SharpRunState makes.)
 *
 * Starting a debug session is done in two parts, on two threads:
 *
 *   on a background thread    everything slow: the editors saved, the files the build reads listed,
 *                             the build's script written (SharpRunState.prepare), and an SdbClient
 *                             set listening - its port goes into the script, for sdb to connect to
 *   on the UI thread          the session itself: the platform requires that, and what runs there
 *                             (starting the script's process) is quick
 *
 * SharpDebugProcess.kt says what happens from there.
 */
package sharp.ide.debug

import com.intellij.execution.ExecutionException
import com.intellij.execution.configurations.RunProfile
import com.intellij.execution.configurations.RunProfileState
import com.intellij.execution.configurations.RunnerSettings
import com.intellij.execution.executors.DefaultDebugExecutor
import com.intellij.execution.process.ProcessHandlerFactory
import com.intellij.execution.process.ProcessTerminatedListener
import com.intellij.execution.runners.AsyncProgramRunner
import com.intellij.execution.runners.ExecutionEnvironment
import com.intellij.execution.ui.RunContentDescriptor
import com.intellij.openapi.application.ApplicationManager
import com.intellij.xdebugger.XDebugProcess
import com.intellij.xdebugger.XDebugProcessStarter
import com.intellij.xdebugger.XDebugSession
import com.intellij.xdebugger.XDebuggerManager
import org.jetbrains.concurrency.AsyncPromise
import org.jetbrains.concurrency.Promise
import sharp.ide.execution.SharpRunConfiguration
import sharp.ide.execution.SharpRunState

class SharpDebugRunner : AsyncProgramRunner<RunnerSettings>() {
    override fun getRunnerId(): String = "SharpDebugRunner"

    override fun canRun(executorId: String, profile: RunProfile): Boolean =
        executorId == DefaultDebugExecutor.EXECUTOR_ID && profile is SharpRunConfiguration

    override fun execute(environment: ExecutionEnvironment, state: RunProfileState): Promise<RunContentDescriptor?> {
        val result = AsyncPromise<RunContentDescriptor?>()
        val run = state as? SharpRunState
        if (run == null) {
            result.setError(ExecutionException("Not a Sharp program"))
            return result
        }

        ApplicationManager.getApplication().executeOnPooledThread {
            try {
                // the session's process is made first (below), the client needs it for its events: one holds the other
                var process: SharpDebugProcess? = null
                val client = SdbClient(onEvent = { message -> process?.event(message) }, onGone = {})
                val command = run.prepare(debugPort = client.port)

                ApplicationManager.getApplication().invokeLater {
                    try {
                        val session = XDebuggerManager.getInstance(environment.project).startSession(environment, object : XDebugProcessStarter() {
                            override fun start(session: XDebugSession): XDebugProcess {
                                val handler = ProcessHandlerFactory.getInstance().createColoredProcessHandler(command)
                                ProcessTerminatedListener.attach(handler)      // "Process finished with exit code N"
                                val made = SharpDebugProcess(session, handler, client)
                                process = made
                                client.start()
                                return made
                            }
                        })
                        result.setResult(session.runContentDescriptor)
                    } catch (e: Throwable) {
                        client.close()
                        result.setError(e)
                    }
                }
            } catch (e: Throwable) {
                result.setError(e)
            }
        }
        return result
    }
}
