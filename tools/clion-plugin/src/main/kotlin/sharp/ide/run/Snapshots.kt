/*
 * Snapshots - the text the compiler saw, kept so its positions can be used later.
 *
 * WHAT THIS IS FOR
 *   The compiler reads files from disk and answers with offsets into THAT text. A few
 *   seconds later the person may have typed more, so an offset no longer points at the
 *   same characters. To map an old offset onto the current document the plugin needs
 *   the old text: the SNAPSHOT (doc/ide.md section 3.6). OffsetMap does the mapping.
 *
 *   Taking a snapshot: when a run starts, the saved text of every Sharp file that is
 *   open in an editor is read (from disk through the virtual file system, which is
 *   what the compiler will read; NOT the editor's unsaved text) together with its hash.
 *   When the run finishes, a snapshot is kept only if its hash equals the manifest's
 *   hash for that file (otherwise the file was saved again in the meantime and the
 *   snapshot is not what the compiler saw).
 *
 *   A file opened after the run started has no snapshot; `adopt` makes one then, if the
 *   file on disk still has the hash the run recorded.
 *
 *   Snapshots belong to a run (the run id the compiler wrote) and only the newest few
 *   runs are kept.
 */
package sharp.ide.run

/** The saved text of one file and its hash. `text` is in the compiler's counting (see SavedText). */
class Snapshot(val text: String, val hash: String)

class Snapshots(private val keepRuns: Int = 3) {

    private val byRun = LinkedHashMap<String, Map<String, Snapshot>>()

    /** The run the current index belongs to: its snapshots are never dropped (doc/ide.md 3.6). */
    private var pinnedRun: String? = null

    @Synchronized
    fun pin(runId: String?) {
        pinnedRun = runId
    }

    /**
     * Records the snapshots taken at the start of the run `runId`: `taken` maps a path in the COMPILER's
     * form to its snapshot; `hashesOfRun` is the manifest's hash per path. A snapshot whose hash differs
     * is dropped.
     */
    @Synchronized
    fun keep(runId: String, taken: Map<String, Snapshot>, hashesOfRun: Map<String, String?>) {
        byRun[runId] = taken.filter { (path, snapshot) -> hashesOfRun[path] == snapshot.hash }
        while (byRun.size > keepRuns) {
            val oldest = byRun.keys.firstOrNull { it != pinnedRun } ?: break
            byRun.remove(oldest)
        }
    }

    @Synchronized
    fun get(runId: String, compilerPath: String): Snapshot? = byRun[runId]?.get(compilerPath)

    /** Adds a snapshot made later (a file opened after the run); it must have been checked against the run's hash. */
    @Synchronized
    fun adopt(runId: String, compilerPath: String, snapshot: Snapshot) {
        val existing = byRun[runId] ?: return
        byRun[runId] = existing + (compilerPath to snapshot)
    }

    @Synchronized
    fun clear() {
        byRun.clear()
    }
}
