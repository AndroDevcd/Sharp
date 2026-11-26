/*
 * StartupRun - what happens when a project opens: the last answer is read back, then one run.
 *
 * WHAT THIS IS FOR
 *   Without it the editor would show no errors and offer no completion until the first save
 *   after starting the IDE. The platform calls a ProjectActivity once per opened project,
 *   after start-up has finished, on a background thread (doc/ide.md section 3.1). Two steps:
 *
 *     1. the run folders the last session left on disk are read back
 *        (AnalysisService.loadLastRunFromDisk): the errors and the index are there at once,
 *        also when the program does not parse at the moment;
 *     2. one run is asked for, which replaces them a few seconds later with today's truth.
 *
 *   A project with no Sharp files costs nothing: it has no run folders, and the run is
 *   skipped when the program has no source files.
 */
package sharp.ide.run

import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.project.Project
import com.intellij.openapi.startup.ProjectActivity
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class StartupRun : ProjectActivity {
    override suspend fun execute(project: Project) {
        if (ApplicationManager.getApplication().isUnitTestMode) return
        // from now on, a pause in typing saves the edited Sharp files, which starts a check
        TypingPauseSaver.getInstance(project).start()
        val service = AnalysisService.getInstance(project)
        // Reading a few megabytes of JSON blocks its thread, so it goes to the dispatcher meant for file work.
        withContext(Dispatchers.IO) { service.loadLastRunFromDisk() }
        service.requestRun(RunReason.STARTUP)
    }
}
