package sharp.ide.run

import com.intellij.codeInsight.completion.CompletionType
import com.intellij.lang.annotation.HighlightSeverity
import com.intellij.openapi.application.runWriteAction
import com.intellij.openapi.command.WriteCommandAction
import com.intellij.openapi.progress.EmptyProgressIndicator
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.documentation.SharpDocumentationTargetProvider
import sharp.ide.index.CompilerOutputFixture
import sharp.ide.index.DiagnosticsRun
import sharp.ide.index.IndexFixture
import sharp.ide.navigation.SharpGotoDeclarationHandler
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpProjectSettings
import java.io.File
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE WHOLE CHAIN WITH THE REAL COMPILER, WITHOUT A SCREEN
 *
 * Nothing is replaced here: the plugin's real default settings (wsl.exe, the Sharp VM and the compiler
 * in WSL), the real ProcessLauncher, the real standard library of the repository, the real
 * AnalysisService, reader, index loader, annotator and completion. A temporary "project" with a
 * program in it is analysed three ways - a correct program, a program with analysis errors, a program
 * with a syntax error - and what comes out is compared with what the owner should see in CLion.
 *
 * Each run takes several seconds (the compiler analyses the whole library), so the scenarios are few.
 * The test is skipped (not failed) on a machine without WSL or without the compiler build: it needs
 * the owner's own setup, exactly what the default settings describe. A skip is silent, so to be SURE
 * the real compiler was run, start Gradle with `-PrequireRealCompiler` (build.gradle.kts hands it on
 * as the system property `sharp.requireRealCompiler`): then a missing setup is a failure, not a skip.
 */
class EndToEndWithRealCompilerTest : BasePlatformTestCase() {
    private lateinit var root: Path
    private lateinit var savedApp: SharpApplicationSettings.State
    private lateinit var savedProject: SharpProjectSettings.State
    private lateinit var service: AnalysisService

    /** False on a machine without the owner's setup: every test then returns at once (a skipped check, not a failure). */
    private var realSetup = false

    override fun setUp() {
        super.setUp()
        realSetup = hasRealSetup()
        if (!realSetup) {
            assertFalse(
                "-PrequireRealCompiler was given, but wsl.exe, the Sharp VM or the compiler build (boot_ide) is missing",
                System.getProperty("sharp.requireRealCompiler") == "true",
            )
            return
        }
        // A path with a space in it, like the owner's: the real thing the default command must survive.
        root = Files.createTempDirectory("sharp end to end ").toRealPath()
        val app = SharpApplicationSettings.getInstance().state
        val proj = project.getService(SharpProjectSettings::class.java).state
        savedApp = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(app, it) }
        savedProject = SharpProjectSettings.State().also { XmlSerializerUtil.copyBean(proj, it) }
        // Everything stays at the defaults of this machine, except: saving a file in this test must not start
        // a second, background run of the real compiler by itself (the listener is real too).
        XmlSerializerUtil.copyBean(SharpApplicationSettings.State(), app)               // the machine's defaults, whatever ran before
        app.debounceMillis = 600_000
        proj.enabled = true
        proj.libraryPath = ProgramFiles.normalise(File(System.getProperty("sharp.repo"), "lib/support/0.4.0").canonicalPath)
        service = AnalysisService.getInstance(project)
        service.runBaseOverride = root.resolve("runs")
        // The project of the platform's light fixture is shared by the test classes of one run, and other classes put
        // a fake launcher into the same service: this test wants the real one, and a clean state.
        service.launcher = PlatformProcessLauncher()
        service.failureSink = { _, _ -> }
        service.infoSink = { _, _ -> }
        service.clearIndex()
        service.failures.clear()
    }

    override fun tearDown() {
        try {
            if (realSetup && ::service.isInitialized) service.clearIndex()
            if (::savedApp.isInitialized) {
                XmlSerializerUtil.copyBean(savedApp, SharpApplicationSettings.getInstance().state)
                XmlSerializerUtil.copyBean(savedProject, project.getService(SharpProjectSettings::class.java).state)
            }
            if (::root.isInitialized) root.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** True when wsl.exe is there and the compiler the default settings name has been built. */
    private fun hasRealSetup(): Boolean {
        if (!System.getProperty("os.name").startsWith("Windows")) return false
        val wsl = File(System.getenv("SystemRoot") ?: "C:\\Windows", "System32/wsl.exe")
        if (!wsl.exists()) return false
        val probe = ProcessBuilder("wsl.exe", "-e", "bash", "-c", "test -f \$HOME/sharp_work/out/boot_ide/sharpc_boot && test -x \$HOME/sharp_work/sharp_cl/sharp")
            .redirectErrorStream(true).start()
        probe.inputStream.readBytes()
        return probe.waitFor() == 0
    }

    /** Writes `bytes` as `<root>/<name>/main.sharp`, makes that folder the program, and returns the file's path. */
    private fun makeProgram(name: String, bytes: ByteArray): String {
        val folder = root.resolve(name)
        Files.createDirectories(folder)
        val file = folder.resolve("main.sharp")
        Files.write(file, bytes)
        project.getService(SharpProjectSettings::class.java).state.sourceRoots = mutableListOf(ProgramFiles.normalise(folder.toString()))
        return ProgramFiles.normalise(file.toString())
    }

    private fun runCompiler(): DiagnosticsRun {
        val report = service.runOnce(EmptyProgressIndicator())
        assertTrue("the run did not finish: $report", report is RunReport.Done)
        return (report as RunReport.Done).run
    }

    private fun open(path: String) {
        val vf = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(Path.of(path))!!
        myFixture.openFileInEditor(vf)
    }

    private fun marks() = myFixture.doHighlighting().filter { it.description?.matches(Regex("[EW]\\d{4}: .*")) == true }.sortedBy { it.startOffset }

    // ------------------------------------------------------------------ a correct program

    fun testACorrectProgramHasNoErrorsAndCompletesFromTheIndexItGave() {
        if (!realSetup) return
        val fixture = IndexFixture("completion")
        val path = makeProgram("good", fixture.sourceBytes)
        open(path)                                         // open first: the run then writes this file's bodies
        val run = runCompiler()

        assertEquals("level", "full", run.manifest.level)
        assertEquals(0, run.manifest.errors)
        assertEquals(0, run.manifest.warnings)
        assertTrue("the library and the program are one list: ${run.manifest.files.size}", run.manifest.files.size > 250)
        assertTrue(failuresOf(run), run.diagnostics.isEmpty())
        assertNotNull("an index was loaded", service.index)
        assertTrue("no marks on a correct program", marks().isEmpty())

        // after a dot, the text the compiler saw: the members of list<string>, with the type argument put in
        val saved = String(fixture.sourceBytes, Charsets.UTF_8).replace("\r\n", "\n")
        val afterDot = saved.indexOf("names.at(0)") + "names.".length
        myFixture.editor.caretModel.moveToOffset(afterDot)
        val offered = myFixture.complete(CompletionType.BASIC)!!.map { it.lookupString }
        assertTrue(offered.toString(), offered.containsAll(listOf("add", "at", "size")))

        // hover and go to declaration on a use the compiler saw: `size` in `names.size()` is a function of the library
        val onSize = saved.indexOf("names.size()") + "names.s".length
        val hover = SharpDocumentationTargetProvider().documentationTargets(myFixture.file, onSize)
        assertEquals(1, hover.size)
        assertEquals("function size : int", hover.single().computeDocumentationHint())
        val jump = SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(onSize), onSize, myFixture.editor)!!
        assertEquals("the real library file", "list.sharp", jump.single().containingFile.name)
        assertEquals("size", jump.single().text)
        // ... and on a local of this file: the jump stays in the file, at the declaration
        val onCount = saved.indexOf("return count.value;") + "return c".length
        val local = SharpGotoDeclarationHandler().getGotoDeclarationTargets(myFixture.file.findElementAt(onCount), onCount, myFixture.editor)!!
        assertEquals(saved.indexOf("count := names.size()"), local.single().textRange.startOffset)

        // after a dot typed since the run (the index is one save behind): own members of `book`, inherited `label`
        val at = saved.indexOf("    return count.value;")
        WriteCommandAction.runWriteCommandAction(project) { myFixture.editor.document.insertString(at, "    owner.\n") }
        com.intellij.psi.PsiDocumentManager.getInstance(project).commitAllDocuments()
        myFixture.editor.caretModel.moveToOffset(at + "    owner.".length)
        val stale = myFixture.complete(CompletionType.BASIC)!!.map { it.lookupString }
        assertTrue(stale.toString(), stale.containsAll(listOf("pages", "summary", "title", "label", "price")))
    }

    // ------------------------------------------------------------------ analysis errors

    fun testAnalysisErrorsComeOutAsMarksOnTheRightText() {
        if (!realSetup) return
        val fixture = CompilerOutputFixture("analysis_errors")
        val path = makeProgram("errors", fixture.sourceBytes)
        open(path)
        val run = runCompiler()

        assertEquals("level", "full", run.manifest.level)           // a declaration error still leaves a full index
        assertEquals(2, run.manifest.errors)
        assertEquals(1, run.manifest.warnings)
        assertEquals(listOf("E0301", "E0400", "W1003"), run.diagnostics.map { it.code }.sorted())
        assertNotNull(service.index)

        val found = marks()
        assertEquals(listOf("strng", "missing_name", "string"), found.map { myFixture.editor.document.text.substring(it.startOffset, it.endOffset) })
        assertEquals(listOf(HighlightSeverity.ERROR, HighlightSeverity.ERROR, HighlightSeverity.WARNING), found.map { it.severity })
        assertTrue("the tooltip carries the compiler's help", found[0].toolTip!!.contains("string"))
    }

    // ------------------------------------------------------------------ a syntax error

    fun testASyntaxErrorShowsOnlyThePlainErrorAndKeepsTheLastGoodIndex() {
        if (!realSetup) return
        val good = IndexFixture("completion")
        val path = makeProgram("syntax", good.sourceBytes)
        open(path)
        runCompiler()
        val goodIndex = service.index
        assertNotNull(goodIndex)

        // the program is saved with a syntax error: this is what the compiler reads next
        val broken = CompilerOutputFixture("syntax_error").sourceBytes
        runWriteAction {
            LocalFileSystem.getInstance().refreshAndFindFileByNioFile(Path.of(path))!!.setBinaryContent(broken)
        }
        val run = runCompiler()

        assertEquals("level", "syntax-errors", run.manifest.level)
        assertTrue("errors: ${run.diagnostics}", run.manifest.errors >= 1)
        assertTrue(run.diagnostics.all { it.code.startsWith("E01") })
        assertSame("the last good index stays", goodIndex, service.index)
        val found = marks()
        assertTrue("the syntax error is marked", found.isNotEmpty())
        assertEquals(HighlightSeverity.ERROR, found[0].severity)
    }

    // ------------------------------------------------------------------ awkward text, and a restart of the IDE

    /**
     * Windows line ends, tabs, accented letters, Japanese and emoji stand BEFORE every mistake, so a mark is on
     * the right word only if the compiler and the editor count characters the same way (doc/ide.md 4.1). Then
     * the IDE "restarts": everything in memory is forgotten and the run folder on disk gives the marks back.
     */
    fun testTabsWindowsLineEndsAndEmojiAreMarkedOnTheRightWordsAlsoAfterARestart() {
        if (!realSetup) return
        val lines = listOf(
            "mod wide_real;",
            "",
            "import ( std.io )",
            "",
            "// café 😀 日本語",
            "class crate {",
            "\tlabel : string = \"naïve 😀\";\t// a tab, then 日本語",
            "\twrong : strng;",
            "}",
            "",
            "def noisy(text: string) : string {",
            "\t/* 😀😀 */ same := text as string;",
            "\treturn same;",
            "}",
            "",
            "def main(args: string[]) : int {",
            "\tarrow := \"→ 🚀\";\ttotal := missing_name + 1;",
            "\treturn 0;",
            "}",
            "",
        )
        val path = makeProgram("wide", lines.joinToString("\r\n").toByteArray(Charsets.UTF_8))
        open(path)
        val run = runCompiler()
        assertEquals(failuresOf(run), listOf("E0301", "E0400", "W1003"), run.diagnostics.map { it.code }.sorted())

        fun markedWords() = marks().map { myFixture.editor.document.text.substring(it.startOffset, it.endOffset) }
        assertEquals(listOf("strng", "string", "missing_name"), markedWords())

        service.forgetAllRuns()                                  // what closing the IDE leaves: nothing in memory
        service.restartHighlighting()                            // (the editor of this test is still open: have it look again)
        assertTrue("nothing is known, nothing is marked", marks().isEmpty())
        service.loadLastRunFromDisk()                            // what StartupRun does when the project opens again
        assertEquals("the same run, read back from its folder", run.run, service.latest!!.run)
        assertNotNull("and its index", service.index)
        assertEquals(listOf("strng", "string", "missing_name"), markedWords())
    }

    private fun failuresOf(run: DiagnosticsRun) = run.diagnostics.joinToString { it.code + " " + it.message }
}
