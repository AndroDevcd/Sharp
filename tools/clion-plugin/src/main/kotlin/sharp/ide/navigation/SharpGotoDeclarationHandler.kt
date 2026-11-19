/*
 * SharpGotoDeclarationHandler - Ctrl+click and Ctrl+B on a name.
 *
 * WHAT THIS IS FOR
 *   A GotoDeclarationHandler is the platform's socket for "take me to where this is declared". It is
 *   asked with the leaf under the caret and answers with the PSI elements to jump to. Sharp's PSI is
 *   flat (the compiler is the parser, doc/ide.md 6.1), so there are no reference objects; the answer comes
 *   from the compiler's table of uses (NavigationTargets) and is returned as a SharpJumpTarget: an
 *   element that stands for the token at the target and opens the file at exactly that offset itself.
 *   (The first version returned the token and let the platform find the place; in CLion that opened
 *   the file at its top.) The same handler makes Ctrl+hover underline the name.
 *
 *   The target's offset is in the text the compiler read; the target file may have been edited since,
 *   so it is moved with an OffsetMap (3.6): a file that is not open is adopted when its hash still
 *   matches the run (the usual case for a library file); when nothing can be said about the text, the
 *   answer is "nowhere" rather than a wrong place.
 *
 *   Never throws, never opens a popup of its own.
 */
package sharp.ide.navigation

import com.intellij.codeInsight.navigation.actions.GotoDeclarationHandler
import com.intellij.openapi.diagnostic.Logger
import com.intellij.openapi.editor.Editor
import com.intellij.openapi.progress.ProcessCanceledException
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiManager
import sharp.ide.editor.FileViews

class SharpGotoDeclarationHandler : GotoDeclarationHandler {

    override fun getGotoDeclarationTargets(sourceElement: PsiElement?, offset: Int, editor: Editor): Array<PsiElement>? {
        try {
            val file = sourceElement?.containingFile ?: return null
            val virtualFile = file.virtualFile ?: return null
            val project = file.project
            val place = targetFor(project, virtualFile, editor.document.text, offset) ?: return null
            val targetPsi = PsiManager.getInstance(project).findFile(place.first) ?: return null
            val at = place.second.coerceIn(0, maxOf(0, targetPsi.textLength - 1))
            val leaf = targetPsi.findElementAt(at) ?: return null
            // not the token itself: an element that carries the exact place (SharpJumpTarget says why)
            return arrayOf(SharpJumpTarget(leaf, place.first, at))
        } catch (e: ProcessCanceledException) {
            throw e
        } catch (e: Throwable) {
            LOG.debug("Sharp go-to failed quietly", e)
            return null
        }
    }

    companion object {
        private val LOG = Logger.getInstance(SharpGotoDeclarationHandler::class.java)

        /** The file and the offset (in that file's text now) where the name at `offset` of `file` is declared; null when unknown. */
        fun targetFor(project: Project, file: VirtualFile, currentText: String, offset: Int): Pair<VirtualFile, Int>? {
            val view = FileViews.forFile(project, file, currentText) ?: return null
            val target = NavigationTargets.at(view, offset) ?: return null
            if (target.file == view.file) {
                val at = view.toCurrent(target.from) ?: return null
                return file to at
            }
            return TargetPlaces.resolve(project, view.index, target.file, target.from)
        }
    }
}
