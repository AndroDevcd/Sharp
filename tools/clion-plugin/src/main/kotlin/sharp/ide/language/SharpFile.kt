package sharp.ide.language

import com.intellij.extapi.psi.PsiFileBase
import com.intellij.openapi.fileTypes.FileType
import com.intellij.psi.FileViewProvider

/**
 * ONE SHARP FILE AS THE PLATFORM SEES IT
 *
 * PSI is the platform's name for a syntax tree. We do not write a parser (the
 * compiler is the parser), so the tree under this file is flat: its tokens, plus
 * a BLOCK element for every { } pair (see SharpParserDefinition).
 */
class SharpFile(viewProvider: FileViewProvider) : PsiFileBase(viewProvider, SharpLanguage) {
    override fun getFileType(): FileType = SharpFileType

    override fun toString(): String = "Sharp file"
}
