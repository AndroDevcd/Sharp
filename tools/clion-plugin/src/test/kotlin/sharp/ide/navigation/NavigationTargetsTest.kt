package sharp.ide.navigation

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import sharp.ide.index.IndexFixture
import sharp.ide.lexer.repoRoot
import java.io.File
import java.nio.file.Files
import java.nio.file.Path

/**
 * GO TO DECLARATION, ON THE COMPILER'S REAL OUTPUT (the standard library included)
 *
 * Every target is checked by cutting the word out of the file it points into: the sample for names
 * declared there, the real library source for names declared in the library.
 */
class NavigationTargetsTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded
    private val saved: String get() = shop.fixture.savedText

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-goto")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    private fun targetAt(needle: String, inside: Int = 0, text: String = saved) =
        NavigationTargets.at(shop.view(text), saved.indexOf(needle).also { require(it >= 0) { "no `$needle`" } } + inside)

    private fun wordInSample(t: NavTarget): String {
        assertEquals("a place in the sample", shop.samplePath, t.compilerPath)
        return saved.substring(t.from, t.to)
    }

    @Test
    fun aFunctionOfTheLibraryLeadsIntoTheLibrarySource() {
        val t = targetAt("names.size", 6)!!
        assertEquals("size", t.name)
        assertTrue(t.compilerPath, t.compilerPath.endsWith("/std/list.sharp"))
        val source = File(repoRoot(), "stdlib/std/list.sharp").readText()
        val decoded = sharp.ide.run.SavedText.decode(source.toByteArray())
        assertEquals("size", decoded.substring(t.from, t.to))
    }

    @Test
    fun aLocalLeadsToWhereItWasDeclared() {
        val t = targetAt("count;", 2)!!
        assertEquals("count", wordInSample(t))
        assertEquals(saved.indexOf("count := names.size()"), t.from)
        val parameter = targetAt("names.size", 2)!!
        assertEquals(saved.indexOf("names: list<string>"), parameter.from)
    }

    @Test
    fun aClassAFieldAFunctionAnEnumValueAnExtension() {
        assertEquals("book", wordInSample(targetAt("new book(\"x\")", 5)!!))
        assertEquals("a use of `new` leads to the constructor", saved.indexOf("book(title: string) ->"), targetAt("new book(\"x\")", 5)!!.from)
        assertEquals("a class name leads to the class", saved.indexOf("item : priced") , targetAt("item.make", 1)!!.from)

        val field = targetAt("owner.title", 6)!!                       // declared in item, reached through book
        assertEquals("title", wordInSample(field))
        assertEquals(saved.indexOf("title : string"), field.from)

        assertEquals("summary", wordInSample(targetAt("local_book.summary", 12)!!))
        assertEquals("green", wordInSample(targetAt("color.green", 8)!!))
        assertEquals("color", wordInSample(targetAt("color.green", 2)!!))
        assertEquals("shout", wordInSample(targetAt("at(0).shout", 8)!!))
        assertEquals("make", wordInSample(targetAt("item.make", 6)!!))
    }

    @Test
    fun aGenericInstanceLeadsToTheGenericAsWritten() {
        val t = targetAt("box.get_value", 6)!!
        assertEquals("get_value", wordInSample(t))
        assertEquals(saved.indexOf("get_value() : T"), t.from)
    }

    @Test
    fun theTargetMovesWithEditsAboveItOnlyThroughTheViewsMap() {
        // the target offsets are in the compiler's text; the view maps the SOURCE offset back first
        val edited = "// added\n" + saved
        val view = shop.view(edited)
        val atNow = edited.indexOf("count;") + 2
        val t = NavigationTargets.at(view, atNow)!!
        assertEquals(saved.indexOf("count := names.size()"), t.from)
        assertEquals(edited.indexOf("count := names.size()"), view.toCurrent(t.from))
    }

    @Test
    fun nothingForSelfForTextBeingEditedOrForPlainText() {
        assertNull(targetAt("self.label", 1))
        val edited = saved.replace("names.size()", "names.sizeXX()")
        assertNull(targetAt("names.size", 6, edited))
        assertNull(targetAt("// It compiles", 4))
        assertNull(targetAt("\"b\"", 1))
    }
}
