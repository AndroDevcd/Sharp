package sharp.ide.language

import com.intellij.extapi.psi.ASTWrapperPsiElement
import com.intellij.lang.ASTNode
import com.intellij.lang.ParserDefinition
import com.intellij.lang.PsiBuilder
import com.intellij.lang.PsiParser
import com.intellij.lexer.Lexer
import com.intellij.openapi.project.Project
import com.intellij.psi.FileViewProvider
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import com.intellij.psi.tree.IElementType
import com.intellij.psi.tree.IFileElementType
import com.intellij.psi.tree.TokenSet
import sharp.ide.lexer.SharpLexer
import sharp.ide.lexer.SharpTokenKinds

/**
 * THE PARSER THE PLATFORM INSISTS ON - AS SMALL AS POSSIBLE
 *
 * The platform wants every language to say how its text becomes a tree (PSI).
 * Normally that is a grammar-driven parser. Here the compiler is the parser: all
 * knowledge about the program comes from the index it writes (doc/ide.md). So
 * this "parser" only makes a flat file of tokens, with one cheap piece of
 * structure: every matched pair of curly braces becomes a BLOCK element.
 *
 * It never fails. A closing brace with no opening one before it, and an opening
 * brace never closed, simply stay plain tokens (an unclosed block ends at the
 * end of the file).
 *
 * Registered as `lang.parserDefinition`.
 */
class SharpParserDefinition : ParserDefinition {
    override fun createLexer(project: Project?): Lexer = SharpLexer()

    override fun createParser(project: Project?): PsiParser = PsiParser { root, builder ->
        parseFile(root, builder)
        builder.treeBuilt
    }

    override fun getFileNodeType(): IFileElementType = SharpElementTypes.FILE

    override fun getCommentTokens(): TokenSet = SharpTokenKinds.COMMENTS

    override fun getStringLiteralElements(): TokenSet = SharpTokenKinds.STRINGS

    override fun createElement(node: ASTNode): PsiElement = ASTWrapperPsiElement(node)

    override fun createFile(viewProvider: FileViewProvider): PsiFile = SharpFile(viewProvider)

    private fun parseFile(root: IElementType, builder: PsiBuilder) {
        val fileMarker = builder.mark()
        // One marker per opening brace that is still open. They are closed in the
        // reverse order they were opened, which is what the platform requires.
        val openBlocks = ArrayDeque<PsiBuilder.Marker>()
        while (!builder.eof()) {
            when (builder.tokenType) {
                SharpTokenKinds.LBRACE -> {
                    openBlocks.addLast(builder.mark())
                    builder.advanceLexer()
                }
                SharpTokenKinds.RBRACE -> {
                    builder.advanceLexer()
                    openBlocks.removeLastOrNull()?.done(SharpElementTypes.BLOCK)
                }
                else -> builder.advanceLexer()
            }
        }
        while (openBlocks.isNotEmpty()) openBlocks.removeLast().done(SharpElementTypes.BLOCK)
        fileMarker.done(root)
    }
}
