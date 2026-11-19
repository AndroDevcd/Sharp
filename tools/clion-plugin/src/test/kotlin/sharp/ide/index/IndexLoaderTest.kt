package sharp.ide.index

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE INDEX READER, ON THE COMPILER'S REAL OUTPUT (standard library included)
 *
 * What is checked: the files of a run become the objects of DeclarationModel / FileBodies with
 * the facts the compiler wrote; every declaration's `at` cuts the right word out of the source;
 * every use of the sample points at a declaration that exists; and a run's files are refused
 * when they do not belong together.
 */
class IndexLoaderTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-index")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    private fun word(file: Int, at: NamePlace): String {
        require(file == shop.file) { "only the sample is checked by text" }
        return shop.fixture.savedText.substring(at.from, at.to)
    }

    @Test
    fun theLibraryAndTheSampleAreThere() {
        val index = shop.index
        assertEquals("full", index.level)
        assertTrue("the library's modules", index.modules.keys.containsAll(listOf("std", "std.io", "shop")))
        assertNotNull(index.classesById["std#list"])
        assertNotNull(index.classesById["shop#book"])
        assertEquals(listOf("shop", "std", "std.io.console", "std.io"), index.fileEntry(shop.file)!!.imports)
    }

    @Test
    fun aClassKnowsItsMembersBaseAndTypeParameters() {
        val book = shop.index.classesById["shop#book"]!!
        assertEquals("item", book.base!!.text)
        assertEquals(listOf("summary"), book.functions.map { it.name })
        assertEquals(listOf("pages"), book.fields.map { it.name })
        assertEquals(listOf("book"), book.constructors.map { it.name })
        val holder = shop.index.classesById["shop#holder"]!!
        assertEquals(listOf("T"), holder.typeParameters.map { it.name })
        assertEquals("T", holder.fields.single().type!!.text)
        assertTrue(holder.fields.single().type!!.isParam)
        val color = shop.index.classesById["shop#color"]!!
        assertEquals(listOf("red", "green", "blue"), color.values.map { it.name })
        assertEquals("interface", shop.index.classesById["shop#priced"]!!.kind)
    }

    @Test
    fun everyDeclarationOfTheSampleNamesItsOwnWord() {
        val inSample = shop.index.declarations.values.filter { it.file == shop.file }
        assertTrue("some declarations", inSample.size > 20)
        for (d in inSample) {
            if (d is ClassInfo || d is FunctionInfo && d.isConstructor || d is FunctionInfo && d.isOperator) continue
            assertEquals("the name of ${d.name}", d.name, word(d.file, d.at))
        }
        for (c in shop.index.classesById.values.filter { it.file == shop.file }) {
            assertEquals(c.name, word(c.file, c.at))
            assertEquals('{', shop.fixture.savedText[c.body!!.first])
            assertEquals('}', shop.fixture.savedText[c.body!!.last - 1])
        }
    }

    @Test
    fun extensionFunctionsAreMembersOfTheirClass() {
        val shout = shop.index.classesById["std#string"]!!.functions.single { it.name == "shout" }
        assertTrue(shout.isExtension)
        assertEquals(shop.file, shout.file)
        assertEquals("shout", word(shout.file, shout.at))
    }

    @Test
    fun theBodiesFileGivesFunctionsLocalsAndUses() {
        val bodies = shop.index.bodiesFor(shop.file)!!
        val names = bodies.functions.map { it.name }
        assertTrue(names.toString(), names.containsAll(listOf("probe", "main", "summary", "make_book")))
        val probe = bodies.functions.single { it.name == "probe" }
        val locals = bodies.locals.filter { bodies.functions[it.function].name == "probe" }.map { it.name }
        assertTrue(locals.toString(), locals.containsAll(listOf("names", "owner", "count", "box", "tagged")))
        assertEquals("list<string>", bodies.locals.single { it.name == "names" }.type!!.text)
        // the use of `names` before `.size()` is a local variable (kind 6) of type list<string>
        val saved = shop.fixture.savedText
        val at = saved.indexOf("names.size") + 1
        val use = bodies.useAt(at)!!
        assertEquals(6, use.kind)
        assertEquals("list<string>", use.type!!.text)
        // and `size` after the dot is a function of std#list
        val size = bodies.useAt(saved.indexOf("size()") + 1)!!
        assertEquals(3, size.kind)
        val declaration = shop.index.declarations[size.file to size.node]!!
        assertEquals("size", declaration.name)
        assertEquals("std#list", (declaration as FunctionInfo).owner!!.id)
        assertTrue(probe.body.first < at && at < probe.body.last)
    }

    @Test
    fun everyUseOfTheSamplePointsAtSomethingTheIndexKnows() {
        val bodies = shop.index.bodiesFor(shop.file)!!
        var checked = 0
        for (use in bodies.uses()) {
            when (use.kind) {
                6 -> assertTrue("local ${use.node}", use.node in bodies.locals.indices)
                7 -> {}
                else -> assertNotNull("use at ${use.from} kind ${use.kind}: ${shop.fixture.savedText.substring(use.from, use.to)}", shop.index.declarations[use.file to use.node])
            }
            checked++
        }
        assertTrue(checked > 50)
    }

    @Test
    fun aTypeIsAValueThatCanHaveItsParametersReplaced() {
        val listOfT = shop.index.classesById["std#list"]!!.functions.single { it.name == "first" }.result!!
        assertEquals("T", listOfT.text)
        val string = shop.index.types.let { t -> (0 until t.size).mapNotNull { t.typeOf(it) }.first { it.text == "string" && it.classId == "std#string" } }
        assertEquals("string", listOfT.substitute(mapOf("T" to string)).text)
        val nullableT = shop.index.classesById["std#list"]!!.functions.single { it.name == "find" }.result!!
        assertEquals("T?", nullableT.text)
        assertEquals("string?", nullableT.substitute(mapOf("T" to string)).text)
    }

    @Test
    fun aRunWhoseFilesDoNotBelongTogetherIsRefused() {
        val folder = temp.resolve("run-completion")
        val declarations = folder.resolve("declarations.json")
        Files.write(declarations, String(Files.readAllBytes(declarations), Charsets.UTF_8).replaceFirst("\"run\":\"", "\"run\":\"0").toByteArray())
        val run = DiagnosticsReader.readFolder(folder)
        try {
            IndexLoader.load(folder, run.manifest)
            throw AssertionError("a leftover file must be refused")
        } catch (e: RunReadException) {
            assertTrue(e.message!!, e.message!!.contains("declarations.json belongs to run"))
        }
    }

    @Test
    fun aRunThatStoppedAtSyntaxErrorsHasNoIndex() {
        val manifest = RunManifest("1", "syntax-errors", 1, 0, emptyList())
        try {
            IndexLoader.load(temp, manifest)
            throw AssertionError("no index at syntax errors")
        } catch (e: RunReadException) {
            assertTrue(e.message!!.contains("syntax errors"))
        }
    }

    /** types.json that is JSON but holds a text where a type row should be: refused with the file's name, not a crash. */
    @Test
    fun anIndexFileOfTheWrongShapeIsRefusedWithAReason() {
        val folder = temp.resolve("run-completion")
        val types = folder.resolve("types.json")
        Files.write(types, String(Files.readAllBytes(types), Charsets.UTF_8).replaceFirst("\"types\":[", "\"types\":[\"x\",").toByteArray())
        val run = DiagnosticsReader.readFolder(folder)
        try {
            IndexLoader.load(folder, run.manifest)
            throw AssertionError("a wrongly shaped file must be refused")
        } catch (e: RunReadException) {
            assertTrue(e.message!!, e.message!!.contains("types.json is not in the shape"))
        }
    }
}
