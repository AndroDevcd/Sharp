/*
 * SavedText - a file's text the way the compiler counted it, and its fingerprint.
 *
 * WHAT THIS IS FOR
 *   The compiler reads a file's BYTES and writes positions as offsets into the text
 *   "as the editor sees it" (doc/ide.md section 4.1): UTF-8 decoded, a byte-order
 *   mark not counted, every kind of line end (`\r\n`, a lone `\r`) shown as one `\n`.
 *   `decode` makes exactly that text, so an offset of the compiler is an index into it.
 *
 *   `hash` is the FNV-1a 32-bit fingerprint the compiler writes for every file in
 *   `ide.json` ("hash"). Comparing the two tells the plugin whether the file on disk
 *   is still the text the compiler saw.
 *
 *     FNV-1a: start with 0x811C9DC5; for each byte: xor it in, multiply by 0x01000193
 *     (keeping 32 bits). Written as eight lowercase hex digits.
 */
package sharp.ide.run

import java.nio.ByteBuffer
import java.nio.charset.CodingErrorAction
import java.nio.charset.StandardCharsets

object SavedText {

    /** FNV-1a (32 bit) over `bytes`, as eight lowercase hex digits (the compiler's `files[].hash`). */
    fun hash(bytes: ByteArray): String {
        var h = 0x811C9DC5.toInt()
        for (b in bytes) {
            h = h xor (b.toInt() and 0xFF)
            h *= 0x01000193
        }
        return String.format("%08x", h)
    }

    /** The text the editor shows for these bytes: no BOM, `\r\n` and lone `\r` turned into `\n`, bad bytes as U+FFFD. */
    fun decode(bytes: ByteArray): String {
        val start = if (bytes.size >= 3 && bytes[0] == 0xEF.toByte() && bytes[1] == 0xBB.toByte() && bytes[2] == 0xBF.toByte()) 3 else 0
        val decoder = StandardCharsets.UTF_8.newDecoder()
            .onMalformedInput(CodingErrorAction.REPLACE)
            .onUnmappableCharacter(CodingErrorAction.REPLACE)
        val text = decoder.decode(ByteBuffer.wrap(bytes, start, bytes.size - start)).toString()
        return if (text.indexOf('\r') < 0) text else text.replace("\r\n", "\n").replace('\r', '\n')
    }
}
