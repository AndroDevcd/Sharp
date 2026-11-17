package sharp.ide.highlighting

import com.intellij.openapi.editor.DefaultLanguageHighlighterColors
import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.editor.colors.TextAttributesKey.createTextAttributesKey

/**
 * THE COLOUR SLOTS OF SHARP
 *
 * A TextAttributesKey is a named colour slot ("Sharp keyword (declaration)").
 * It does not hold a colour: each has a FALLBACK, one of the platform's standard
 * slots, and the colour scheme the user chose (Darcula, Light ...) colours that
 * slot. So Sharp looks right in every scheme without a single colour code here,
 * and the user can still give any Sharp slot its own colour under
 * Settings > Editor > Color Scheme > Sharp.
 *
 * Primer idea 3 in doc/ide.md section 2.
 */
object SharpColors {
    @JvmField val KEYWORD_DECLARATION = createTextAttributesKey("SHARP_KEYWORD_DECLARATION", DefaultLanguageHighlighterColors.KEYWORD)
    @JvmField val KEYWORD_CONTROL = createTextAttributesKey("SHARP_KEYWORD_CONTROL", DefaultLanguageHighlighterColors.KEYWORD)
    @JvmField val KEYWORD_MODIFIER = createTextAttributesKey("SHARP_KEYWORD_MODIFIER", DefaultLanguageHighlighterColors.KEYWORD)
    @JvmField val KEYWORD_OPERATOR = createTextAttributesKey("SHARP_KEYWORD_OPERATOR", DefaultLanguageHighlighterColors.KEYWORD)
    @JvmField val BUILT_IN_TYPE = createTextAttributesKey("SHARP_BUILT_IN_TYPE", DefaultLanguageHighlighterColors.CLASS_NAME)
    @JvmField val BUILT_IN_VALUE = createTextAttributesKey("SHARP_BUILT_IN_VALUE", DefaultLanguageHighlighterColors.CONSTANT)

    // names that read like part of the language where they stand (SharpBuiltInWords)
    @JvmField val BUILT_IN_CALL = createTextAttributesKey("SHARP_BUILT_IN_CALL", DefaultLanguageHighlighterColors.KEYWORD)
    @JvmField val TESTING_WORD = createTextAttributesKey("SHARP_TESTING_WORD", DefaultLanguageHighlighterColors.PREDEFINED_SYMBOL)

    @JvmField val IDENTIFIER = createTextAttributesKey("SHARP_IDENTIFIER", DefaultLanguageHighlighterColors.IDENTIFIER)
    @JvmField val NUMBER = createTextAttributesKey("SHARP_NUMBER", DefaultLanguageHighlighterColors.NUMBER)
    @JvmField val CHARACTER = createTextAttributesKey("SHARP_CHARACTER", DefaultLanguageHighlighterColors.STRING)
    @JvmField val STRING = createTextAttributesKey("SHARP_STRING", DefaultLanguageHighlighterColors.STRING)
    @JvmField val STRING_ESCAPE = createTextAttributesKey("SHARP_STRING_ESCAPE", DefaultLanguageHighlighterColors.VALID_STRING_ESCAPE)
    @JvmField val INTERPOLATION_DELIMITER = createTextAttributesKey("SHARP_INTERPOLATION_DELIMITER", DefaultLanguageHighlighterColors.TEMPLATE_LANGUAGE_COLOR)
    @JvmField val INTERPOLATION_NAME = createTextAttributesKey("SHARP_INTERPOLATION_NAME", DefaultLanguageHighlighterColors.TEMPLATE_LANGUAGE_COLOR)

    @JvmField val LINE_COMMENT = createTextAttributesKey("SHARP_LINE_COMMENT", DefaultLanguageHighlighterColors.LINE_COMMENT)
    @JvmField val BLOCK_COMMENT = createTextAttributesKey("SHARP_BLOCK_COMMENT", DefaultLanguageHighlighterColors.BLOCK_COMMENT)
    @JvmField val DOC_COMMENT = createTextAttributesKey("SHARP_DOC_COMMENT", DefaultLanguageHighlighterColors.DOC_COMMENT)

    @JvmField val OPERATOR = createTextAttributesKey("SHARP_OPERATOR", DefaultLanguageHighlighterColors.OPERATION_SIGN)
    @JvmField val BRACES = createTextAttributesKey("SHARP_BRACES", DefaultLanguageHighlighterColors.BRACES)
    @JvmField val PARENTHESES = createTextAttributesKey("SHARP_PARENTHESES", DefaultLanguageHighlighterColors.PARENTHESES)
    @JvmField val BRACKETS = createTextAttributesKey("SHARP_BRACKETS", DefaultLanguageHighlighterColors.BRACKETS)
    @JvmField val COMMA = createTextAttributesKey("SHARP_COMMA", DefaultLanguageHighlighterColors.COMMA)
    @JvmField val SEMICOLON = createTextAttributesKey("SHARP_SEMICOLON", DefaultLanguageHighlighterColors.SEMICOLON)
    @JvmField val DOT = createTextAttributesKey("SHARP_DOT", DefaultLanguageHighlighterColors.DOT)

    @JvmField val BAD_CHARACTER = createTextAttributesKey("SHARP_BAD_CHARACTER", com.intellij.openapi.editor.HighlighterColors.BAD_CHARACTER)
}
