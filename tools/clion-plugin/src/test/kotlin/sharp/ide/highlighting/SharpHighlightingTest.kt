package sharp.ide.highlighting

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import sharp.ide.lexer.SharpLexer
import sharp.ide.lexer.SharpTokenKinds

/**
 * HIGHLIGHTING FIXTURES: WHICH COLOUR SLOT EACH PIECE OF A SMALL PROGRAM GETS
 *
 * Runs the highlighter's own lexer over a text and prints every coloured piece as
 * `text -> SLOT`. Whitespace has no slot and is left out. The expectations are
 * written by hand, one fixture per colour group.
 */
class SharpHighlightingTest {
    private fun colours(text: String): List<String> {
        val highlighter = SharpSyntaxHighlighter()
        val lexer = highlighter.highlightingLexer
        lexer.start(text)
        val out = ArrayList<String>()
        while (lexer.tokenType != null) {
            val type = lexer.tokenType!!
            if (type != SharpTokenKinds.WHITE_SPACE) {
                val keys = highlighter.getTokenHighlights(type)
                val name = keys.joinToString("+") { it.externalName.removePrefix("SHARP_") }
                out.add("${text.substring(lexer.tokenStart, lexer.tokenEnd)} -> $name")
            }
            lexer.advance()
        }
        return out
    }

    @Test
    fun keywordsByGroup() {
        assertEquals(
            listOf(
                "class -> KEYWORD_DECLARATION", "def -> KEYWORD_DECLARATION",
                "if -> KEYWORD_CONTROL", "return -> KEYWORD_CONTROL",
                "static -> KEYWORD_MODIFIER", "private -> KEYWORD_MODIFIER",
                "_int64 -> BUILT_IN_TYPE", "bool -> BUILT_IN_TYPE", "decimal -> BUILT_IN_TYPE",
                "true -> BUILT_IN_VALUE", "null -> BUILT_IN_VALUE", "self -> BUILT_IN_VALUE",
                "new -> KEYWORD_OPERATOR", "as -> KEYWORD_OPERATOR",
                "name -> IDENTIFIER",
            ),
            colours("class def if return static private _int64 bool decimal true null self new as name"),
        )
    }

    @Test
    fun numbersAndCharacters() {
        assertEquals(
            listOf("12 -> NUMBER", "0xFF -> NUMBER", "1.5e3 -> NUMBER", "'a' -> CHARACTER"),
            colours("12 0xFF 1.5e3 'a'"),
        )
    }

    @Test
    fun stringWithInterpolationAndEscapes() {
        assertEquals(
            listOf(
                "\"a -> STRING", "\\n -> STRING_ESCAPE", "\$name -> INTERPOLATION_NAME",
                // the piece between `$name` and `${` is a single blank
                "  -> STRING",
                "\${ -> INTERPOLATION_DELIMITER", "x -> IDENTIFIER", "} -> INTERPOLATION_DELIMITER",
                "\$\$ -> STRING_ESCAPE", "\" -> STRING",
            ),
            colours("\"a\\n\$name \${x}\$\$\""),
        )
    }

    @Test
    fun commentsAndPunctuation() {
        assertEquals(
            listOf(
                "// note -> LINE_COMMENT", "/* b */ -> BLOCK_COMMENT", "/** d */ -> DOC_COMMENT",
                "{ -> BRACES", "} -> BRACES", "( -> PARENTHESES", ") -> PARENTHESES", "[ -> BRACKETS", "] -> BRACKETS",
                ", -> COMMA", "; -> SEMICOLON", ". -> DOT", "?. -> DOT", "+ -> OPERATOR", ":= -> OPERATOR",
            ),
            colours("// note\n/* b */ /** d */ { } ( ) [ ] , ; . ?. + :="),
        )
    }

    @Test
    fun aBadCharacterIsMarked() {
        assertEquals(listOf("` -> BAD_CHARACTER"), colours("`"))
    }

    @Test
    fun everyTokenKindHasASlotExceptWhitespace() {
        val highlighter = SharpSyntaxHighlighter()
        val kinds = listOf(
            SharpTokenKinds.IDENTIFIER, SharpTokenKinds.INTEGER, SharpTokenKinds.DECIMAL, SharpTokenKinds.HEX,
            SharpTokenKinds.CHAR, SharpTokenKinds.STRING_TEXT, SharpTokenKinds.STRING_ESCAPE, SharpTokenKinds.INTERP_NAME,
            SharpTokenKinds.INTERP_OPEN, SharpTokenKinds.INTERP_CLOSE, SharpTokenKinds.LINE_COMMENT,
            SharpTokenKinds.BLOCK_COMMENT, SharpTokenKinds.DOC_COMMENT, SharpTokenKinds.LBRACE, SharpTokenKinds.RBRACE,
            SharpTokenKinds.LPAREN, SharpTokenKinds.RPAREN, SharpTokenKinds.LBRACKET, SharpTokenKinds.RBRACKET,
            SharpTokenKinds.COMMA, SharpTokenKinds.SEMICOLON, SharpTokenKinds.DOT, SharpTokenKinds.OPERATOR,
            SharpTokenKinds.KEYWORD_DECLARATION, SharpTokenKinds.KEYWORD_CONTROL, SharpTokenKinds.KEYWORD_MODIFIER,
            SharpTokenKinds.KEYWORD_TYPE, SharpTokenKinds.KEYWORD_VALUE, SharpTokenKinds.KEYWORD_OPERATOR,
            SharpTokenKinds.BUILT_IN_CALL, SharpTokenKinds.TESTING_WORD,
            SharpTokenKinds.BAD_CHARACTER,
        )
        for (kind in kinds) assertTrue("$kind has no colour slot", highlighter.getTokenHighlights(kind).isNotEmpty())
        assertTrue(highlighter.getTokenHighlights(SharpTokenKinds.WHITE_SPACE).isEmpty())
    }

    @Test
    fun theDemoTextOfTheColourPageUsesEverySlotWithoutBadCharacters() {
        val demo = SharpColorSettingsPage().demoText
        val lexer = SharpLexer()
        lexer.start(demo)
        val seen = HashSet<String>()
        val highlighter = SharpSyntaxHighlighter()
        while (lexer.tokenType != null) {
            assertTrue("bad character in the demo text at ${lexer.tokenStart}", lexer.tokenType != SharpTokenKinds.BAD_CHARACTER)
            highlighter.getTokenHighlights(lexer.tokenType).forEach { seen.add(it.externalName) }
            lexer.advance()
        }
        val missing = SharpColorSettingsPage().attributeDescriptors.map { it.key.externalName }.filter { it !in seen && it != "SHARP_BAD_CHARACTER" }   // the demo is good code: no bad character
        assertTrue("slots the demo text never shows: $missing", missing.isEmpty())
    }
}
