package sharp.ide.debug

import com.google.gson.JsonObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Test
import java.io.File
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit

/**
 * SdbClient against the REAL sdb, on Windows: one session as the IDE holds it - a breakpoint, the
 * stop, the calls, the variables, an array opened, a value set, on to the end.
 *
 * It needs what the repository's own check of the Windows sdb leaves behind
 * (`build-scripts/windows/build_sdb.sh`, then `build-scripts/windows/sdb_tests.sh`): sdb.exe and the
 * small program `values`, built with -dbg. Without them the test is skipped.
 */
class SdbClientWithRealSdbTest {
    private val root = sharp.ide.lexer.repoRoot()
    private val sdb = File(root, "cmake-build-windows-sdb/sdb.exe")
    private val program = File(root, "build/windows-sdb-tests/values.dbg")
    private val source = File(root, "debugger/tests/programs/values.sharp")

    private val events = LinkedBlockingQueue<JsonObject>()
    private val client = SdbClient(onEvent = { events.put(it) }, onGone = {})

    /** The next event named `name`; what sdb says in between ("running", "say") is passed over. */
    private fun event(name: String): JsonObject {
        while (true) {
            val next = events.poll(20, TimeUnit.SECONDS) ?: error("no `$name` event within 20 seconds")
            if (SdbClient.text(next, "event") == name) return next
        }
    }

    /** Asks and waits for the answer (the test's thread may wait; the IDE's never does). */
    private fun ask(what: String, fill: (JsonObject) -> Unit = {}): JsonObject {
        val answers = LinkedBlockingQueue<JsonObject>()
        client.ask(what, fill) { answers.put(it) }
        return answers.poll(20, TimeUnit.SECONDS) ?: error("no answer to `$what` within 20 seconds")
    }

    private fun variables(answer: JsonObject): Map<String, JsonObject> =
        answer.getAsJsonArray("variables").map { it.asJsonObject }.associateBy { SdbClient.text(it, "name") }

    /**
     * What the Debug button starts, without the IDE: the debug plan of a folder, made and run with the
     * INSTALLED Sharp - its compiler builds with -dbg, its sdb connects here - and a stop at a breakpoint
     * with the variable of that line's function. Skipped where the installed Sharp has no debugger.
     */
    @Test
    fun theDebugPlanOfAFolderRunsUnderTheInstalledDebugger() {
        val toolchain = sharp.ide.toolchain.SharpToolchains.at(
            sharp.ide.toolchain.SharpToolchains.defaultHome({ if (it == "SHARP_HOME") null else System.getenv(it) }))
        assumeTrue("the installed Sharp has no debugger", toolchain?.debugger != null)
        val base = java.nio.file.Files.createTempDirectory("sharp debug plan").toFile()
        val main = File(base, "app/main.sharp")
        main.parentFile.mkdirs()
        main.writeText("mod main;\n\ndef main(args: string[]) {\n    count : _int64 = 41;\n    count = count + 1;\n    println(\"count \" + count);\n}\n")

        client.start()
        val plan = sharp.ide.execution.SharpRunState.plan(toolchain!!, main.parentFile.path, base.path, emptyList(), "", emptyList(), emptyList(), "",
            false, false, System.getProperty("os.name", "").lowercase().contains("win"), debugPort = client.port)
        sharp.ide.execution.SharpRunState.write(plan)
        val process = ProcessBuilder(plan.command).directory(File(plan.workingDirectory)).redirectErrorStream(true).start()
        val printed = StringBuffer()
        Thread { printed.append(process.inputStream.bufferedReader().readText()) }.start()
        try {
            event("ready")
            val set = ask("break") { it.addProperty("file", main.path); it.addProperty("line", 5) }
            assertTrue("$set\n$printed", SdbClient.ok(set))
            assertTrue(SdbClient.ok(ask("start")))
            val stop = event("stopped")
            assertEquals(5, SdbClient.number(stop, "line"))
            val fiber = SdbClient.number(stop, "fiber")
            val locals = variables(ask("variables") { it.addProperty("fiber", fiber); it.addProperty("frame", 0) })
            assertEquals(locals.toString(), "41", SdbClient.text(locals["count"]!!, "value"))
            assertTrue(SdbClient.ok(ask("continue") { it.addProperty("fiber", fiber) }))
            event("ended")
            assertTrue(process.waitFor(30, TimeUnit.SECONDS))
            Thread.sleep(200)
            assertTrue(printed.toString(), printed.toString().contains("count 42"))
        } finally {
            client.close()
            process.destroyForcibly()
            base.deleteRecursively()
        }
    }

    @Test
    fun aWholeSessionWithTheRealDebugger() {
        assumeTrue("sdb.exe and its test programs are not built", sdb.isFile && program.isFile)
        val line = source.readLines().indexOfFirst { it.contains("total : _int64 = sum(numbers);") } + 1
        assertTrue(line > 0)

        client.start()
        val process = ProcessBuilder(sdb.path, "-machine", client.port.toString(), program.path)
            .redirectErrorStream(true).redirectOutput(ProcessBuilder.Redirect.DISCARD).start()
        try {
            event("ready")

            // a line without code is refused, with the reason; the real one gets a number
            val refused = ask("break") { it.addProperty("file", source.path); it.addProperty("line", 1) }
            assertFalse(refused.toString(), SdbClient.ok(refused))
            assertTrue(SdbClient.text(refused, "why").isNotEmpty())
            val set = ask("break") { it.addProperty("file", source.path); it.addProperty("line", line) }
            assertTrue(set.toString(), SdbClient.ok(set))

            assertTrue(SdbClient.ok(ask("start")))
            val stop = event("stopped")
            assertEquals("breakpoint", SdbClient.text(stop, "reason"))
            assertEquals(SdbClient.number(set, "number"), SdbClient.number(stop, "number"))
            assertEquals(line, SdbClient.number(stop, "line"))
            val fiber = SdbClient.number(stop, "fiber")

            val threads = ask("threads")
            assertTrue(threads.toString(), threads.getAsJsonArray("threads").any { SdbClient.number(it.asJsonObject, "fiber", -1) == fiber })

            val frames = ask("frames") { it.addProperty("fiber", fiber) }.getAsJsonArray("frames").map { it.asJsonObject }
            assertEquals(line, SdbClient.number(frames[0], "line"))
            assertTrue(SdbClient.text(frames[0], "file").replace('\\', '/').endsWith("/values.sharp"))

            val locals = variables(ask("variables") { it.addProperty("fiber", fiber); it.addProperty("frame", 0) })
            assertTrue(locals.keys.toString(), locals.keys.containsAll(listOf("corner", "numbers")))
            val numbers = locals["numbers"]!!
            assertTrue(numbers.get("parts").asBoolean)

            // an array opened, two elements at a time
            val firstTwo = ask("children") {
                it.addProperty("fiber", fiber); it.addProperty("frame", 0); it.addProperty("path", SdbClient.text(numbers, "path"))
                it.addProperty("from", 0); it.addProperty("count", 2)
            }
            assertEquals(4, SdbClient.number(firstTwo, "total"))
            val elements = firstTwo.getAsJsonArray("variables").map { it.asJsonObject }
            assertEquals(listOf("10", "20"), elements.map { SdbClient.text(it, "value") })

            // a value set, read back; one that cannot be is refused with the reason
            val path = SdbClient.text(elements[1], "path")
            val changed = ask("set") { it.addProperty("fiber", fiber); it.addProperty("frame", 0); it.addProperty("path", path); it.addProperty("value", "77") }
            assertTrue(changed.toString(), SdbClient.ok(changed))
            assertEquals("77", SdbClient.text(changed, "value"))
            val wrong = ask("set") { it.addProperty("fiber", fiber); it.addProperty("frame", 0); it.addProperty("path", path); it.addProperty("value", "\"text\"") }
            assertFalse(wrong.toString(), SdbClient.ok(wrong))

            // a step stays in the function and lands on a later line; then on to the end
            assertTrue(SdbClient.ok(ask("next") { it.addProperty("fiber", fiber) }))
            assertTrue(SdbClient.number(event("stopped"), "line") > line)
            assertTrue(SdbClient.ok(ask("continue") { it.addProperty("fiber", fiber) }))
            event("ended")
            assertTrue(process.waitFor(20, TimeUnit.SECONDS))
        } finally {
            client.close()
            process.destroyForcibly()
        }
    }
}
