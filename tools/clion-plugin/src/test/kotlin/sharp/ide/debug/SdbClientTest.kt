package sharp.ide.debug

import com.google.gson.JsonObject
import com.google.gson.JsonParser
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.InetAddress
import java.net.Socket
import java.nio.charset.StandardCharsets
import java.util.concurrent.CountDownLatch
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit

/**
 * SdbClient against a made-up sdb: a thread that connects to the client's port, as sdb does, and
 * answers and reports what the test tells it to. No IDE is needed.
 */
class SdbClientTest {
    /** The made-up sdb's end of the connection. */
    private class FakeSdb(port: Int) {
        private val socket = Socket(InetAddress.getLoopbackAddress(), port)
        private val reader = BufferedReader(InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))

        fun received(): JsonObject = JsonParser.parseString(reader.readLine()).asJsonObject
        fun send(json: String) {
            socket.getOutputStream().write((json + "\n").toByteArray(StandardCharsets.UTF_8))
            socket.getOutputStream().flush()
        }
        fun close() = socket.close()
    }

    private val events = LinkedBlockingQueue<JsonObject>()
    private val gone = CountDownLatch(1)
    private val client = SdbClient(onEvent = { events.put(it) }, onGone = { gone.countDown() })

    private fun <T> soon(queue: LinkedBlockingQueue<T>): T = queue.poll(5, TimeUnit.SECONDS) ?: error("nothing arrived within 5 seconds")

    @Test
    fun `a request is sent as one line and its answer goes to who asked`() {
        client.start()
        val sdb = FakeSdb(client.port)
        val answers = LinkedBlockingQueue<JsonObject>()

        client.ask("break", { it.addProperty("file", "C:/app/notebook.sharp"); it.addProperty("line", 41) }) { answers.put(it) }
        val request = sdb.received()
        assertEquals("break", request.get("do").asString)
        assertEquals("C:/app/notebook.sharp", request.get("file").asString)
        assertEquals(41, request.get("line").asInt)

        sdb.send("""{"id": ${request.get("id").asInt}, "ok": true, "number": 3, "line": 41}""")
        val answer = soon(answers)
        assertTrue(SdbClient.ok(answer))
        assertEquals(3, SdbClient.number(answer, "number"))
        sdb.close()
    }

    @Test
    fun `answers find their requests whatever order they come in`() {
        client.start()
        val sdb = FakeSdb(client.port)
        val first = LinkedBlockingQueue<JsonObject>()
        val second = LinkedBlockingQueue<JsonObject>()

        client.ask("threads") { first.put(it) }
        client.ask("frames") { second.put(it) }
        val one = sdb.received().get("id").asInt
        val two = sdb.received().get("id").asInt
        sdb.send("""{"id": $two, "ok": true, "frames": []}""")
        sdb.send("""{"id": $one, "ok": false, "why": "the program is running"}""")

        assertTrue(soon(second).has("frames"))
        val refused = soon(first)
        assertFalse(SdbClient.ok(refused))
        assertEquals("the program is running", SdbClient.text(refused, "why"))
        sdb.close()
    }

    @Test
    fun `what sdb says by itself is an event, not an answer`() {
        client.start()
        val sdb = FakeSdb(client.port)
        sdb.send("""{"event": "stopped", "reason": "breakpoint", "number": 2, "thread": 0, "fiber": 5, "file": "C:/app/x.sharp", "line": 12}""")

        val event = soon(events)
        assertEquals("stopped", SdbClient.text(event, "event"))
        assertEquals(5, SdbClient.number(event, "fiber"))
        assertEquals("", SdbClient.text(event, "nothing"))
        assertEquals(-1, SdbClient.number(event, "nothing", -1))
        sdb.close()
    }

    @Test
    fun `what is asked before sdb connects is sent when it does`() {
        client.start()
        client.ask("start")
        client.ask("pause")
        val sdb = FakeSdb(client.port)

        assertEquals("start", sdb.received().get("do").asString)
        assertEquals("pause", sdb.received().get("do").asString)
        sdb.close()
    }

    @Test
    fun `when sdb goes away, a request still waiting is refused and the client says it is gone`() {
        client.start()
        val sdb = FakeSdb(client.port)
        val answers = LinkedBlockingQueue<JsonObject>()
        client.ask("variables") { answers.put(it) }
        sdb.received()
        sdb.close()

        assertFalse(SdbClient.ok(soon(answers)))
        assertTrue(gone.await(5, TimeUnit.SECONDS))

        // ... and a request made afterwards is refused at once
        client.ask("threads") { answers.put(it) }
        assertFalse(SdbClient.ok(soon(answers)))
    }
}
