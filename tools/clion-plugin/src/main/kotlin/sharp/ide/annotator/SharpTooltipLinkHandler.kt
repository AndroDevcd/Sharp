/*
 * SharpTooltipLinkHandler - makes the links inside an error tooltip jump somewhere.
 *
 * WHAT THIS IS FOR
 *   A diagnostic can name a second place ("the first one is here"). In the tooltip that
 *   place is a link whose address starts with `#sharp` (TooltipLinks). The platform
 *   hands every click on a tooltip link to the handler registered for its prefix; this
 *   one opens the file and puts the caret at the place.
 *
 *   The offset in the link is an offset in the text the compiler saw. The file may have
 *   been edited since, so it is moved onto the current text with the same OffsetMap the
 *   squiggles use; if the edit swallowed it, the caret goes to the nearest valid offset.
 *
 *   Registered in plugin.xml as `<codeInsight.linkHandler prefix="#sharp" .../>`.
 */
package sharp.ide.annotator

import com.intellij.codeInsight.highlighting.TooltipLinkHandler
import com.intellij.openapi.editor.Editor
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.fileEditor.OpenFileDescriptor
import com.intellij.openapi.vfs.LocalFileSystem
import sharp.ide.run.AnalysisService
import sharp.ide.settings.SharpApplicationSettings

class SharpTooltipLinkHandler : TooltipLinkHandler() {

    override fun handleLink(refSuffix: String, editor: Editor): Boolean {
        val project = editor.project ?: return false
        val (run, offset, compilerPath) = TooltipLinks.parse(refSuffix) ?: return false

        val translator = AnalysisService.translatorFor(SharpApplicationSettings.getInstance().state, project.basePath)
        val editorPath = translator?.toEditorPath(compilerPath) ?: compilerPath
        val file = LocalFileSystem.getInstance().findFileByPath(editorPath)
            ?: LocalFileSystem.getInstance().refreshAndFindFileByPath(editorPath)
            ?: return false

        val document = FileDocumentManager.getInstance().getDocument(file)
        var target = offset
        if (document != null) {
            val snapshot = AnalysisService.getInstance(project).snapshots.get(run, compilerPath)
            if (snapshot != null) target = OffsetMap(snapshot.text, document.charsSequence).mapPoint(offset) ?: target
            target = target.coerceIn(0, document.textLength)
        }
        OpenFileDescriptor(project, file, target).navigate(true)
        return true
    }
}
