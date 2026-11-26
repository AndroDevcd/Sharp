package sharp.ide.lexer

import com.intellij.psi.TokenType
import com.intellij.psi.tree.IElementType
import com.intellij.psi.tree.TokenSet
import sharp.ide.language.SharpLanguage

/**
 * THE KINDS OF TOKEN THE LEXER PRODUCES
 *
 * An IElementType is the platform's "kind" of a piece of text. The highlighter
 * colours by kind, the parser definition names the comment and string kinds, the
 * brace matcher names the bracket kinds.
 *
 * The compiler's lexer (compiler/frontend/lexer) has one kind per
 * keyword and per operator, because its parser needs them. The editor only needs
 * to know how to COLOUR, so related things share a kind: the 67 keywords fall
 * into six groups, all the operators into one.
 *
 * Strings are cut into finer pieces than in the compiler, so that escapes and
 * interpolations can be coloured on their own:
 *
 *      "total: ${a + b} $unit\n"
 *
 *      STRING_TEXT    "total:                  (the opening quote and the text)
 *      INTERP_OPEN    ${
 *      a  +  b        ordinary tokens
 *      INTERP_CLOSE   }
 *      STRING_TEXT    (one space)
 *      INTERP_NAME    $unit                    (the dollar belongs to the token)
 *      STRING_ESCAPE  \n
 *      STRING_TEXT    "                        (the closing quote)
 *
 * Whitespace is TokenType.WHITE_SPACE; a character that starts nothing is
 * TokenType.BAD_CHARACTER.
 */
class SharpTokenType(debugName: String) : IElementType(debugName, SharpLanguage)

object SharpTokenKinds {
    // ---- names and literals
    @JvmField val IDENTIFIER = SharpTokenType("IDENTIFIER")
    @JvmField val INTEGER = SharpTokenType("INTEGER")
    @JvmField val DECIMAL = SharpTokenType("DECIMAL")
    @JvmField val HEX = SharpTokenType("HEX")
    @JvmField val CHAR = SharpTokenType("CHAR")

    // ---- strings (see the picture above)
    @JvmField val STRING_TEXT = SharpTokenType("STRING_TEXT")
    @JvmField val STRING_ESCAPE = SharpTokenType("STRING_ESCAPE")
    @JvmField val INTERP_NAME = SharpTokenType("INTERP_NAME")
    @JvmField val INTERP_OPEN = SharpTokenType("INTERP_OPEN")
    @JvmField val INTERP_CLOSE = SharpTokenType("INTERP_CLOSE")

    // ---- comments
    @JvmField val LINE_COMMENT = SharpTokenType("LINE_COMMENT")
    @JvmField val BLOCK_COMMENT = SharpTokenType("BLOCK_COMMENT")
    @JvmField val DOC_COMMENT = SharpTokenType("DOC_COMMENT")

    // ---- keywords, by group (the groups are listed in SharpKeywords)
    @JvmField val KEYWORD_DECLARATION = SharpTokenType("KEYWORD_DECLARATION")
    @JvmField val KEYWORD_CONTROL = SharpTokenType("KEYWORD_CONTROL")
    @JvmField val KEYWORD_MODIFIER = SharpTokenType("KEYWORD_MODIFIER")
    @JvmField val KEYWORD_TYPE = SharpTokenType("KEYWORD_TYPE")
    @JvmField val KEYWORD_VALUE = SharpTokenType("KEYWORD_VALUE")
    @JvmField val KEYWORD_OPERATOR = SharpTokenType("KEYWORD_OPERATOR")

    // ---- names that read like part of the language where they stand (SharpBuiltInWords)
    @JvmField val BUILT_IN_CALL = SharpTokenType("BUILT_IN_CALL")       // mock<t>()
    @JvmField val TESTING_WORD = SharpTokenType("TESTING_WORD")         // tests, test, expect, given, verify ...
    @JvmField val C_TYPE = SharpTokenType("C_TYPE")                     // c_int, c_float, pointer ... (std.c)
    @JvmField val CONTEXT_KEYWORD = SharpTokenType("CONTEXT_KEYWORD")   // extern "library" { }

    /** every kind a NAME can have: a plain identifier, or one of those above (they are still names) */
    @JvmField val NAMES = TokenSet.create(IDENTIFIER, BUILT_IN_CALL, TESTING_WORD, C_TYPE, CONTEXT_KEYWORD)

    // ---- punctuation
    @JvmField val LBRACE = SharpTokenType("LBRACE")
    @JvmField val RBRACE = SharpTokenType("RBRACE")
    @JvmField val LPAREN = SharpTokenType("LPAREN")
    @JvmField val RPAREN = SharpTokenType("RPAREN")
    @JvmField val LBRACKET = SharpTokenType("LBRACKET")
    @JvmField val RBRACKET = SharpTokenType("RBRACKET")
    @JvmField val COMMA = SharpTokenType("COMMA")
    @JvmField val SEMICOLON = SharpTokenType("SEMICOLON")
    /** `.`  `?.`  `!!.` */
    @JvmField val DOT = SharpTokenType("DOT")
    /** every other operator and mark: `+ - == := -> ?: ## ...` */
    @JvmField val OPERATOR = SharpTokenType("OPERATOR")

    @JvmField val WHITE_SPACE: IElementType = TokenType.WHITE_SPACE
    @JvmField val BAD_CHARACTER: IElementType = TokenType.BAD_CHARACTER

    @JvmField val COMMENTS = TokenSet.create(LINE_COMMENT, BLOCK_COMMENT, DOC_COMMENT)
    @JvmField val STRINGS = TokenSet.create(STRING_TEXT, STRING_ESCAPE, CHAR)
    @JvmField val KEYWORDS = TokenSet.create(
        KEYWORD_DECLARATION, KEYWORD_CONTROL, KEYWORD_MODIFIER,
        KEYWORD_TYPE, KEYWORD_VALUE, KEYWORD_OPERATOR,
    )
}
