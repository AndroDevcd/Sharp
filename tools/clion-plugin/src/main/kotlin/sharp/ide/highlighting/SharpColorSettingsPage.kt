package sharp.ide.highlighting

import com.intellij.openapi.editor.colors.TextAttributesKey
import com.intellij.openapi.fileTypes.SyntaxHighlighter
import com.intellij.openapi.options.colors.AttributesDescriptor
import com.intellij.openapi.options.colors.ColorDescriptor
import com.intellij.openapi.options.colors.ColorSettingsPage
import sharp.ide.language.SharpIcons
import javax.swing.Icon

/**
 * THE PAGE Settings > Editor > Color Scheme > Sharp
 *
 * Lists every colour slot of SharpColors with a readable name and shows a demo
 * text that uses all of them, so the user sees what a change does. Registered as
 * `colorSettingsPage` (the extension point of 2025.2; from 2026.3 the platform
 * replaces it with `colorSettings`).
 */
class SharpColorSettingsPage : ColorSettingsPage {
    override fun getDisplayName(): String = "Sharp"

    override fun getIcon(): Icon = SharpIcons.FILE

    override fun getHighlighter(): SyntaxHighlighter = SharpSyntaxHighlighter()

    override fun getAttributeDescriptors(): Array<AttributesDescriptor> = DESCRIPTORS

    override fun getColorDescriptors(): Array<ColorDescriptor> = ColorDescriptor.EMPTY_ARRAY

    override fun getAdditionalHighlightingTagToDescriptorMap(): Map<String, TextAttributesKey>? = null

    override fun getDemoText(): String = DEMO_TEXT

    companion object {
        private val DESCRIPTORS = arrayOf(
            AttributesDescriptor("Keywords//Declarations (class, def, mod ...)", SharpColors.KEYWORD_DECLARATION),
            AttributesDescriptor("Keywords//Control flow (if, while, return ...)", SharpColors.KEYWORD_CONTROL),
            AttributesDescriptor("Keywords//Modifiers (static, private ...)", SharpColors.KEYWORD_MODIFIER),
            AttributesDescriptor("Keywords//Operator words (new, as, is, in, sizeof)", SharpColors.KEYWORD_OPERATOR),
            AttributesDescriptor("Keywords//Built-in types (_int64, decimal, object)", SharpColors.BUILT_IN_TYPE),
            AttributesDescriptor("Keywords//Built-in values (true, false, null, self ...)", SharpColors.BUILT_IN_VALUE),
            AttributesDescriptor("Built-ins//Built-in call (mock<t>())", SharpColors.BUILT_IN_CALL),
            AttributesDescriptor("Built-ins//Testing words (tests, test, expect, given, verify ...)", SharpColors.TESTING_WORD),
            AttributesDescriptor("Identifier", SharpColors.IDENTIFIER),
            AttributesDescriptor("Number", SharpColors.NUMBER),
            AttributesDescriptor("Strings//String", SharpColors.STRING),
            AttributesDescriptor("Strings//Character", SharpColors.CHARACTER),
            AttributesDescriptor("Strings//Escape sequence", SharpColors.STRING_ESCAPE),
            AttributesDescriptor("Strings//Interpolation delimiters (dollar-brace, closing brace)", SharpColors.INTERPOLATION_DELIMITER),
            AttributesDescriptor("Strings//Interpolated name (dollar name)", SharpColors.INTERPOLATION_NAME),
            AttributesDescriptor("Comments//Line comment", SharpColors.LINE_COMMENT),
            AttributesDescriptor("Comments//Block comment", SharpColors.BLOCK_COMMENT),
            AttributesDescriptor("Comments//Doc comment", SharpColors.DOC_COMMENT),
            AttributesDescriptor("Operators and punctuation//Operator", SharpColors.OPERATOR),
            AttributesDescriptor("Operators and punctuation//Braces", SharpColors.BRACES),
            AttributesDescriptor("Operators and punctuation//Parentheses", SharpColors.PARENTHESES),
            AttributesDescriptor("Operators and punctuation//Brackets", SharpColors.BRACKETS),
            AttributesDescriptor("Operators and punctuation//Comma", SharpColors.COMMA),
            AttributesDescriptor("Operators and punctuation//Semicolon", SharpColors.SEMICOLON),
            AttributesDescriptor("Operators and punctuation//Dot", SharpColors.DOT),
            AttributesDescriptor("Bad character", SharpColors.BAD_CHARACTER),
        )

        // A short program that shows every slot. The doubled dollar signs are
        // Sharp's own way of writing one.
        private val DEMO_TEXT = """
            mod demo;

            import (
                std.io
            )

            /** A shape with an area. */
            class circle : shape {
                private radius : decimal = 1.5e0;
                static const SIDES : _int64 = 0xFF_ff;

                def area() : decimal {
                    return radius * radius * 3.14159;
                }

                def describe(label: string) : string {
                    // string with all three kinds of interpolation
                    text := "${'$'}{label}: area ${'$'}{area()} for ${'$'}label, costs ${'$'}${'$'}5\n";
                    /* nested /* comment */ still a comment */
                    if (radius > 0 && label != null) {
                        return text;
                    }
                    ch : _int8 = 'x';
                    shape?.area()!!.to_string();
                    return new string[] { "a", "b" }[0];
                }
            }

            def main() {
                tests("a circle") {
                    test("its area is used by whoever asks") {
                        drawn := mock<shape>();
                        given { drawn.area(); }.returns(3);
                        expect(drawn.area()).should_be(3);
                    };
                };
            }
        """.trimIndent()
    }
}
