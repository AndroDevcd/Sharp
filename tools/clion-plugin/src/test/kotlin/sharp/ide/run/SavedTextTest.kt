package sharp.ide.run

import org.junit.Assert.assertEquals
import org.junit.Test

/** The saved text as the compiler counts it, and the FNV-1a fingerprint. */
class SavedTextTest {
    @Test fun fnvOfKnownValues() {
        assertEquals("811c9dc5", SavedText.hash(ByteArray(0)))              // the offset basis
        assertEquals("e40c292c", SavedText.hash("a".toByteArray()))           // published FNV-1a 32 test vectors
        assertEquals("bf9cf968", SavedText.hash("foobar".toByteArray()))
    }

    @Test fun lineEndsBecomeOneCharacterEach() {
        assertEquals("a\nb\nc\nd", SavedText.decode("a\r\nb\rc\nd".toByteArray()))
    }

    @Test fun aByteOrderMarkIsNotCounted() {
        assertEquals("x", SavedText.decode(byteArrayOf(0xEF.toByte(), 0xBB.toByte(), 0xBF.toByte(), 'x'.code.toByte())))
    }

    @Test fun anEmojiIsTwoUnitsAndAnAccentOne() {
        val text = SavedText.decode("é😀".toByteArray(Charsets.UTF_8))
        assertEquals(3, text.length)
    }

    @Test fun aBrokenByteIsOneReplacementCharacter() {
        assertEquals("a�b", SavedText.decode(byteArrayOf('a'.code.toByte(), 0xFF.toByte(), 'b'.code.toByte())))
    }
}
