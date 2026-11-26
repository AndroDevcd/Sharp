/*
 * THE TEST VIEW, inside the IDE's test environment: the lines a Sharp test program prints for it
 * become the tree - the groups at one level, their tests inside, passed and failed - and a group or
 * a test leads to the line it is written on.
 */
package sharp.ide.testing

import com.intellij.execution.RunManager
import com.intellij.execution.executors.DefaultRunExecutor
import com.intellij.execution.process.NopProcessHandler
import com.intellij.execution.process.ProcessOutputTypes
import com.intellij.execution.testframework.sm.runner.SMTestProxy
import com.intellij.execution.testframework.sm.runner.ui.SMTRunnerConsoleView
import com.intellij.openapi.util.Disposer
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.psi.search.GlobalSearchScope
import com.intellij.testFramework.PlatformTestUtil
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import sharp.ide.execution.SharpRunConfigurationType
import java.io.File
import java.nio.file.Files

class SharpTestViewInTheIdeTest : BasePlatformTestCase() {

    fun testTheLinesOfATestProgramBecomeGroupsWithTheirTestsAndEachLeadsToItsLine() {
        val base = Files.createTempDirectory("sharp-test-view").toFile()
        try {
            val source = File(base, "tests.sharp")
            source.writeText("mod main;\n\ndef main() {\n    tests(\"a basket\") {\n        test(\"it is empty\") { };\n        test(\"adding\") { };\n    };\n}\n")
            val file = LocalFileSystem.getInstance().refreshAndFindFileByIoFile(source)!!
            val path = file.path

            val settings = RunManager.getInstance(project).createConfiguration("checks", SharpRunConfigurationType.instance().factory)
            val handler = NopProcessHandler()
            val console = SharpTestView.consoleFor(settings.configuration, DefaultRunExecutor.getRunExecutorInstance(), handler) as SMTRunnerConsoleView
            Disposer.register(testRootDisposable, console)
            handler.startNotify()
            for (line in listOf(
                "##teamcity[testSuiteStarted name='a basket' locationHint='sharp://$path:4']",
                "##teamcity[testStarted name='it is empty' locationHint='sharp://$path:5']",
                "##teamcity[testFinished name='it is empty' duration='1']",
                "##teamcity[testStarted name='adding' locationHint='sharp://$path:6']",
                "##teamcity[testFailed name='adding' message='expected 1' details='expected 1|nbut was  2' type='comparisonFailure' expected='1' actual='2']",
                "##teamcity[testFinished name='adding' duration='2']",
                "##teamcity[testSuiteFinished name='a basket']",
                "##teamcity[testSuiteStarted name='second group']",
                "##teamcity[testStarted name='fine']",
                "##teamcity[testFinished name='fine' duration='0']",
                "##teamcity[testSuiteFinished name='second group']",
            )) handler.notifyTextAvailable(line + "\n", ProcessOutputTypes.STDOUT)
            handler.destroyProcess()

            val root = console.resultsViewer.testsRootNode
            PlatformTestUtil.waitWithEventsDispatching("the two groups", { root.children.size == 2 && !root.isInProgress }, 20)

            // the groups at one level, each with its mark; the tests inside
            val groups = root.children
            assertEquals(listOf("a basket", "second group"), groups.map { it.name })
            assertTrue("a group with a failed test is failed", groups[0].isDefect)
            assertFalse(groups[1].isDefect)
            assertTrue(groups[1].isPassed)
            assertEquals(listOf("it is empty", "adding"), groups[0].children.map { it.name })
            assertTrue(groups[0].children[0].isPassed)
            assertTrue(groups[0].children[1].isDefect)
            assertNotNull("the two values of a failed comparison, for the diff", groups[0].children[1].diffViewerProvider)

            // a group and a test lead to the line they are written on
            fun lineOf(proxy: SMTestProxy): Int {
                val location = proxy.getLocation(project, GlobalSearchScope.allScope(project))!!
                assertEquals(file, location.virtualFile)
                val document = com.intellij.openapi.fileEditor.FileDocumentManager.getInstance().getDocument(file)!!
                return document.getLineNumber(location.psiElement.textOffset) + 1
            }
            assertEquals(4, lineOf(groups[0]))
            assertEquals(5, lineOf(groups[0].children[0]))
            assertEquals(6, lineOf(groups[0].children[1]))
            assertNull("no place said: nothing to jump to", groups[1].getLocation(project, GlobalSearchScope.allScope(project)))
        } finally {
            base.deleteRecursively()
        }
    }

    fun testATestProgramsConfigurationKeepsItsMark() {
        val settings = RunManager.getInstance(project).createConfiguration("checks", SharpRunConfigurationType.instance().factory)
        val configuration = settings.configuration as sharp.ide.execution.SharpRunConfiguration
        assertFalse(configuration.fields.testProgram)
        configuration.fields.testProgram = true
        val copy = settings.createFactory().create().configuration as sharp.ide.execution.SharpRunConfiguration
        assertTrue(copy.fields.testProgram)
    }

    /**
     * The folder opened in the IDE HOLDS the Sharp project (workspace/shop/settings.json): the check
     * after a save still takes it for a project - its tests are compiled with the program, and the
     * compiler is told where they are. (They were left out: `tests` is a folder the plugin skips in
     * a folder that is no project.)
     */
    fun testAProjectBelowTheOpenedFolderHasItsTestsChecked() {
        val base = Files.createTempDirectory("sharp-nested-project").toFile()
        val settings = project.getService(sharp.ide.settings.SharpProjectSettings::class.java)
        val before = settings.state.sourceRoots.toMutableList()
        try {
            File(base, "shop").mkdirs()
            val folder = LocalFileSystem.getInstance().refreshAndFindFileByIoFile(File(base, "shop"))!!
            sharp.ide.execution.NewSharpProjectAction.create(folder)
            val workspace = sharp.ide.paths.ProgramFiles.normalise(base.path)
            settings.state.sourceRoots = mutableListOf(workspace)

            val files = sharp.ide.paths.ProgramFiles.list(settings.programSettings())
            assertTrue(files.toString(), "$workspace/shop/src/main.sharp" in files)
            assertTrue("the tests are checked too", "$workspace/shop/tests/main_tests.sharp" in files)
            assertEquals("$workspace/shop/tests", settings.testsFolderOfProgram())

            // a folder called tests that belongs to no project stays out
            File(base, "tests").mkdirs()
            File(base, "tests/loose.sharp").writeText("mod loose;\n")
            File(base, "shop/settings.json").delete()
            assertFalse("$workspace/tests/loose.sharp" in sharp.ide.paths.ProgramFiles.list(settings.programSettings()))
            assertNull(settings.testsFolderOfProgram())
        } finally {
            settings.state.sourceRoots = before
            base.deleteRecursively()
        }
    }

    /**
     * A project with a tests folder: the words of its test file get an arrow, each runs its own part of
     * the project's tests; a file of the program gets none; a new project comes with its first test.
     */
    fun testTheArrowsOfATestFileRunTheirPartOfTheProjectsTests() {
        val base = Files.createTempDirectory("sharp-test-arrows").toFile()
        try {
            File(base, "shop").mkdirs()
            val folder = LocalFileSystem.getInstance().refreshAndFindFileByIoFile(File(base, "shop"))!!
            val main = sharp.ide.execution.NewSharpProjectAction.create(folder)!!
            val tests = folder.findChild("tests")!!.findChild("main_tests.sharp")!!
            val shop = sharp.ide.paths.ProgramFiles.normalise(folder.path)

            val psi = com.intellij.psi.PsiManager.getInstance(project).findFile(tests)!!
            val text = psi.text
            fun choice(word: String) = SharpTestPlaces.choiceAt(psi.findElementAt(text.indexOf(word))!!)

            val one = choice("test(\"the program greets\")")!!
            assertEquals(shop, one.project.folder)
            assertEquals(listOf("hello world > the program greets"), one.filters)
            assertEquals("hello world > the program greets", one.title)
            assertEquals(listOf("hello world"), choice("tests(\"hello world\")")!!.filters)
            val all = SharpTestPlaces.choiceAt(psi.findElementAt(text.indexOf("def main()") + 4)!!)!!
            assertEquals(emptyList<String>(), all.filters)
            assertEquals("shop tests", all.title)
            assertNull("a word that runs nothing", choice("expect("))
            assertNotNull("the arrow itself", SharpTestRunLineMarker().getInfo(psi.findElementAt(text.indexOf("test(\"the program greets\")"))!!))

            // the program's own file is not a test: no arrow, though it has a main
            val program = com.intellij.psi.PsiManager.getInstance(project).findFile(main)!!
            assertNull(SharpTestPlaces.choiceAt(program.findElementAt(program.text.indexOf("def main(") + 4)!!))
            assertNull(sharp.ide.project.SharpProjectFiles.projectOfTest(main.path))
            assertEquals(shop, sharp.ide.project.SharpProjectFiles.projectOfTest(tests.path)!!.folder)
            assertEquals("the tests folder itself", shop, sharp.ide.project.SharpProjectFiles.projectOfTest(folder.findChild("tests")!!.path)!!.folder)
        } finally {
            base.deleteRecursively()
        }
    }
}
