/*
 * FileChangeListener - notices that a .sharp file changed on disk.
 *
 * WHAT THIS IS FOR
 *   The trigger of the whole plugin (doc/ide.md section 3.1). The platform tells every
 *   registered BulkFileListener about every change of every file in the IDE: a
 *   document written to disk (Ctrl+S, auto-save when the IDE is idle or loses focus,
 *   Save All), a file created, deleted, moved or renamed, a change made by another
 *   program that the IDE then noticed.
 *
 *   It is called on the UI thread, inside a write action, for ALL files, so it must be
 *   nearly free: it looks at the end of each path (`.sharp`), asks each open project
 *   whether the file belongs to its program (a string comparison) and, if so, says
 *   "a run is wanted". Debouncing and the compiler itself happen elsewhere.
 */
package sharp.ide.run

import com.intellij.openapi.project.ProjectManager
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.openapi.vfs.newvfs.BulkFileListener
import com.intellij.openapi.vfs.newvfs.events.VFileEvent
import com.intellij.openapi.vfs.newvfs.events.VFileMoveEvent
import com.intellij.openapi.vfs.newvfs.events.VFilePropertyChangeEvent

class FileChangeListener : BulkFileListener {

    override fun after(events: List<VFileEvent>) {
        var paths: ArrayList<String>? = null
        for (event in events) {
            for (path in pathsOf(event)) {
                if (!path.endsWith(".sharp")) continue
                if (paths == null) paths = ArrayList()
                paths.add(path)
            }
        }
        if (paths == null) return
        for (project in ProjectManager.getInstance().openProjects) {
            if (project.isDisposed) continue
            val service = AnalysisService.getInstance(project)
            if (paths.any { service.wantsRunFor(it) }) service.requestRun(RunReason.SAVE)
        }
    }

    /** The path an event is about; a move or rename is about the old AND the new path (either may be in the program). */
    private fun pathsOf(event: VFileEvent): List<String> = when {
        event is VFileMoveEvent -> listOf(event.oldPath, event.newPath)
        event is VFilePropertyChangeEvent && event.propertyName == VirtualFile.PROP_NAME -> listOf(event.oldPath, event.newPath)
        event is VFilePropertyChangeEvent -> emptyList()      // attributes (read-only, ...) change no text
        else -> listOf(event.path)
    }
}
