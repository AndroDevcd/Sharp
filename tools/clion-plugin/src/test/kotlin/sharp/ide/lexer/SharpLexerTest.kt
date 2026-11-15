package sharp.ide.lexer

import org.junit.Assert.assertEquals
import org.junit.Test

/**
 * HAND-WRITTEN EXPECTATIONS FOR THE TRICKY CASES OF THE LEXER
 *
 * Each test names a rule of the compiler's lexer (frontend/lexer/lexer.sharp)
 * and checks that SharpLexer cuts the text the same way. Tokens print as
 * KIND[text]; whitespace is left out.
 */
class SharpLexerTest {
    @Test
    fun numbers() {
        assertEquals(
            listOf("INTEGER[12]", "INTEGER[1_000]", "HEX[0xFF_ff]", "DECIMAL[1.5]", "DECIMAL[1e9]", "DECIMAL[2.5E-3]", "HEX[0x]"),
            kinds("12 1_000 0xFF_ff 1.5 1e9 2.5E-3 0x"),
        )
    }

    @Test
    fun aDotJoinsANumberOnlyWhenADigitFollows() {
        assertEquals(
            listOf("INTEGER[5]", "DOT[.]", "IDENTIFIER[to_string]", "LPAREN[(]", "RPAREN[)]"),
            kinds("5.to_string()"),
        )
    }

    @Test
    fun anExponentNeedsDigits() {
        assertEquals(listOf("INTEGER[2]", "IDENTIFIER[e]"), kinds("2e"))
        assertEquals(listOf("INTEGER[2]", "IDENTIFIER[e]", "OPERATOR[+]"), kinds("2e+"))
    }

    @Test
    fun characters() {
        assertEquals(listOf("CHAR['a']", "CHAR['\\n']", "CHAR['\\'']", "CHAR['\"']"), kinds("'a' '\\n' '\\'' '\"'"))
    }

    @Test
    fun plainStringWithEscapes() {
        assertEquals(
            listOf("STRING_TEXT[\"a]", "STRING_ESCAPE[\\n]", "STRING_TEXT[b]", "STRING_ESCAPE[\\\"]", "STRING_TEXT[c\"]"),
            kinds("\"a\\nb\\\"c\""),
        )
    }

    @Test
    fun dollarForms() {
        assertEquals(
            listOf(
                "STRING_TEXT[\"cost ]", "STRING_ESCAPE[\$\$]", "STRING_TEXT[5 for ]", "INTERP_NAME[\$unit]",
                "STRING_TEXT[ and ]", "INTERP_OPEN[\${]", "IDENTIFIER[a]", "OPERATOR[+]", "IDENTIFIER[b]",
                "INTERP_CLOSE[}]", "STRING_TEXT[\"]",
            ),
            kinds("\"cost \$\$5 for \$unit and \${a + b}\""),
        )
    }

    @Test
    fun aDollarNotFollowedByANameIsPlainText() {
        assertEquals(listOf("STRING_TEXT[\"5\$ \$1 \$\"]"), kinds("\"5\$ \$1 \$\""))
    }

    @Test
    fun nestedInterpolation() {
        // "${ "${x}" }"
        val tokens = kinds("\"\${ \"\${x}\" }\"")
        assertEquals(
            listOf(
                "STRING_TEXT[\"]", "INTERP_OPEN[\${]",
                "STRING_TEXT[\"]", "INTERP_OPEN[\${]", "IDENTIFIER[x]", "INTERP_CLOSE[}]", "STRING_TEXT[\"]",
                "INTERP_CLOSE[}]", "STRING_TEXT[\"]",
            ),
            tokens,
        )
    }

    @Test
    fun aLambdasBracesInsideAnInterpolationDoNotEndIt() {
        assertEquals(
            listOf(
                "STRING_TEXT[\"]", "INTERP_OPEN[\${]", "LBRACE[{]", "IDENTIFIER[x]", "RBRACE[}]",
                "INTERP_CLOSE[}]", "STRING_TEXT[ tail\"]",
            ),
            kinds("\"\${{x}} tail\""),
        )
    }

    @Test
    fun aStringContainingCommentMarkersIsJustAString() {
        assertEquals(listOf("STRING_TEXT[\"// not /* a comment */\"]"), kinds("\"// not /* a comment */\""))
    }

    @Test
    fun unterminatedStringStopsAtTheLineEnd() {
        assertEquals(listOf("STRING_TEXT[\"abc]", "IDENTIFIER[next]"), kinds("\"abc\nnext"))
    }

    @Test
    fun unfinishedInterpolationEndsAtTheLineEnd() {
        // the next line is ordinary code again, not "the inside of a string"
        assertEquals(
            listOf("STRING_TEXT[\"]", "INTERP_OPEN[\${]", "IDENTIFIER[x]", "IDENTIFIER[y]", "RBRACE[}]"),
            kinds("\"\${x\ny }"),
        )
    }

    @Test
    fun comments() {
        assertEquals(
            listOf("LINE_COMMENT[// to the end]", "IDENTIFIER[a]", "BLOCK_COMMENT[/* b */]", "DOC_COMMENT[/** c */]", "BLOCK_COMMENT[/**/]"),
            kinds("// to the end\na /* b */ /** c */ /**/"),
        )
    }

    @Test
    fun blockCommentsNest() {
        assertEquals(
            listOf("BLOCK_COMMENT[/* a /* b */ still a comment */]", "IDENTIFIER[x]"),
            kinds("/* a /* b */ still a comment */ x"),
        )
    }

    @Test
    fun unterminatedBlockCommentRunsToTheEnd() {
        assertEquals(listOf("IDENTIFIER[a]", "BLOCK_COMMENT[/* never closed\nstill]"), kinds("a /* never closed\nstill"))
    }

    @Test
    fun operatorsAndDots() {
        assertEquals(
            listOf(
                "IDENTIFIER[a]", "DOT[?.]", "IDENTIFIER[b]", "DOT[!!.]", "IDENTIFIER[c]", "OPERATOR[!!]",
                "OPERATOR[:=]", "OPERATOR[->]", "OPERATOR[?:]", "OPERATOR[**]", "OPERATOR[<<]", "OPERATOR[>>]",
                "OPERATOR[>=]", "OPERATOR[!=]", "OPERATOR[&&]", "OPERATOR[||]", "OPERATOR[++]", "OPERATOR[+=]",
            ),
            kinds("a?.b!!.c !! := -> ?: ** << >> >= != && || ++ +="),
        )
    }

    @Test
    fun genericsCloseWithTwoGreaterThanSigns() {
        assertEquals(
            listOf("IDENTIFIER[list]", "OPERATOR[<]", "IDENTIFIER[list]", "OPERATOR[<]", "IDENTIFIER[int]", "OPERATOR[>>]"),
            kinds("list<list<int>>"),
        )
    }

    @Test
    fun punctuation() {
        assertEquals(
            listOf("LPAREN[(]", "RPAREN[)]", "LBRACE[{]", "RBRACE[}]", "LBRACKET[[]", "RBRACKET[]]", "COMMA[,]", "SEMICOLON[;]", "OPERATOR[#]", "OPERATOR[@]", "OPERATOR[$]"),
            kinds("(){}[],;#@$"),
        )
    }

    @Test
    fun aCharacterThatStartsNothingIsBad() {
        assertEquals(listOf("IDENTIFIER[a]", "BAD_CHARACTER[`]", "BAD_CHARACTER[é]"), kinds("a ` é"))
    }

    @Test
    fun nonAsciiTextInsideStringsAndCommentsIsFine() {
        assertEquals(listOf("STRING_TEXT[\"héllo 😀\"]", "LINE_COMMENT[// über]"), kinds("\"héllo 😀\" // über"))
    }

    @Test
    fun stateIsZeroOutsideStringsAndOneInside() {
        val tokens = lex("a \"x \${b} y\" c")
        val states = tokens.filter { it.type != SharpTokenKinds.WHITE_SPACE }.map { it.text to it.state }
        assertEquals(
            listOf("a" to 0, "\"x " to 0, "\${" to 1, "b" to 1, "}" to 1, " y\"" to 1, "c" to 0),
            states,
        )
    }

    @Test
    fun restartingAtAStateZeroTokenGivesTheSameTokens() {
        // The platform restarts at an earlier token whose state was 0 (see SharpLexer).
        val text = "a := \"x \${f({1})} \\n y\"; /* c */ b := 'q'; \"end\"\nc"
        val whole = lex(text)
        for (token in whole.filter { it.state == 0 }) {
            val lexer = SharpLexer()
            lexer.start(text, token.start, text.length, 0)
            val rest = ArrayList<LexedToken>()
            while (lexer.tokenType != null) {
                rest.add(LexedToken(lexer.tokenType!!, lexer.tokenStart, lexer.tokenEnd, lexer.state, text.substring(lexer.tokenStart, lexer.tokenEnd)))
                lexer.advance()
            }
            assertEquals("restart at ${token.start}", whole.filter { it.start >= token.start }, rest)
        }
    }
}
