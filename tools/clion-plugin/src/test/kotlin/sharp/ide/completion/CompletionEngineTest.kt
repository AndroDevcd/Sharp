package sharp.ide.completion

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import sharp.ide.index.IndexFixture
import java.nio.file.Files
import java.nio.file.Path

/**
 * COMPLETION, WITHOUT THE PLATFORM: the ten situations of the design review (doc/ide.md 6.8) and more
 *
 * The index is the compiler's real output for `index-fixtures/completion/source.sharp` compiled with the
 * whole standard library. Each situation is asked twice:
 *   FRESH  the text is exactly what the compiler saw; the caret sits right after a dot that was already
 *          there (`names.|size()`), so the compiler's own table of uses knows the receiver.
 *   STALE  the person has typed a new line since: the text before the caret was not in the file when
 *          the compiler ran (and the file would not even parse). Only the last good index and the text
 *          are there.
 */
class CompletionEngineTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded
    private val saved: String get() = shop.fixture.savedText

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-completion")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    // ---------------------------------------------------------------- helpers

    private fun fresh(needle: String): CompletionResult {
        val caret = saved.indexOf(needle) + needle.length
        require(saved.indexOf(needle) >= 0) { "no `$needle`" }
        return CompletionEngine.complete(shop.view(), saved, caret)
    }

    /** Types `code` on a new line before the line that starts with `marker`, and asks at the end of it. */
    private fun stale(code: String, marker: String = "    return count.value;"): CompletionResult {
        val at = saved.indexOf(marker)
        require(at >= 0) { "no `$marker`" }
        val text = saved.substring(0, at) + "    " + code + "\n" + saved.substring(at)
        return CompletionEngine.complete(shop.view(text), text, at + 4 + code.length)
    }

    private fun names(r: CompletionResult) = r.candidates.map { it.name }
    private fun item(r: CompletionResult, name: String) = r.candidates.firstOrNull { it.name == name }

    // ---------------------------------------------------------------- 1: a local after a dot

    @Test
    fun situation1_aLocalAfterADot_fresh() {
        val r = fresh("names.")
        assertEquals(SiteKind.MEMBER, r.site.kind)
        assertEquals("", r.prefix)
        val n = names(r)
        assertTrue(n.toString(), n.containsAll(listOf("add", "at", "size", "first", "last", "clear", "sort")))
        assertFalse("private members stay hidden", "expand" in n)
        assertFalse("an operator is not offered after a dot", "operator[]" in n)
        assertEquals("string", item(r, "at")!!.typeText)                // T replaced by string, not shown as T
        assertEquals("(element: string)", item(r, "add")!!.tailText)
        assertEquals(Insertion.CALL, item(r, "add")!!.insertion)
        assertTrue(item(r, "add")!!.hasParameters)
        assertFalse(item(r, "size")!!.hasParameters)
    }

    @Test
    fun situation1_aLocalAfterADot_stale() {
        val r = stale("names.")
        assertTrue(names(r).containsAll(listOf("add", "at", "size")))
        assertEquals("string", item(r, "first")!!.typeText)
    }

    @Test
    fun aLocalDeclaredSinceTheLastRunIsReadFromTheText() {
        val typed = stale("fresh_book : book;\n    fresh_book.")
        assertEquals(SiteKind.MEMBER, typed.site.kind)
        assertTrue(names(typed).containsAll(listOf("pages", "summary", "title", "label")))

        val made = stale("made_it := new book(\"z\");\n    made_it.")
        assertTrue(names(made).containsAll(listOf("pages", "summary", "price")))

        val text = stale("words := names;\n    words.")
        assertTrue("a name copied from a parameter", names(text).containsAll(listOf("add", "size")))

        val literal = stale("greeting := \"hi\";\n    greeting.")
        assertTrue(names(literal).containsAll(listOf("shout", "size")))
    }

    @Test
    fun aForeachVariableTakesTheItemTypeOfItsCollection() {
        val r = stale("foreach(word in names) {\n        word.")
        assertTrue(names(r).toString(), names(r).containsAll(listOf("shout", "size")))
    }

    // ---------------------------------------------------------------- 2: a field, inherited members, bold

    @Test
    fun situation2_aParameterOfOurOwnClass_fresh() {
        val r = fresh("owner.")
        val n = names(r)
        assertTrue(n.toString(), n.containsAll(listOf("pages", "summary", "title", "cost", "label", "price")))
        assertFalse("private", "secret" in n)
        assertFalse("static members are not on a value", "make" in n || "created" in n)
        assertTrue(item(r, "summary")!!.bold)                            // declared in book itself
        assertFalse(item(r, "label")!!.bold)                             // inherited
        assertTrue("own members rank above inherited ones", item(r, "summary")!!.priority > item(r, "label")!!.priority)
        assertEquals("string", item(r, "title")!!.typeText)
        assertEquals(CandidateKind.FIELD, item(r, "title")!!.kind)
    }

    @Test
    fun situation2_aParameterOfOurOwnClass_stale() {
        val r = stale("owner.")
        assertTrue(names(r).containsAll(listOf("pages", "summary", "title", "label")))
    }

    @Test
    fun inheritedMembersComeFromTheWholeChainAndTheInterface() {
        val r = stale("owner.")
        // book -> item -> (interface priced) -> _object_
        val price = item(r, "price")!!
        assertEquals(80, price.priority)                                 // item is one step up
        assertTrue(r.candidates.any { it.priority == 30 })               // what every class gets from _object_
        assertTrue("object members come last", item(r, "summary")!!.priority > 80)
    }

    // ---------------------------------------------------------------- 3: a call result

    @Test
    fun situation3_aCallResult_fresh() {
        val r = fresh("make_book().")
        assertTrue(names(r).containsAll(listOf("pages", "summary", "label")))
    }

    @Test
    fun situation3_aCallResult_stale() {
        val r = stale("make_book().")
        assertTrue(names(r).containsAll(listOf("pages", "summary", "label")))
        val method = stale("owner.summary().")
        assertTrue("the result of a method: string", names(method).containsAll(listOf("shout", "size")))
    }

    // ---------------------------------------------------------------- 4: self

    @Test
    fun situation4_self_fresh() {
        val r = fresh("self.")
        assertTrue(names(r).toString(), names(r).containsAll(listOf("pages", "summary", "title", "label")))
    }

    @Test
    fun situation4_selfAndAnArrow_stale() {
        val marker = "        return text;"
        val at = saved.indexOf(marker)
        for (typed in listOf("self.", "self->")) {
            val text = saved.substring(0, at) + "        " + typed + "\n" + saved.substring(at)
            val r = CompletionEngine.complete(shop.view(text), text, at + 8 + typed.length)
            assertEquals(typed, SiteKind.MEMBER, r.site.kind)
            assertTrue(typed + names(r), names(r).containsAll(listOf("pages", "summary", "title", "label")))
        }
    }

    @Test
    fun selfInAStaticFunctionOffersNothing() {
        val r = stale("self.")                                           // inside `probe`, a global function
        assertTrue(names(r).toString(), r.candidates.isEmpty())
    }

    // ---------------------------------------------------------------- 5: generics

    @Test
    fun situation5_typeArgumentsArePutIn_fresh() {
        val chain = fresh("names.at(0).")
        assertTrue(names(chain).containsAll(listOf("shout", "size", "substring")) || names(chain).containsAll(listOf("shout", "size")))
        val box = fresh("box.")
        assertEquals("string", item(box, "get_value")!!.typeText)
        assertEquals("string", item(box, "value")!!.typeText)
    }

    @Test
    fun situation5_typeArgumentsArePutIn_stale() {
        val chain = stale("names.at(0).")
        assertTrue(names(chain).containsAll(listOf("shout", "size")))
        val box = stale("box.get_value().")
        assertTrue("the result is a string", names(box).containsAll(listOf("shout", "size")))
    }

    @Test
    fun typeArgumentsGoThroughAGenericBaseClass() {
        val r = fresh("tagged.")
        // labelled<K> base holder<K>: with K = string, holder's T becomes string too
        assertEquals("string", item(r, "get_value")!!.typeText)
        assertEquals("string", item(r, "value")!!.typeText)
        assertTrue(names(r).contains("caption"))
        assertTrue(item(r, "caption")!!.priority > item(r, "get_value")!!.priority)
    }

    @Test
    fun anArrayOfTheTypeParameterGetsTheArgumentToo() {
        val r = fresh("names.")
        assertEquals("string[]", item(r, "to_array")!!.typeText)
        assertEquals("string[]?", item(r, "get_elements")!!.typeText)
    }

    @Test
    fun anExtensionFunctionIsOneOfTheMembers() {
        val r = stale("owner.title.")
        assertTrue(names(r).contains("shout"))
    }

    // ---------------------------------------------------------------- 6: a class name

    @Test
    fun situation6_aClassName_fresh() {
        val r = fresh("item.")
        val n = names(r)
        assertTrue(n.toString(), n.containsAll(listOf("make", "created")))
        assertFalse("instance members are not offered on a class name", "title" in n || "label" in n)
    }

    @Test
    fun situation6_aClassName_stale() {
        val r = stale("item.")
        assertTrue(names(r).containsAll(listOf("make", "created")))
        assertFalse(names(r).contains("price"))
    }

    // ---------------------------------------------------------------- 7: plain names

    @Test
    fun situation7_aPlainNameInAFunction() {
        val r = stale("cou")
        assertEquals(SiteKind.PLAIN, r.site.kind)
        assertEquals("cou", r.prefix)
        val count = item(r, "count")!!
        assertEquals(CandidateKind.LOCAL, count.kind)
        assertEquals(100, count.priority)
        assertEquals("int", count.typeText)
        // parameters, other locals, functions and classes of the program and the keywords are there too
        for (name in listOf("names", "owner", "local_book", "make_book", "item", "book", "return", "color")) assertNotNull(name, item(r, name))
        assertEquals(CandidateKind.PARAMETER, item(r, "names")!!.kind)
        assertTrue(item(r, "return")!!.priority < item(r, "make_book")!!.priority)
    }

    @Test
    fun aPlainNameInAMethodOffersTheClassItsBasesAndNotTheCallersLocals() {
        val marker = "        return text;"
        val at = saved.indexOf(marker)
        val text = saved.substring(0, at) + "        ti\n" + saved.substring(at)
        val r = CompletionEngine.complete(shop.view(text), text, at + 10)
        assertEquals("ti", r.prefix)
        for (name in listOf("title", "label", "pages", "summary", "price", "text", "make", "created")) assertNotNull(name, item(r, name))
        assertNull("a local of another function", item(r, "count"))
        assertEquals(100, item(r, "text")!!.priority)
    }

    @Test
    fun aLocalTypedSinceTheLastRunIsOfferedByName() {
        val r = stale("brand_new := 5;\n    bran")
        assertEquals(100, item(r, "brand_new")!!.priority)
    }

    // ---------------------------------------------------------------- 8: types

    @Test
    fun situation8_typesAfterAColonNewAndAs() {
        val afterColon = stale("n : ")
        assertEquals(SiteKind.TYPE, afterColon.site.kind)
        for (name in listOf("item", "book", "priced", "color", "holder", "string", "list", "decimal", "_int64", "object")) assertNotNull(name, item(afterColon, name))
        assertNull("no keywords or locals where a type goes", item(afterColon, "return") ?: item(afterColon, "count"))

        val afterNew = stale("n := new ")
        assertEquals(SiteKind.TYPE_NEW, afterNew.site.kind)
        assertNotNull(item(afterNew, "book"))
        assertEquals(Insertion.NEW, item(afterNew, "book")!!.insertion)
        assertNull("an interface or enum cannot be made", item(afterNew, "priced") ?: item(afterNew, "color"))
        assertNull("not a type word", item(afterNew, "decimal"))

        val afterAs = stale("n := owner as ")
        assertEquals(SiteKind.TYPE, afterAs.site.kind)
        assertNotNull(item(afterAs, "item"))

        val inParameters = stale("def inner(a: ")
        assertNotNull(item(inParameters, "book"))

        val afterResult = stale("def inner() : ")
        assertEquals(SiteKind.TYPE, afterResult.site.kind)
    }

    @Test
    fun typesInsideTheArgumentsOfAGenericType() {
        assertEquals(SiteKind.TYPE, stale("n : list<").site.kind)
        assertEquals(SiteKind.TYPE, stale("n : hash_map<string, ").site.kind)
        assertEquals("a comparison is not a type", SiteKind.PLAIN, stale("n := count < ").site.kind)
    }

    @Test
    fun aTernaryColonOffersTypesAndNames() {
        val r = stale("n := count ? owner : ")
        assertEquals(SiteKind.TYPE_OR_NAME, r.site.kind)
        assertNotNull(item(r, "owner"))
        assertNotNull(item(r, "book"))
    }

    @Test
    fun typeParametersOfTheClassAreTypes() {
        val marker = "    def get_value() : T { return value; }"
        val at = saved.indexOf(marker)
        val text = saved.substring(0, at) + "    x : " + "\n" + saved.substring(at)
        val r = CompletionEngine.complete(shop.view(text), text, at + 8)
        assertNotNull("T inside the generic class holder", item(r, "T"))
        assertEquals(CandidateKind.TYPE_PARAMETER, item(r, "T")!!.kind)
        assertNull("not in a function of another class", item(stale("n : "), "T"))
    }

    // ---------------------------------------------------------------- 9: module names

    @Test
    fun situation9_moduleNamesInAnImportList() {
        val text = "mod shop;\n\nimport ( std."
        val view = shop.view(text)
        val r = CompletionEngine.complete(view, text, text.length)
        assertEquals(SiteKind.MODULE_NAME, r.site.kind)
        assertEquals("std.", r.prefix)
        assertTrue(names(r).toString(), names(r).containsAll(listOf("std", "std.io", "std.io.console")))
        assertTrue(r.candidates.all { it.kind == CandidateKind.MODULE })

        val second = "mod shop;\n\nimport ( std.io, sho"
        val r2 = CompletionEngine.complete(shop.view(second), second, second.length)
        assertEquals("sho", r2.prefix)
        assertTrue(names(r2).contains("shop"))
    }

    @Test
    fun aModuleNameAfterMod() {
        val text = "mod sh"
        val r = CompletionEngine.complete(shop.view(text), text, text.length)
        assertEquals(SiteKind.MODULE_NAME, r.site.kind)
        assertTrue(names(r).contains("shop"))
    }

    @Test
    fun aModuleQualifiedName() {
        val r = stale("std.io#")
        assertEquals(SiteKind.MODULE_MEMBER, r.site.kind)
        assertTrue(names(r).toString(), names(r).contains("file"))
        assertTrue(names(r).contains("println") || names(r).contains("file"))
    }

    // ---------------------------------------------------------------- 10: an enum

    @Test
    fun situation10_anEnumsValues_fresh() {
        val r = fresh("color.")
        assertTrue(names(r).toString(), names(r).containsAll(listOf("red", "green", "blue")))
        assertEquals(CandidateKind.ENUM_VALUE, item(r, "red")!!.kind)
    }

    @Test
    fun situation10_anEnumsValues_stale() {
        val r = stale("color.")
        assertEquals(listOf("blue", "green", "red"), names(r).filter { it in setOf("red", "green", "blue") }.sorted())
    }

    // ---------------------------------------------------------------- not offered, degrading

    @Test
    fun nothingInsideACommentOrAString() {
        for (code in listOf("// owner.", "/* count", "text := \"abc owner", "text := \"${'$'}{count} abc ")) {
            val r = stale(code)
            assertEquals(code, SiteKind.NOWHERE, r.site.kind)
            assertTrue(code, r.candidates.isEmpty())
        }
    }

    @Test
    fun insideAnInterpolationNamesAreOffered() {
        val r = stale("text := \"total: \${cou")
        assertEquals(SiteKind.PLAIN, r.site.kind)
        assertNotNull(item(r, "count"))
    }

    @Test
    fun withoutAnIndexOnlyKeywordsAreOffered() {
        val r = CompletionEngine.complete(null, "ret", 3)
        assertTrue(names(r).contains("return"))
        assertTrue(CompletionEngine.complete(null, "x.", 2).candidates.isEmpty())
    }

    @Test
    fun aFileTheCompilerNeverSawStillGetsDeclarationsByName() {
        val text = "mod shop;\nimport ( std.io )\n\ndef scratch() {\n    local_one := new book(\"a\");\n    local_one.\n}\n"
        val view = sharp.ide.editor.FileView(shop.index, null, null, text)
        val caret = text.indexOf("local_one.\n") + "local_one.".length
        val r = CompletionEngine.complete(view, text, caret)
        assertTrue("the variable is read from the text, the class by name", names(r).containsAll(listOf("pages", "summary", "title")))

        val plain = "mod shop;\n\ndef scratch() {\n    boo\n}\n"
        val r2 = CompletionEngine.complete(sharp.ide.editor.FileView(shop.index, null, null, plain), plain, plain.indexOf("boo") + 3)
        assertNotNull(item(r2, "book"))
    }

    @Test
    fun anUnknownReceiverGivesAnEmptyListNotAWrongOne() {
        assertTrue(stale("nothing_like_it.").candidates.isEmpty())
        assertTrue(stale("(count + count).").candidates.isEmpty())
        assertTrue(stale("owner.no_such_field.").candidates.isEmpty())
    }

    // ---------------------------------------------------------------- classes of modules that are not imported

    @Test
    fun aClassOfAModuleThatIsNotImportedComesWithItsImport() {
        val scope = shop.view().lookup.scopeFor(shop.file, saved)
        val other = shop.index.modules.entries.first { it.key !in scope.modules && it.key.isNotEmpty() && it.value.classes.any { c -> c.kind == "class" && !c.hasFlag("private") } }
        val target = other.value.classes.first { it.kind == "class" && !it.hasFlag("private") }.name
        val r = stale("n : $target")
        val candidate = r.candidates.first { it.name == target && it.importModule == other.key }
        assertEquals(35, candidate.priority)
        assertEquals("  " + other.key, candidate.tailText)
        val viaNew = stale("n := new $target")
        assertNotNull(viaNew.candidates.firstOrNull { it.name == target && it.importModule == other.key })
        // an imported class has no import attached
        assertNull(item(stale("n : "), "book")!!.importModule)
    }
}
