/*
 * ONE DEBUG SESSION OF A SHARP PROGRAM
 *
 * The Debug button on a "Sharp Program" run configuration ends here. What happens:
 *
 *   1. SharpDebugRunner prepares the run as a plain run is prepared (the editors saved, the build's
 *      script written - sharp.ide.execution), with two differences: the program is built with -dbg,
 *      and what the script starts is `sdb -machine <port> program`, not `sharp program`.
 *      <port> is where an SdbClient of this session listens.
 *   2. The script is started. Its output - the compiler's errors, then everything the program
 *      prints - is the Debug window's Console, and what is typed there goes to the program.
 *   3. sdb connects and says "ready": the program is loaded, nothing has run. The breakpoints of the
 *      IDE are sent, then "start".
 *   4. From then on: sdb says "stopped", and the platform is shown where (the threads, the calls, the
 *      variables: SharpDebugFrames.kt); the buttons of the Debug window become requests (continue,
 *      step over / into / out, pause); a breakpoint set or removed is sent at once, also while the
 *      program runs.
 *   5. The program ends, the process ends, and the platform ends the session.
 *
 * WHAT sdb DOES NOT HAVE, and so the IDE does not offer: evaluating expressions and watches (sdb names
 * values by paths, it computes nothing), conditions on breakpoints.
 *
 * THREADS. The platform calls this class on the UI thread and must not be kept waiting: everything
 * here only SENDS a request and returns. Answers and events arrive on the connection's thread
 * (SdbClient), and what the session is told there (`positionReached`, `breakpointReached`) may be said
 * from any thread.
 */
package sharp.ide.debug

import com.google.gson.JsonObject
import com.intellij.execution.filters.TextConsoleBuilderFactory
import com.intellij.execution.process.ProcessHandler
import com.intellij.execution.ui.ConsoleViewContentType
import com.intellij.execution.ui.ExecutionConsole
import com.intellij.openapi.application.ReadAction
import com.intellij.openapi.vfs.VfsUtilCore
import com.intellij.xdebugger.XDebugProcess
import com.intellij.xdebugger.XDebugSession
import com.intellij.xdebugger.XSourcePosition
import com.intellij.xdebugger.breakpoints.XBreakpointHandler
import com.intellij.xdebugger.breakpoints.XLineBreakpoint
import com.intellij.xdebugger.evaluation.XDebuggerEditorsProvider
import com.intellij.xdebugger.frame.XSuspendContext
import sharp.ide.execution.SharpConsoleFilter
import java.util.concurrent.ConcurrentHashMap

class SharpDebugProcess(
    session: XDebugSession,
    private val handler: ProcessHandler,
    private val client: SdbClient,
) : XDebugProcess(session) {
    private val console by lazy {
        val view = TextConsoleBuilderFactory.getInstance().createBuilder(session.project).console
        view.addMessageFilter(SharpConsoleFilter(session.project))      // the compiler's errors are links
        view.attachToProcess(handler)
        view
    }

    // the breakpoints sdb has, by the number sdb gave each; and the ones taken away before sdb had answered
    private val numbers = ConcurrentHashMap<XLineBreakpoint<SharpBreakpointProperties>, Int>()
    private val byNumber = ConcurrentHashMap<Int, XLineBreakpoint<SharpBreakpointProperties>>()
    private val removedEarly = ConcurrentHashMap.newKeySet<XLineBreakpoint<SharpBreakpointProperties>>()

    @Volatile private var ready = false                 // sdb said "ready": breakpoints can be sent
    @Volatile private var lastStop: SharpSuspendContext? = null
    @Volatile private var stoppedFiber = 0
    @Volatile private var runToNumber = 0               // the breakpoint "Run to Cursor" made; taken away at the next stop

    private val breakpointHandler = object : XBreakpointHandler<XLineBreakpoint<SharpBreakpointProperties>>(SharpLineBreakpointType::class.java) {
        override fun registerBreakpoint(breakpoint: XLineBreakpoint<SharpBreakpointProperties>) = send(breakpoint)

        override fun unregisterBreakpoint(breakpoint: XLineBreakpoint<SharpBreakpointProperties>, temporary: Boolean) {
            val number = numbers.remove(breakpoint)
            if (number == null) {
                removedEarly += breakpoint                  // sdb has not said its number yet: taken away when it does
            } else {
                byNumber.remove(number)
                client.ask("clear", { it.addProperty("number", number) })
            }
        }
    }

    override fun getEditorsProvider(): XDebuggerEditorsProvider = SharpDebuggerEditorsProvider
    override fun getBreakpointHandlers(): Array<XBreakpointHandler<*>> = arrayOf(breakpointHandler)
    override fun doGetProcessHandler(): ProcessHandler = handler
    override fun createConsole(): ExecutionConsole = console

    /** Not yet: sdb is not connected when the session begins. They are sent when it says "ready". */
    override fun checkCanInitBreakpoints(): Boolean = ready

    // ------------------------------------------------------------------ the buttons of the Debug window

    override fun resume(context: XSuspendContext?) = go("continue", context)
    override fun startStepOver(context: XSuspendContext?) = go("next", context)
    override fun startStepInto(context: XSuspendContext?) = go("into", context)
    override fun startStepOut(context: XSuspendContext?) = go("out", context)

    override fun startPausing() {
        client.ask("pause")
    }

    /** "Run to Cursor": a breakpoint of its own at that line, taken away again at the next stop. */
    override fun runToPosition(position: XSourcePosition, context: XSuspendContext?) {
        client.ask("break", { it.addProperty("file", position.file.path); it.addProperty("line", position.line + 1) }) { answer ->
            if (SdbClient.ok(answer)) {
                runToNumber = SdbClient.number(answer, "number")
                go("continue", context)
            } else {
                say("Run to Cursor: ${SdbClient.text(answer, "why")}\n")
                stayStopped()
            }
        }
    }

    override fun stop() {
        client.ask("quit")
        client.close()
    }

    /** Let the program go: on, or a step of the thread the Debug window has selected. */
    private fun go(what: String, context: XSuspendContext?) {
        val fiber = (context?.activeExecutionStack as? SharpExecutionStack)?.fiber ?: stoppedFiber
        client.ask(what, { it.addProperty("fiber", fiber) }) { answer ->
            if (!SdbClient.ok(answer)) {
                // sdb did not let it go (nothing to step: no thread was running Sharp code): it is still stopped
                say("${SdbClient.text(answer, "why")}\n")
                stayStopped()
            }
        }
    }

    /** The platform took the program for running; it is not: show the stop again. */
    private fun stayStopped() {
        lastStop?.let { session.positionReached(it) }
    }

    // ------------------------------------------------------------------ breakpoints

    private fun send(breakpoint: XLineBreakpoint<SharpBreakpointProperties>) {
        val file = VfsUtilCore.urlToPath(breakpoint.fileUrl)
        client.ask("break", { it.addProperty("file", file); it.addProperty("line", breakpoint.line + 1) }) { answer ->
            if (SdbClient.ok(answer)) {
                val number = SdbClient.number(answer, "number")
                if (removedEarly.remove(breakpoint)) {
                    client.ask("clear", { it.addProperty("number", number) })
                } else {
                    numbers[breakpoint] = number
                    byNumber[number] = breakpoint
                    session.setBreakpointVerified(breakpoint)
                }
            } else {
                removedEarly.remove(breakpoint)
                val next = SdbClient.number(answer, "next", -1)
                val why = SdbClient.text(answer, "why") + if (next > 0) " (the next line that does is $next)" else ""
                session.setBreakpointInvalid(breakpoint, why)
            }
        }
    }

    // ------------------------------------------------------------------ what sdb says by itself

    /** An event of sdb's (on the connection's thread). */
    fun event(message: JsonObject) {
        when (SdbClient.text(message, "event")) {
            "ready" -> {
                // the program is loaded and nothing has run: the breakpoints first, then let it start
                ready = true
                ReadAction.run<RuntimeException> { session.initBreakpoints() }
                client.ask("start")
            }
            "stopped" -> stopped(message)
            "say" -> say(SdbClient.text(message, "text") + "\n")
        }
    }

    /** The program stopped: ask for its threads and for the calls of the one that stopped, then show the platform. */
    private fun stopped(event: JsonObject) {
        val fiber = SdbClient.number(event, "fiber")
        stoppedFiber = fiber
        val temporary = runToNumber
        if (temporary != 0) {
            runToNumber = 0
            client.ask("clear", { it.addProperty("number", temporary) })
        }

        client.ask("threads") { threads ->
            client.ask("frames", { it.addProperty("fiber", fiber) }) { frames ->
                val top = if (SdbClient.ok(frames)) SharpExecutionStack.framesOf(client, fiber, frames).firstOrNull() else null
                var active: SharpExecutionStack? = null
                val stacks = ArrayList<SharpExecutionStack>()
                threads.getAsJsonArray("threads")?.map { it.asJsonObject }?.forEach { thread ->
                    val itsFiber = SdbClient.number(thread, "fiber", -1)
                    if (itsFiber < 0) return@forEach            // it runs no fiber: there are no calls to show
                    val name = "${SdbClient.text(thread, "name")} (thread ${SdbClient.number(thread, "id")}): ${SdbClient.text(thread, "state")}"
                    val isStopped = itsFiber == fiber
                    val stack = SharpExecutionStack(client, name, itsFiber, if (isStopped) top else null)
                    if (isStopped) active = stack
                    stacks += stack
                }
                // (a stop that no thread answered - all of them asleep - is shown on the fiber sdb named)
                val shown = active ?: SharpExecutionStack(client, "fiber $fiber", fiber, top).also { stacks += it }
                val context = SharpSuspendContext(shown, stacks)
                lastStop = context

                val number = SdbClient.number(event, "number")
                val breakpoint = if (SdbClient.text(event, "reason") == "breakpoint" && number != temporary) byNumber[number] else null
                if (breakpoint == null) {
                    session.positionReached(context)
                } else if (!session.breakpointReached(breakpoint, null, context)) {
                    resume(context)                             // a breakpoint that is set not to suspend
                }
            }
        }
    }

    private fun say(text: String) {
        console.print(text, ConsoleViewContentType.SYSTEM_OUTPUT)
    }
}
