/*
 * TALKING TO sdb, THE SHARP DEBUGGER
 *
 * sdb is a program of its own (debugger in the Sharp repository): the Sharp VM with a debugger
 * compiled in. Started as  `sdb -machine <port> program`  it connects to that port of this computer
 * and from then on the two exchange JSON, one object on a line each (debugger/machine/machine.h
 * has every request and event):
 *
 *     the IDE asks     {"id": 7, "do": "break", "file": "C:/app/notebook.sharp", "line": 41}
 *     sdb answers      {"id": 7, "ok": true, "number": 1, "line": 41}        - or  "ok": false, "why": "..."
 *     sdb reports      {"event": "stopped", "reason": "breakpoint", "number": 1, "thread": 0, "fiber": 0, ...}
 *
 * The program's own input and output are NOT on this connection: they are the process's streams,
 * which the Debug console shows. So the IDE can ask while the program runs (a new breakpoint, a pause).
 *
 * THIS CLASS is the IDE's end: it listens, waits for sdb to connect, sends requests and hands each
 * answer to whoever asked. It has no IDE classes in it, so it is tested with a made-up sdb.
 *
 * THREADS. One thread of this class reads the connection for as long as it lasts; every answer and
 * every event is handed over ON THAT THREAD, in the order sdb sent them. So what is handed over must
 * not wait there for another answer - it asks again and gives the next step as that request's answer.
 * `ask` may be called from any thread and never waits: what is asked before sdb has connected is
 * kept and sent as soon as it has.
 *
 * Lines are counted from 1 on this connection, as sdb counts them. (The IDE counts from 0.)
 */
package sharp.ide.debug

import com.google.gson.JsonObject
import com.google.gson.JsonParser
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.nio.charset.StandardCharsets
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicInteger

class SdbClient(
    /** An event sdb sent by itself: ready, stopped, running, ended, say. */
    private val onEvent: (JsonObject) -> Unit,
    /** The connection is gone (sdb ended, or never connected). Called once. */
    private val onGone: () -> Unit,
) {
    private val server = ServerSocket(0, 1, InetAddress.getLoopbackAddress())
    private val answers = ConcurrentHashMap<Int, (JsonObject) -> Unit>()
    private val nextId = AtomicInteger(0)
    private val lock = Any()
    private var output: OutputStream? = null
    private val waiting = ArrayList<String>()          // asked before sdb connected
    @Volatile private var closed = false

    /** The port sdb is told to connect to. */
    val port: Int get() = server.localPort

    /** Start waiting for sdb to connect, on a thread of this class. */
    fun start() {
        val thread = Thread({ serve() }, "Sharp debugger connection")
        thread.isDaemon = true
        thread.start()
    }

    /**
     * Send a request. `answer` gets what sdb says back - always: when the connection goes before the
     * answer came, it gets `{"ok": false, "why": ...}`.
     */
    fun ask(what: String, fill: (JsonObject) -> Unit = {}, answer: (JsonObject) -> Unit = {}) {
        val id = nextId.incrementAndGet()
        val request = JsonObject()
        request.addProperty("id", id)
        request.addProperty("do", what)
        fill(request)
        answers[id] = answer
        if (closed) {
            answers.remove(id)?.invoke(gone())
            return
        }
        send(request.toString())
    }

    /** Let go of the connection. Requests still waiting for an answer get a refusal. */
    fun close() {
        if (closed) return
        closed = true
        try { server.close() } catch (_: Exception) {}
        synchronized(lock) { try { output?.close() } catch (_: Exception) {} }
    }

    private fun send(line: String) {
        synchronized(lock) {
            val out = output
            if (out == null) {
                waiting += line
                return
            }
            try {
                out.write((line + "\n").toByteArray(StandardCharsets.UTF_8))
                out.flush()
            } catch (_: Exception) {
                // the reading thread sees the same end of the connection and tells everyone
            }
        }
    }

    private fun serve() {
        var socket: Socket? = null
        try {
            socket = server.accept()
            socket.tcpNoDelay = true
            synchronized(lock) {
                output = socket.getOutputStream()
                for (line in waiting) {
                    output!!.write((line + "\n").toByteArray(StandardCharsets.UTF_8))
                }
                output!!.flush()
                waiting.clear()
            }
            val reader = BufferedReader(InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))
            while (true) {
                val line = reader.readLine() ?: break
                if (line.isBlank()) continue
                val message = try { JsonParser.parseString(line).asJsonObject } catch (_: Exception) { continue }
                if (message.has("event")) {
                    onEvent(message)
                } else {
                    answers.remove(message.get("id")?.asInt ?: 0)?.invoke(message)
                }
            }
        } catch (_: Exception) {
            // closed from here, or sdb went away
        } finally {
            closed = true
            try { socket?.close() } catch (_: Exception) {}
            try { server.close() } catch (_: Exception) {}
            for (id in answers.keys.toList()) answers.remove(id)?.invoke(gone())
            onGone()
        }
    }

    private fun gone(): JsonObject {
        val refusal = JsonObject()
        refusal.addProperty("ok", false)
        refusal.addProperty("why", "the debugger is not connected")
        return refusal
    }

    companion object {
        /** `"ok": true` in an answer. */
        fun ok(answer: JsonObject): Boolean = answer.get("ok")?.asBoolean == true

        /** A text member of a message, or "" when it has none. */
        fun text(message: JsonObject, name: String): String = message.get(name)?.takeIf { it.isJsonPrimitive }?.asString ?: ""

        /** A whole-number member of a message, or `otherwise`. */
        fun number(message: JsonObject, name: String, otherwise: Int = 0): Int =
            message.get(name)?.takeIf { it.isJsonPrimitive }?.let { try { it.asInt } catch (_: Exception) { null } } ?: otherwise
    }
}
