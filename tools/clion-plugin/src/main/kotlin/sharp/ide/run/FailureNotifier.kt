/*
 * FailureNotifier - tells the owner, quietly and once, that a run did not work.
 *
 * WHAT THIS IS FOR
 *   A compiler that cannot start (wsl.exe missing, a wrong path in the settings), that
 *   runs too long, or that wrote no answer would otherwise fail on EVERY save. The rule
 *   (doc/ide.md 3.6): one balloon notification per distinct reason, with the command
 *   that was run and the last lines of its output; the same reason again is silent until
 *   a run succeeds. The earlier errors stay in the editor meanwhile.
 *
 *   `FailureTracker` is the "once" part (pure, tested alone); `FailureNotifier` posts the
 *   balloon through the platform's notification group "Sharp" (declared in plugin.xml).
 */
package sharp.ide.run

import com.intellij.notification.NotificationAction
import com.intellij.notification.NotificationGroupManager
import com.intellij.notification.NotificationType
import com.intellij.openapi.options.ShowSettingsUtil
import com.intellij.openapi.project.Project
import sharp.ide.annotator.DiagnosticHtml
import sharp.ide.settings.SharpConfigurable

/** Remembers the reason of the last reported failure. */
class FailureTracker {
    private var last: String? = null

    /** True when `reason` differs from the one last reported (and remembers it). */
    @Synchronized
    fun shouldReport(reason: String): Boolean {
        if (reason == last) return false
        last = reason
        return true
    }

    /** A run succeeded: the next failure, whatever its reason, is reported again. */
    @Synchronized
    fun clear() {
        last = null
    }

    @Synchronized
    fun lastReason(): String? = last
}

object FailureNotifier {
    private const val GROUP = "Sharp"

    fun report(project: Project, failure: RunFailure) {
        val content = StringBuilder(DiagnosticHtml.escape(failure.reason))
        if (failure.command.isNotEmpty()) content.append("<br/><br/>Command: <code>").append(DiagnosticHtml.escape(failure.command)).append("</code>")
        if (failure.outputTail.isNotEmpty()) content.append("<br/><br/>Output:<br/>").append(DiagnosticHtml.escape(failure.outputTail))
        val notification = NotificationGroupManager.getInstance().getNotificationGroup(GROUP)
            .createNotification("Sharp analysis failed", content.toString(), NotificationType.WARNING)
        notification.addAction(NotificationAction.createSimple("Open Sharp settings") {
            ShowSettingsUtil.getInstance().showSettingsDialog(project, SharpConfigurable::class.java)
        })
        notification.notify(project)
    }

    /** A short note after a run the person asked for by hand ("Run Sharp check now"). */
    fun info(project: Project, text: String) {
        NotificationGroupManager.getInstance().getNotificationGroup(GROUP)
            .createNotification("Sharp check", DiagnosticHtml.escape(text), NotificationType.INFORMATION)
            .notify(project)
    }
}
