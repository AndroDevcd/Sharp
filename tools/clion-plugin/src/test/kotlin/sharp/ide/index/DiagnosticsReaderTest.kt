package sharp.ide.index

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import sharp.ide.run.SavedText

/**
 * THE DIAGNOSTICS READER ON REAL COMPILER OUTPUT
 *
 * Every range is cut out of the sample's text by the editor convention (UTF-16 offsets into
 * the text with line ends as one character) and compared with the words the compiler meant to
 * mark. That proves the reader AND the plugin's idea of the text agree with the compiler.
 */
class DiagnosticsReaderTest {
    @get:Rule val temp = TemporaryFolder()

    private fun read(name: String): Pair<DiagnosticsRun, String> {
        val fixture = CompilerOutputFixture(name)
        val folder = fixture.writeRunFolder(temp.newFolder(name).toPath(), "/src/$name.sharp")
        return DiagnosticsReader.readFolder(folder) to SavedText.decode(fixture.sourceBytes)
    }

    private fun marked(text: String, place: Place) = text.substring(place.from, place.to)

    @Test fun errorsAndAWarningOfAProgramThatParses() {
        val (run, text) = read("analysis_errors")
        assertEquals("full", run.manifest.level)
        assertEquals(2, run.manifest.errors)
        assertEquals(1, run.manifest.warnings)
        assertEquals(3, run.diagnostics.size)
        val (first, second, third) = run.diagnostics
        assertEquals("E0301", first.code); assertEquals(DiagnosticSeverity.ERROR, first.severity)
        assertEquals("strng", marked(text, first.place))
        assertEquals(listOf("did you mean `string`?"), first.helps)
        assertTrue(first.label.startsWith("not declared in this file"))
        assertEquals("missing_name", marked(text, second.place))
        assertEquals("W1003", third.code); assertEquals(DiagnosticSeverity.WARNING, third.severity)
        assertEquals("string", marked(text, third.place))
        assertEquals(listOf("remove ` as string`"), third.helps)
    }

    @Test fun everyDiagnosticSaysTheSameThingInOffsetsAndInLinesAndColumns() {
        for (name in listOf("analysis_errors", "related_places", "syntax_error")) {
            val (run, text) = read(name)
            for (d in run.diagnostics) {
                val lines = text.substring(0, d.place.from).count { it == '\n' } + 1
                val column = d.place.from - (text.lastIndexOf('\n', d.place.from - 1) + 1) + 1
                assertEquals("$name ${d.code} line", lines, d.place.line)
                assertEquals("$name ${d.code} column", column, d.place.column)
            }
        }
    }

    @Test fun aRelatedPlaceAndANote() {
        val (run, text) = read("related_places")
        val twice = run.diagnostics[0]
        assertEquals("E0302", twice.code)
        assertEquals("thing", marked(text, twice.place))
        assertEquals(1, twice.related.size)
        assertEquals("thing", marked(text, twice.related[0]))
        assertEquals("the first one is here", twice.related[0].label)
        assertEquals(3, twice.related[0].line)
        assertEquals("/src/related_places.sharp", twice.related[0].file)
        val mismatch = run.diagnostics[1]
        assertEquals("\"five\"", marked(text, mismatch.place))
        assertEquals(1, mismatch.notes.size)
        assertTrue(mismatch.notes[0].startsWith("text in quotes is text"))
    }

    @Test fun aProgramThatDoesNotParse() {
        val (run, text) = read("syntax_error")
        assertEquals("syntax-errors", run.manifest.level)
        assertEquals(1, run.diagnostics.size)
        assertEquals("E0102", run.diagnostics[0].code)
        val place = run.diagnostics[0].place
        assertEquals(" ", marked(text, place))                          // the compiler marks the gap where the value is missing
        assertEquals(';', text[place.to])
    }

    @Test fun crlfTabsNonAsciiAndAnEmojiAreCountedAsTheEditorCountsThem() {
        val (run, text) = read("wide")
        assertTrue("the fixture really has all of it", text.contains("\t") && text.contains("café") && text.contains("😀"))
        assertEquals("missing_name", marked(text, run.diagnostics[0].place))
        assertEquals(6, run.diagnostics[0].place.line)
        assertEquals(11, run.diagnostics[0].place.column)                 // a tab is one column
    }

    @Test fun theFingerprintMatchesTheCompilersHashForEveryFixture() {
        for (name in listOf("analysis_errors", "related_places", "syntax_error", "wide")) {
            val fixture = CompilerOutputFixture(name)
            val hash = Regex("\"path\":\"" + Regex.escape(fixture.originalPath) + "\"[^}]*?\"hash\":\"([0-9a-f]+)\"")
                .find(fixture.manifestText)!!.groupValues[1]
            assertEquals(name, hash, SavedText.hash(fixture.sourceBytes))
        }
    }

    @Test fun theManifestListsEveryFileWithItsHash() {
        val fixture = CompilerOutputFixture("analysis_errors")
        val manifest = DiagnosticsReader.parseManifest(fixture.manifestText)
        assertTrue(manifest.files.size > 200)
        assertEquals(SavedText.hash(fixture.sourceBytes), manifest.hashByPath[fixture.originalPath])
    }

    @Test fun aCleanProgramHasNoDiagnosticsAndTheManifestStillSaysFull() {
        val (run, _) = read("clean_program")
        assertEquals("full", run.manifest.level)
        assertEquals(0, run.manifest.errors)
        assertTrue(run.diagnostics.isEmpty())
        assertTrue(run.forFile("/src/clean_program.sharp").isEmpty())
    }

    private fun expectFailure(textOfReason: String, action: () -> Unit) {
        try {
            action()
            fail("expected a RunReadException mentioning: $textOfReason")
        } catch (e: RunReadException) {
            assertTrue("'${e.message}' should mention '$textOfReason'", e.message!!.contains(textOfReason))
        }
    }

    @Test fun noManifestMeansTheRunFailed() {
        expectFailure("no ide.json") { DiagnosticsReader.readFolder(temp.newFolder("empty").toPath()) }
    }

    @Test fun theCompilersOwnFailureNoteIsTheReason() {
        val folder = temp.newFolder("failed")
        java.io.File(folder, "failed.txt").writeText("could not write /x/diagnostics.json: permission denied\nstack...\n")
        expectFailure("permission denied") { DiagnosticsReader.readFolder(folder.toPath()) }
    }

    @Test fun aFileFromAnotherRunIsRefused() {
        val fixture = CompilerOutputFixture("analysis_errors")
        val folder = fixture.writeRunFolder(temp.newFolder("mixed").toPath(), "/src/a.sharp")
        val other = fixture.diagnosticsText.replace("\"run\":\"", "\"run\":\"ff")
        java.nio.file.Files.write(folder.resolve("diagnostics.json"), other.toByteArray())
        expectFailure("leftover") { DiagnosticsReader.readFolder(folder) }
    }

    @Test fun aFormatNumberTheReaderDoesNotKnowNamesWhichSideIsOld() {
        val fixture = CompilerOutputFixture("analysis_errors")
        expectFailure("update the plugin") { DiagnosticsReader.parseManifest(fixture.manifestText.replaceFirst("\"format\":1", "\"format\":2")) }
        expectFailure("update the compiler") { DiagnosticsReader.parseManifest(fixture.manifestText.replaceFirst("\"format\":1", "\"format\":0")) }
    }

    @Test fun brokenJsonIsAReasonNotACrash() {
        expectFailure("not valid JSON") { DiagnosticsReader.parseManifest("{\"format\":1,") }
        expectFailure("not a JSON object") { DiagnosticsReader.parseManifest("[]") }
    }

    /** Valid JSON, but a number where a file entry and a text where a diagnostic should be: a reason in words, never a crash. */
    @Test fun jsonOfTheWrongShapeIsAReasonNotACrash() {
        val fixture = CompilerOutputFixture("analysis_errors")
        expectFailure("ide.json is not in the shape") {
            DiagnosticsReader.parseManifest(fixture.manifestText.replaceFirst("\"files\":[", "\"files\":[7,"))
        }
        val manifest = DiagnosticsReader.parseManifest(fixture.manifestText)
        expectFailure("diagnostics.json is not in the shape") {
            DiagnosticsReader.parseDiagnostics(fixture.diagnosticsText.replaceFirst("\"diagnostics\":[", "\"diagnostics\":[\"x\","), manifest)
        }
    }
}
