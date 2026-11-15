/*
 * SharpExternalAnnotator - puts the compiler's errors and warnings into the editor.
 *
 * WHAT THIS IS FOR
 *   An "annotator" is the platform's socket for marking text: a red or yellow squiggle,
 *   a stripe in the gutter next to the scroll bar, an entry in the Problems view, a
 *   tooltip. An EXTERNAL annotator is one whose answer comes from outside the IDE (here:
 *   from a compiler run some seconds ago), so the platform calls it in three steps on
 *   three different threads (doc/ide.md section 6.7):
 *
 *     collectInformation   UI/read thread, quick: pick up what is needed (this file's path
 *                          and its current text, the newest run, the snapshot of the text
 *                          that run saw). Returns null when there is nothing to show.
 *     doAnnotate           background thread, may take its time: choose this file's
 *                          diagnostics, move their ranges onto the CURRENT text
 *                          (OffsetMap), build the tooltips.
 *     apply                read thread: hand each finished mark to the platform.
 *
 *   The platform runs this again after every highlighting restart (AnalysisService asks
 *   for one when a run finishes) and after edits. Marks are never kept by us: each pass
 *   makes them afresh from the newest run, so a stale mark disappears as soon as a new
 *   result arrives, or when the file no longer has diagnostics.
 *
 *   WHEN THE TEXT CHANGED SINCE THE RUN: the compiler's offsets refer to the saved text.
 *   If you have typed since, a range is moved with the edit; a range whose text you are
 *   editing is dropped until the next run. A file whose saved text the plugin never saw
 *   (opened after the run) is checked against the run's hash and, if it still matches,
 *   adopted; if not, nothing is marked in that file until the next run.
 */
package sharp.ide.annotator

import com.intellij.lang.annotation.AnnotationHolder
import com.intellij.lang.annotation.ExternalAnnotator
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.editor.Editor
import com.intellij.openapi.project.DumbAware
import com.intellij.openapi.util.TextRange
import com.intellij.psi.PsiFile
import sharp.ide.index.DiagnosticSeverity
import sharp.ide.index.DiagnosticsRun
import sharp.ide.index.Place
import sharp.ide.index.SharpDiagnostic
import sharp.ide.run.AnalysisService
import sharp.ide.run.SavedText
import sharp.ide.run.Snapshot
import sharp.ide.settings.SharpApplicationSettings
import java.net.URLEncoder
import java.nio.charset.StandardCharsets

/** What `collectInformation` hands to `doAnnotate`: plain values, no PSI, nothing that needs a lock. */
class AnnotatorInput(
    val compilerPath: String,
    val run: DiagnosticsRun,
    /** The text the compiler saw; null when unknown (nothing can be marked). */
    val snapshotText: String?,
    val currentText: String,
)

/** One finished mark. */
class MarkedRange(
    val start: Int,
    val end: Int,
    val severity: HighlightSeverity,
    val message: String,
    val tooltip: String,
)

class AnnotatorResult(val marks: List<MarkedRange>)

class SharpExternalAnnotator : ExternalAnnotator<AnnotatorInput, AnnotatorResult>(), DumbAware {

    override fun collectInformation(file: PsiFile, editor: Editor, hasErrors: Boolean): AnnotatorInput? = collect(file)

    override fun collectInformation(file: PsiFile): AnnotatorInput? = collect(file)

    private fun collect(file: PsiFile): AnnotatorInput? {
        val virtualFile = file.virtualFile ?: return null
        val service = AnalysisService.getInstance(file.project)
        val run = service.latest ?: return null
        val translator = AnalysisService.translatorFor(SharpApplicationSettings.getInstance().state, file.project.basePath)
        val compilerPath = translator?.toWsl(virtualFile.path) ?: virtualFile.path
        if (run.forFile(compilerPath).isEmpty()) return null

        var snapshot = service.snapshots.get(run.run, compilerPath)
        if (snapshot == null) {
            // Not open when the run started: if the file on disk is what the compiler read, its text is the snapshot.
            val bytes = try { virtualFile.contentsToByteArray() } catch (_: java.io.IOException) { null }
            if (bytes != null && SavedText.hash(bytes) == run.manifest.hashByPath[compilerPath]) {
                snapshot = Snapshot(SavedText.decode(bytes), SavedText.hash(bytes))
                service.snapshots.adopt(run.run, compilerPath, snapshot)
            }
        }
        val document = file.viewProvider.document
        val current = document?.text ?: file.text
        return AnnotatorInput(compilerPath, run, snapshot?.text, current)
    }

    override fun doAnnotate(input: AnnotatorInput?): AnnotatorResult? {
        if (input == null) return null
        return AnnotatorResult(buildMarks(input))
    }

    override fun apply(file: PsiFile, result: AnnotatorResult?, holder: AnnotationHolder) {
        if (result == null) return
        for (mark in result.marks) {
            holder.newAnnotation(mark.severity, mark.message)
                .range(TextRange(mark.start, mark.end))
                .tooltip(mark.tooltip)
                .create()
        }
    }

    companion object {

        /** Everything `doAnnotate` does, as a function of plain values (so it can be tested without the platform). */
        fun buildMarks(input: AnnotatorInput): List<MarkedRange> {
            val snapshotText = input.snapshotText ?: return emptyList()
            val map = OffsetMap(snapshotText, input.currentText)
            val marks = ArrayList<MarkedRange>()
            for (diagnostic in input.run.forFile(input.compilerPath)) {
                val moved = map.mapRange(diagnostic.place.from, diagnostic.place.to) ?: continue
                val range = widen(moved.start, moved.end, input.currentText.length) ?: continue
                marks.add(
                    MarkedRange(
                        start = range.first,
                        end = range.second,
                        severity = if (diagnostic.severity == DiagnosticSeverity.ERROR) HighlightSeverity.ERROR else HighlightSeverity.WARNING,
                        message = DiagnosticHtml.shortMessage(diagnostic),
                        tooltip = tooltipOf(diagnostic, input.run),
                    ),
                )
            }
            return marks
        }

        /** An empty range ("something should be here") is drawn over one character; a range is kept inside the text. */
        fun widen(start: Int, end: Int, length: Int): Pair<Int, Int>? {
            if (length == 0) return null
            var from = start.coerceIn(0, length)
            var to = end.coerceIn(0, length)
            if (to <= from) {
                if (to < length) to = from + 1
                else if (from > 0) from = to - 1
                else return null
            }
            return from to to
        }

        private fun tooltipOf(diagnostic: SharpDiagnostic, run: DiagnosticsRun): String =
            DiagnosticHtml.tooltip(
                diagnostic,
                linkFor = { place -> TooltipLinks.hrefFor(run.run, place) },
                fileLabel = { place -> place.file.substringAfterLast('/') + ":" + place.line },
            )
    }
}

/** The links in a tooltip: `#sharp/<run>/<offset>/<path>`, understood by SharpTooltipLinkHandler. */
object TooltipLinks {
    const val PREFIX = "#sharp"

    fun hrefFor(run: String, place: Place): String =
        "$PREFIX/$run/${place.from}/" + URLEncoder.encode(place.file, StandardCharsets.UTF_8)

    /** The parts of a link, or null when it is not one of ours. `suffix` is what follows the prefix. */
    fun parse(suffix: String): Triple<String, Int, String>? {
        val parts = suffix.trimStart('/').split('/', limit = 3)
        if (parts.size != 3) return null
        val offset = parts[1].toIntOrNull() ?: return null
        val path = try {
            java.net.URLDecoder.decode(parts[2], StandardCharsets.UTF_8)
        } catch (_: IllegalArgumentException) {
            return null
        }
        return Triple(parts[0], offset, path)
    }
}
