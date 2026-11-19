/*
 * RunSharpCheckNowAction - the menu item "Run Sharp check now" (Tools menu).
 *
 * WHAT THIS IS FOR
 *   Runs the compiler on the program straight away: no debounce, and even when checking on
 *   save is switched off. A failure is always shown (not "once"), and a short note says how
 *   many errors and warnings the run found. Registered in plugin.xml.
 */
package sharp.ide.run

import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.AnAction
import com.intellij.openapi.actionSystem.AnActionEvent

class RunSharpCheckNowAction : AnAction() {

    override fun actionPerformed(event: AnActionEvent) {
        val project = event.project ?: return
        AnalysisService.getInstance(project).requestRun(RunReason.MANUAL)
    }

    override fun update(event: AnActionEvent) {
        event.presentation.isEnabledAndVisible = event.project != null
    }

    /** `update` reads nothing from the editor, so it may run on a background thread. */
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT
}
