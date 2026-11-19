package sharp.ide.documentation

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import sharp.ide.index.IndexFixture
import java.nio.file.Files
import java.nio.file.Path

/**
 * HOVER TEXT, ON THE COMPILER'S REAL OUTPUT
 *
 * `HoverModel.at(view, offset)` is what the quick documentation shows: the declaration and the type of
 * what is under the offset, its owner, its module and where it is declared. The offsets are found in the
 * sample's own text, so the tests read like the question "what is this word?".
 */
class HoverModelTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded
    private val saved: String get() = shop.fixture.savedText

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-hover")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    /** The hover on the word that starts at the n-th occurrence of `needle` plus `inside` characters. */
    private fun hover(needle: String, inside: Int = 0, text: String = saved, occurrence: Int = 0): HoverInfo? {
        var at = -1
        repeat(occurrence + 1) { at = saved.indexOf(needle, at + 1) }
        require(at >= 0) { "no `$needle`" }
        return HoverModel.at(shop.view(text), at + inside)
    }

    @Test
    fun aFunctionCallShowsItsSignatureItsTypeItsOwnerAndWhereItIsDeclared() {
        val info = hover("names.size", 6)!!
        assertEquals("function", info.kind)
        assertEquals("def size() : int", info.definition.replace("public ", ""))
        assertEquals("int", info.type)
        assertEquals("std#list", info.memberOf)
        assertEquals("std", info.module)
        assertTrue(info.declaredIn!!, info.declaredIn!!.startsWith("list.sharp:"))
        assertEquals("function size : int", info.hint)
    }

    @Test
    fun aMethodOfOurOwnClass() {
        val info = hover("owner.title", 6)!!                           // `title`, a field of item reached through book
        assertEquals("field", info.kind)
        assertEquals("title : string", info.definition)
        assertEquals("string", info.type)
        assertEquals("shop#item", info.memberOf)
        assertEquals("shop", info.module)
        assertEquals("shop.sharp:13", info.declaredIn)

        val call = hover("local_book.summary", 12)!!
        assertEquals("function", call.kind)
        assertEquals("def summary() : string", call.definition)
        assertEquals("shop#book", call.memberOf)
    }

    @Test
    fun aVariableAndAParameter() {
        val local = hover("count.value", 2)!!
        assertEquals("local variable", local.kind)
        assertEquals("count : int", local.definition)
        assertEquals("function `probe`", local.memberOf)
        assertTrue(local.declaredIn!!.startsWith("shop.sharp:"))

        val parameter = hover("names.size", 2)!!
        assertEquals("parameter", parameter.kind)
        assertEquals("names : list<string>", parameter.definition)
        assertEquals("list<string>", parameter.type)
    }

    @Test
    fun aClassNameShowsTheClassWithItsBaseAndMembers() {
        val info = hover("item.make(\"m\")", 1)!!                    // `item` in front of a dot
        assertEquals("class", info.kind)
        assertEquals("class item : priced", info.definition)
        assertEquals("shop", info.module)
        assertTrue(info.note!!, info.note!!.contains("function"))

        val derived = hover("class book base item", 8)!!              // the declaration itself
        assertEquals("class book base item", derived.definition)

        val constructor = hover("new book(\"x\")", 5)!!               // `new book(...)` runs the constructor
        assertEquals("constructor", constructor.kind)
        assertEquals("book(title: string)", constructor.definition)

        val iface = hover("class item : priced", 14)!!                 // on `priced`: an interface
        assertEquals("interface", iface.kind)
    }

    @Test
    fun anEnumValueAndItsEnum() {
        val value = hover("color.green", 8)!!
        assertEquals("enum value", value.kind)
        assertEquals("green", value.name)
        assertEquals("shop#color", value.memberOf)
        val type = hover("color.green", 2)!!
        assertEquals("enum", type.kind)
    }

    @Test
    fun anExtensionFunctionAndAStaticFunction() {
        val shout = hover("at(0).shout", 8)!!
        assertEquals("extension function", shout.kind)
        assertEquals("std#string", shout.memberOf)
        assertEquals("shop.sharp:" + (saved.substring(0, saved.indexOf("def string.shout")).count { it == '\n' } + 1), shout.declaredIn)

        val make = hover("item.make(\"m\")", 7)!!
        assertEquals("function", make.kind)
        assertTrue(make.definition, make.definition.contains("static"))
    }

    @Test
    fun selfShowsOnlyItsType() {
        val info = hover("self.label", 1)!!
        assertEquals("keyword", info.kind)
        assertEquals("self", info.name)
        assertEquals("book", info.type)
    }

    @Test
    fun hoveringOnADeclarationShowsTheDeclaration() {
        val info = hover("def summary", 6)!!
        assertEquals("function", info.kind)
        assertEquals("def summary() : string", info.definition)
        val field = hover("pages : _int64", 1)!!
        assertEquals("field", field.kind)
        assertEquals("pages : _int64", field.definition)
    }

    @Test
    fun codeWorkedOutForOneInstanceOfAGenericSaysSo() {
        val info = hover("return value;", 8)!!                         // `value` inside holder<T>
        assertEquals("field", info.kind)
        assertTrue(info.note!!, info.note!!.contains("holder<string>"))
    }

    @Test
    fun nothingWhereTheTextWasEditedOrIsNotAName() {
        val edited = saved.replace("names.size()", "names.sizeXX()")
        assertNull("inside edited text", hover("names.size", 6, edited))
        assertNotNull("text before the edit still works", hover("names.size", 2, edited))
        assertNull("a comment", hover("// It compiles", 5))
        assertNull("white space", hover("count := names", -1))
        assertNull("a number or a string", hover("\"b\"", 1))
    }

    @Test
    fun theTextIsEscapedAndHasTheParts() {
        val html = HoverHtml.page(hover("names.size", 6)!!)
        assertTrue(html, html.contains("<div class='definition'><pre>"))
        assertTrue(html, html.contains("<b>Type:</b> <code>int</code>"))
        assertTrue(html, html.contains("<b>Member of:</b> <code>std#list</code>"))
        assertTrue(html, html.contains("<b>Module:</b> <code>std</code>"))
        val generic = HoverHtml.page(hover("names.size", 2)!!)
        assertTrue(generic, generic.contains("list&lt;string&gt;"))
        assertEquals("a &lt; b &amp; &quot;c&quot;", HoverHtml.escape("a < b & \"c\""))
    }
}
