/*
 * WHAT THE DEBUG WINDOW SHOWS WHILE THE PROGRAM IS STOPPED
 *
 * The platform's picture of a stopped program, and what it is here:
 *
 *     XSuspendContext     the stop itself                  SharpSuspendContext
 *       XExecutionStack   a thread, with its calls         SharpExecutionStack   (a Sharp thread and the fiber it runs)
 *         XStackFrame     one call                         SharpStackFrame       (a function, a file, a line)
 *           XValue        a variable, and what is in it    SharpValue            (opened: fields, or elements)
 *
 * Each of them answers the platform's questions by asking sdb (SdbClient) and handing the answer
 * over when it comes. The platform asks on the UI thread and must never wait there; the answers
 * arrive on the connection's thread. A question must ALWAYS be answered - with the result or with an
 * error - or the window shows "Collecting data..." for ever.
 *
 * A variable is known to sdb by a PATH (`corner`, `corner.x`, `numbers[3]`): sdb hands it out with
 * each variable, and it is handed back to open the variable or to set it.
 *
 * LINES: sdb counts from 1, the IDE from 0. The one place that converts is SharpStackFrame.
 */
package sharp.ide.debug

import com.google.gson.JsonObject
import com.intellij.icons.AllIcons
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.ui.ColoredTextContainer
import com.intellij.ui.SimpleTextAttributes
import com.intellij.xdebugger.XDebuggerUtil
import com.intellij.xdebugger.XExpression
import com.intellij.xdebugger.XSourcePosition
import com.intellij.xdebugger.frame.XCompositeNode
import com.intellij.xdebugger.frame.XExecutionStack
import com.intellij.xdebugger.frame.XNamedValue
import com.intellij.xdebugger.frame.XStackFrame
import com.intellij.xdebugger.frame.XSuspendContext
import com.intellij.xdebugger.frame.XValueChildrenList
import com.intellij.xdebugger.frame.XValueModifier
import com.intellij.xdebugger.frame.XValueNode
import com.intellij.xdebugger.frame.XValuePlace

/** How many elements of an array are asked for at a time (the platform's own "show more" step). */
private const val PARTS_AT_ONCE = XCompositeNode.MAX_CHILDREN_TO_SHOW

class SharpSuspendContext(
    private val active: SharpExecutionStack,
    private val all: List<SharpExecutionStack>,
) : XSuspendContext() {
    override fun getActiveExecutionStack(): XExecutionStack = active
    override fun getExecutionStacks(): Array<XExecutionStack> = all.toTypedArray()
}

/** One thread of the program, with the fiber it runs: the calls are that fiber's. */
class SharpExecutionStack(
    private val client: SdbClient,
    name: String,
    val fiber: Int,
    private val top: SharpStackFrame?,
) : XExecutionStack(name) {
    override fun getTopFrame(): XStackFrame? = top

    override fun computeStackFrames(firstFrameIndex: Int, container: XStackFrameContainer) {
        client.ask("frames", { it.addProperty("fiber", fiber) }) { answer ->
            if (!SdbClient.ok(answer)) {
                container.errorOccurred(SdbClient.text(answer, "why"))
            } else {
                val frames = framesOf(client, fiber, answer)
                container.addStackFrames(frames.drop(firstFrameIndex), true)
            }
        }
    }

    companion object {
        /** The frames of a "frames" answer, the innermost first. */
        fun framesOf(client: SdbClient, fiber: Int, answer: JsonObject): List<SharpStackFrame> =
            answer.getAsJsonArray("frames")?.map { it.asJsonObject }?.map { frame ->
                SharpStackFrame(
                    client, fiber,
                    index = SdbClient.number(frame, "index"),
                    function = SdbClient.text(frame, "function"),
                    file = SdbClient.text(frame, "file"),
                    line = SdbClient.number(frame, "line", -1),
                )
            } ?: emptyList()
    }
}

/** One call: a function, and the line of its file it is at (counted from 1, as sdb counts). */
class SharpStackFrame(
    private val client: SdbClient,
    private val fiber: Int,
    private val index: Int,
    private val function: String,
    private val file: String,
    private val line: Int,
) : XStackFrame() {
    private val position: XSourcePosition? by lazy {
        if (file.isEmpty() || line < 1) null
        else LocalFileSystem.getInstance().findFileByPath(file.replace('\\', '/'))?.let { XDebuggerUtil.getInstance().createPosition(it, line - 1) }
    }

    override fun getSourcePosition(): XSourcePosition? = position

    /** The same call in the same function keeps what was opened in the Variables view from one stop to the next. */
    override fun getEqualityObject(): Any = function

    override fun customizePresentation(component: ColoredTextContainer) {
        component.append(function, SimpleTextAttributes.REGULAR_ATTRIBUTES)
        if (file.isNotEmpty() && line >= 1) {
            component.append("  ${file.replace('\\', '/').substringAfterLast('/')}:$line", SimpleTextAttributes.GRAYED_ATTRIBUTES)
        }
        component.setIcon(AllIcons.Debugger.Frame)
    }

    override fun computeChildren(node: XCompositeNode) {
        client.ask("variables", { it.addProperty("fiber", fiber); it.addProperty("frame", index) }) { answer ->
            if (!SdbClient.ok(answer)) {
                node.setErrorMessage(SdbClient.text(answer, "why"))
            } else {
                node.addChildren(SharpValue.listOf(client, fiber, index, answer), true)
            }
        }
    }
}

/** One variable, field or element: its name, what it holds as text, its kind, and whether it can be opened. */
class SharpValue(
    private val client: SdbClient,
    private val fiber: Int,
    private val frame: Int,
    name: String,
    private var value: String,
    private var kind: String,
    private var hasParts: Boolean,
    private val path: String,
) : XNamedValue(name) {

    override fun computePresentation(node: XValueNode, place: XValuePlace) {
        val icon = when {
            hasParts && kind.endsWith("]") -> AllIcons.Debugger.Db_array
            hasParts -> AllIcons.Debugger.Value
            else -> AllIcons.Debugger.Db_primitive
        }
        // (the kind is left out where the value says it already: an object shows as its class)
        node.setPresentation(icon, if (kind == value) null else kind.ifEmpty { null }, value, hasParts)
    }

    override fun computeChildren(node: XCompositeNode) = addParts(node, 0)

    /** The parts from `from` on, PARTS_AT_ONCE at a time; when more are left, the platform's "show more" asks for the next ones. */
    private fun addParts(node: XCompositeNode, from: Int) {
        client.ask("children", {
            it.addProperty("fiber", fiber); it.addProperty("frame", frame); it.addProperty("path", path)
            it.addProperty("from", from); it.addProperty("count", PARTS_AT_ONCE)
        }) { answer ->
            if (node.isObsolete) return@ask
            if (!SdbClient.ok(answer)) {
                node.setErrorMessage(SdbClient.text(answer, "why"))
                return@ask
            }
            val parts = listOf(client, fiber, frame, answer)
            val left = SdbClient.number(answer, "total") - from - parts.size()
            if (left > 0) {
                node.addChildren(parts, false)
                node.tooManyChildren(left) { addParts(node, from + parts.size()) }
            } else {
                node.addChildren(parts, true)
            }
        }
    }

    /** "Set Value": what is typed goes to sdb as it is - `12`, `true`, `"bread"`, `null`. */
    override fun getModifier(): XValueModifier? = object : XValueModifier() {
        override fun getInitialValueEditorText(): String = value

        override fun setValue(expression: XExpression, callback: XModificationCallback) {
            client.ask("set", {
                it.addProperty("fiber", fiber); it.addProperty("frame", frame); it.addProperty("path", path)
                it.addProperty("value", expression.expression.trim())
            }) { answer ->
                if (!SdbClient.ok(answer)) {
                    callback.errorOccurred(SdbClient.text(answer, "why"))
                } else {
                    value = SdbClient.text(answer, "value")
                    kind = SdbClient.text(answer, "kind")
                    hasParts = answer.get("parts")?.asBoolean == true
                    callback.valueModified()
                }
            }
        }
    }

    companion object {
        /** The variables of a "variables" or "children" answer. */
        fun listOf(client: SdbClient, fiber: Int, frame: Int, answer: JsonObject): XValueChildrenList {
            val list = XValueChildrenList()
            answer.getAsJsonArray("variables")?.map { it.asJsonObject }?.forEach { variable ->
                list.add(SharpValue(
                    client, fiber, frame,
                    name = SdbClient.text(variable, "name"),
                    value = SdbClient.text(variable, "value"),
                    kind = SdbClient.text(variable, "kind"),
                    hasParts = variable.get("parts")?.asBoolean == true,
                    path = SdbClient.text(variable, "path"),
                ))
            }
            return list
        }
    }
}
