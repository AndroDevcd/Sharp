/*
 * CodeTokens - the text as a list of tokens, for the features that read code backwards.
 *
 * WHAT THIS IS FOR
 *   Completion has to answer "what is before the caret": is it `names.`, `import ( std.`,
 *   `x : `, `new `? The editor's own PSI is flat (the compiler is the parser, doc/ide.md
 *   6.1), so the features read the TOKENS: the same SharpLexer the highlighter uses, run
 *   over the current text into a plain list that can be walked in both directions.
 *
 *   `all` keeps everything (white space and comments, so a caret inside a comment is
 *   recognised); `code` is the same list without white space and comments, which is what
 *   the backwards scans want.
 *
 *   Positions are offsets into the text that was scanned.
 */
package sharp.ide.lexer

import com.intellij.psi.TokenType
import com.intellij.psi.tree.IElementType

class Tok(val type: IElementType, val start: Int, val end: Int, val text: String) {
    val isIdentifier: Boolean get() = type == SharpTokenKinds.IDENTIFIER
    fun isKeyword(word: String): Boolean = SharpTokenKinds.KEYWORDS.contains(type) && text == word
    /** An operator or punctuation token with exactly this text. */
    fun isMark(mark: String): Boolean = text == mark && !isIdentifier && !SharpTokenKinds.KEYWORDS.contains(type)
    val isDot: Boolean get() = type == SharpTokenKinds.DOT
    override fun toString(): String = "$type[$text]"
}

class CodeTokens(val all: List<Tok>) {
    /** The tokens that are code: no white space, no comments. */
    val code: List<Tok> = all.filter { it.type != TokenType.WHITE_SPACE && !SharpTokenKinds.COMMENTS.contains(it.type) }

    /** The index in `code` of the first token that ends after `offset`; `code.size` when there is none. */
    fun firstCodeEndingAfter(offset: Int): Int {
        var lo = 0
        var hi = code.size
        while (lo < hi) {
            val mid = (lo + hi) ushr 1
            if (code[mid].end > offset) hi = mid else lo = mid + 1
        }
        return lo
    }

    companion object {
        fun scan(text: CharSequence): CodeTokens {
            val lexer = SharpLexer()
            lexer.start(text, 0, text.length, 0)
            val tokens = ArrayList<Tok>()
            while (true) {
                val type: IElementType = lexer.tokenType ?: break
                tokens.add(Tok(type, lexer.tokenStart, lexer.tokenEnd, text.subSequence(lexer.tokenStart, lexer.tokenEnd).toString()))
                lexer.advance()
            }
            return CodeTokens(tokens)
        }
    }
}
