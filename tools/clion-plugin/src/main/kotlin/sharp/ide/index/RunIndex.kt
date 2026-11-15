/*
 * RunIndex - the index of ONE compiler run, whole, and the lookups the features ask of it.
 *
 * WHAT THIS IS FOR
 *   doc/ide.md 3.5 and 6.6: the plugin's index is the newest run that parsed, as a unit.
 *   File, node and type numbers only mean something inside one run, so nothing of two runs
 *   is ever combined. The object is immutable once built; AnalysisService swaps a new one in
 *   with one assignment, so completion, hover and go-to read it on any thread without a lock.
 *
 *   The lookups here are the plain ones (find a class by id, the class around an offset, the
 *   declaration under an offset). The questions that need real thought - the members of a
 *   type, the type before a dot - live in TypeLookup and ReceiverResolver.
 */
package sharp.ide.index

class RunIndex(
    val run: String,
    /** `declarations` or `full` (a run that stopped at syntax errors has no index). */
    val level: String,
    val files: List<ManifestFile>,
    val types: TypeTable,
    val modules: Map<String, ModuleInfo>,
    val classesById: Map<String, ClassInfo>,
    val declarations: Map<Pair<Int, Int>, Declaration>,
    private val bodies: Map<Int, FileBodies>,
) {
    /** File number by the path the compiler used. */
    val fileByPath: Map<String, Int> = files.withIndex().associate { it.value.path to it.index }

    fun bodiesFor(file: Int): FileBodies? = bodies[file]

    fun fileEntry(file: Int): ManifestFile? = files.getOrNull(file)

    /** Every class, interface and enum (nested ones too). */
    val allClasses: Collection<ClassInfo> get() = classesById.values

    /** Classes by their own short name: `list` -> [std#list]. */
    val classesByName: Map<String, List<ClassInfo>> by lazy { allClasses.groupBy { it.name } }

    private val classesByFile: Map<Int, List<ClassInfo>> by lazy { allClasses.groupBy { it.file } }

    private val declarationsByFile: Map<Int, List<Declaration>> by lazy {
        declarations.values.groupBy { it.file }.mapValues { (_, list) -> list.sortedBy { it.at.from } }
    }

    fun classOf(type: SharpType?): ClassInfo? = type?.classId?.let { classesById[it] }

    /** The innermost class whose `{ ... }` contains the offset, written in this file. */
    fun classAt(file: Int, offset: Int): ClassInfo? {
        var best: ClassInfo? = null
        for (c in classesByFile[file] ?: return null) {
            val body = c.body ?: continue
            if (offset in body.first..body.last && (best == null || body.last - body.first < best.body!!.last - best.body!!.first)) best = c
        }
        return best
    }

    /** The declaration whose NAME contains the offset (hover on the declaration itself). */
    fun declarationAt(file: Int, offset: Int): Declaration? =
        declarationsByFile[file]?.firstOrNull { offset >= it.at.from && offset <= it.at.to }

    /** The base class of `c`: the written one, or `std#_object_`. Null for `_object_` itself and for an unknown base. */
    fun baseOf(c: ClassInfo): ClassInfo? {
        c.base?.let { return classOf(it) }
        return if (c.id == OBJECT_CLASS) null else classesById[OBJECT_CLASS]
    }

    fun moduleOfFile(file: Int): String? = files.getOrNull(file)?.module

    /** The file's declared index entries for a declaration: its module-level path, for the hover ("declared in"). */
    fun fileNameOf(file: Int): String = files.getOrNull(file)?.path?.substringAfterLast('/') ?: "?"

    companion object {
        const val OBJECT_CLASS = "std#_object_"
    }
}
