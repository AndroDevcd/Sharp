package sharp.ide.language

import com.intellij.testFramework.fixtures.BasePlatformTestCase
import sharp.ide.lexer.SharpTokenKinds

/**
 * THE EDITOR FEATURES, IN THE PLATFORM'S HEADLESS TEST FIXTURE
 *
 * BasePlatformTestCase starts a light copy of the IDE without a window and loads
 * this plugin's plugin.xml, so these tests also prove that every extension point
 * is registered and finds its class. `myFixture` stands for the editor: put text
 * in, press keys, read the text back.
 */
class SharpEditorFeaturesTest : BasePlatformTestCase() {
    fun testDotSharpFilesAreSharpFiles() {
        val file = myFixture.configureByText("a.sharp", "mod a;")
        assertTrue(file is SharpFile)
        assertEquals(SharpFileType, file.fileType)
        assertEquals(SharpLanguage, file.language)
    }

    fun testBracesBecomeBlocks() {
        val file = myFixture.configureByText("a.sharp", "def f() { if (x) { y; } }\n}")
        val blocks = ArrayList<String>()
        fun walk(node: com.intellij.lang.ASTNode) {
            if (node.elementType == SharpElementTypes.BLOCK) blocks.add(node.text)
            var child = node.firstChildNode
            while (child != null) { walk(child); child = child.treeNext }
        }
        walk(file.node)
        assertEquals(listOf("{ if (x) { y; } }", "{ y; }"), blocks)
    }

    fun testAnUnclosedBraceStillMakesABlock() {
        val file = myFixture.configureByText("a.sharp", "def f() { x;")
        assertTrue(file.node.getChildren(null).any { it.elementType == SharpElementTypes.BLOCK })
    }

    fun testFoldingOfBlocksAndBlockComments() {
        val text = "/* one\n two */\ndef f() {\n  g();\n}\ndef h() { g(); }\n"
        val file = myFixture.configureByText("a.sharp", text)
        val document = myFixture.editor.document
        val regions = SharpFoldingBuilder().buildFoldRegions(file, document, false)
        // the comment and the block of f fold; the one-line block of h does not
        val folded = regions.map { text.substring(it.range.startOffset, it.range.endOffset) to SharpFoldingBuilder().getPlaceholderText(it.element) }
        assertEquals(
            listOf("/* one\n two */" to "/*...*/", "{\n  g();\n}" to "{...}"),
            folded,
        )
    }

    fun testTheFoldingBuilderIsRegistered() {
        // found through the platform's own lookup, so plugin.xml is proved too
        val builders = com.intellij.lang.folding.LanguageFolding.INSTANCE.allForLanguage(SharpLanguage)
        assertTrue(builders.any { it is SharpFoldingBuilder })
    }

    fun testCommentingALine() {
        myFixture.configureByText("a.sharp", "<caret>x := 1;\n")
        myFixture.performEditorAction("CommentByLineComment")
        myFixture.checkResult("//x := 1;\n")
    }

    fun testCommentingASelectionWithABlockComment() {
        myFixture.configureByText("a.sharp", "a <selection>b</selection> c\n")
        myFixture.performEditorAction("CommentByBlockComment")
        myFixture.checkResult("a /*b*/ c\n")
    }

    fun testTypingAQuoteAddsTheClosingQuote() {
        myFixture.configureByText("a.sharp", "x := <caret>;")
        myFixture.type('"')
        myFixture.checkResult("x := \"<caret>\";")
    }

    fun testTypingAnOpeningParenthesisAddsTheClosingOne() {
        myFixture.configureByText("a.sharp", "f<caret>")
        myFixture.type('(')
        myFixture.checkResult("f(<caret>)")
    }

    fun testHighlightingDoesNotReportErrors() {
        // Highlighting is lexer-only: a Sharp file produces no problems of its own.
        myFixture.configureByText("a.sharp", "class a { def f() : _int64 { return 1; } }")
        assertTrue(myFixture.doHighlighting().none { it.severity.myVal >= com.intellij.lang.annotation.HighlightSeverity.WARNING.myVal })
    }

    fun testTheCommentKindsAreTheParsersComments() {
        val definition = SharpParserDefinition()
        assertTrue(definition.commentTokens.contains(SharpTokenKinds.DOC_COMMENT))
        assertTrue(definition.stringLiteralElements.contains(SharpTokenKinds.STRING_TEXT))
    }
}
