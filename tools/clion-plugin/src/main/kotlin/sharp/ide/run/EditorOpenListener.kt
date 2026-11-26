/*
 * EditorOpenListener - opening a file asks for a run when the index has nothing about its code.
 *
 * WHAT THIS IS FOR
 *   The compiler writes the table of uses and the locals ("bodies") only for the files that are OPEN
 *   in an editor when it runs (`bodies_for.txt`, doc/ide.md 4.2); it needs nothing for a closed file.
 *   So a file opened LATER - a library file reached by Ctrl+click, say - has declarations but no
 *   bodies. Opening it asks for one run; a few seconds later hover and go-to work inside it.
 *
 *   The listener only decides and asks (`shouldAskForRun` is a string test and two map lookups), and
 *   does it off the UI thread because the first look at the program's layout touches the disk.
 */
package sharp.ide.run

import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.openapi.fileEditor.FileEditorManagerListener
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import sharp.ide.editor.FileViews

class EditorOpenListener : FileEditorManagerListener {

    override fun fileOpened(source: FileEditorManager, file: VirtualFile) {
        if (file.extension != "sharp") return
        val project = source.project
        ApplicationManager.getApplication().executeOnPooledThread {
            if (!project.isDisposed && shouldAskForRun(project, file)) AnalysisService.getInstance(project).requestRun(RunReason.EDITOR_OPENED)
        }
    }

    companion object {
        /**
         * Is `file` a file of the program that the current index has no bodies for (no index yet, a file the
         * run did not list, or a file listed without a bodies file)?
         */
        fun shouldAskForRun(project: Project, file: VirtualFile): Boolean {
            if (file.extension != "sharp") return false
            val service = AnalysisService.getInstance(project)
            if (!service.wantsRunFor(file.path)) return false
            val index = service.index ?: return true
            val number = index.fileByPath[FileViews.compilerPath(project, file)] ?: return true
            return index.bodiesFor(number) == null
        }
    }
}
