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
}
