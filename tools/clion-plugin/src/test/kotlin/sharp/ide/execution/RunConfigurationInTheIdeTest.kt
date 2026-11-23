/*
 * THE RUN CONFIGURATION AND THE PROGRAM FOLDER, inside the IDE's test environment: the pieces are
 * registered (plugin.xml names them rightly), a configuration keeps its fields, and choosing a
 * program folder changes what the plugin takes for the program.
 */
package sharp.ide.execution

import com.intellij.execution.RunManager
import com.intellij.execution.configurations.RuntimeConfigurationError
import com.intellij.ide.projectView.ProjectViewNodeDecorator
import com.intellij.openapi.actionSystem.ActionManager
import com.intellij.openapi.components.service
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpProjectSettings
import java.io.File
import java.nio.file.Files

class RunConfigurationInTheIdeTest : BasePlatformTestCase() {

    override fun tearDown() {
        try {
            project.service<SharpProjectSettings>().resetToDefaults()
        } finally {
            super.tearDown()
        }
    }

    fun testTheConfigurationTypeTheActionAndTheDecoratorAreRegistered() {
        val type = SharpRunConfigurationType.instance()
        assertEquals("Sharp Program", type.displayName)
        assertEquals(1, type.configurationFactories.size)
        assertNotNull("the right-click action", ActionManager.getInstance().getAction("Sharp.SetProgramFolder"))
        assertTrue("the program folder's mark in the Project view",
            project.extensionArea.getExtensionPoint<ProjectViewNodeDecorator>("com.intellij.projectViewNodeDecorator")
                .extensionList.any { it is SharpProgramFolderDecorator })
    }

    fun testAConfigurationKeepsItsFieldsAndSaysWhatIsMissing() {
        val settings = RunManager.getInstance(project).createConfiguration("shapes", SharpRunConfigurationType.instance().factory)
        val configuration = settings.configuration as SharpRunConfiguration
        try {
            configuration.checkConfiguration()
            fail("a configuration without a program folder must not pass")
        } catch (e: RuntimeConfigurationError) {
            assertTrue(e.message!!.contains("program folder"))
        }
        val folder = Files.createTempDirectory("sharp-run-config").toFile()
        try {
            configuration.fields.programFolder = folder.path
            configuration.fields.compilerOptions = "-O"
            configuration.fields.programArguments = "one \"two words\""
            configuration.fields.buildOnly = true
            assertEquals(ProgramFiles.normalise(folder.path), configuration.programFolderPath())

            // written to the workspace file and read back: a copy made the way the platform makes one
            val copy = settings.createFactory().create().configuration as SharpRunConfiguration
            assertEquals(folder.path, copy.fields.programFolder)
            assertEquals("-O", copy.fields.compilerOptions)
            assertEquals("one \"two words\"", copy.fields.programArguments)
            assertTrue(copy.fields.buildOnly)
            assertFalse(copy.fields.unoptimizedCompiler)

            // a folder written relative to the project is below the project
            configuration.fields.programFolder = "apps/shapes"
            assertEquals(ProgramFiles.normalise(project.basePath!!) + "/apps/shapes", configuration.programFolderPath())
        } finally {
            folder.deleteRecursively()
        }
    }

    fun testChoosingAProgramFolderMakesItTheProgram() {
        val base = Files.createTempDirectory("sharp-program-folders").toFile()
        try {
            File(base, "tools/shapes").mkdirs()
            File(base, "tools/shapes/main.sharp").writeText("mod main;\n")
            File(base, "notes").mkdirs()
            File(base, "notes/readme.txt").writeText("x")
            val files = LocalFileSystem.getInstance()
            val shapes = files.refreshAndFindFileByIoFile(File(base, "tools/shapes"))!!
            val notes = files.refreshAndFindFileByIoFile(File(base, "notes"))!!
            val main = files.refreshAndFindFileByIoFile(File(base, "tools/shapes/main.sharp"))!!
            val readme = files.refreshAndFindFileByIoFile(File(base, "notes/readme.txt"))!!
            val shapesPath = ProgramFiles.normalise(shapes.path)

            assertFalse(SharpProgramFolders.isProgramFolder(project, shapes))
            assertTrue(SharpProgramFolders.chosen(project).isEmpty())

            SharpProgramFolders.choose(project, shapes)
            assertEquals(listOf(shapesPath), SharpProgramFolders.chosen(project))
            assertEquals(listOf(shapesPath), SharpProgramFolders.current(project))
            assertTrue(SharpProgramFolders.isProgramFolder(project, shapes))
            assertFalse(SharpProgramFolders.isProgramFolder(project, notes))

            // what "Run" on a file or a folder means
            assertEquals(shapesPath, SharpProgramFolders.folderToRun(project, main))
            assertEquals(shapesPath, SharpProgramFolders.folderToRun(project, shapes))
            assertNull("a folder without Sharp files is nothing to run", SharpProgramFolders.folderToRun(project, notes))
            assertNull(SharpProgramFolders.folderToRun(project, readme))

            SharpProgramFolders.choose(project, null)               // back to automatic
            assertTrue(SharpProgramFolders.chosen(project).isEmpty())
            assertFalse(SharpProgramFolders.isProgramFolder(project, shapes))
            // a .sharp file in no program folder: its own folder is what runs
            assertEquals(shapesPath, SharpProgramFolders.folderToRun(project, main))
        } finally {
            base.deleteRecursively()
        }
    }

    fun testNewSharpProjectHereMakesTheFolderAProjectAndNothingIsOverwritten() {
        val base = Files.createTempDirectory("sharp-new-project").toFile()
        try {
            File(base, "my app").mkdirs()
            File(base, "has code").mkdirs()
            File(base, "has code/start.sharp").writeText("mod main;\n")
            val files = LocalFileSystem.getInstance()
            val fresh = files.refreshAndFindFileByIoFile(File(base, "my app"))!!
            val withCode = files.refreshAndFindFileByIoFile(File(base, "has code"))!!

            assertNotNull("the action is registered", ActionManager.getInstance().getAction("Sharp.NewProject"))
            val main = NewSharpProjectAction.create(fresh)!!
            assertEquals("main.sharp", main.name)
            assertTrue(File(base, "my app/src/main.sharp").readText().contains("def main(args: string[])"))
            assertTrue(File(base, "my app/build").isDirectory)
            val project = sharp.ide.project.SharpProjectFiles.read(File(base, "my app").path)!!
            assertEquals("my_app", project.name)                    // a name the compiler takes for a file
            assertEquals("my_app", project.output)
            assertEquals(listOf("build"), project.ignoredInsideProject())
            assertNull("a second time: it is a project already", NewSharpProjectAction.create(fresh))

            // a folder that has Sharp code gets the settings file only
            assertNull(NewSharpProjectAction.create(withCode))
            assertTrue(File(base, "has code/settings.json").isFile)
            assertFalse(File(base, "has code/src").exists())
            assertEquals("mod main;\n", File(base, "has code/start.sharp").readText())

            // a project folder is something to run, and a file inside it means that project
            val fresh2 = files.refreshAndFindFileByIoFile(File(base, "my app"))!!
            assertEquals(ProgramFiles.normalise(fresh2.path), SharpProgramFolders.folderToRun(this.project, fresh2))
            val inside = files.refreshAndFindFileByIoFile(File(base, "my app/src/main.sharp"))!!
            assertEquals(ProgramFiles.normalise(fresh2.path), SharpProgramFolders.folderToRun(this.project, inside))
        } finally {
            base.deleteRecursively()
        }
    }

    /** File > New > Project > Sharp: the generator is registered, fills the folder and leaves a run configuration for it, selected. */
    fun testTheNewProjectWindowsSharpEntryMakesAProjectThatCanBeRun() {
        val generator = com.intellij.openapi.extensions.ExtensionPointName.create<com.intellij.platform.DirectoryProjectGenerator<*>>("com.intellij.directoryProjectGenerator").extensionList
            .filterIsInstance<sharp.ide.project.SharpProjectGenerator>().single()
        assertEquals("Sharp", generator.name)
        assertNotNull(generator.logo)

        val base = Files.createTempDirectory("sharp-generated").toFile()
        val runs = RunManager.getInstance(project)
        try {
            File(base, "first game").mkdirs()
            val folder = LocalFileSystem.getInstance().refreshAndFindFileByIoFile(File(base, "first game"))!!
            generator.generateProject(project, folder, Any(), module)

            assertTrue(File(base, "first game/settings.json").isFile)
            assertTrue(File(base, "first game/src/main.sharp").readText().contains("def main(args: string[])"))
            assertTrue(File(base, "first game/build").isDirectory)
            val made = runs.allSettings.single { (it.configuration as? SharpRunConfiguration)?.programFolderPath() == ProgramFiles.normalise(folder.path) }
            assertEquals("first_game", made.name)
            assertSame("the new configuration is the one the Run button starts", made, runs.selectedConfiguration)

            // a second time changes nothing: no second configuration, the files as they were
            File(base, "first game/src/main.sharp").writeText("mod main;\n// mine\n")
            sharp.ide.project.SharpProjectGenerator.fill(project, folder)
            assertEquals(1, runs.allSettings.count { (it.configuration as? SharpRunConfiguration)?.programFolderPath() == ProgramFiles.normalise(folder.path) })
            assertEquals("mod main;\n// mine\n", File(base, "first game/src/main.sharp").readText())
        } finally {
            runs.allSettings.filter { it.configuration is SharpRunConfiguration }.forEach { runs.removeConfiguration(it) }
            base.deleteRecursively()
        }
    }
}
