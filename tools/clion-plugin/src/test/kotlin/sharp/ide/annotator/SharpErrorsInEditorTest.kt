package sharp.ide.annotator

import com.intellij.codeInsight.daemon.impl.HighlightInfo
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.command.WriteCommandAction
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.index.CompilerOutputFixture
import sharp.ide.index.DiagnosticsReader
import sharp.ide.index.DiagnosticsRun
import sharp.ide.run.AnalysisService
import sharp.ide.run.SavedText
import sharp.ide.run.Snapshot
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.TranslateMode
import java.nio.file.Files
import java.nio.file.Path

/**
 * ERRORS IN THE EDITOR, THROUGH THE PLATFORM'S HEADLESS FIXTURE
 *
 * A real editor without a window: the file is put in, a run's diagnostics (real compiler
 * output) are published as AnalysisService would after a compiler run, and the platform's own
 * highlighting is run (`doHighlighting`), which calls SharpExternalAnnotator. What comes back
 * are the marks the owner would see: their ranges, severities, messages and tooltips.
 */
class SharpErrorsInEditorTest : BasePlatformTestCase() {
    private lateinit var tempDir: Path
    private lateinit var savedSettings: SharpApplicationSettings.State

    override fun setUp() {
        super.setUp()
        tempDir = Files.createTempDirectory("sharp-annotator")
        val state = SharpApplicationSettings.getInstance().state
        savedSettings = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(state, it) }
        state.translateMode = TranslateMode.NEVER.name              // the fixture's paths are not Windows paths
    }

    override fun tearDown() {
        try {
            XmlSerializerUtil.copyBean(savedSettings, SharpApplicationSettings.getInstance().state)
            tempDir.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** Opens the fixture's sample in the editor and publishes the compiler's output for it. Returns the saved text. */
    private fun open(name: String, snapshot: Boolean = true): Pair<DiagnosticsRun, String> {
        val fixture = CompilerOutputFixture(name)
        val saved = SavedText.decode(fixture.sourceBytes)
        val file = myFixture.configureByText("$name.sharp", saved)
        val path = file.virtualFile.path
        val run = DiagnosticsReader.readFolder(fixture.writeRunFolder(tempDir.resolve(name), path))
        val snapshots = if (snapshot) mapOf(path to Snapshot(saved, SavedText.hash(fixture.sourceBytes))) else emptyMap()
        AnalysisService.getInstance(project).publish(run, snapshots)
        return run to saved
    }

    /** The compiler's marks currently in the editor, in text order. */
    private fun marks(): List<HighlightInfo> =
        myFixture.doHighlighting().filter { it.description?.matches(Regex("[EW]\\d{4}: .*")) == true }.sortedBy { it.startOffset }

    private fun text(info: HighlightInfo) = myFixture.editor.document.text.substring(info.startOffset, info.endOffset)

    fun testErrorsAndWarningsAreMarkedOnTheRightText() {
        open("analysis_errors")
        val found = marks()
        assertEquals(listOf("strng", "missing_name", "string"), found.map { text(it) })
        assertEquals(listOf(HighlightSeverity.ERROR, HighlightSeverity.ERROR, HighlightSeverity.WARNING), found.map { it.severity })
        assertEquals("E0301: there is no type named `strng`", found[0].description)
    }

    fun testTheTooltipHasTheLabelTheHelpAndTheCode() {
        open("analysis_errors")
        val tooltip = marks()[0].toolTip!!
        assertTrue(tooltip, tooltip.contains("E0301"))
        assertTrue(tooltip, tooltip.contains("<code>strng</code>"))
        assertTrue(tooltip, tooltip.contains("not declared in this file"))
        assertTrue(tooltip, tooltip.contains("help:") && tooltip.contains("did you mean <code>string</code>?"))
    }

    fun testARelatedPlaceIsALinkInTheTooltip() {
        val (run, _) = open("related_places")
        val tooltip = marks()[0].toolTip!!
        assertTrue(tooltip, tooltip.contains("see also:") && tooltip.contains("the first one is here"))
        assertTrue(tooltip, tooltip.contains("href=\"#sharp/${run.run}/16/"))
        val second = marks()[1].toolTip!!
        assertTrue(second, second.contains("note:") && second.contains("text in quotes is text"))
    }

    fun testTextAddedAboveMovesTheMarks() {
        open("analysis_errors")
        WriteCommandAction.runWriteCommandAction(project) { myFixture.editor.document.insertString(0, "// added later\n// and more\n") }
        assertEquals(listOf("strng", "missing_name", "string"), marks().map { text(it) })
    }

    fun testAMarkVanishesWhereTheTextIsBeingEdited() {
        open("analysis_errors")
        WriteCommandAction.runWriteCommandAction(project) {
            val document = myFixture.editor.document
            val at = document.text.indexOf("total := missing_name") + "total := ".length
            document.insertString(at + 3, "X")
        }
        assertEquals(listOf("strng", "string"), marks().map { text(it) })
    }

    fun testANewResultReplacesTheOldMarks() {
        open("analysis_errors")
        assertEquals(3, marks().size)
        // the next run says the program now has no problems: the file's marks must all go
        val folder = tempDir.resolve("empty")
        Files.createDirectories(folder)
        val fixture = CompilerOutputFixture("analysis_errors")
        val path = myFixture.file.virtualFile.path
        Files.write(folder.resolve("ide.json"), fixture.manifestText.replace(fixture.originalPath, path).toByteArray())
        Files.write(
            folder.resolve("diagnostics.json"),
            ("{\"format\":1,\"run\":\"" + Regex("\"run\":\"([0-9a-f]+)\"").find(fixture.manifestText)!!.groupValues[1] +
                "\",\"truncated\":false,\"diagnostics\":[],\"unreadable\":[]}").toByteArray(),
        )
        AnalysisService.getInstance(project).publish(DiagnosticsReader.readFolder(folder), emptyMap())
        assertEquals(emptyList<HighlightInfo>(), marks())
    }

    fun testAFileOpenedAfterTheRunIsAdoptedWhenItStillHasTheTextTheCompilerSaw() {
        open("analysis_errors", snapshot = false)
        // no snapshot was taken, but the file on disk still has the hash of the run: it is adopted
        // (a light-fixture file lives in memory, and its bytes ARE the saved text)
        assertEquals(3, marks().size)
    }

    fun testAFileWhoseSavedTextIsNotWhatTheCompilerSawGetsNoMarks() {
        val fixture = CompilerOutputFixture("analysis_errors")
        val file = myFixture.configureByText("analysis_errors.sharp", SavedText.decode(fixture.sourceBytes) + "// saved again since\n")
        val run = DiagnosticsReader.readFolder(fixture.writeRunFolder(tempDir.resolve("other"), file.virtualFile.path))
        AnalysisService.getInstance(project).publish(run, emptyMap())
        assertEquals(emptyList<HighlightInfo>(), marks())
    }

    fun testASyntaxErrorRunShowsItsOwnErrorAndNotTheOlderOnes() {
        open("analysis_errors")
        val fixture = CompilerOutputFixture("syntax_error")
        val saved = SavedText.decode(fixture.sourceBytes)
        WriteCommandAction.runWriteCommandAction(project) { myFixture.editor.document.setText(saved) }
        val path = myFixture.file.virtualFile.path
        val run = DiagnosticsReader.readFolder(fixture.writeRunFolder(tempDir.resolve("syntax"), path))
        AnalysisService.getInstance(project).publish(run, mapOf(path to Snapshot(saved, SavedText.hash(fixture.sourceBytes))))
        val found = marks()
        assertEquals(listOf(" "), found.map { text(it) })                         // the compiler marks the gap before the `;`
        assertEquals(';', myFixture.editor.document.text[found[0].endOffset])
        assertTrue(found[0].description!!.startsWith("E0102"))
    }

    fun testACleanProgramHasNoMarks() {
        open("clean_program")
        assertEquals(emptyList<HighlightInfo>(), marks())
    }

    fun testMarksOnAFileWithCrlfTabsAndAnEmoji() {
        open("wide")
        assertEquals(listOf("missing_name"), marks().map { text(it) })
    }
}
