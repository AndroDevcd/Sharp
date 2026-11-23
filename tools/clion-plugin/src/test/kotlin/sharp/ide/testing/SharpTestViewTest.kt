package sharp.ide.testing

import jetbrains.buildServer.messages.serviceMessages.ServiceMessage
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Test
import sharp.ide.execution.SharpRunState
import sharp.ide.toolchain.SharpToolchains
import java.io.File
import java.nio.charset.StandardCharsets
import java.util.concurrent.TimeUnit

/**
 * The test view's two ends that need no IDE window: the place a location hint names, and - with the
 * installed Sharp - what a real test program prints when it is started for the test view, read by
 * the platform's own reader of such lines.
 */
class SharpTestViewTest {
    @Test
    fun aLocationHintNamesAFileAndALine() {
        assertEquals(TestPlace("C:/work/my app/tests.sharp", 12), SharpTestLocator.placeOf("C:/work/my app/tests.sharp:12"))
        assertEquals(TestPlace("C:/work/tests.sharp", 3), SharpTestLocator.placeOf("C:\\work\\tests.sharp:3"))
        assertEquals(TestPlace("/home/me/tests.sharp", 1), SharpTestLocator.placeOf("/home/me/tests.sharp:1"))
        assertNull(SharpTestLocator.placeOf("C:/work/tests.sharp"))          // no line: the last colon is the drive's
        assertNull(SharpTestLocator.placeOf("tests.sharp:0"))
        assertNull(SharpTestLocator.placeOf("tests.sharp:x"))
    }

    @Test
    fun aRealTestProgramReportsItsGroupsAndTestsForTheView() {
        // (the tests run with SHARP_HOME pointing at an empty place - build.gradle.kts; this one wants the real one)
        val toolchain = SharpToolchains.at(SharpToolchains.defaultHome({ if (it == "SHARP_HOME") null else System.getenv(it) }))
        assumeTrue("Sharp is not installed on this machine: nothing to run", toolchain != null)
        val base = java.nio.file.Files.createTempDirectory("sharp test view").toFile()
        val main = File(base, "checks/main.sharp")
        main.parentFile.mkdirs()
        main.writeText("""
            mod main;

            import ( std.io, std.testing )

            def main(args: string[]) {
                tests("a basket") {
                    test("it's empty") { expect(0).should_be(0); };
                    test("adding puts one in") {
                        expect(2).should_be(1);
                    };
                };
                tests("second group") {
                    test("fine") { expect("a").should_be("a"); };
                };
                finish_tests();
            }
        """.trimIndent())
        try {
            val windows = System.getProperty("os.name", "").lowercase().contains("win")
            val plan = SharpRunState.plan(toolchain!!, main.parentFile.path, base.path, emptyList(), "", emptyList(),
                listOf(SharpTestView.ARGUMENT), "", false, false, windows)
            SharpRunState.write(plan)
            val process = ProcessBuilder(plan.command).directory(File(plan.workingDirectory)).redirectErrorStream(true).start()
            val output = process.inputStream.bufferedReader(StandardCharsets.UTF_8).readText().replace("\r\n", "\n")
            assertTrue("the run did not end in three minutes", process.waitFor(180, TimeUnit.SECONDS))
            assertEquals(output, 1, process.exitValue())                    // a test failed: the program says so

            // every line for the view is one the platform's reader takes
            val messages = output.lines().filter { it.startsWith("##teamcity[") }.map { ServiceMessage.parse(it) ?: error("not a service message: $it") }
            assumeTrue("the installed Sharp's std.testing does not report for an editor yet (older than this plugin)", messages.isNotEmpty())
            assertEquals(output, listOf(
                "testSuiteStarted a basket", "testStarted it's empty", "testFinished it's empty",
                "testStarted adding puts one in", "testFailed adding puts one in", "testFinished adding puts one in",
                "testSuiteFinished a basket",
                "testSuiteStarted second group", "testStarted fine", "testFinished fine", "testSuiteFinished second group",
            ), messages.map { "${it.messageName} ${it.attributes["name"]}" })

            // where each is written: this file, the line of its tests(...) or test(...)
            val lines = main.readLines()
            fun lineOf(text: String) = lines.indexOfFirst { it.contains(text) } + 1
            fun placeOf(message: ServiceMessage): TestPlace = SharpTestLocator.placeOf(message.attributes["locationHint"]!!.removePrefix("sharp://"))!!
            val group = placeOf(messages[0])
            assertEquals(main.canonicalPath.replace('\\', '/').lowercase(), File(group.file).canonicalPath.replace('\\', '/').lowercase())
            assertEquals(lineOf("tests(\"a basket\")"), group.line)
            assertEquals(lineOf("test(\"it's empty\")"), placeOf(messages[1]).line)
            assertEquals(lineOf("test(\"adding puts one in\")"), placeOf(messages[3]).line)

            // the failed comparison hands over both values, and says the line of the check
            val failure = messages[4].attributes
            assertEquals("comparisonFailure", failure["type"])
            assertEquals("1", failure["expected"])
            assertEquals("2", failure["actual"])
            assertTrue(failure["details"]!!, failure["details"]!!.contains(", line ${lineOf("expect(2).should_be(1)")}"))
        } finally {
            base.deleteRecursively()
        }
    }
}
