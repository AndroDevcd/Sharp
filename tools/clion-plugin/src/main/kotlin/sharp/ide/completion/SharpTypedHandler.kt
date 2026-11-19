/*
 * SharpTypedHandler - opens the completion list by itself after a `.`
 *
 * WHAT THIS IS FOR
 *   By itself the platform opens the completion list only while letters, digits or `_` are
 *   typed. In Sharp the list matters most right after a `.` (also `?.` and `!!.`), after the `#`
 *   of `std.io#`, and after the `->` of `self->` / `base->`. A TypedHandlerDelegate sees every
 *   typed character before the editor handles it; `checkAutoPopup` is the documented place to
 *   ask for the pop-up (doc/ide.md 6.8):
 *
 *       AutoPopupController.scheduleAutoPopup(editor)   "open the list soon, as if Ctrl+Space"
 *
 *   Ctrl+Space always works as well. Only Sharp files are touched.
 */
package sharp.ide.completion

import com.intellij.codeInsight.AutoPopupController
import com.intellij.codeInsight.editorActions.TypedHandlerDelegate
import com.intellij.openapi.editor.Editor
import com.intellij.openapi.project.Project
import com.intellij.psi.PsiFile
import sharp.ide.language.SharpLanguage

class SharpTypedHandler : TypedHandlerDelegate() {

    override fun checkAutoPopup(charTyped: Char, project: Project, editor: Editor, file: PsiFile): Result {
        if (file.language != SharpLanguage) return Result.CONTINUE
        val opensList = when (charTyped) {
            '.', '#' -> true
            '>' -> {
                // `->` after self or base; the `>` is not in the document yet, the `-` is
                val offset = editor.caretModel.offset
                val text = editor.document.charsSequence
                offset >= 1 && text[offset - 1] == '-'
            }
            else -> false
        }
        if (!opensList) return Result.CONTINUE
        AutoPopupController.getInstance(project).scheduleAutoPopup(editor)
        return Result.STOP
    }
}
