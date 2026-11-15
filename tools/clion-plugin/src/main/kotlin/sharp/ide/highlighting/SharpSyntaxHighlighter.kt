package sharp.ide.highlighting

import com.intellij.lexer.Lexer
import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.fileTypes.SyntaxHighlighterBase
import com.intellij.psi.tree.IElementType
import sharp.ide.lexer.SharpLexer
import sharp.ide.lexer.SharpTokenKinds

/**
 * TOKEN KIND -> COLOUR SLOT
 *
 * The editor asks two questions: "which lexer cuts the text" (SharpLexer) and
 * "which colour slots does a token of this kind get" (the table below). That is
 * all of syntax highlighting; no compiler is involved.
 */
class SharpSyntaxHighlighter : SyntaxHighlighterBase() {
    override fun getHighlightingLexer(): Lexer = SharpLexer()

    override fun getTokenHighlights(tokenType: IElementType?): Array<TextAttributesKey> =
        pack(KEY_FOR_KIND[tokenType])

    private companion object {
        val KEY_FOR_KIND: Map<IElementType, TextAttributesKey> = mapOf(
            SharpTokenKinds.KEYWORD_DECLARATION to SharpColors.KEYWORD_DECLARATION,
            SharpTokenKinds.KEYWORD_CONTROL to SharpColors.KEYWORD_CONTROL,
            SharpTokenKinds.KEYWORD_MODIFIER to SharpColors.KEYWORD_MODIFIER,
            SharpTokenKinds.KEYWORD_OPERATOR to SharpColors.KEYWORD_OPERATOR,
            SharpTokenKinds.KEYWORD_TYPE to SharpColors.BUILT_IN_TYPE,
            SharpTokenKinds.KEYWORD_VALUE to SharpColors.BUILT_IN_VALUE,
            SharpTokenKinds.IDENTIFIER to SharpColors.IDENTIFIER,
            SharpTokenKinds.INTEGER to SharpColors.NUMBER,
            SharpTokenKinds.DECIMAL to SharpColors.NUMBER,
            SharpTokenKinds.HEX to SharpColors.NUMBER,
            SharpTokenKinds.CHAR to SharpColors.CHARACTER,
            SharpTokenKinds.STRING_TEXT to SharpColors.STRING,
            SharpTokenKinds.STRING_ESCAPE to SharpColors.STRING_ESCAPE,
            SharpTokenKinds.INTERP_OPEN to SharpColors.INTERPOLATION_DELIMITER,
            SharpTokenKinds.INTERP_CLOSE to SharpColors.INTERPOLATION_DELIMITER,
            SharpTokenKinds.INTERP_NAME to SharpColors.INTERPOLATION_NAME,
            SharpTokenKinds.LINE_COMMENT to SharpColors.LINE_COMMENT,
            SharpTokenKinds.BLOCK_COMMENT to SharpColors.BLOCK_COMMENT,
            SharpTokenKinds.DOC_COMMENT to SharpColors.DOC_COMMENT,
            SharpTokenKinds.OPERATOR to SharpColors.OPERATOR,
            SharpTokenKinds.LBRACE to SharpColors.BRACES,
            SharpTokenKinds.RBRACE to SharpColors.BRACES,
            SharpTokenKinds.LPAREN to SharpColors.PARENTHESES,
            SharpTokenKinds.RPAREN to SharpColors.PARENTHESES,
            SharpTokenKinds.LBRACKET to SharpColors.BRACKETS,
            SharpTokenKinds.RBRACKET to SharpColors.BRACKETS,
            SharpTokenKinds.COMMA to SharpColors.COMMA,
            SharpTokenKinds.SEMICOLON to SharpColors.SEMICOLON,
            SharpTokenKinds.DOT to SharpColors.DOT,
            SharpTokenKinds.BAD_CHARACTER to SharpColors.BAD_CHARACTER,
        )
    }
}
