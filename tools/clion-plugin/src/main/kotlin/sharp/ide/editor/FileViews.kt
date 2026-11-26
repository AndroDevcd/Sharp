/*
 * FileViews - makes a FileView for an open file, from the plugin's running state.
 *
 * WHAT THIS IS FOR
 *   The platform tells a feature which file and which text. This asks AnalysisService for the
 *   current index, works out the file's path in the compiler's form (`C:\x\a.sharp` ->
 *   `/mnt/c/x/a.sharp` when the compiler runs in WSL), finds the file's number in that run
 *   and finds the text the compiler saw (doc/ide.md 3.6):
 *
 *     - a snapshot taken when the run started, if the file was open then;
 *     - otherwise the file's bytes on disk, IF their hash is the one the run recorded - the
 *       file is then exactly what the compiler read, and that text is ADOPTED as the snapshot;
 *     - otherwise none: the run's offsets for this file are not used, and only what can be found
 *       by name is offered.
 *
 *   Every answer is "null" rather than an exception: no index yet, a disposed project, an
 *   unreadable file all mean "this feature has nothing to say".
 */
package sharp.ide.editor

import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import sharp.ide.index.RunIndex
import sharp.ide.run.AnalysisService
import sharp.ide.run.SavedText
import sharp.ide.run.Snapshot
import sharp.ide.settings.SharpApplicationSettings

object FileViews {

    /** The path of an editor file as the compiler wrote it. */
    fun compilerPath(project: Project, file: VirtualFile): String {
        val translator = AnalysisService.translatorFor(SharpApplicationSettings.getInstance().state, project.basePath)
        return translator?.toWsl(file.path) ?: file.path
    }

    /** The view of `file` whose text now is `currentText`; null when there is no index yet. */
    fun forFile(project: Project, file: VirtualFile, currentText: String): FileView? {
        if (project.isDisposed) return null
        val service = AnalysisService.getInstance(project)
        val index = service.index ?: return null
        val path = compilerPath(project, file)
        val number = index.fileByPath[path]
        val snapshot = if (number != null) snapshotOf(service, index, path, number, file) else null
        return FileView(index, number, snapshot, currentText)
    }

    /**
     * The text the compiler saw of file `path` of `index`'s run: the snapshot taken then, or - if the file on
     * disk still has the recorded hash - the disk text (which is adopted for the next question).
     */
    fun snapshotOf(service: AnalysisService, index: RunIndex, path: String, number: Int, file: VirtualFile): String? {
        service.snapshots.get(index.run, path)?.let { return it.text }
        val expected = index.fileEntry(number)?.hash ?: return null
        val bytes = try { file.contentsToByteArray() } catch (_: java.io.IOException) { return null }
        if (SavedText.hash(bytes) != expected) return null
        val snapshot = Snapshot(SavedText.decode(bytes), expected)
        service.snapshots.adopt(index.run, path, snapshot)
        return snapshot.text
    }
}
