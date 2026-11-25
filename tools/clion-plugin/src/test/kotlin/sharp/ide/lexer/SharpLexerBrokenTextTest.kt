package sharp.ide.lexer

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File
import java.util.Random

/**
 * THE LEXER ON HALF-TYPED TEXT
 *
 * An editor lexer sees broken code all the time: a string not yet closed, a `${`
 * with nothing after it, a file cut in the middle of a comment. The rule is that
 * it never fails, never loops, and still covers every character with exactly one
 * token. This test takes real files, cuts them at many places and also deletes /
 * inserts random characters, and checks that rule for every result.
 */
class SharpLexerBrokenTextTest {
    private fun assertCovers(text: String, what: String) {
        var expectedStart = 0
        for (token in lex(text)) {
            assertEquals("$what: gap or overlap", expectedStart, token.start)
            assertTrue("$what: empty token ${token.type} at ${token.start}", token.end > token.start)
            expectedStart = token.end
        }
        assertEquals("$what: tokens stop early", text.length, expectedStart)
    }

    @Test
    fun everyPrefixOfTrickyTextIsCovered() {
        val text = "x := \"a\\n \$\$ \$name \${ f({ 1 }) + \"in \${ y }\" } end\" + 'q' /* c /* d */ */ 0x1F 1.5e-3 !!. ?. // end\n"
        for (cut in 0..text.length) assertCovers(text.substring(0, cut), "prefix $cut")
    }

    @Test
    fun randomDamageToRealFiles() {
        val random = Random(42)
        val files = sharpFilesBelow(File(repoRoot(), "stdlib/std")).take(25)
        val noise = "\"'\\\$\${}/*#\né😀".toCharArray()
        for (file in files) {
            val original = file.readText(Charsets.UTF_8)
            repeat(6) {
                val damaged = StringBuilder(original)
                repeat(8) {
                    val at = random.nextInt(damaged.length + 1)
                    if (random.nextBoolean() && at < damaged.length) damaged.deleteCharAt(at)
                    else damaged.insert(at, noise[random.nextInt(noise.size)])
                }
                assertCovers(damaged.toString(), file.name)
            }
        }
    }
}
