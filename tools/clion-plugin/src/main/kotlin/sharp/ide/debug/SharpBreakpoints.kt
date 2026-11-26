/*
 * BREAKPOINTS IN SHARP FILES
 *
 * Two things the platform asks for:
 *
 *   SharpLineBreakpointType     "a line of a .sharp file can hold a breakpoint": with it the gutter of a
 *                               Sharp file takes a click. It keeps nothing beyond the file and the line.
 *   SharpDebuggerEditorsProvider   the small editor the debugger opens for typed text - here only for
 *                               "Set Value". sdb names values by PATHS (`book.title`, `numbers[3]`) and
 *                               evaluates no expressions, so there is no expression field and no
 *                               evaluator; what is typed is plain text: `12`, `true`, `"bread"`, `null`.
 *
 * Whether a line really holds code is decided by sdb when the breakpoint is sent to it
 * (SharpDebugProcess): one on a comment or a blank line is shown as not usable, with sdb's reason.
 */
package sharp.ide.debug

import com.intellij.openapi.editor.Document
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.fileTypes.FileType
import com.intellij.openapi.fileTypes.PlainTextFileType
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.testFramework.LightVirtualFile
import com.intellij.xdebugger.XExpression
import com.intellij.xdebugger.XSourcePosition
import com.intellij.xdebugger.breakpoints.XBreakpointProperties
import com.intellij.xdebugger.breakpoints.XLineBreakpointTypeBase
import com.intellij.xdebugger.evaluation.EvaluationMode
import com.intellij.xdebugger.evaluation.XDebuggerEditorsProvider
import sharp.ide.language.SharpFileType

class SharpLineBreakpointType : XLineBreakpointTypeBase(ID, "Sharp Line Breakpoints", SharpDebuggerEditorsProvider) {
    override fun canPutAt(file: VirtualFile, line: Int, project: Project): Boolean = file.fileType === SharpFileType

    companion object {
        const val ID = "sharp-line"
    }
}

/** Breakpoints of this type carry nothing but their file and line. */
typealias SharpBreakpointProperties = XBreakpointProperties<*>

object SharpDebuggerEditorsProvider : XDebuggerEditorsProvider() {
    override fun getFileType(): FileType = PlainTextFileType.INSTANCE

    override fun createDocument(project: Project, expression: XExpression, sourcePosition: XSourcePosition?, mode: EvaluationMode): Document {
        val file = LightVirtualFile("sharp-value.txt", PlainTextFileType.INSTANCE, expression.expression)
        return FileDocumentManager.getInstance().getDocument(file)!!
    }

    /** There is nothing to evaluate: the Variables view shows no "evaluate expression" field. */
    override fun isEvaluateExpressionFieldEnabled(): Boolean = false
}
