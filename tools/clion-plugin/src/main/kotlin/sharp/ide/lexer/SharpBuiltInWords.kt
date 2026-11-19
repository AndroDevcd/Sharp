package sharp.ide.lexer

import com.intellij.psi.tree.IElementType

/**
 * WORDS THAT ARE NOT KEYWORDS BUT READ LIKE PART OF THE LANGUAGE
 *
 * A keyword is a word the compiler's lexer reserves (SharpKeywords). These are
 * ordinary names - a program may have a variable called `test` - that mean
 * something special only WHERE THEY STAND:
 *
 *   built-in calls   `mock<note_store>()` - the compiler writes a class for it
 *                    (lib/compiler/frontend/syntax/tree/mocks.sharp). Only `mock`
 *                    directly followed by `<` counts: `class x base mock` and a
 *                    variable named mock stay plain names.
 *
 *   testing words    the vocabulary of std.testing: `tests("...") { }`,
 *                    `expect(x)`, `given { }`, `verify { }`, `provide<t>() { }` ...
 *                    They count when they are CALLED - followed by `(`, `<` or
 *                    `{` - and are not a member (`x.test(...)`) and not being
 *                    declared (`def test(...)`).
 *
 * So the colour says "this is the framework speaking" without taking any name
 * away from the programmer. The lexer asks `kindAt` for every word that is not
 * a keyword.
 */
object SharpBuiltInWords {
    private val builtInCalls = setOf("mock")

    private val testingWords = setOf(
        "tests", "test", "before_each", "after_each", "finish_tests", "fail_test",
        "expect", "expect_true", "expect_false", "expect_same", "expect_null", "expect_not_null", "expect_throws",
        "given", "verify", "verify_nothing_else", "provide", "any_text", "any_number",
        "replaceable", "replaceable_single",
    )

    /** every word of both groups (for tests, and for the colour settings page) */
    val all: Set<String> get() = builtInCalls + testingWords

    /**
     * The token kind of the word buffer[start, end), or null when it is a plain
     * name here. `bufferEnd` is where the text being lexed stops.
     */
    fun kindAt(buffer: CharSequence, start: Int, end: Int, bufferEnd: Int): IElementType? {
        val word = buffer.subSequence(start, end).toString()
        val isCall = word in builtInCalls
        if (!isCall && word !in testingWords) return null

        // what comes after it (spaces skipped)
        var after = end
        while (after < bufferEnd && (buffer[after] == ' ' || buffer[after] == '\t')) after++
        val next = if (after < bufferEnd) buffer[after] else '\u0000'

        // what comes before it (spaces skipped): a member, or a declaration, is a plain name
        var before = start - 1
        while (before >= 0 && (buffer[before] == ' ' || buffer[before] == '\t')) before--
        if (before >= 0 && buffer[before] == '.') return null
        if (before >= 2 && buffer.subSequence(before - 2, before + 1).toString() == "def" &&
            (before - 3 < 0 || !isNameChar(buffer[before - 3]))
        ) return null

        if (isCall) {
            // `mock<` and nothing else: the `<` must touch the word
            return if (end < bufferEnd && buffer[end] == '<') SharpTokenKinds.BUILT_IN_CALL else null
        }
        return if (next == '(' || next == '<' || next == '{') SharpTokenKinds.TESTING_WORD else null
    }

    private fun isNameChar(c: Char) = c == '_' || c in 'a'..'z' || c in 'A'..'Z' || c in '0'..'9'
}
