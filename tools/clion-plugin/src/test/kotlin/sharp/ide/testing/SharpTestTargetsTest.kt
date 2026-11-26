package sharp.ide.testing

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/** What can be run in a test file, found in its text: each test, each group, a function that holds groups, `main`. */
class SharpTestTargetsTest {
    private val text = """
        mod shop_tests;

        import ( std.testing, main )

        // tests("in a comment") { test("nothing") { }; };
        def main() {
            basket_tests();
            tests("prices") {
                test("two cost six") { expect(price(2)).should_be(6); };
                test("a \"quoted\" name") { };
            };
            finish_tests();
        }

        def basket_tests() {
            before_all := "tests(\"in a text\")";
            tests("a shopping basket") {
                before_each { fresh(); };
                test("a new basket is empty") {
                    if(true) { expect(0).should_be(0); }
                };
                test("named " + 3) { };
                test("with ${'$'}{n} in it") { };
            };
            tests("the till") {
                test("opens") { };
            };
        }

        def helper(count: _int64) : _int64 { return count; }
        def one_line() := helper(2);
        class basket { def test(n: _int64) { } def run() { self.test(1); other.tests("x"); } }
    """.trimIndent()

    private fun at(word: String): TestTarget? = SharpTestTargets.at(text, text.indexOf(word))

    @Test
    fun aTestRunsAsItsGroupAndItsName() {
        val test = at("test(\"two cost six\")")!!
        assertEquals(TestTargetKind.TEST, test.kind)
        assertEquals("two cost six", test.title)
        assertEquals(listOf("prices > two cost six"), test.filters)
        assertEquals(listOf("a shopping basket > a new basket is empty"), at("test(\"a new basket is empty\")")!!.filters)
        assertEquals("an escaped quote is the quote", listOf("prices > a \"quoted\" name"), at("test(\"a \\\"quoted\\\" name\")")!!.filters)
    }

    @Test
    fun aGroupRunsByItsName() {
        val group = at("tests(\"a shopping basket\")")!!
        assertEquals(TestTargetKind.GROUP, group.kind)
        assertEquals(listOf("a shopping basket"), group.filters)
    }

    @Test
    fun aFunctionRunsTheGroupsWrittenInItAndMainRunsEverything() {
        val function = at("basket_tests() {")!!
        assertEquals(TestTargetKind.FUNCTION, function.kind)
        assertEquals("basket_tests", function.title)
        assertEquals(listOf("a shopping basket", "the till"), function.filters)

        val main = SharpTestTargets.at(text, text.indexOf("def main()") + 4)!!
        assertEquals(TestTargetKind.ALL, main.kind)
        assertEquals(emptyList<String>(), main.filters)
    }

    @Test
    fun whatCannotBeNamedOrIsNotATestGetsNothing() {
        assertNull("a name that is worked out", at("test(\"named \" + 3)"))
        assertNull("a name with a value put into it", at("test(\"with"))
        assertNull("in a comment", at("tests(\"in a comment\")"))
        assertNull("in a text", at("tests(\\\"in a text"))
        assertNull("a function without groups", at("helper(count"))
        assertNull("a function without a body", at("one_line()"))
        assertNull("a function someone called test", SharpTestTargets.at(text, text.indexOf("def test(n") + 4))
        assertNull("a member called test", at("test(1)"))
        assertNull("a member called tests", at("tests(\"x\")"))
        // every target of the file, and no other
        assertEquals(
            listOf("ALL all tests", "GROUP prices", "TEST two cost six", "TEST a \"quoted\" name",
                "FUNCTION basket_tests", "GROUP a shopping basket", "TEST a new basket is empty", "GROUP the till", "TEST opens"),
            SharpTestTargets.of(text).map { "${it.kind} ${it.title}" },
        )
    }
}
