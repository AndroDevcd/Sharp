package sharp.ide.highlighting

import com.intellij.openapi.fileTypes.SyntaxHighlighter
import com.intellij.openapi.fileTypes.SyntaxHighlighterFactory
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile

/**
 * HANDS THE HIGHLIGHTER TO THE PLATFORM
 *
 * Registered as `lang.syntaxHighlighterFactory language="Sharp"`: the platform
 * asks it for the highlighter whenever it opens a Sharp file.
 */
class SharpSyntaxHighlighterFactory : SyntaxHighlighterFactory() {
    override fun getSyntaxHighlighter(project: Project?, virtualFile: VirtualFile?): SyntaxHighlighter =
        SharpSyntaxHighlighter()
}
