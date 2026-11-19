package sharp.ide.annotator

import com.intellij.lang.annotation.HighlightSeverity
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import sharp.ide.index.CompilerOutputFixture
import sharp.ide.index.DiagnosticsReader
import sharp.ide.index.DiagnosticsRun
import sharp.ide.run.SavedText

/**
 * THE MARKS THE ANNOTATOR MAKES, WITHOUT THE PLATFORM
 *
 * `buildMarks` is everything doAnnotate does: pick the file's diagnostics, move them onto the
 * current text, widen empty ranges, make the tooltips. Real compiler output; the "current text"
 * is the saved text, then edited in the ways a person edits.
 */
class AnnotatorMarksTest {
    @get:Rule val temp = TemporaryFolder()

    private val path = "/src/a.sharp"

    private fun run(name: String): Pair<DiagnosticsRun, String> {
        val fixture = CompilerOutputFixture(name)
        val folder = fixture.writeRunFolder(temp.newFolder(name).toPath(), path)
        return DiagnosticsReader.readFolder(folder) to SavedText.decode(fixture.sourceBytes)
    }

    private fun marks(run: DiagnosticsRun, saved: String?, current: String) =
        SharpExternalAnnotator.buildMarks(AnnotatorInput(path, run, saved, current))

    @Test fun markedTextIsTheTextTheCompilerMeant() {
        val (run, saved) = run("analysis_errors")
        val marks = marks(run, saved, saved)
        assertEquals(listOf("strng", "missing_name", "string"), marks.map { saved.substring(it.start, it.end) })
        assertEquals(listOf(HighlightSeverity.ERROR, HighlightSeverity.ERROR, HighlightSeverity.WARNING), marks.map { it.severity })
        assertTrue(marks[0].message.startsWith("E0301: there is no type named"))
        assertTrue(marks[0].tooltip.contains("did you mean <code>string</code>?"))
    }

    @Test fun marksMoveWithAnEditAbove() {
        val (run, saved) = run("analysis_errors")
        val current = "// a new first line\n// and another\n$saved"
        val marks = marks(run, saved, current)
        assertEquals(listOf("strng", "missing_name", "string"), marks.map { current.substring(it.start, it.end) })
    }

    @Test fun aMarkWhoseTextIsBeingEditedVanishesAndTheOthersStay() {
        val (run, saved) = run("analysis_errors")
        val current = saved.replace("total := missing_name +", "total := missing_nXame +")
        val marks = marks(run, saved, current)
        assertEquals(listOf("strng", "string"), marks.map { current.substring(it.start, it.end) })
    }

    @Test fun typingRightBesideAMarkKeepsIt() {
        val (run, saved) = run("analysis_errors")
        val current = saved.replace("total := missing_name +", "total := missing_name2 +")
        assertEquals(listOf("strng", "missing_name", "string"), marks(run, saved, current).map { current.substring(it.start, it.end) })
    }

    @Test fun theFixedLineLosesItsMark() {
        val (run, saved) = run("analysis_errors")
        val current = saved.replace("title : strng", "title : string")
        assertEquals(2, marks(run, saved, current).size)
    }

    @Test fun withoutASnapshotNothingIsMarked() {
        val (run, saved) = run("analysis_errors")
        assertTrue(marks(run, null, saved).isEmpty())
    }

    @Test fun aFileWithoutDiagnosticsHasNoMarks() {
        val (run, saved) = run("analysis_errors")
        assertTrue(SharpExternalAnnotator.buildMarks(AnnotatorInput("/src/other.sharp", run, saved, saved)).isEmpty())
    }

    @Test fun crlfTabsAndEmojiBeforeTheErrorDoNotShiftIt() {
        val (run, saved) = run("wide")
        val marks = marks(run, saved, saved)
        assertEquals("missing_name", saved.substring(marks.single().start, marks.single().end))
    }

    @Test fun theRelatedPlaceLinkCarriesTheRunAndTheOffset() {
        val (run, saved) = run("related_places")
        val first = marks(run, saved, saved)[0]
        assertTrue(first.tooltip, first.tooltip.contains("href=\"#sharp/${run.run}/16/"))
        assertTrue(first.tooltip.contains("the first one is here"))
        assertTrue(first.tooltip.contains("a.sharp:3"))
    }

    @Test fun widenKeepsRangesInsideTheTextAndMakesEmptyOnesVisible() {
        assertEquals(5 to 6, SharpExternalAnnotator.widen(5, 5, 10))
        assertEquals(9 to 10, SharpExternalAnnotator.widen(10, 10, 10))      // at the very end: the character before
        assertEquals(2 to 4, SharpExternalAnnotator.widen(2, 4, 10))
        assertEquals(8 to 10, SharpExternalAnnotator.widen(8, 99, 10))
        assertNull(SharpExternalAnnotator.widen(0, 0, 0))
    }
}
