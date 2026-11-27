package sharp.ide.lexer

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.File

/**
 * THE LEXER ON THE COMPILER'S OWN SHARP CODE
 *
 * The standard library (stdlib), the files of the language's suite
 * (tests/suite) and the self-hosted compiler (compiler) are all
 * code the compiler accepts. For every file:
 *   1. the tokens cover every character exactly once, in order (so putting the
 *      token texts together gives the file back);
 *   2. there is no BAD_CHARACTER;
 *   3. restarting at every state-0 token gives the same tokens as one pass
 *      (what the editor relies on after an edit);
 *   4. no string runs into the next line (the compiler would have rejected it).
 */
class SharpLexerOnRealFilesTest {
    private fun check(files: List<File>, what: String) {
        assertTrue("no files found for $what", files.isNotEmpty())
        for (file in files) {
            val text = file.readText(Charsets.UTF_8)
            val tokens = lex(text)

            var expectedStart = 0
            for (token in tokens) {
                assertEquals("${file.path}: gap or overlap before ${token}", expectedStart, token.start)
                assertTrue("${file.path}: empty token ${token.type} at ${token.start}", token.end > token.start)
                expectedStart = token.end
            }
            assertEquals("${file.path}: tokens stop early", text.length, expectedStart)

            val bad = tokens.firstOrNull { it.type == SharpTokenKinds.BAD_CHARACTER }
            assertTrue("${file.path}: bad character ${bad?.text} at ${bad?.start}", bad == null)

            // an unterminated string would show as a string-text token that does not end in a quote
            // and is followed directly by a line end
            for ((i, token) in tokens.withIndex()) {
                if (token.type == SharpTokenKinds.STRING_TEXT && token.start > 0 && text[token.start] == '"' && !token.text.endsWith("\"")) {
                    val next = tokens.getOrNull(i + 1)
                    val continues = next != null && (next.type == SharpTokenKinds.STRING_ESCAPE || next.type == SharpTokenKinds.INTERP_NAME || next.type == SharpTokenKinds.INTERP_OPEN)
                    assertTrue("${file.path}: unterminated string at ${token.start}", continues)
                }
            }

            // restarting at state-0 tokens (checked at a sample of them: every 97th, to stay fast)
            val zeroes = tokens.filter { it.state == 0 }
            for (token in zeroes.filterIndexed { index, _ -> index % 97 == 0 }) {
                val lexer = SharpLexer()
                lexer.start(text, token.start, text.length, 0)
                val restarted = ArrayList<Pair<Int, Int>>()
                while (lexer.tokenType != null) {
                    restarted.add(lexer.tokenStart to lexer.tokenEnd)
                    lexer.advance()
                }
                val expected = tokens.filter { it.start >= token.start }.map { it.start to it.end }
                assertEquals("${file.path}: restart at ${token.start}", expected, restarted)
            }
        }
    }

    @Test
    fun standardLibrary() = check(sharpFilesBelow(File(repoRoot(), "stdlib")), "the standard library")

    @Test
    fun suiteFiles() {
        // (known_bugs may hold programs that do not compile, on purpose)
        val files = sharpFilesBelow(File(repoRoot(), "tests/suite")).filter { !it.invariantSeparatorsPath.contains("/known_bugs/") }
        check(files, "the suite")
    }

    @Test
    fun selfHostedCompiler() {
        val root = File(repoRoot(), "compiler")
        // the compiler's tests include deliberately broken programs
        val files = sharpFilesBelow(root).filter { !it.invariantSeparatorsPath.contains("/tests/") }
        check(files, "the bootstrap compiler")
    }
}
