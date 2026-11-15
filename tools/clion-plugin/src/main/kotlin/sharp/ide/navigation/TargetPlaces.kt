/*
 * TargetPlaces - a place of the compiler's index, found again in the editor's files.
 *
 * WHAT THIS IS FOR
 *   The index names a place as (the compiler's path of a file, an offset in the text the compiler read).
 *   To open it the plugin needs the editor's own file (`C:\x\a.sharp`, or a library file inside WSL) and
 *   an offset in that file's text NOW. This does both steps (doc/ide.md 6.10):
 *
 *     1. compiler path -> editor file (PathTranslator, then the virtual file system);
 *     2. offset -> offset now, by OffsetMap between the text the compiler saw and the text the file has
 *        now (the open document, else the file on disk). The text the compiler saw is the snapshot, or the
 *        file's bytes when they still have the recorded hash. When neither is known, nothing is returned:
 *        a wrong place is worse than none.
 *
 *   Used by go-to-declaration, "go to class" and the structure view.
 */
package sharp.ide.navigation

import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.openapi.vfs.VirtualFile
import sharp.ide.annotator.OffsetMap
import sharp.ide.editor.FileViews
import sharp.ide.index.RunIndex
import sharp.ide.run.AnalysisService
import sharp.ide.run.SavedText
import sharp.ide.settings.SharpApplicationSettings

object TargetPlaces {

    /** The editor file for a path the compiler wrote; null when it cannot be found. */
    fun fileFor(project: Project, compilerPath: String): VirtualFile? {
        val translator = AnalysisService.translatorFor(SharpApplicationSettings.getInstance().state, project.basePath)
        val editorPath = translator?.toEditorPath(compilerPath) ?: compilerPath
        return LocalFileSystem.getInstance().findFileByPath(editorPath)
    }

    /** The place (file, offset now) of `from` in file number `fileNumber` of `index`'s run; null when it cannot be said. */
    fun resolve(project: Project, index: RunIndex, fileNumber: Int, from: Int): Pair<VirtualFile, Int>? {
        val compilerPath = index.fileEntry(fileNumber)?.path ?: return null
        val file = fileFor(project, compilerPath) ?: return null
        val service = AnalysisService.getInstance(project)
        val snapshot = FileViews.snapshotOf(service, index, compilerPath, fileNumber, file) ?: return null
        val now = FileDocumentManager.getInstance().getCachedDocument(file)?.text ?: SavedText.decode(file.contentsToByteArray())
        val at = OffsetMap(snapshot, now).mapPoint(from) ?: return null
        return file to at
    }
}
