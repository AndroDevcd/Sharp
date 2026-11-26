package sharp.ide.editor

import com.intellij.codeInsight.completion.CompletionType
import com.intellij.codeInsight.lookup.Lookup
import com.intellij.codeInsight.lookup.LookupElement
import com.intellij.lang.parameterInfo.CreateParameterInfoContext
import com.intellij.openapi.command.WriteCommandAction
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.testFramework.utils.parameterInfo.MockCreateParameterInfoContext
import com.intellij.testFramework.utils.parameterInfo.MockParameterInfoUIContext
import com.intellij.testFramework.utils.parameterInfo.MockUpdateParameterInfoContext
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.completion.SharpParameterInfoHandler
import sharp.ide.completion.SharpTypedHandler
import sharp.ide.documentation.SharpDocumentationTargetProvider
import sharp.ide.index.IndexFixture
import sharp.ide.navigation.SharpGotoClassContributor
import sharp.ide.navigation.SharpGotoDeclarationHandler
import sharp.ide.navigation.SharpGotoSymbolContributor
import sharp.ide.run.AnalysisService
import sharp.ide.run.SavedText
import sharp.ide.run.Snapshot
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.TranslateMode
import sharp.ide.structure.SharpFileElement
import java.io.File
import java.nio.file.Files
import java.nio.file.Path

/**
 * COMPLETION, HOVER, GO-TO, PARAMETER INFO AND THE STRUCTURE VIEW THROUGH THE PLATFORM'S HEADLESS FIXTURE
 *
 * A real editor without a window. The compiler's real output for `index-fixtures/completion` (standard
 * library included) is published as AnalysisService would after a run; then the platform's own
 * completion, documentation, go-to and parameter-info machinery is driven and the answers read back:
 * what the owner would see.
 *
 * The file is typed into the editor with `<caret>`. When the typed text differs from the text the
 * compiler saw the index is STALE - which is the normal state while somebody is typing.
 */
class SharpIntelligenceInEditorTest : BasePlatformTestCase() {
    private lateinit var tempDir: Path
    private lateinit var savedSettings: SharpApplicationSettings.State
    private val fixture = IndexFixture("completion")
    private val saved: String get() = fixture.savedText

    override fun setUp() {
        super.setUp()
        tempDir = Files.createTempDirectory("sharp-intelligence")
        val state = SharpApplicationSettings.getInstance().state
        savedSettings = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(state, it) }
        state.translateMode = TranslateMode.NEVER.name
    }

    override fun tearDown() {
        try {
            AnalysisService.getInstance(project).clearIndex()
            XmlSerializerUtil.copyBean(savedSettings, SharpApplicationSettings.getInstance().state)
            tempDir.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** Puts `textWithCaret` in the editor and publishes the compiler's output for the sample, saved as `saved`. */
    private fun open(textWithCaret: String, publish: Boolean = true, libraryRoot: String? = null, realFile: Boolean = false): IndexFixture.Loaded? {
        val virtualFile = if (realFile) {
            // a file of the real file system, which go-to and "go to class" can find again by path
            val real = tempDir.resolve("real-shop.sharp")
            Files.write(real, textWithCaret.replace("<caret>", "").toByteArray(Charsets.UTF_8))
            val vf = com.intellij.openapi.vfs.LocalFileSystem.getInstance().refreshAndFindFileByNioFile(real)!!
            myFixture.openFileInEditor(vf)
            vf
        } else myFixture.configureByText("shop.sharp", textWithCaret).virtualFile
        if (!publish) return null
        val path = virtualFile.path
        val loaded = fixture.load(tempDir, path, libraryRoot = libraryRoot)
        val service = AnalysisService.getInstance(project)
        service.publish(loaded.run, mapOf(path to Snapshot(saved, SavedText.hash(fixture.sourceBytes))))
        service.publishIndex(loaded.index)
        return loaded
    }

    /** The saved text with `<caret>` put after the first `needle`. */
    private fun savedWithCaretAfter(needle: String): String {
        val at = saved.indexOf(needle)
        require(at >= 0) { "no `$needle`" }
        return saved.substring(0, at + needle.length) + "<caret>" + saved.substring(at + needle.length)
    }

    /** The saved text with `code` typed on a new line before `return count;` and the caret after it. */
    private fun typed(code: String): String {
        val at = saved.indexOf("    return count;")
        return saved.substring(0, at) + "    " + code + "<caret>\n" + saved.substring(at)
    }

    private fun complete(): List<String> {
        val items: Array<LookupElement>? = myFixture.complete(CompletionType.BASIC)
        // null: exactly one item matched and the platform inserted it by itself
        return items?.map { it.lookupString } ?: emptyList()
    }

    // ---------------------------------------------------------------- completion

    fun testTheListAfterADotOfAFreshIndex() {
        open(savedWithCaretAfter("names."))
        val offered = complete()
        assertTrue(offered.toString(), offered.containsAll(listOf("add", "at", "size", "first", "last")))
        assertFalse("private members stay hidden", "expand" in offered)
    }

    fun testTheListAfterATypedDotOfAStaleIndex() {
        open(typed("owner."))
        val offered = complete()
        assertTrue(offered.toString(), offered.containsAll(listOf("pages", "summary", "title", "label", "price")))
        assertFalse(offered.contains("make"))
    }

    fun testTheItemsShowTheirTypeAndParameters() {
        open(typed("names."))
        myFixture.complete(CompletionType.BASIC)
        val at = myFixture.lookup.items.first { it.lookupString == "at" }
        val presentation = com.intellij.codeInsight.lookup.LookupElementPresentation()
        at.renderElement(presentation)
        assertEquals("string", presentation.typeText)
        assertTrue(presentation.tailText!!, presentation.tailText!!.startsWith("(index: "))
        assertNotNull(presentation.icon)
    }

    fun testChoosingAFunctionAddsParenthesesAndPutsTheCaretInsideWhenThereAreParameters() {
        open(typed("names.ad"))
        myFixture.complete(CompletionType.BASIC)
        val add = myFixture.lookup.items.first { it.lookupString == "add" }
        myFixture.lookup.currentItem = add
        myFixture.finishLookup(Lookup.NORMAL_SELECT_CHAR)
        val text = myFixture.editor.document.text
        val caret = myFixture.editor.caretModel.offset
        assertTrue(text, text.contains("names.add()\n"))
        assertEquals("the caret is between the parentheses", text.indexOf("names.add()") + "names.add(".length, caret)
    }

    fun testAFunctionWithNoParametersLeavesTheCaretAfterTheParentheses() {
        open(typed("names.siz"))
        val offered = complete()                                // `size` alone: inserted at once
        assertTrue(offered.isEmpty() || offered == listOf("size"))
        val text = myFixture.editor.document.text
        assertTrue(text, text.contains("names.size()\n    return"))
        assertEquals(text.lastIndexOf("names.size()") + "names.size()".length, myFixture.editor.caretModel.offset)
    }

    fun testChoosingAClassAfterNewAddsParentheses() {
        open(typed("n := new boo"))
        val offered = complete()
        assertTrue(offered.toString(), "book" in offered)
        myFixture.lookup.currentItem = myFixture.lookup.items.first { it.lookupString == "book" }
        myFixture.finishLookup(Lookup.NORMAL_SELECT_CHAR)
        val text = myFixture.editor.document.text
        assertTrue(text, text.contains("n := new book()"))
        assertEquals(text.lastIndexOf("new book(") + "new book(".length, myFixture.editor.caretModel.offset)
    }

    fun testAClassOfAModuleThatIsNotImportedBringsItsImport() {
        val loaded = open(typed("n : ")) ?: return
        val scope = loaded.view().lookup.scopeFor(loaded.file, saved)
        val module = loaded.index.modules.entries.first { it.key !in scope.modules && it.key.isNotEmpty() && it.value.classes.any { c -> c.kind == "class" && !c.hasFlag("private") } }
        val name = module.value.classes.first { it.kind == "class" && !it.hasFlag("private") }.name
        WriteCommandAction.runWriteCommandAction(project) { myFixture.editor.document.insertString(myFixture.editor.caretModel.offset, name) }
        myFixture.editor.caretModel.moveToOffset(myFixture.editor.caretModel.offset + name.length)
        val offered = complete()
        if (myFixture.lookup != null) {
            val candidate = myFixture.lookup.items.first { it.lookupString == name }
            myFixture.lookup.currentItem = candidate
            myFixture.finishLookup(Lookup.NORMAL_SELECT_CHAR)
        }
        val text = myFixture.editor.document.text
        assertTrue("the import list now names ${module.key}: ${text.lines().take(4)}", text.contains("import ( std.io, ${module.key} )") || Regex("import \\([^)]*\\b" + Regex.escape(module.key) + "\\b").containsMatchIn(text))
        assertTrue(offered.isEmpty() || offered.contains(name))
    }

    fun testModuleNamesInAnImportList() {
        open("mod shop;\n\nimport ( std.<caret>")
        val offered = complete()
        assertTrue(offered.toString(), offered.containsAll(listOf("std.io", "std.io.console")))
    }

    fun testNothingInsideAComment() {
        open(typed("// "))
        val items = myFixture.complete(CompletionType.BASIC)?.map { it.lookupString } ?: emptyList()
        // the platform may offer plain words of the file in a comment; nothing of ours (keywords, types, members) comes
        assertTrue("no Sharp items in a comment: $items", items.none { it in setOf("foreach", "sizeof", "_int8", "static", "operator") })
    }

    fun testWithoutAnIndexOnlyKeywordsComeAndNothingThrows() {
        open("def f() {\n    ret<caret>\n}\n", publish = false)
        val offered = complete()
        assertTrue(offered.isEmpty() || offered.contains("return"))
        open("def f() {\n    x.<caret>\n}\n", publish = false)
        assertEquals(emptyList<String>(), complete())
    }

    // ---------------------------------------------------------------- the list opens by itself after a dot

    fun testTheTypedHandlerAsksForTheListAfterADotAndNotAfterALetter() {
        open(typed("names"))
        val handler = SharpTypedHandler()
        val editor = myFixture.editor
        val file = myFixture.file
        assertEquals(com.intellij.codeInsight.editorActions.TypedHandlerDelegate.Result.STOP, handler.checkAutoPopup('.', project, editor, file))
        assertEquals(com.intellij.codeInsight.editorActions.TypedHandlerDelegate.Result.CONTINUE, handler.checkAutoPopup('a', project, editor, file))
        assertEquals(com.intellij.codeInsight.editorActions.TypedHandlerDelegate.Result.CONTINUE, handler.checkAutoPopup(';', project, editor, file))
        val plain = myFixture.configureByText("notes.txt", "x<caret>")
        assertEquals("only Sharp files", com.intellij.codeInsight.editorActions.TypedHandlerDelegate.Result.CONTINUE, handler.checkAutoPopup('.', project, myFixture.editor, plain))
    }

    // ---------------------------------------------------------------- hover

    fun testHoverShowsTheDeclarationTypeAndOwner() {
        open(saved.replaceFirst("names.size()", "names.<caret>size()"))
        val offset = myFixture.editor.caretModel.offset
        val targets = SharpDocumentationTargetProvider().documentationTargets(myFixture.file, offset)
        assertEquals(1, targets.size)
        val target = targets.single()
        assertEquals("function size : _int64", target.computeDocumentationHint())
        assertEquals("size", target.computePresentation().presentableText)
        assertNotNull(target.computeDocumentation())
    }

    fun testHoverInsideEditedTextOrWithoutAnIndexShowsNothing() {
        open(saved.replaceFirst("names.size()", "names.siz<caret>eXX()"))
        val offset = myFixture.editor.caretModel.offset
        assertTrue(SharpDocumentationTargetProvider().documentationTargets(myFixture.file, offset).isEmpty())
        AnalysisService.getInstance(project).clearIndex()
        assertTrue(SharpDocumentationTargetProvider().documentationTargets(myFixture.file, 5).isEmpty())
    }

    // ---------------------------------------------------------------- go to declaration

    fun testGoToDeclarationOfALocalLeadsToItsDeclaration() {
        open(saved.replaceFirst("count;", "cou<caret>nt;"))
        val offset = myFixture.editor.caretModel.offset
        val element = myFixture.file.findElementAt(offset)
        val targets = SharpGotoDeclarationHandler().getGotoDeclarationTargets(element, offset, myFixture.editor)!!
        assertEquals(1, targets.size)
        assertEquals(saved.indexOf("count := names.size()"), targets[0].textRange.startOffset)
        assertEquals("count", targets[0].text)
    }

    fun testGoToDeclarationFollowsTextAddedAboveTheTarget() {
        open(saved.replaceFirst("count;", "cou<caret>nt;"))
        WriteCommandAction.runWriteCommandAction(project) { myFixture.editor.document.insertString(0, "// two\n// lines\n") }
        com.intellij.psi.PsiDocumentManager.getInstance(project).commitAllDocuments()      // the go-to action does this before it asks a handler
        val offset = myFixture.editor.caretModel.offset
        val targets = SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(offset), offset, myFixture.editor)!!
        assertEquals(myFixture.editor.document.text.indexOf("count := names.size()"), targets[0].textRange.startOffset)
    }

    fun testGoToDeclarationIntoAFileOfTheLibrary() {
        val library = tempDir.resolve("stdlib")
        Files.createDirectories(library.resolve("std"))
        val real = File(sharp.ide.lexer.repoRoot(), "stdlib/std/list.sharp")
        // (with Unix line endings, as the compiler that wrote the fixture read it: a Windows checkout has the library with CRLF)
        Files.write(library.resolve("std/list.sharp"), real.readText(Charsets.UTF_8).replace("\r\n", "\n").toByteArray(Charsets.UTF_8))
        open(saved.replaceFirst("names.size()", "names.<caret>size()"), libraryRoot = library.toRealPath().toString().replace('\\', '/'))
        val offset = myFixture.editor.caretModel.offset
        val place = SharpGotoDeclarationHandler.targetFor(project, myFixture.file.virtualFile, myFixture.editor.document.text, offset)!!
        assertEquals("list.sharp", place.first.name)
        val text = String(place.first.contentsToByteArray(), Charsets.UTF_8)
        assertEquals("size", text.substring(place.second, place.second + 4))
        val targets = SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(offset), offset, myFixture.editor)!!
        assertEquals("list.sharp", targets[0].containingFile.name)
        assertEquals("size", targets[0].text)

        // The jump itself: the target says its own place (not "wherever the platform puts a token"),
        // and following it opens list.sharp with the caret ON the declaration, not at the top of the file.
        assertEquals(place.second, targets[0].textOffset)
        (targets[0] as com.intellij.pom.Navigatable).navigate(true)
        val opened = com.intellij.openapi.fileEditor.FileEditorManager.getInstance(project).selectedTextEditor!!
        assertEquals("list.sharp", com.intellij.openapi.fileEditor.FileDocumentManager.getInstance().getFile(opened.document)!!.name)
        assertEquals(place.second, opened.caretModel.offset)
        assertTrue("the declaration is not on the first line", opened.caretModel.logicalPosition.line > 0)
    }

    fun testGoToDeclarationDegradesQuietly() {
        open(saved.replaceFirst("names.size()", "names.<caret>size()"))                     // the library files do not exist here
        val offset = myFixture.editor.caretModel.offset
        assertNull(SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(offset), offset, myFixture.editor))
        AnalysisService.getInstance(project).clearIndex()
        assertNull(SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(offset), offset, myFixture.editor))
        assertNull(SharpGotoDeclarationHandler().getGotoDeclarationTargets(null, 0, myFixture.editor))
    }

    // ---------------------------------------------------------------- parameter info

    fun testParameterInfoShowsTheParametersAndTheCurrentOne() {
        open(typed("names.insert(1, "))
        val handler = SharpParameterInfoHandler()
        val create = MockCreateParameterInfoContext(myFixture.editor, myFixture.file)
        val owner = handler.findElementForParameterInfo(create)!!
        assertEquals("(", owner.text)
        assertEquals(1, create.itemsToShow!!.size)
        val update = MockUpdateParameterInfoContext(myFixture.editor, myFixture.file, create.itemsToShow)
        assertEquals(owner.textRange.startOffset, handler.findElementForUpdatingParameterInfo(update)!!.textRange.startOffset)
        handler.updateParameterInfo(owner, update)
        assertEquals(1, update.currentParameter)
        val ui = MockParameterInfoUIContext(owner)
        ui.currentParameterIndex = 1
        handler.updateUI(create.itemsToShow!![0] as sharp.ide.completion.ParameterHint, ui)
        assertEquals("index: int, element: string", ui.text)
        assertEquals("element: string", ui.text!!.substring(ui.highlightStart, ui.highlightEnd))
    }

    fun testParameterInfoWhereThereIsNoCallOrNoIndex() {
        open(typed("x := 5"))
        val handler = SharpParameterInfoHandler()
        assertNull(handler.findElementForParameterInfo(MockCreateParameterInfoContext(myFixture.editor, myFixture.file)))
        AnalysisService.getInstance(project).clearIndex()
        assertNull(handler.findElementForParameterInfo(MockCreateParameterInfoContext(myFixture.editor, myFixture.file)))
    }

    // ---------------------------------------------------------------- structure view

    fun testTheStructureViewListsClassesWithTheirMembersAndTheGlobals() {
        open(saved)
        val root = SharpFileElement(myFixture.file)
        val top = root.children.map { it.presentation.presentableText!! }
        assertTrue(top.toString(), top.containsAll(listOf("item", "book base item", "color", "holder<T>", "labelled<K> base holder<K>")))
        assertTrue(top.toString(), top.any { it.startsWith("probe(") } && top.any { it.startsWith("make_book(") })
        assertTrue("the extension function", top.any { it.startsWith("string.shout(") })
        val book = root.children.first { it.presentation.presentableText == "book base item" }
        assertEquals(listOf("pages : _int64", "book(title: string)", "summary() : string"), book.children.map { it.presentation.presentableText })
        val color = root.children.first { it.presentation.presentableText == "color" }
        assertEquals(listOf("red", "green", "blue"), color.children.map { it.presentation.presentableText })
    }

    fun testTheStructureViewIsEmptyWithoutAnIndex() {
        open(saved, publish = false)
        assertTrue(SharpFileElement(myFixture.file).children.isEmpty())
    }

    // ---------------------------------------------------------------- go to class / symbol

    fun testGoToClassAndSymbolFindTheProgramAndTheLibrary() {
        open(saved, realFile = true)
        val classes = SharpGotoClassContributor()
        val ours = classes.getNames(project, false).toSet()
        assertTrue(ours.toString(), ours.containsAll(listOf("item", "book", "holder", "color")))
        assertFalse("library classes only when asked for", "list" in ours)
        assertTrue("list" in classes.getNames(project, true))
        val book = classes.getItemsByName("book", "book", project, false).single()
        assertEquals("shop#book", classes.getQualifiedName(book))
        assertEquals("book", book.presentation!!.presentableText)

        val symbols = SharpGotoSymbolContributor()
        val names = symbols.getNames(project, false).toSet()
        assertTrue(names.toString(), names.containsAll(listOf("probe", "make_book", "summary", "title", "red")))
        assertFalse("add" in names)

        book.navigate(true)
        val editor = FileEditorManager.getInstance(project).selectedTextEditor!!
        assertEquals(saved.indexOf("class book") + "class ".length, editor.caretModel.offset)
    }

    fun testGoToClassWithoutAnIndexIsEmpty() {
        open(saved, publish = false)
        assertEquals(0, SharpGotoClassContributor().getNames(project, true).size)
        assertEquals(0, SharpGotoSymbolContributor().getNames(project, true).size)
    }
}
