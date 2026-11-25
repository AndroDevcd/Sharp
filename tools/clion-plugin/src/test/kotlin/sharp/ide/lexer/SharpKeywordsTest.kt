package sharp.ide.lexer

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Test
import java.io.File

/**
 * THE KEYWORD LIST AGREES WITH THE COMPILER'S
 *
 * Reads `keyword_list()` out of the compiler's lexer source in the repository and
 * compares it with SharpKeywords. A keyword added to the compiler turns this red
 * until it is added to the plugin in the group where it belongs.
 */
class SharpKeywordsTest {
    private fun compilerKeywords(): List<String> {
        val source = File(repoRoot(), "compiler/frontend/lexer/lexer.sharp").readText()
        val start = source.indexOf("def keyword_list()")
        assert(start >= 0) { "keyword_list() not found in lexer.sharp" }
        val body = source.substring(start, source.indexOf('}', source.indexOf("return {", start)))
        return Regex("\"([^\"]+)\"").findAll(body.substringAfter("return {")).map { it.groupValues[1] }.toList()
    }

    @Test
    fun theWordsAreTheSame() {
        val compiler = compilerKeywords()
        assertEquals("the compiler has 68 keywords today", 68, compiler.size)
        assertEquals(compiler.toSet(), SharpKeywords.all)
    }

    @Test
    fun everyKeywordHasAKeywordKind() {
        for (word in compilerKeywords()) {
            val kind = SharpKeywords.kindOf(word)
            assertNotNull("keyword $word has no kind", kind)
            assert(kind in SharpTokenKinds.KEYWORDS) { "$word: $kind is not a keyword kind" }
        }
    }

    @Test
    fun aWordThatMerelyStartsWithAKeywordIsAnIdentifier() {
        assertEquals(listOf("IDENTIFIER[classes]", "IDENTIFIER[iffy]", "IDENTIFIER[_int64x]"), kinds("classes iffy _int64x"))
    }
}
