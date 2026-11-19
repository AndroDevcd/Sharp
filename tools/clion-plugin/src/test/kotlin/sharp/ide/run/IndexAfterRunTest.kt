package sharp.ide.run

import com.intellij.openapi.progress.EmptyProgressIndicator
import com.intellij.openapi.progress.ProgressIndicator
import com.intellij.openapi.project.Project
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.index.CompilerOutputFixture
import sharp.ide.index.IndexFixture
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpDefaults
import sharp.ide.settings.SharpProjectSettings
import sharp.ide.settings.TranslateMode
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE INDEX AFTER EACH RUN, THROUGH THE WHOLE PIPELINE WITH A FAKE COMPILER
 *
 * doc/ide.md 3.5: the index is the newest run that parsed, the whole of it; a run that stops at syntax
 * errors, one that fails, and one whose files cannot be read all leave the last good index in place;
 * a newer good run replaces it in one assignment. The fake compiler writes REAL compiler output
 * (the `completion` fixture, the standard library included) into the run folder the plugin made.
 */
class IndexAfterRunTest : BasePlatformTestCase() {
    private lateinit var root: Path
    private lateinit var sourcePath: String
    private lateinit var shop: IndexFixture
    private lateinit var service: AnalysisService
    private lateinit var savedApp: SharpApplicationSettings.State
    private lateinit var savedProject: SharpProjectSettings.State
    private var counter = 0
    private var behaviour: (Path) -> LaunchResult = { LaunchResult.Finished(0, "") }

    override fun setUp() {
        super.setUp()
        root = Files.createTempDirectory("sharp-index-run").toRealPath()
        shop = IndexFixture("completion")
        Files.createDirectories(root.resolve("lib/std"))
        Files.write(root.resolve("lib/std/core.sharp"), "mod std;\n".toByteArray())
        Files.createDirectories(root.resolve("app"))
        Files.write(root.resolve("app/main.sharp"), shop.sourceBytes)
        sourcePath = ProgramFiles.normalise(root.resolve("app/main.sharp").toString())

        val app = SharpApplicationSettings.getInstance().state
        val proj = project.getService(SharpProjectSettings::class.java).state
        savedApp = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(app, it) }
        savedProject = SharpProjectSettings.State().also { XmlSerializerUtil.copyBean(proj, it) }
        app.launcher = SharpDefaults.LAUNCHER_NATIVE
        app.translateMode = TranslateMode.NEVER.name
        proj.enabled = true
        proj.sourceRoots = mutableListOf(ProgramFiles.normalise(root.resolve("app").toString()))
        proj.libraryPath = ProgramFiles.normalise(root.resolve("lib").toString())

        service = AnalysisService.getInstance(project)
        service.clearIndex()
        service.runBaseOverride = root.resolve("runs")
        service.failureSink = { _: Project, _ -> }
        service.infoSink = { _: Project, _ -> }
        service.launcher = object : ProcessLauncher {
            override fun launch(command: List<String>, timeoutMillis: Long, indicator: ProgressIndicator): LaunchResult =
                behaviour(Path.of(command.last()).parent)
        }
    }

    override fun tearDown() {
        try {
            service.clearIndex()
            XmlSerializerUtil.copyBean(savedApp, SharpApplicationSettings.getInstance().state)
            XmlSerializerUtil.copyBean(savedProject, project.getService(SharpProjectSettings::class.java).state)
            root.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** A compiler that parses the program and writes the sample's real index, as run number `id`. */
    private fun compilerWritesIndex(id: String) {
        behaviour = { folder ->
            shop.writeRunFolder(folder, sourcePath, runId = id)
            LaunchResult.Finished(0, "")
        }
    }

    private fun oneRun(): RunReport = service.runOnce(EmptyProgressIndicator())

    fun testTheIndexIsLoadedAfterARunThatParsed() {
        assertNull(service.index)
        compilerWritesIndex("aaaa01")
        assertTrue(oneRun() is RunReport.Done)
        val index = service.index!!
        assertEquals("aaaa01", index.run)
        assertEquals("full", index.level)
        assertNotNull(index.classesById["shop#book"])
        assertNotNull(index.classesById["std#list"])
        assertEquals("aaaa01", service.latest!!.run)
    }

    fun testASyntaxErrorRunKeepsTheLastGoodIndexButShowsItsOwnErrors() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val before = service.index!!
        val broken = CompilerOutputFixture("syntax_error")
        behaviour = { folder ->
            broken.writeRunFolder(folder, sourcePath)
            LaunchResult.Finished(0, "")
        }
        oneRun()
        assertEquals("syntax-errors", service.latest!!.manifest.level)
        assertSame("completion keeps using the last good index", before, service.index)
    }

    fun testAFailedRunKeepsTheLastGoodIndex() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val before = service.index
        behaviour = { LaunchResult.CouldNotStart("wsl.exe is not installed") }
        assertTrue(oneRun() is RunReport.Failed)
        assertSame(before, service.index)
        behaviour = { LaunchResult.Finished(1, "boom") }                       // ran, but wrote no ide.json
        assertTrue(oneRun() is RunReport.Failed)
        assertSame(before, service.index)
    }

    fun testANewerGoodRunReplacesTheIndexWhole() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val first = service.index!!
        compilerWritesIndex("aaaa02")
        oneRun()
        val second = service.index!!
        assertNotSame(first, second)
        assertEquals("aaaa02", second.run)
        assertEquals("aaaa02", service.latest!!.run)
    }

    fun testAnIndexWhoseFilesDoNotBelongTogetherIsNotUsed() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val before = service.index
        behaviour = { folder ->
            shop.writeRunFolder(folder, sourcePath, runId = "aaaa03")
            // a leftover of an older run: the declarations of run aaaa01
            val old = Files.readAllBytes(shop.writeRunFolder(root.resolve("leftover-${counter++}"), sourcePath, runId = "aaaa01").resolve("declarations.json"))
            Files.write(folder.resolve("declarations.json"), old)
            LaunchResult.Finished(0, "")
        }
        val report = oneRun()
        assertTrue(report.toString(), report is RunReport.Done)                  // the diagnostics are good and are shown
        assertEquals("aaaa03", service.latest!!.run)
        assertSame("but the index of that run is refused", before, service.index)
    }

    fun testTheIndexRunKeepsItsSnapshotsThroughLaterSyntaxErrorRuns() {
        compilerWritesIndex("aaaa01")
        oneRun()
        service.snapshots.keep("aaaa01", mapOf(sourcePath to Snapshot(shop.savedText, SavedText.hash(shop.sourceBytes))), mapOf(sourcePath to SavedText.hash(shop.sourceBytes)))
        val broken = CompilerOutputFixture("syntax_error")
        repeat(5) { n ->
            behaviour = { folder ->
                broken.writeRunFolder(folder, sourcePath)
                LaunchResult.Finished(0, "")
            }
            service.snapshots.keep("bad$n", emptyMap(), emptyMap())
            oneRun()
        }
        assertNotNull("pinned: not pushed out by five newer runs", service.snapshots.get("aaaa01", sourcePath))
    }

    fun testOpeningAFileTheIndexHasNoBodiesForAsksForARun() {
        val vfs = com.intellij.openapi.vfs.LocalFileSystem.getInstance()
        Files.write(root.resolve("app/second.sharp"), "mod shop;".toByteArray())
        val main = vfs.refreshAndFindFileByNioFile(root.resolve("app/main.sharp"))!!
        val second = vfs.refreshAndFindFileByNioFile(root.resolve("app/second.sharp"))!!
        val outside = Files.write(root.resolve("elsewhere.sharp"), "mod x;".toByteArray())
        val elsewhere = vfs.refreshAndFindFileByNioFile(outside)!!
        assertTrue("no index yet", EditorOpenListener.shouldAskForRun(project, main))
        compilerWritesIndex("aaaa01")
        oneRun()
        assertFalse("the run wrote bodies for the sample", EditorOpenListener.shouldAskForRun(project, main))
        assertTrue("a file the run did not list", EditorOpenListener.shouldAskForRun(project, second))
        assertFalse("not a file of the program", EditorOpenListener.shouldAskForRun(project, elsewhere))
    }

    // ---- after the IDE is restarted: the last answer is read back from the run folders (doc/ide.md 3.1) ----

    fun testAfterARestartTheLastRunIsReadBackFromDisk() {
        compilerWritesIndex("aaaa01")
        oneRun()
        service.forgetAllRuns()                                                  // what a restart leaves: the folders, nothing in memory
        assertNull(service.latest)
        assertNull(service.index)

        service.loadLastRunFromDisk()
        assertEquals("aaaa01", service.latest!!.run)
        val index = service.index!!
        assertEquals("aaaa01", index.run)
        assertNotNull(index.classesById["shop#book"])
        // No text of the last session is kept, but the file on disk still has the recorded hash, so it is adopted.
        val main = com.intellij.openapi.vfs.LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve("app/main.sharp"))!!
        assertEquals(shop.savedText, sharp.ide.editor.FileViews.snapshotOf(service, index, sourcePath, index.fileByPath[sourcePath]!!, main))
        assertFalse("its bodies are there: no extra run is asked for", EditorOpenListener.shouldAskForRun(project, main))
    }

    fun testAfterARestartWhileTheProgramDoesNotParseTheOlderIndexIsReadBack() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val broken = CompilerOutputFixture("syntax_error")
        behaviour = { folder ->
            broken.writeRunFolder(folder, sourcePath)
            LaunchResult.Finished(0, "")
        }
        repeat(3) { oneRun() }
        val kept = RunFolders.numberedFolders(root.resolve("runs")).map { it.first }
        assertEquals("the newest two, and the folder the index came from", listOf(1L, 3L, 4L), kept)

        service.forgetAllRuns()
        service.loadLastRunFromDisk()
        assertEquals("the errors of the newest run", "syntax-errors", service.latest!!.manifest.level)
        assertEquals("the index of the last run that parsed", "aaaa01", service.index!!.run)
    }

    fun testReadingBackNeverReplacesWhatARunOfThisSessionGave() {
        compilerWritesIndex("aaaa01")
        oneRun()
        val latest = service.latest
        val index = service.index
        service.loadLastRunFromDisk()
        assertSame(latest, service.latest)
        assertSame(index, service.index)
    }

    fun testFoldersThatCannotBeReadAreSkippedQuietly() {
        service.forgetAllRuns()
        Files.createDirectories(root.resolve("runs/run-1"))                       // a run that was killed: no ide.json
        Files.createDirectories(root.resolve("runs/run-2"))
        Files.write(root.resolve("runs/run-2/ide.json"), "{\"format\":1,".toByteArray())
        service.loadLastRunFromDisk()
        assertNull(service.latest)
        assertNull(service.index)
    }
}
