/*
 * THE GREEN ARROWS IN A TEST FILE, AND "RUN" ON A PROJECT'S TESTS
 *
 * In a file of a project's tests folder the gutter shows an arrow beside everything that can be run
 * on its own - one test, a group, a function that holds groups, the tests' `main` for all of them
 * (SharpTestTargets.kt finds them). A click offers Run and Debug, as the arrow beside a C++ test does.
 *
 * Two pieces of the platform make that:
 *
 *     SharpTestRunLineMarker    "does this word get an arrow?" - asked for every word the editor shows
 *     SharpTestRunProducer      "what is run for this place?" - fills in a "Sharp Program" configuration:
 *                               the project's folder, "run the project's tests", and which of them.
 *                               It also answers Run on the tests folder, or on a test file, in the
 *                               Project view: all the tests.
 *
 * The configuration is the ordinary one (sharp.ide.execution): the project is built with `-tests` and
 * the result is shown in the test view (SharpTestView.kt).
 */
package sharp.ide.testing

import com.intellij.execution.actions.ConfigurationContext
import com.intellij.execution.actions.ConfigurationFromContext
import com.intellij.execution.actions.LazyRunConfigurationProducer
import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.execution.lineMarker.ExecutorAction
import com.intellij.execution.lineMarker.RunLineMarkerContributor
import com.intellij.icons.AllIcons
import com.intellij.openapi.util.Key
import com.intellij.openapi.util.Ref
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import sharp.ide.execution.SharpRunConfiguration
import sharp.ide.execution.SharpRunConfigurationType
import sharp.ide.execution.joinFilters
import sharp.ide.lexer.SharpTokenKinds
import sharp.ide.project.SharpProjectFile
import sharp.ide.project.SharpProjectFiles

/** What a place in a test file runs: the project, and which of its tests. */
class SharpTestChoice(val project: SharpProjectFile, val target: TestTarget?) {
    val filters: List<String> get() = target?.filters ?: emptyList()

    /** The run configuration's name: `shop tests`, or what is run of them. */
    val title: String get() {
        val project = (this.project.name ?: this.project.folder.substringAfterLast('/')) + " tests"
        return when (target?.kind) {
            null, TestTargetKind.ALL -> project
            TestTargetKind.FUNCTION -> "${target.title}()"
            TestTargetKind.GROUP -> target.title
            TestTargetKind.TEST -> target.filters.first()
        }
    }
}

object SharpTestPlaces {
    private class Kept(val stamp: Long, val targets: List<TestTarget>)
    private val KEPT = Key.create<Kept>("sharp.test.targets")

    /** The targets of a file's text as it is now; found once for each change of the text. */
    fun targetsOf(file: PsiFile): List<TestTarget> {
        val stamp = file.modificationStamp
        file.getUserData(KEPT)?.let { if (it.stamp == stamp) return it.targets }
        val targets = SharpTestTargets.of(file.text)
        file.putUserData(KEPT, Kept(stamp, targets))
        return targets
    }

    /** What the word `element` runs, when it is the word of a target in a file of a project's tests. */
    fun choiceAt(element: PsiElement): SharpTestChoice? {
        if (!SharpTokenKinds.NAMES.contains(element.node?.elementType)) return null
        val file = element.containingFile ?: return null
        val path = file.virtualFile?.path ?: return null
        val project = SharpProjectFiles.projectOfTest(path) ?: return null
        val target = targetsOf(file).firstOrNull { it.offset == element.textRange.startOffset } ?: return null
        return SharpTestChoice(project, target)
    }
}

class SharpTestRunLineMarker : RunLineMarkerContributor() {
    override fun getInfo(element: PsiElement): Info? {
        val choice = SharpTestPlaces.choiceAt(element) ?: return null
        val one = choice.target?.kind == TestTargetKind.TEST
        val icon = if (one) AllIcons.RunConfigurations.TestState.Run else AllIcons.RunConfigurations.TestState.Run_run
        return Info(icon, ExecutorAction.getActions(0)) { "Run ${choice.title}" }
    }
}

class SharpTestRunProducer : LazyRunConfigurationProducer<SharpRunConfiguration>() {
    override fun getConfigurationFactory(): ConfigurationFactory = SharpRunConfigurationType.instance().factory

    /** A word with an arrow: that target. Anywhere else in a test file, or on the tests folder: all the tests. */
    private fun choiceFor(context: ConfigurationContext): SharpTestChoice? {
        val element = context.psiLocation
        if (element != null) SharpTestPlaces.choiceAt(element)?.let { return it }
        val file = context.location?.virtualFile ?: return null
        val project = SharpProjectFiles.projectOfTest(file.path) ?: return null
        return SharpTestChoice(project, null)
    }

    override fun setupConfigurationFromContext(configuration: SharpRunConfiguration, context: ConfigurationContext, sourceElement: Ref<PsiElement>): Boolean {
        val choice = choiceFor(context) ?: return false
        configuration.fields.programFolder = choice.project.folder
        configuration.fields.projectTests = true
        configuration.fields.testFilter = joinFilters(choice.filters)
        configuration.name = choice.title
        return true
    }

    override fun isConfigurationFromContext(configuration: SharpRunConfiguration, context: ConfigurationContext): Boolean {
        val choice = choiceFor(context) ?: return false
        return configuration.fields.projectTests &&
            configuration.programFolderPath() == choice.project.folder &&
            configuration.testFilters() == choice.filters
    }

    /** In a test file this is the answer, whatever else offers to run the file. */
    override fun isPreferredConfiguration(self: ConfigurationFromContext, other: ConfigurationFromContext): Boolean = true
}
