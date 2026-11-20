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
 *   C types          the names std.c gives C's types: `c_int`, `c_float`, `c_size` ...
 *                    They count wherever they are not a member (`x.c_int`).
 *                    `pointer` is too good a name for a variable to take away, so
 *                    it counts only where a TYPE stands: after a `:` or `as`, and
 *                    inside the brackets of a function type, `(pointer, pointer)(c_int)`.
 *
 *   extern           `extern "sdl" { ... }` - a word only in front of a library's
 *                    name (a text), exactly as the compiler reads it.
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

    /** the C types of std.c (lib/support/<version>/std/c/types.sharp): a type wherever they stand */
    private val cTypes = setOf(
        "c_char", "c_uchar", "c_short", "c_ushort", "c_int", "c_uint", "c_longlong", "c_ulonglong",
        "c_long", "c_ulong", "c_size", "c_ssize", "c_float", "c_double",
    )

    /** every word of the groups (for tests, and for the colour settings page) */
    val all: Set<String> get() = builtInCalls + testingWords + cTypes + setOf("pointer", "extern", "c_bytes")

    /** `extern`, `pointer` and the C types, where they mean what std.c means by them; else null */
    private fun cWordAt(word: String, buffer: CharSequence, start: Int, end: Int, bufferEnd: Int): IElementType? {
        if (word != "extern" && word != "pointer" && word != "c_bytes" && word !in cTypes) return null
        var after = end
        while (after < bufferEnd && (buffer[after] == ' ' || buffer[after] == '\t')) after++
        val next = if (after < bufferEnd) buffer[after] else '\u0000'
        var before = start - 1
        while (before >= 0 && (buffer[before] == ' ' || buffer[before] == '\t')) before--
        val previous = if (before >= 0) buffer[before] else '\u0000'

        if (word == "extern") return if (next == '"') SharpTokenKinds.CONTEXT_KEYWORD else null
        if (previous == '.') return null
        // `static const c_bytes : _int64 = 16;` in a struct class: the size C has for it (std/c/struct.sharp)
        if (word == "c_bytes") return if (next == ':') SharpTokenKinds.CONTEXT_KEYWORD else null
        if (word in cTypes) return SharpTokenKinds.C_TYPE

        // `pointer`: only where a type stands
        if (previous == ':') return SharpTokenKinds.C_TYPE
        if (before >= 1 && buffer[before] == 's' && buffer[before - 1] == 'a' && (before - 2 < 0 || !isNameChar(buffer[before - 2]))) {
            return SharpTokenKinds.C_TYPE                                   // alias _int64 as pointer, x as pointer
        }
        // one of the types of a function type, `(pointer, pointer)(c_int)` - and not an argument of a call, `free(pointer)`
        if ((previous == '(' || previous == ',') && (next == ',' || next == ')')) {
            return if (bracketOpensAType(buffer, before)) SharpTokenKinds.C_TYPE else null
        }
        return null
    }

    /**
     * Is the bracket around position `from` the bracket of a function TYPE? A call's bracket
     * stands right after a name (`free(`); a type's after a `:`, a `,`, another bracket or nothing.
     */
    private fun bracketOpensAType(buffer: CharSequence, from: Int): Boolean {
        var at = from
        var depth = 0
        while (at >= 0 && buffer[at] != ';' && buffer[at] != '{' && buffer[at] != '}') {
            val c = buffer[at]
            if (c == ')') depth++
            if (c == '(') {
                if (depth == 0) break
                depth--
            }
            at--
        }
        if (at < 0 || buffer[at] != '(') return false
        var before = at - 1
        while (before >= 0 && (buffer[before] == ' ' || buffer[before] == '\t')) before--
        return before < 0 || !isNameChar(buffer[before])
    }

    /**
     * The token kind of the word buffer[start, end), or null when it is a plain
     * name here. `bufferEnd` is where the text being lexed stops.
     */
    fun kindAt(buffer: CharSequence, start: Int, end: Int, bufferEnd: Int): IElementType? {
        val word = buffer.subSequence(start, end).toString()
        cWordAt(word, buffer, start, end, bufferEnd)?.let { return it }
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
