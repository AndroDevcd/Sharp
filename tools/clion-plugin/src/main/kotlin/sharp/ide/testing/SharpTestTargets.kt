/*
 * WHAT IN A TEST FILE CAN BE RUN, AND WHERE IT IS WRITTEN
 *
 * A project's tests are Sharp files in its tests folder (sharp.ide.project). In such a file three
 * things can be run on their own, and each gets a green arrow in the gutter (SharpTestGutter.kt):
 *
 *     def main() {                            <- every test of the project
 *         basket_tests();
 *     }
 *
 *     def basket_tests() {                    <- the groups written in this function
 *         tests("shopping_basket") {        <- this group
 *             test("a new basket is empty") { <- this one test
 *                 ...
 *             };
 *         };
 *     }
 *
 * "Run one of them" is a filter the test program is started with (std.testing's `-test-only`): the
 * group's name, or `group > test`; a function is the groups written in it.
 *
 * This file finds them in the TEXT of a file - the compiler is not asked: the arrows must be there
 * while the code is being typed. It reads the tokens (the lexer the highlighter uses) and follows the
 * braces. A name that is not a plain text (`tests("basket " + n)`, or one with `${...}` in it) cannot
 * be named beforehand: it gets no arrow of its own, and is still run with its function or with all.
 *
 * No IDE classes here: SharpTestTargetsTest asks it directly.
 */
package sharp.ide.testing

import sharp.ide.lexer.CodeTokens
import sharp.ide.lexer.SharpTokenKinds
import sharp.ide.lexer.Tok

enum class TestTargetKind { ALL, FUNCTION, GROUP, TEST }

/** Something that can be run: where its word starts in the text, what it is called, and the filter that runs it (empty: everything). */
data class TestTarget(val offset: Int, val kind: TestTargetKind, val title: String, val filters: List<String>)

object SharpTestTargets {
    /** Between a group and a test in a filter and in what is shown: `shopping_basket > a new basket is empty`. */
    const val INSIDE = " > "

    /** Everything in the file that can be run, in the order it is written. */
    fun of(text: CharSequence): List<TestTarget> {
        val code = CodeTokens.scan(text).code
        val found = ArrayList<TestTarget>()

        // the blocks that are open where the scan is: a function's body, a group's, or any other `{`
        class Block(val function: Tok?, val group: String?, val groups: ArrayList<String> = ArrayList())
        val open = ArrayList<Block>()
        var pendingFunction: Tok? = null          // `def name` seen, its `{` not yet
        var pendingGroup: String? = null          // `tests("name")` seen, its `{` not yet
        var parens = 0

        var i = 0
        while (i < code.size) {
            val token = code[i]
            when {
                token.type == SharpTokenKinds.LPAREN -> parens++
                token.type == SharpTokenKinds.RPAREN -> if (parens > 0) parens--
                token.isKeyword("def") && i + 1 < code.size && code[i + 1].isIdentifier -> pendingFunction = code[i + 1]
                // (`def total() := price * 2;` has no body of its own)
                token.isMark(";") && parens == 0 -> { pendingFunction = null; pendingGroup = null }

                token.type == SharpTokenKinds.LBRACE -> {
                    open += Block(pendingFunction, pendingGroup)
                    pendingFunction = null
                    pendingGroup = null
                }
                token.type == SharpTokenKinds.RBRACE -> {
                    val closed = open.removeLastOrNull()
                    val function = closed?.function
                    if (function != null) {
                        // a function that holds groups runs them; the tests' `main` runs everything
                        if (function.text == "main" && open.isEmpty()) {
                            found += TestTarget(function.start, TestTargetKind.ALL, "all tests", emptyList())
                        } else if (closed.groups.isNotEmpty()) {
                            found += TestTarget(function.start, TestTargetKind.FUNCTION, function.text, closed.groups.distinct())
                        }
                    }
                }

                token.isIdentifier && (token.text == "tests" || token.text == "test") &&
                    i + 1 < code.size && code[i + 1].type == SharpTokenKinds.LPAREN && !isMemberOrDeclaration(code, i) -> {
                    val name = plainText(code, i + 2)
                    if (name != null) {
                        if (token.text == "tests") {
                            found += TestTarget(token.start, TestTargetKind.GROUP, name, listOf(name))
                            pendingGroup = name
                            // the function this group is written in runs it
                            open.lastOrNull { it.function != null }?.groups?.add(name)
                        } else {
                            val group = open.lastOrNull { it.group != null }?.group
                            if (group != null) found += TestTarget(token.start, TestTargetKind.TEST, name, listOf(group + INSIDE + name))
                        }
                    }
                }
            }
            i++
        }
        return found.sortedBy { it.offset }
    }

    /** What can be run at this offset: the target whose word starts there. */
    fun at(text: CharSequence, offset: Int): TestTarget? = of(text).firstOrNull { it.offset == offset }

    /** `basket.tests(...)` is someone's own function, and `def test(...)` declares one: neither is std.testing's word. */
    private fun isMemberOrDeclaration(code: List<Tok>, index: Int): Boolean {
        val before = code.getOrNull(index - 1) ?: return false
        return before.isDot || before.isKeyword("def") || before.isMark("#")
    }

    /**
     * The text of the string that starts at `code[from]` when it is all there is up to the `)` and it
     * is plain (no `${...}`, no `$name`); null otherwise. The lexer cuts a string into pieces: the
     * opening quote with text, escapes, text, the closing quote.
     */
    private fun plainText(code: List<Tok>, from: Int): String? {
        val text = StringBuilder()
        var i = from
        while (i < code.size && (code[i].type == SharpTokenKinds.STRING_TEXT || code[i].type == SharpTokenKinds.STRING_ESCAPE)) {
            if (code[i].type == SharpTokenKinds.STRING_ESCAPE) {
                text.append(when (val escaped = code[i].text.drop(1)) { "n" -> "\n"; "t" -> "\t"; else -> escaped })
            } else {
                text.append(code[i].text)
            }
            i++
        }
        if (i == from || i >= code.size || code[i].type != SharpTokenKinds.RPAREN) return null
        // the quotes are part of the first and the last piece
        if (text.length < 2 || text[0] != '"' || text[text.length - 1] != '"') return null
        return text.substring(1, text.length - 1)
    }
}
