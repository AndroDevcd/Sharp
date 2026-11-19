package sharp.ide.completion

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import sharp.ide.index.IndexFixture
import sharp.ide.lexer.CodeTokens
import java.nio.file.Files
import java.nio.file.Path

/**
 * PARAMETER INFO, THE IMPORT THAT COMES WITH A CLASS, AND THE PLACES COMPLETION TELLS APART
 */
class CallSiteAndImportTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded
    private val saved: String get() = shop.fixture.savedText

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-callsite")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    private fun site(text: String, caret: Int = text.length) = CallSite.at(CodeTokens.scan(text), caret)

    /** Types `code` before `return count.value;` of the sample and asks for parameter info at its end. */
    private fun hints(code: String): Pair<CallSiteInfo, List<ParameterHint>>? {
        val at = saved.indexOf("    return count.value;")
        val text = saved.substring(0, at) + "    " + code + "\n" + saved.substring(at)
        return ParameterHints.overloads(shop.view(text), text, at + 4 + code.length)
    }

    // ---------------------------------------------------------------- CallSite

    @Test
    fun theCaretIsInTheArgumentsOfACall() {
        val s = site("x := owner.add(first, sec")!!
        assertEquals(1, s.argumentIndex)
        assertEquals(0, site("new book(")!!.argumentIndex)
        assertEquals(2, site("f(a, g(b, c), ")!!.argumentIndex)
        assertEquals("nested call: the inner one", 1, site("f(a, g(b, ")!!.argumentIndex)
        assertEquals(1, site("f(a[1, 2], ")!!.argumentIndex)
    }

    @Test
    fun notACallWhenNothingIsOpenOrWhenItIsAStatement() {
        assertNull(site("x := f(1);"))
        assertNull(site("x := 1"))
        assertNull(site("if (x"))
        assertNull(site("while (x < 3"))
        assertNull(site("foreach(a in "))
        assertNull(site("f(a); g"))
    }

    @Test
    fun aGenericConstructorCall() {
        val s = site("new holder<string>(")!!
        assertEquals("holder", CodeTokens.scan("new holder<string>(").code[s.nameIndex].text)
    }

    // ---------------------------------------------------------------- ParameterHints

    @Test
    fun theOverloadsOfAMethodWithTheTypeArgumentsPutIn() {
        val (site, found) = hints("names.add(")!!
        assertEquals(0, site.argumentIndex)
        assertEquals(listOf(listOf("element: string")), found.map { it.parameters })

        val (_, insert) = hints("names.insert(1, ")!!
        assertEquals(listOf("index: int", "element: string"), insert.single().parameters)
        val (current, _) = hints("names.insert(1, ")!!
        assertEquals(1, current.argumentIndex)
        assertEquals(Triple("index: int, element: string", 12, 27), ParameterHints.render(insert.single(), 1))
        assertEquals(Triple("index: int, element: string", 0, 10), ParameterHints.render(insert.single(), 0))
        assertEquals(Triple("index: int, element: string", -1, -1), ParameterHints.render(insert.single(), 2))
    }

    @Test
    fun everyOverloadIsShown() {
        val (_, found) = hints("names.at(")!!
        assertEquals(setOf("index: int", "index: _int64"), found.flatMap { it.parameters }.toSet())
    }

    @Test
    fun aConstructorAfterNew() {
        val (_, found) = hints("n := new book(")!!
        assertEquals(listOf(listOf("title: string")), found.map { it.parameters })
        assertTrue(found.single().isConstructor)
        val (_, generic) = hints("n := new holder<string>(")!!
        assertEquals("T replaced by the written argument", listOf(listOf("value: string")), generic.map { it.parameters })
    }

    @Test
    fun aGlobalFunctionAStaticFunctionAndAFunctionWithNoParameters() {
        assertEquals(listOf(listOf<String>()), hints("m := make_book(")!!.second.map { it.parameters })
        assertEquals(listOf(listOf("title: string")), hints("m := item.make(")!!.second.map { it.parameters })
        assertEquals("<no parameters>", ParameterHints.render(hints("m := make_book(")!!.second.single(), 0).first)
    }

    @Test
    fun nothingWhenTheCalleeIsUnknown() {
        assertNull(hints("nothing_like_it("))
        assertNull(hints("owner.no_such_function("))
    }

    // ---------------------------------------------------------------- ImportEdit

    @Test
    fun anImportIsAddedToTheListOrCreated() {
        fun apply(text: String, module: String): String? {
            val plan = ImportEdit.plan(text, module) ?: return null
            return text.substring(0, plan.offset) + plan.text + text.substring(plan.offset)
        }
        assertEquals("mod a;\nimport ( std.io, other )\n", apply("mod a;\nimport ( std.io )\n", "other"))
        assertEquals("mod a;\nimport ( other   )\n", apply("mod a;\nimport (  )\n", "other"))
        assertEquals("mod a;\n\nimport ( other )\n\ndef f() {}", apply("mod a;\n\ndef f() {}", "other"))
        assertEquals("a group is not the plain list", "import ( other )\n\nimport ( x ) as g", apply("import ( x ) as g", "other"))
        assertEquals(
            "import (\n    platform,\n    platform.kernel,\n    other\n)\n",
            apply("import (\n    platform,\n    platform.kernel\n)\n", "other"),
        )
        assertNull("already imported", apply("import ( std.io )", "std.io"))
        assertEquals("import ( other )\n\ndef f() {}", apply("def f() {}", "other"))
    }

    // ---------------------------------------------------------------- CompletionSite

    @Test
    fun theKindsOfPlace() {
        fun kind(text: String) = CompletionSite.of(text, text.length, CodeTokens.scan(text)) { it == "list" }.kind
        assertEquals(SiteKind.MEMBER, kind("a.b."))
        assertEquals(SiteKind.MEMBER, kind("a?."))
        assertEquals(SiteKind.MEMBER, kind("a!!."))
        assertEquals(SiteKind.MEMBER, kind("self->"))
        assertEquals("a lambda arrow is not a member access", SiteKind.PLAIN, kind("x := { a -> "))
        assertEquals(SiteKind.MODULE_MEMBER, kind("std.io#"))
        assertEquals(SiteKind.MODULE_NAME, kind("import ( std.io, pla"))
        assertEquals(SiteKind.MODULE_NAME, kind("import (\n    platform,\n    platform."))
        assertEquals(SiteKind.PLAIN, kind("import ( std.io )\nx := y"))
        assertEquals(SiteKind.TYPE, kind("x : "))
        assertEquals(SiteKind.TYPE, kind("def f() : "))
        assertEquals(SiteKind.TYPE_NEW, kind("new "))
        assertEquals(SiteKind.TYPE, kind("class b base "))
        assertEquals(SiteKind.TYPE, kind("catch ("))
        assertEquals(SiteKind.TYPE, kind("x : list<"))
        assertEquals(SiteKind.PLAIN, kind("x := a < "))
        assertEquals(SiteKind.TYPE_OR_NAME, kind("x := c ? a : "))
        assertEquals(SiteKind.NOWHERE, kind("// comment"))
        assertEquals(SiteKind.NOWHERE, kind("x := \"text"))
        assertEquals(SiteKind.NOWHERE, kind("/* open comment"))
        assertEquals(SiteKind.PLAIN, kind("x := \"closed\" + "))
        assertEquals(SiteKind.PLAIN, kind("/* closed */ x"))
    }

    @Test
    fun thePrefixIsWhatWasTypedOfTheName() {
        val text = "a.b.fo"
        val site = CompletionSite.of(text, text.length, CodeTokens.scan(text)) { false }
        assertEquals("fo", site.prefix)
        assertEquals(4, site.prefixStart)
        val inner = "a.fo()"
        val middle = CompletionSite.of(inner, 4, CodeTokens.scan(inner)) { false }
        assertEquals("fo", middle.prefix)
        assertNotNull(middle)
    }
}
