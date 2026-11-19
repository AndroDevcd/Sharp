/*
 * CompletionSite - WHAT KIND of thing is being typed at the caret.
 *
 * WHAT THIS IS FOR
 *   Completion offers different lists in different places (doc/ide.md 6.8):
 *
 *      names.|            members of what `names` is          MEMBER
 *      std.io#|           members of a module                  MODULE_MEMBER
 *      import ( std.|     module names                        MODULE_NAME
 *      x : |   new |   as |   list<|      types                TYPE, TYPE_NEW
 *      c ? a : |          a type OR a name (`:` of a ternary)  TYPE_OR_NAME
 *      to|                a plain name                         PLAIN
 *      "inside text|"   // a comment|                           NOWHERE
 *
 *   It is decided by reading the TOKENS before the caret backwards (no index needed, except
 *   to tell `list<` from the comparison `a < b`: a name before `<` must be a known class).
 *   The identifier characters right before the caret are the PREFIX the person has typed;
 *   what stands before the prefix decides the site.
 *
 *   C++ comparison: this is the "completion context" a C++ IDE works out with its parser; here
 *   the compiler is the parser and the editor only has tokens, so it is a small backward scan.
 */
package sharp.ide.completion

import sharp.ide.lexer.CodeTokens
import sharp.ide.lexer.SharpTokenKinds
import sharp.ide.lexer.Tok

enum class SiteKind { NOWHERE, MODULE_NAME, MEMBER, MODULE_MEMBER, TYPE, TYPE_NEW, TYPE_OR_NAME, PLAIN }

class CompletionSite(
    val kind: SiteKind,
    /** What has been typed of the name (for a module name: the whole dotted run, `std.i`). */
    val prefix: String,
    val prefixStart: Int,
    /** The code tokens that end before the prefix, in order. */
    val before: List<Tok>,
    /** For MODULE_MEMBER: the module (or group) name before the `#`. */
    val qualifier: String? = null,
    /** For MEMBER after `self->` / `base->`. */
    val arrow: Boolean = false,
) {
    companion object {
        /**
         * Decides the site at `offset` of `text`. `isGenericClassName` tells whether a name is a known class
         * (to read `list<` as a type and `a < b` as a comparison).
         */
        fun of(text: CharSequence, offset: Int, tokens: CodeTokens, isGenericClassName: (String) -> Boolean): CompletionSite {
            val nowhere = CompletionSite(SiteKind.NOWHERE, "", offset, emptyList())
            if (offset < 0 || offset > text.length) return nowhere
            if (insideCommentOrString(text, offset, tokens)) return nowhere

            var prefixStart = offset
            while (prefixStart > 0 && isIdentifierChar(text[prefixStart - 1])) prefixStart--
            val before = tokens.code.filter { it.end <= prefixStart }
            val last = before.lastOrNull()

            // 1. inside `import ( ... )` or after `mod`: module names, where the dots are part of the name
            val moduleRun = dottedRunBefore(text, prefixStart, before)
            val runStart = moduleRun?.first ?: prefixStart
            val beforeRun = before.filter { it.end <= runStart }
            if (isInsideImportList(beforeRun) || beforeRun.lastOrNull()?.isKeyword("mod") == true) {
                return CompletionSite(SiteKind.MODULE_NAME, text.substring(runStart, offset), runStart, beforeRun)
            }

            if (last == null) return CompletionSite(SiteKind.PLAIN, text.substring(prefixStart, offset), prefixStart, before)
            val prefix = text.substring(prefixStart, offset)
            fun site(kind: SiteKind, qualifier: String? = null, arrow: Boolean = false) =
                CompletionSite(kind, prefix, prefixStart, before, qualifier, arrow)

            // 2. a member of a receiver
            if (last.isDot) return site(SiteKind.MEMBER)
            if (last.isMark("->") && before.size >= 2 && (before[before.size - 2].isKeyword("self") || before[before.size - 2].isKeyword("base"))) {
                return site(SiteKind.MEMBER, arrow = true)
            }
            // 3. `std.io#` or `g#`
            if (last.isMark("#")) {
                val qualifierRun = dottedRunBefore(text, last.start, before.dropLast(1))
                if (qualifierRun != null) return site(SiteKind.MODULE_MEMBER, qualifier = qualifierRun.second)
                return nowhere
            }
            // 4. types
            if (last.isKeyword("new")) return site(SiteKind.TYPE_NEW)
            if (last.isKeyword("as") || last.isKeyword("is") || last.isKeyword("base")) return site(SiteKind.TYPE)
            if (last.type == SharpTokenKinds.LPAREN && before.size >= 2 && before[before.size - 2].isKeyword("catch")) return site(SiteKind.TYPE)
            if (last.isMark("<") && before.size >= 2 && before[before.size - 2].isIdentifier && isGenericClassName(before[before.size - 2].text)) {
                return site(SiteKind.TYPE)
            }
            if (last.type == SharpTokenKinds.COMMA && insideTypeArguments(before, isGenericClassName)) return site(SiteKind.TYPE)
            if (last.isMark(":")) {
                val previous = before.getOrNull(before.size - 2)
                val afterAName = previous != null && (previous.isIdentifier || previous.type == SharpTokenKinds.RPAREN ||
                    previous.type == SharpTokenKinds.RBRACKET || previous.isMark("?") || previous.isMark(">"))
                if (afterAName) return site(if (aTernaryMayBeOpen(before)) SiteKind.TYPE_OR_NAME else SiteKind.TYPE)
            }
            return site(SiteKind.PLAIN)
        }

        private fun isIdentifierChar(c: Char) = c == '_' || (c in 'a'..'z') || (c in 'A'..'Z') || (c in '0'..'9')

        /** Is the caret inside a comment, or inside the text of a string (not inside its `${ }`)? */
        private fun insideCommentOrString(text: CharSequence, offset: Int, tokens: CodeTokens): Boolean {
            val all = tokens.all
            var i = 0
            // binary search would do; a file's token list is short enough and this runs once per completion
            while (i < all.size && all[i].end < offset) i++
            if (i >= all.size) return false
            val t = all[i]
            val next = all.getOrNull(i + 1)
            when (t.type) {
                SharpTokenKinds.LINE_COMMENT -> return offset > t.start
                SharpTokenKinds.BLOCK_COMMENT, SharpTokenKinds.DOC_COMMENT ->
                    return offset > t.start && (offset < t.end || !t.text.endsWith("*/") || t.text.length < 4)
                SharpTokenKinds.STRING_TEXT, SharpTokenKinds.STRING_ESCAPE, SharpTokenKinds.INTERP_NAME -> {
                    if (offset <= t.start) return false
                    if (offset < t.end) return true
                    // the caret is right after the token: still in the string when the string goes on
                    val previous = all.getOrNull(i - 1)
                    val closing = t.type == SharpTokenKinds.STRING_TEXT && t.text.endsWith("\"") &&
                        (t.text.length >= 2 || (previous != null && previous.end == t.start && isStringPiece(previous.type)))
                    return !closing || (next != null && next.start == t.end && isStringPiece(next.type))
                }
                SharpTokenKinds.CHAR -> return offset > t.start && offset < t.end
            }
            return false
        }

        private fun isStringPiece(type: com.intellij.psi.tree.IElementType) =
            type == SharpTokenKinds.STRING_TEXT || type == SharpTokenKinds.STRING_ESCAPE ||
                type == SharpTokenKinds.INTERP_NAME || type == SharpTokenKinds.INTERP_OPEN

        /**
         * The run of `a.b.c` written without a gap right before `end` (identifier, dot, identifier ...), as
         * (start offset, text). Null when there is none. `*` counts as a part (`std.*`).
         */
        private fun dottedRunBefore(text: CharSequence, end: Int, before: List<Tok>): Pair<Int, String>? {
            var start = end
            var i = before.size - 1
            var expectedEnd = end
            var sawAny = false
            while (i >= 0) {
                val t = before[i]
                if (t.end != expectedEnd) break
                if (!(t.isDot || t.isIdentifier || t.isMark("*"))) break
                start = t.start
                expectedEnd = t.start
                sawAny = true
                i--
            }
            return if (sawAny) start to text.substring(start, end) else null
        }

        /** Walks back from the end: an unclosed `(` right after the keyword `import`, with only names, dots, commas and `*` between. */
        private fun isInsideImportList(before: List<Tok>): Boolean {
            var i = before.size - 1
            while (i >= 0) {
                val t = before[i]
                when {
                    t.type == SharpTokenKinds.LPAREN -> return i >= 1 && before[i - 1].isKeyword("import")
                    t.isIdentifier || t.isDot || t.type == SharpTokenKinds.COMMA || t.isMark("*") -> i--
                    else -> return false
                }
            }
            return false
        }

        /** Walks back over a `< a, b,` list: is the caret in the arguments of a generic class's type? */
        private fun insideTypeArguments(before: List<Tok>, isGenericClassName: (String) -> Boolean): Boolean {
            var depth = 0
            var i = before.size - 1
            while (i >= 0) {
                val t = before[i]
                when {
                    t.isMark(">") -> depth++
                    t.isMark(">>") -> depth += 2
                    t.isMark("<") -> {
                        if (depth == 0) return before.getOrNull(i - 1)?.let { it.isIdentifier && isGenericClassName(it.text) } == true
                        depth--
                    }
                    t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.LBRACE || t.type == SharpTokenKinds.RBRACE ||
                        t.type == SharpTokenKinds.LPAREN || t.type == SharpTokenKinds.RPAREN -> return false
                }
                i--
            }
            return false
        }

        /** A `?` earlier in the same statement: the `:` may be the one of `c ? a : b`. */
        private fun aTernaryMayBeOpen(before: List<Tok>): Boolean {
            var i = before.size - 2
            while (i >= 0) {
                val t = before[i]
                if (t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.LBRACE || t.type == SharpTokenKinds.RBRACE) return false
                if (t.isMark("?")) return true
                i--
            }
            return false
        }
    }
}
