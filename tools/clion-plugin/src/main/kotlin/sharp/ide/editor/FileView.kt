/*
 * FileView - one file as the editor features see it: the index, the text the compiler saw and the text now.
 *
 * WHAT THIS IS FOR
 *   Completion, hover, go-to and the structure view all ask the same kind of question: "what does
 *   the index know about THIS place in THIS file, whose text may have been edited since the
 *   compiler read it?" (doc/ide.md 3.6). A FileView bundles what they need, with no platform
 *   object in it, so everything built on it is tested with plain strings:
 *
 *     index      the one run the answers come from
 *     file       the file's number in that run's manifest; null for a file the compiler has not seen
 *     snapshot   the text the compiler read (null when it is not known: then the run's offsets
 *                for this file are not used, and only what is found by NAME is offered)
 *     text       the text now
 *     map        OffsetMap(snapshot, text): the way between the two texts
 *
 *   `placeAt(offset)` is the scope question: which function, class and locals surround the caret.
 *   An offset inside freshly typed text goes through the caret rule (3.6, rule 4) - the start of
 *   the edit - because that is where the compiler's tables still describe the code around it.
 */
package sharp.ide.editor

import sharp.ide.annotator.OffsetMap
import sharp.ide.index.BodyFunction
import sharp.ide.index.ClassInfo
import sharp.ide.index.FileBodies
import sharp.ide.index.FunctionInfo
import sharp.ide.index.LocalInfo
import sharp.ide.index.RunIndex
import sharp.ide.index.TypeLookup

/** What surrounds a caret: the locals in scope, the function and the class it is in. */
class PlaceInfo(
    val locals: List<LocalInfo>,
    val function: BodyFunction?,
    val functionIndex: Int,
    val enclosingClass: ClassInfo?,
    /** No `self` here: global code, or inside a static function. */
    val isStatic: Boolean,
    /** The type parameters of the class and the function around the caret (`T`). */
    val typeParameters: Set<String>,
) {
    companion object {
        val NOWHERE = PlaceInfo(emptyList(), null, -1, null, true, emptySet())
    }
}

class FileView(
    val index: RunIndex,
    val file: Int?,
    val snapshot: String?,
    val text: String,
) {
    val lookup = TypeLookup(index)

    val map: OffsetMap? = snapshot?.let { OffsetMap(it, text) }

    val bodies: FileBodies? get() = file?.let { index.bodiesFor(it) }

    /** True when the run's offsets can be used for this file at all. */
    val hasOffsets: Boolean get() = file != null && map != null

    /** An offset of the current text in the compiler's text, exactly; null inside edited text or without a snapshot. */
    fun toSnapshot(offset: Int): Int? = map?.mapPointBack(offset)

    /** An offset of the current text in the compiler's text, by the caret rule (never null when there is a snapshot). */
    fun toSnapshotForScope(offset: Int): Int? = map?.mapPointBackForScope(offset)

    /** An offset of the compiler's text in the current text; null inside edited text. */
    fun toCurrent(offset: Int): Int? = map?.mapPoint(offset)

    /** The surroundings of `offset` (an offset of the CURRENT text). */
    fun placeAt(offset: Int): PlaceInfo {
        val at = file?.let { f -> toSnapshotForScope(offset)?.let { f to it } } ?: return PlaceInfo.NOWHERE
        val (fileNumber, old) = at
        val enclosingClass = index.classAt(fileNumber, old)
        val b = index.bodiesFor(fileNumber)
        val functionIndex = b?.functionAt(old) ?: -1
        val function = b?.functions?.getOrNull(functionIndex)
        val locals = b?.localsVisibleAt(old) ?: emptyList()
        val isStatic = when {
            function != null -> function.isStatic
            enclosingClass != null -> false
            else -> true
        }
        val typeParameters = HashSet<String>()
        var c: ClassInfo? = enclosingClass
        while (c != null) { c.typeParameters.forEach { typeParameters.add(it.name) }; c = c.owner }
        if (function != null) {
            (index.declarations[fileNumber to function.node] as? FunctionInfo)?.typeParameters?.forEach { typeParameters.add(it.name) }
        }
        return PlaceInfo(locals, function, functionIndex, enclosingClass, isStatic, typeParameters)
    }
}
