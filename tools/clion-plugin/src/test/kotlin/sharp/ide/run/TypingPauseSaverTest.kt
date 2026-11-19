package sharp.ide.run

import com.intellij.openapi.command.WriteCommandAction
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.testFramework.PlatformTestUtil
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.settings.SharpProjectSettings
import java.nio.file.Files
import java.nio.file.Path

/**
 * A PAUSE IN TYPING SAVES THE EDITED SHARP FILES
 *
 * The compiler reads the disk, so "check when I stop typing" is "save when I stop typing".
 * These tests edit a document without saving it and look at what TypingPauseSaver does.
 */
class TypingPauseSaverTest : BasePlatformTestCase() {
    private lateinit var root: Path
    private lateinit var savedApp: SharpApplicationSettings.State
    private lateinit var savedProject: SharpProjectSettings.State
    private lateinit var saver: TypingPauseSaver

    override fun setUp() {
        super.setUp()
        root = Files.createTempDirectory("sharp-typing").toRealPath()
        Files.createDirectories(root.resolve("lib/std"))
        Files.write(root.resolve("lib/std/core.sharp"), "mod std;\n".toByteArray())
        Files.createDirectories(root.resolve("app"))
        Files.write(root.resolve("app/main.sharp"), "mod app;\n\ndef main() { }\n".toByteArray())
        Files.createDirectories(root.resolve("elsewhere"))
        Files.write(root.resolve("elsewhere/other.sharp"), "mod other;\n".toByteArray())

        val app = SharpApplicationSettings.getInstance().state
        val proj = project.getService(SharpProjectSettings::class.java).state
        savedApp = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(app, it) }
        savedProject = SharpProjectSettings.State().also { XmlSerializerUtil.copyBean(proj, it) }
        proj.enabled = true
        proj.sourceRoots = mutableListOf(ProgramFiles.normalise(root.resolve("app").toString()))
        proj.libraryPath = ProgramFiles.normalise(root.resolve("lib").toString())
        saver = TypingPauseSaver.getInstance(project)
    }

    override fun tearDown() {
        try {
            saver.pauseOverrideMillis = null
            XmlSerializerUtil.copyBean(savedApp, SharpApplicationSettings.getInstance().state)
            XmlSerializerUtil.copyBean(savedProject, project.getService(SharpProjectSettings::class.java).state)
            root.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** Opens the file and types a comment at its top, WITHOUT saving. */
    private fun edit(relative: String): com.intellij.openapi.editor.Document {
        val file = LocalFileSystem.getInstance().refreshAndFindFileByNioFile(root.resolve(relative))!!
        myFixture.openFileInEditor(file)
        val document = myFixture.editor.document
        WriteCommandAction.runWriteCommandAction(project) { document.insertString(0, "// typed\n") }
        assertTrue(FileDocumentManager.getInstance().isDocumentUnsaved(document))
        return document
    }

    private fun onDisk(relative: String): String = String(Files.readAllBytes(root.resolve(relative)), Charsets.UTF_8)

    fun testAnEditedFileOfTheProgramIsSavedWhenThePauseIsOver() {
        val document = edit("app/main.sharp")
        saver.saveWhatWasEdited()
        PlatformTestUtil.dispatchAllInvocationEventsInIdeEventQueue()
        assertFalse(FileDocumentManager.getInstance().isDocumentUnsaved(document))
        assertTrue(onDisk("app/main.sharp").startsWith("// typed\n"))
    }

    fun testASharpFileThatIsNotPartOfTheProgramIsLeftAlone() {
        val document = edit("elsewhere/other.sharp")
        saver.saveWhatWasEdited()
        PlatformTestUtil.dispatchAllInvocationEventsInIdeEventQueue()
        assertTrue(FileDocumentManager.getInstance().isDocumentUnsaved(document))
        assertEquals("mod other;\n", onDisk("elsewhere/other.sharp"))
    }

    fun testNothingIsSavedWhileCheckingIsSwitchedOff() {
        project.getService(SharpProjectSettings::class.java).state.enabled = false
        val document = edit("app/main.sharp")
        saver.saveWhatWasEdited()
        PlatformTestUtil.dispatchAllInvocationEventsInIdeEventQueue()
        assertTrue(FileDocumentManager.getInstance().isDocumentUnsaved(document))
    }

    fun testTheTimerSavesByItselfAfterThePause() {
        saver.pauseOverrideMillis = 50
        val document = edit("app/main.sharp")
        saver.edited(document)                                     // what the document listener calls on every key press
        val deadline = System.currentTimeMillis() + 5000
        while (FileDocumentManager.getInstance().isDocumentUnsaved(document) && System.currentTimeMillis() < deadline) {
            Thread.sleep(20)
            PlatformTestUtil.dispatchAllInvocationEventsInIdeEventQueue()
        }
        assertFalse("the file was not saved within 5 seconds of a 50 ms pause", FileDocumentManager.getInstance().isDocumentUnsaved(document))
    }

    fun testAPauseOfZeroSwitchesItOff() {
        saver.pauseOverrideMillis = 0
        val document = edit("app/main.sharp")
        saver.edited(document)
        Thread.sleep(200)
        PlatformTestUtil.dispatchAllInvocationEventsInIdeEventQueue()
        assertTrue(FileDocumentManager.getInstance().isDocumentUnsaved(document))
    }
}
