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
                        result.setResult(startSession(environment, object : XDebugProcessStarter() {
                            override fun start(session: XDebugSession): XDebugProcess {
                                val handler = ProcessHandlerFactory.getInstance().createColoredProcessHandler(command)
                                ProcessTerminatedListener.attach(handler)      // "Process finished with exit code N"
                                val made = SharpDebugProcess(session, handler, client)
                                process = made
                                client.start()
                                return made
                            }
                        }))
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

    /**
     * Starts the session and gives what the Debug window shows it in.
     *
     * The platform has two ways to say this, by its version:
     *
     *     2026.1 and newer    XDebuggerManager.newSessionBuilder(starter).environment(env).startSession()
     *                         -> a result that holds the session and its window
     *     before              XDebuggerManager.startSession(env, starter), then session.runContentDescriptor
     *
     * The older way still works in the newer versions but logs an error there on every start
     * ("RunContentDescriptor should not be used in split mode"). The plugin is built against a version
     * that has only the older way, so the newer one is looked up by name: when all of it is there it is
     * used, else the older way.
     */
    private fun startSession(environment: ExecutionEnvironment, starter: XDebugProcessStarter): RunContentDescriptor? {
        val manager = XDebuggerManager.getInstance(environment.project)
        val newer = try {
            val builder = Class.forName("com.intellij.xdebugger.XDebugSessionBuilder")
            val started = Class.forName("com.intellij.xdebugger.XSessionStartedResult")
            listOf(
                XDebuggerManager::class.java.getMethod("newSessionBuilder", XDebugProcessStarter::class.java),
                builder.getMethod("environment", ExecutionEnvironment::class.java),
                builder.getMethod("startSession"),
                started.getMethod("getRunContentDescriptor"),
            )
        } catch (_: ReflectiveOperationException) {
            null
        }
        if (newer == null) return manager.startSession(environment, starter).runContentDescriptor
        try {
            val builder = newer[1].invoke(newer[0].invoke(manager, starter), environment)
            return newer[3].invoke(newer[2].invoke(builder)) as RunContentDescriptor?
        } catch (e: java.lang.reflect.InvocationTargetException) {
            throw e.targetException                           // what the start itself threw (an ExecutionException with its message)
        }
    }
}
