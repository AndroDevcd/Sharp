package sharp.ide.lexer

import com.intellij.psi.tree.IElementType
import java.io.File

/**
 * HELPERS SHARED BY THE LEXER TESTS
 *
 * `lex(text)` runs SharpLexer over a text and returns its tokens as a list;
 * `repoRoot()` finds the Sharp repository (the Gradle build passes its path as
 * the system property `sharp.repo`).
 */
data class LexedToken(val type: IElementType, val start: Int, val end: Int, val state: Int, val text: String) {
    override fun toString(): String = "${type}[$text]"
}

fun lex(text: CharSequence): List<LexedToken> {
    val lexer = SharpLexer()
    lexer.start(text, 0, text.length, 0)
    val tokens = ArrayList<LexedToken>()
    while (lexer.tokenType != null) {
        tokens.add(
            LexedToken(
                lexer.tokenType!!, lexer.tokenStart, lexer.tokenEnd, lexer.state,
                text.substring(lexer.tokenStart, lexer.tokenEnd),
            ),
        )
        lexer.advance()
    }
    return tokens
}

/** the tokens without whitespace, as `KIND[text]` strings, for readable expectations */
fun kinds(text: String): List<String> =
    lex(text).filter { it.type != SharpTokenKinds.WHITE_SPACE }.map { it.toString() }

fun repoRoot(): File = File(System.getProperty("sharp.repo") ?: error("system property sharp.repo is not set")).canonicalFile

fun sharpFilesBelow(folder: File): List<File> =
    folder.walkTopDown().filter { it.isFile && it.extension == "sharp" }.sortedBy { it.path }.toList()
