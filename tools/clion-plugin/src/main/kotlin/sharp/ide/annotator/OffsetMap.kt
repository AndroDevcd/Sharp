/*
 * OffsetMap - where did a position go after the text was edited?
 *
 * WHAT THIS IS FOR
 *   The compiler's positions are offsets into the text it read (the SNAPSHOT). If the
 *   person has typed since, the same offset points somewhere else. This class answers
 *   "which offset in the current text is the character that used to be at offset N?"
 *   (doc/ide.md section 3.6).
 *
 *   HOW: the two texts are compared from both ends. The longest common beginning (the
 *   prefix) and the longest common ending (the suffix) are the parts nothing happened
 *   to; what is between them is the CHANGED REGION: old text [p, q) is now new text
 *   [p, q + delta). One contiguous change is assumed; several scattered edits just make
 *   the region larger, never wrong.
 *
 *       old:  ........[ AAAA ].........
 *       new:  ........[ BBBBBB ]........      p = start of region, q = old end, delta = +2
 *
 *     - before the region:  same offset
 *     - after the region:   offset + delta
 *     - inside:             no counterpart
 *
 *   THE RULE FOR A RANGE [from, to): it is kept only if it lies completely BEFORE the
 *   changed region (to <= p) or completely AFTER it (from >= q). Anything that overlaps
 *   the edit - including an edit in the middle of the range - is DROPPED, never stretched
 *   or guessed: an error whose text you are editing disappears until the next save brings
 *   it back if it is still wrong. Typing right beside a range (just before or just after
 *   it) keeps it.
 *
 *   Everything is in UTF-16 code units, the same as the compiler's offsets and the
 *   platform's Document.
 */
package sharp.ide.annotator

class OffsetMap(old: CharSequence, new: CharSequence) {

    /** Start of the changed region (in both texts). */
    private val regionStart: Int

    /** End of the changed region in the OLD text. */
    private val oldRegionEnd: Int

    /** new length - old length. */
    private val delta: Int = new.length - old.length

    /** The two texts are identical. */
    val unchanged: Boolean

    init {
        val limit = minOf(old.length, new.length)
        var prefix = 0
        while (prefix < limit && old[prefix] == new[prefix]) prefix++
        var suffix = 0
        while (suffix < limit - prefix && old[old.length - 1 - suffix] == new[new.length - 1 - suffix]) suffix++
        regionStart = prefix
        oldRegionEnd = old.length - suffix
        unchanged = old.length == new.length && prefix == old.length
    }

    /**
     * The new offset of a POINT of the old text (a caret, the start of a range), or null when the
     * point is inside the edited text. A point exactly where text was inserted lands after the insertion.
     */
    fun mapPoint(offset: Int): Int? = when {
        unchanged -> offset
        offset < regionStart -> offset
        offset >= oldRegionEnd -> offset + delta
        else -> null
    }

    /**
     * The opposite direction: the offset in the OLD (compiler) text of a POINT of the current text, or null
     * when that point lies inside the edited text. Used to ask the compiler's tables about a place the
     * person points at (a hover, a Ctrl+click).
     */
    fun mapPointBack(offset: Int): Int? = when {
        unchanged -> offset
        offset < regionStart -> offset
        offset >= regionStart + (oldRegionEnd - regionStart) + delta -> offset - delta
        else -> null
    }

    /**
     * THE CARET RULE (doc/ide.md 3.6, rule 4): like `mapPointBack`, but a point inside the edited text maps to
     * the START of the edit. Completion always happens inside freshly typed text, and the one question it asks
     * - "which function, class and locals are around this place" - has the same answer at the start of the
     * edit, unless the edit itself added or removed a block.
     */
    fun mapPointBackForScope(offset: Int): Int = mapPointBack(offset) ?: regionStart

    /** The range [from, to) of the old text in the new text, or null when it overlaps the edit (see the top of this file). */
    fun mapRange(from: Int, to: Int): MappedRange? {
        val end = maxOf(from, to)
        return when {
            unchanged -> MappedRange(from, end)
            end == from -> mapPoint(from)?.let { MappedRange(it, it) }          // a point
            end <= regionStart -> MappedRange(from, end)                        // completely before the edit
            from >= oldRegionEnd -> MappedRange(from + delta, end + delta)      // completely after it
            else -> null
        }
    }
}

/** A range [start, end) in the current text. */
class MappedRange(val start: Int, val end: Int)
