package sharp.ide.lexer

import com.intellij.psi.tree.IElementType

/**
 * THE KEYWORDS OF SHARP, BY GROUP
 *
 * The list is copied from `keyword_list()` in
 * compiler/frontend/lexer/lexer.sharp (67 words today). A test
 * (SharpKeywordsTest) reads that function out of the repository and compares it
 * with the words below, so a keyword added to the compiler turns a test red until
 * it is added here too, in the group where it belongs.
 *
 * The groups only decide the colour:
 *   declarations   words that introduce or shape a declaration
 *   control flow   statements that decide what runs next
 *   modifiers      words in front of a declaration that change its meaning
 *   built-in types the number types and `object`
 *   values         words that stand for a value
 *   operators      words that act like operators: `new`, `as`, `is`, `in`, `sizeof`
 */
object SharpKeywords {
    private val declarations = listOf(
        "mod", "import", "class", "interface", "enum", "def", "alias", "operator",
        "component", "factory", "inject", "init", "get", "set", "mutate", "ext",
    )
    private val controlFlow = listOf(
        "return", "if", "else", "while", "do", "for", "foreach", "try", "catch",
        "finally", "throw", "continue", "break", "goto", "lock", "when", "asm", "excuse",
    )
    private val modifiers = listOf(
        "static", "protected", "private", "public", "const", "native", "localized",
        "local", "stable", "override", "obfuscate", "single",
    )
    private val builtInTypes = listOf(
        "decimal", "object", "bool", "_int8", "_int16", "_int32", "_int64",
        "_uint8", "_uint16", "_uint32", "_uint64",
    )
    private val valueWords = listOf("true", "false", "null", "nil", "self", "base")
    private val operatorWords = listOf("new", "as", "is", "in", "sizeof")

    /** word -> the kind of token it makes */
    private val kinds: Map<String, IElementType> = buildMap {
        declarations.forEach { put(it, SharpTokenKinds.KEYWORD_DECLARATION) }
        controlFlow.forEach { put(it, SharpTokenKinds.KEYWORD_CONTROL) }
        modifiers.forEach { put(it, SharpTokenKinds.KEYWORD_MODIFIER) }
        builtInTypes.forEach { put(it, SharpTokenKinds.KEYWORD_TYPE) }
        valueWords.forEach { put(it, SharpTokenKinds.KEYWORD_VALUE) }
        operatorWords.forEach { put(it, SharpTokenKinds.KEYWORD_OPERATOR) }
    }

    /** every keyword */
    val all: Set<String> get() = kinds.keys

    /** the token kind of `word`, or null when it is not a keyword */
    fun kindOf(word: CharSequence): IElementType? = kinds[word.toString()]
}
