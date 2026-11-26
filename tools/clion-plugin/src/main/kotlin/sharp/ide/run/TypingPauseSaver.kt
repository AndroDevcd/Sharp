/*
 * TypingPauseSaver - checks the program when you STOP TYPING, without waiting for a save.
 *
 * WHAT THIS IS FOR
 *   The compiler reads files from the disk, so a check can only start after a file was
 *   SAVED (FileChangeListener). The IDE saves by itself, but late: when its window loses the
 *   focus, or after 15 seconds without a key press. In between, a mistake you just typed
 *   stays unmarked until you press Ctrl+S - which is exactly what an editor should not ask.
 *
 *   This closes the gap the simple way: when a Sharp file of the program has been edited and
 *   then left alone for a moment (the "typing pause", a setting; 3 seconds by default), the
 *   plugin saves it. The save is an ordinary save, so everything else follows as before:
 *   the file watcher sees it, waits its own short moment for more saves, and starts ONE run.
 *
 *      type ... type ... (pause)  ->  save  ->  (wait after a save)  ->  compiler  ->  marks
 *
 *   It is NOT checking as you type: nothing runs while keys are being pressed, every new key
 *   press starts the pause again, and one run is never interrupted by the next (RunScheduler).
 *
 *   A pause of 0 switches this off: then only the IDE's own saves and Ctrl+S start a check.
 *
 *   C++ comparison: a debounce timer - restart a one-shot timer on every event, act when it
 *   finally fires.
 */
package sharp.ide.run

import com.intellij.openapi.Disposable
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.service
import com.intellij.openapi.editor.Document
import com.intellij.openapi.editor.EditorFactory
import com.intellij.openapi.editor.event.DocumentEvent
import com.intellij.openapi.editor.event.DocumentListener
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.project.Project
import com.intellij.util.Alarm
import sharp.ide.settings.SharpApplicationSettings

@Service(Service.Level.PROJECT)
class TypingPauseSaver(private val project: Project) : Disposable {

    /** The one-shot timer. It fires on a background thread: deciding which files to save may touch the disk. */
    private val timer = Alarm(Alarm.ThreadToUse.POOLED_THREAD, this)

    /** For tests: the pause to use instead of the setting; null = the setting. */
    @Volatile
    var pauseOverrideMillis: Long? = null

    /** Called once when the project opens (StartupRun). Listening ends by itself when the project closes. */
    fun start() {
        EditorFactory.getInstance().eventMulticaster.addDocumentListener(object : DocumentListener {
            override fun documentChanged(event: DocumentEvent) = edited(event.document)
        }, this)
    }

    /** A document changed: if it is a Sharp file, (re)start the pause. Runs on the UI thread, so it only looks at a name. */
    fun edited(document: Document) {
        val pause = pauseMillis()
        if (pause <= 0 || project.isDisposed) return
        val file = FileDocumentManager.getInstance().getFile(document) ?: return
        if (file.extension != "sharp" || !file.isInLocalFileSystem) return
        timer.cancelAllRequests()
        timer.addRequest({ saveWhatWasEdited() }, pause)
    }

    /**
     * The pause is over: save every edited Sharp file that belongs to THIS project's program.
     * (Other unsaved files - a C++ file, a Sharp file of another project - are not ours to save.)
     */
    fun saveWhatWasEdited() {
        if (project.isDisposed) return
        val service = AnalysisService.getInstance(project)
        val documents = FileDocumentManager.getInstance()
        val ours = documents.unsavedDocuments.filter { document ->
            val file = documents.getFile(document)
            file != null && file.isValid && file.extension == "sharp" && service.wantsRunFor(file.path)
        }
        if (ours.isEmpty()) return

        // saving is done on the UI thread, at a moment the IDE allows changes
        ApplicationManager.getApplication().invokeLater({
            if (project.isDisposed) return@invokeLater
            for (document in ours) {
                if (documents.isDocumentUnsaved(document)) documents.saveDocument(document)
            }
        }, project.disposed)
    }

    private fun pauseMillis(): Long =
        pauseOverrideMillis ?: SharpApplicationSettings.getInstance().state.typingPauseMillis.toLong()

    override fun dispose() {
        // the timer and the listener are tied to this object and end with it
    }

    companion object {
        fun getInstance(project: Project): TypingPauseSaver = project.service()
    }
}
