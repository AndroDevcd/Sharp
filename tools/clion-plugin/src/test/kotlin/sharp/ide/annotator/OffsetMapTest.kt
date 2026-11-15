package sharp.ide.annotator

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.Random

/**
 * MAPPING A RANGE OF THE OLD TEXT ONTO EDITED TEXT
 *
 * Old text:  "aaa BBB ccc ddd"   (BBB is the marked range, [4,7))
 */
class OffsetMapTest {
    private val old = "aaa BBB ccc ddd"

    private fun mapped(new: String, from: Int = 4, to: Int = 7): String? {
        val range = OffsetMap(old, new).mapRange(from, to) ?: return null
        return new.substring(range.start, range.end)
    }

    @Test fun anUnchangedTextMapsToItself() {
        assertEquals("BBB", mapped(old))
        assertTrue(OffsetMap(old, old).unchanged)
    }

    @Test fun anEditBeforeShiftsTheRange() {
        assertEquals("BBB", mapped("// new line\n$old"))
        assertEquals("BBB", mapped("a BBB ccc ddd"))
    }

    @Test fun anEditAfterLeavesTheRange() {
        assertEquals("BBB", mapped("aaa BBB ccc ddd and more"))
        assertEquals("BBB", mapped("aaa BBB"))
    }

    @Test fun anEditInsideTheMarkedTextDropsTheRange() {
        assertNull(mapped("aaa BXB ccc ddd"))
        assertNull(mapped("aaa BB ccc ddd"))
        assertNull(mapped("aaa B ccc ddd"))
        assertNull(mapped("aaa  ccc ddd"))                         // the marked text was deleted
    }

    @Test fun anEditThatTouchesAnEdgeOfTheMarkedTextDropsTheRange() {
        assertNull(mapped("aaa XBB ccc ddd"))                      // first character replaced
        assertNull(mapped("aaa BBX ccc ddd"))                      // last character replaced
    }

    @Test fun typingRightBesideTheRangeKeepsIt() {
        assertEquals("BBB", mapped("aaa XBBB ccc ddd"))            // inserted just before
        assertEquals("BBB", mapped("aaa BBBX ccc ddd"))            // inserted just after
        assertEquals("BBB", mapped("aaa BBB ccc ddd".replace(" BBB", "X BBB")))
    }

    @Test fun anEditInTheMiddleOfALongRangeDropsItToo() {
        val text = "call(a, b)"                                          // the range is the whole call, the edit is one argument
        assertNull(OffsetMap(text, "call(a, bb)").mapRange(0, text.length))
        assertNull(OffsetMap(text, "call(x, b)").mapRange(0, text.length))
    }

    @Test fun aZeroLengthRangeIsAPoint() {
        val map = OffsetMap(old, "XX$old")
        val range = map.mapRange(4, 4)!!
        assertEquals(6, range.start); assertEquals(6, range.end)
        assertNull(OffsetMap("abcd", "abXd").mapRange(2, 2))       // the point was where the edit is
    }

    @Test fun deletionsShiftBackwards() {
        assertEquals("BBB", mapped("BBB ccc ddd".let { "a $it" }))  // "aaa " became "a "
    }

    @Test fun emptyAndCompletelyDifferentTexts() {
        assertNull(OffsetMap("abc", "").mapRange(0, 1))
        assertNull(OffsetMap("abc", "xyz").mapRange(0, 1))
        assertNotNull(OffsetMap("", "").mapRange(0, 0))
    }

    /** A range that is kept must still be the same characters: whatever random edit is made, never a wrong range. */
    @Test fun aMappedRangeAlwaysHoldsTheSameCharactersAsBefore() {
        val random = Random(42)
        val alphabet = "abc \n"
        repeat(3000) {
            val oldText = (1..random.nextInt(30)).map { alphabet[random.nextInt(alphabet.length)] }.joinToString("")
            if (oldText.isEmpty()) return@repeat
            val from = random.nextInt(oldText.length)
            val to = from + random.nextInt(oldText.length - from + 1)
            val cutFrom = random.nextInt(oldText.length + 1)
            val cutTo = cutFrom + random.nextInt(oldText.length - cutFrom + 1)
            val inserted = (0..random.nextInt(4)).map { alphabet[random.nextInt(alphabet.length)] }.joinToString("")
            val newText = oldText.substring(0, cutFrom) + inserted + oldText.substring(cutTo)
            val range = OffsetMap(oldText, newText).mapRange(from, to) ?: return@repeat
            if (to > from) {
                assertEquals(
                    "old='$oldText' new='$newText' range=$from..$to",
                    oldText.substring(from, to),
                    newText.substring(range.start, range.end),
                )
            }
        }
    }
}
