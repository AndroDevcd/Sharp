/*
 * FileBodies - what the compiler wrote about the code of ONE file (`bodies/<n>.json`).
 *
 * WHAT THIS IS FOR
 *   The functions of the file with the text each one covers, the local variables with the
 *   text each is visible in, and the TABLE OF USES: every name in the code that the compiler
 *   resolved, with what it refers to and what type the value has (doc/ide.md 4.7).
 *
 *   The uses are kept in parallel int arrays sorted by start, so `useAt(offset)` is a binary
 *   search and a file with 25,000 uses costs a few hundred kilobytes, not 25,000 objects.
 *
 *   All offsets are in the SAVED text the compiler read; the OffsetMap (3.6) moves them.
 */
package sharp.ide.index

class BodyFunction(
    val name: String,
    /** The class id it is written in; null in global code. */
    val classId: String?,
    /** There is no `self` in it. */
    val isStatic: Boolean,
    val isLambda: Boolean,
    val node: Int,
    /** `[from, to]` of the code it covers. */
    val body: IntRange,
    /** Index (in `functions`) of the function a lambda is written in; -1 when none. */
    val enclosing: Int,
    /** For code worked out for one instance of a generic class or function: `box<string>`; null otherwise (doc/ide.md 4.7). */
    val instance: String? = null,
)

class LocalInfo(
    val name: String,
    val type: SharpType?,
    /** `parameter`, `local`, `foreach`, `catch`, `lambda_parameter`. */
    val kind: String,
    val isConst: Boolean,
    /** Index into `functions`. */
    val function: Int,
    val declFrom: Int,
    val declTo: Int,
    val visible: IntRange,
)

/** One entry of the table of uses. */
class Use(
    val from: Int,
    val to: Int,
    /** 1 type, 2 field or global, 3 function, 4 constructor, 5 enum value, 6 local (node = index into locals), 7 self / base. */
    val kind: Int,
    val file: Int,
    val node: Int,
    val type: SharpType?,
)

class FileBodies(
    val file: Int,
    /** `lines[i]` is the offset at which line `i + 1` starts. */
    val lines: IntArray,
    val functions: List<BodyFunction>,
    val locals: List<LocalInfo>,
    private val useFrom: IntArray,
    private val useTo: IntArray,
    private val useKind: IntArray,
    private val useFile: IntArray,
    private val useNode: IntArray,
    private val useType: Array<SharpType?>,
) {
    val useCount: Int get() = useFrom.size

    /** The 1-based line of an offset. */
    fun lineOf(offset: Int): Int {
        var lo = 0
        var hi = lines.size - 1
        var found = 0
        while (lo <= hi) {
            val mid = (lo + hi) ushr 1
            if (lines[mid] <= offset) { found = mid; lo = mid + 1 } else hi = mid - 1
        }
        return found + 1
    }

    fun use(i: Int): Use = Use(useFrom[i], useTo[i], useKind[i], useFile[i], useNode[i], useType[i])

    /** The use whose name token contains `offset` (a caret right after the name counts), or null. */
    fun useAt(offset: Int): Use? {
        var lo = 0
        var hi = useFrom.size - 1
        var found = -1
        while (lo <= hi) {                         // the last use that starts at or before the offset
            val mid = (lo + hi) ushr 1
            if (useFrom[mid] <= offset) { found = mid; lo = mid + 1 } else hi = mid - 1
        }
        if (found < 0) return null
        return if (offset <= useTo[found]) use(found) else null
    }

    /** Every use, in order of position. */
    fun uses(): Sequence<Use> = (0 until useCount).asSequence().map { use(it) }

    /** The innermost function whose text contains the offset, as an index into `functions`; -1 when none. */
    fun functionAt(offset: Int): Int {
        var best = -1
        var bestSize = Int.MAX_VALUE
        for ((i, f) in functions.withIndex()) {
            if (offset in f.body.first..f.body.last && f.body.last - f.body.first < bestSize) {
                best = i
                bestSize = f.body.last - f.body.first
            }
        }
        return best
    }

    /** The locals and parameters whose visible text contains the offset, in order of declaration. */
    fun localsVisibleAt(offset: Int): List<LocalInfo> =
        locals.filter { offset >= it.visible.first && offset <= it.visible.last }
}
