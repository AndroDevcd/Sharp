/*
 * CallSite - is the caret inside the arguments of a call, and in which argument?
 *
 * WHAT THIS IS FOR
 *   Parameter info (the hint `(element: string)` with the argument being typed in bold) needs two facts:
 *   which call the caret is in, and the number of the argument. Both come from the tokens before the
 *   caret: walk back to the `(` that is still open, check that a NAME stands before it (not `if (`,
 *   `while (`, `for (`, `foreach (`, `catch (`, `switch (`, a lambda's `(`), and count the commas at
 *   depth zero between that `(` and the caret.
 *
 *       owner.add(first, sec|ond)      the call `add`, argument 1
 *       new book(|                     the call `book` (a constructor), argument 0
 */
package sharp.ide.completion

import sharp.ide.lexer.CodeTokens
import sharp.ide.lexer.SharpTokenKinds

/** The call the caret is in: `code[parenIndex]` is its `(`, `code[nameIndex]` the name before it. */
class CallSiteInfo(val parenIndex: Int, val nameIndex: Int, val argumentIndex: Int, val parenOffset: Int)

object CallSite {
    private val notCalls = setOf("if", "while", "for", "foreach", "catch", "switch", "when", "lock", "return", "sizeof", "asm")

    fun at(tokens: CodeTokens, caret: Int): CallSiteInfo? {
        val code = tokens.code
        var end = code.size
        while (end > 0 && code[end - 1].start >= caret) end--         // only what is before the caret
        var depth = 0
        var commas = 0
        var i = end - 1
        while (i >= 0) {
            val t = code[i]
            when (t.type) {
                SharpTokenKinds.RPAREN, SharpTokenKinds.RBRACKET, SharpTokenKinds.RBRACE -> depth++
                SharpTokenKinds.LBRACKET -> if (depth > 0) depth--
                SharpTokenKinds.LBRACE -> if (depth == 0) return null else depth--
                SharpTokenKinds.SEMICOLON -> if (depth == 0) return null
                SharpTokenKinds.COMMA -> if (depth == 0) commas++
                SharpTokenKinds.LPAREN -> {
                    if (depth > 0) depth-- else return callBefore(code, i, commas)
                }
            }
            i--
        }
        return null
    }

    private fun callBefore(code: List<sharp.ide.lexer.Tok>, paren: Int, commas: Int): CallSiteInfo? {
        var nameIndex = paren - 1
        // `new holder<string>(`: step back over the type arguments to the class name
        if (nameIndex >= 0 && (code[nameIndex].isMark(">") || code[nameIndex].isMark(">>"))) {
            var depth = 0
            while (nameIndex >= 0) {
                val t = code[nameIndex]
                if (t.isMark(">")) depth++ else if (t.isMark(">>")) depth += 2
                else if (t.isMark("<")) { depth--; if (depth == 0) break }
                else if (t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.LBRACE) return null
                nameIndex--
            }
            nameIndex--
        }
        val name = code.getOrNull(nameIndex) ?: return null
        if (!name.isIdentifier || name.text in notCalls) return null
        return CallSiteInfo(paren, nameIndex, commas, code[paren].start)
    }
}
