/*
 * A TEST PROGRAM'S RUN, SHOWN AS A TREE OF TESTS
 *
 * A Sharp test program is an ordinary program that uses std.testing: groups (`tests("a basket") { ... }`)
 * with tests in them (`test("a new basket is empty") { ... }`). Run plainly it prints lines for a
 * person. Started with the argument `-ide-tests` it reports for a program instead - one line as each
 * group and each test starts and ends, in the form the platform's test view reads:
 *
 *     ##teamcity[testSuiteStarted name='a basket' locationHint='sharp://C:/app/tests.sharp:6']
 *     ##teamcity[testStarted name='a new basket is empty' locationHint='sharp://C:/app/tests.sharp:7']
 *     ##teamcity[testFailed name='...' message='expected 1' details='...' type='comparisonFailure' expected='1' actual='2']
 *     ##teamcity[testFinished name='a new basket is empty' duration='3']
 *     ##teamcity[testSuiteFinished name='a basket']
 *
 * (stdlib/std/testing/testing.sharp writes them.) A "Sharp Program" run configuration that is
 * marked as a test program is started that way (SharpRunState), and its console is the platform's
 * test view, made here: the groups at one level, each with a mark for passed or failed, their tests
 * inside; a failed test shows its message, and a failed comparison the two values side by side.
 *
 * WHERE A TEST IS WRITTEN. Each line carries `locationHint='sharp://<file>:<line>'`. The platform
 * hands such a hint back to SharpTestLocator when a group or a test is double-clicked (or F4), and
 * the locator turns it into a place in the file. The line of the check that failed is in the
 * failure's text ("at <file>, line <n>"), which the console shows as a link (SharpConsoleFilter).
 */
package sharp.ide.testing

import com.intellij.execution.Executor
import com.intellij.execution.Location
import com.intellij.execution.PsiLocation
import com.intellij.execution.configurations.RunConfiguration
import com.intellij.execution.process.ProcessHandler
import com.intellij.execution.testframework.sm.SMTestRunnerConnectionUtil
import com.intellij.execution.testframework.sm.runner.SMTRunnerConsoleProperties
import com.intellij.execution.testframework.sm.runner.SMTestLocator
import com.intellij.execution.ui.ConsoleView
import com.intellij.openapi.fileEditor.FileDocumentManager
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.psi.PsiManager
import com.intellij.psi.search.GlobalSearchScope

object SharpTestView {
    /** The argument a test program is started with to report for the test view. */
    const val ARGUMENT = "-ide-tests"

    /** Before each entry of what to run: a group, or `group > test` (std.testing's `-test-only`). */
    const val ONLY_ARGUMENT = "-test-only"

    /** The test view for this run, reading what `handler`'s process prints. */
    fun consoleFor(configuration: RunConfiguration, executor: Executor, handler: ProcessHandler): ConsoleView =
        SMTestRunnerConnectionUtil.createAndAttachConsole("Sharp", handler, SharpTestConsoleProperties(configuration, executor))
}

class SharpTestConsoleProperties(configuration: RunConfiguration, executor: Executor) :
    SMTRunnerConsoleProperties(configuration, "Sharp", executor) {
    override fun getTestLocator(): SMTestLocator = SharpTestLocator
}

/** A place a location hint names: the file, and its line counted from 1. */
data class TestPlace(val file: String, val line: Int)

object SharpTestLocator : SMTestLocator {
    const val PROTOCOL = "sharp"

    /**
     * `<file>:<line>` as the hint holds it after `sharp://`. The file may hold colons itself (C:/...):
     * the number after the LAST colon is the line. Null when it is not of that form.
     */
    fun placeOf(path: String): TestPlace? {
        val colon = path.lastIndexOf(':')
        if (colon <= 0) return null
        val line = path.substring(colon + 1).toIntOrNull() ?: return null
        if (line < 1) return null
        return TestPlace(path.substring(0, colon).replace('\\', '/'), line)
    }

    override fun getLocation(protocol: String, path: String, project: Project, scope: GlobalSearchScope): List<Location<*>> {
        if (protocol != PROTOCOL) return emptyList()
        val place = placeOf(path) ?: return emptyList()
        val file = LocalFileSystem.getInstance().findFileByPath(place.file) ?: return emptyList()
        val psiFile = PsiManager.getInstance(project).findFile(file) ?: return emptyList()
        val document = FileDocumentManager.getInstance().getDocument(file) ?: return emptyList()
        if (place.line > document.lineCount) return listOf(PsiLocation(project, psiFile))
        // the first thing written on that line: the word `tests` or `test`
        val text = document.charsSequence
        var offset = document.getLineStartOffset(place.line - 1)
        val end = document.getLineEndOffset(place.line - 1)
        while (offset < end && text[offset].isWhitespace()) offset++
        val element = psiFile.findElementAt(offset) ?: return listOf(PsiLocation(project, psiFile))
        return listOf(PsiLocation(project, element))
    }
}
