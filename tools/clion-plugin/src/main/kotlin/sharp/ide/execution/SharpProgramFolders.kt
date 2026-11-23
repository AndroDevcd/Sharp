/*
 * THE PROGRAM FOLDER: which folder of the project is "the program", and how it is shown and chosen
 *
 * A project folder often holds more than one thing: a program, its tests, tools, notes. The plugin
 * works on a PROGRAM FOLDER - the folder whose .sharp files (with the standard library) are one
 * program. It is what the checks after a save compile, and what a new run configuration runs.
 *
 *     chosen by hand     right-click a folder in the Project view > "Set as Sharp Program Folder"
 *                        (the same menu takes the mark off again); or Settings > Sharp > Program folder
 *     not chosen         automatic: the project folder (in the Sharp repository: lib/compiler)
 *
 * A folder with a Sharp settings.json is a Sharp PROJECT (sharp.ide.project): the compiler's project
 * mode builds it. "New Sharp Project Here" in the same right-click menu makes one: settings.json,
 * src/main.sharp and build/.
 *
 * In the Project view these folders carry a module icon and a few words, the way Android Studio
 * marks its modules, so it is plain which code the IDE treats as what:
 *
 *     shapes   Sharp program · settings.json        the program folder, and a project
 *     shapes   Sharp program                        the program folder (chosen by hand)
 *     shapes   Sharp program (automatic)            the program folder nobody chose
 *     tools    Sharp project                        another project in the same tree: it can be run,
 *                                                   but it is not what the editor checks
 *
 *     SharpProgramFolders               the questions: is this folder the program; which program is this file in
 *     SetSharpProgramFolderAction       the right-click action
 *     NewSharpProjectAction             "New Sharp Project Here"
 *     SharpProgramFolderDecorator       the icon and the words in the Project view
 *     SharpRunConfigurationProducer     "Run 'name'" on a folder or a .sharp file
 */
package sharp.ide.execution

import com.intellij.execution.actions.ConfigurationContext
import com.intellij.execution.actions.LazyRunConfigurationProducer
import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.icons.AllIcons
import com.intellij.ide.projectView.PresentationData
import com.intellij.ide.projectView.ProjectView
import com.intellij.ide.projectView.ProjectViewNode
import com.intellij.ide.projectView.ProjectViewNodeDecorator
import com.intellij.openapi.actionSystem.ActionUpdateThread
import com.intellij.openapi.actionSystem.AnAction
import com.intellij.openapi.actionSystem.AnActionEvent
import com.intellij.openapi.actionSystem.CommonDataKeys
import com.intellij.openapi.application.runWriteAction
import com.intellij.openapi.components.service
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.openapi.project.Project
import com.intellij.openapi.util.Key
import com.intellij.openapi.util.Ref
import com.intellij.openapi.vfs.VfsUtil
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.psi.PsiElement
import sharp.ide.library.SharpLibraryRootsProvider
import sharp.ide.paths.ProgramFiles
import sharp.ide.project.SharpProjectFiles
import sharp.ide.run.AnalysisService
import sharp.ide.run.RunReason
import sharp.ide.settings.SharpProjectSettings

object SharpProgramFolders {
    // The Project view asks for every folder it draws; the answer is worked out from the settings and
    // the disk (is there a settings.json, an installed library ...), so it is kept for a moment.
    private class Kept(val at: Long, val folders: List<String>)
    private val KEPT = Key.create<Kept>("sharp.program.folders")
    private const val KEEP_MILLIS = 1_500L

    /** The program folders in force (chosen, or the automatic one), absolute, forward slashes. */
    fun current(project: Project): List<String> {
        val now = System.currentTimeMillis()
        project.getUserData(KEPT)?.let { if (now - it.at < KEEP_MILLIS) return it.folders }
        val base = ProgramFiles.normalise(project.basePath ?: "")
        val chosen = chosen(project)
        // (the libraries a settings.json adds are part of the program's files, but they are not "the program folder")
        val folders = if (chosen.isNotEmpty()) chosen else {
            val settings = project.service<SharpProjectSettings>()
            ProgramFiles.layoutOf(settings.programSettings().copy(sourceRoots = emptyList())).sourceRoots.ifEmpty { listOfNotNull(base.takeIf { it.isNotEmpty() }) }
        }
        project.putUserData(KEPT, Kept(now, folders))
        return folders
    }

    /** The folders the settings name by hand, made absolute; empty when the choice is automatic. */
    fun chosen(project: Project): List<String> {
        val base = ProgramFiles.normalise(project.basePath ?: "")
        return project.service<SharpProjectSettings>().state.sourceRoots
            .map { ProgramFiles.normalise(it) }
            .filter { it.isNotEmpty() }
            .map { if (it.startsWith("/") || (it.length >= 2 && it[1] == ':') || base.isEmpty()) it else "$base/$it" }
    }

    fun isProgramFolder(project: Project, folder: VirtualFile): Boolean =
        folder.isDirectory && ProgramFiles.normalise(folder.path) in current(project)

    /** Makes `folder` THE program folder of the project (null: back to automatic), and checks the program again. */
    fun choose(project: Project, folder: VirtualFile?) {
        val state = project.service<SharpProjectSettings>().state
        val libraryBefore = SharpLibraryRootsProvider.externalLibraryRoot(project)
        state.sourceRoots = if (folder == null) mutableListOf() else mutableListOf(relativeToProject(project, folder.path))
        project.putUserData(KEPT, null)
        SharpLibraryRootsProvider.libraryRootsChanged(project, libraryBefore, SharpLibraryRootsProvider.externalLibraryRoot(project))
        ProjectView.getInstance(project).refresh()
        AnalysisService.getInstance(project).requestRun(RunReason.SETTINGS)
    }

    /**
     * The folder a "Run" on `file` means: a Sharp project, the program folder, or a folder that holds
     * .sharp files itself is that folder; a .sharp file is the nearest project or program folder it
     * lies in, or its own folder when it is in none. Null: nothing to run here.
     */
    fun folderToRun(project: Project, file: VirtualFile): String? {
        val path = ProgramFiles.normalise(file.path)
        if (file.isDirectory) {
            if (path in current(project) || SharpProjectFiles.isProject(path)) return path
            return if (file.children.any { !it.isDirectory && it.name.endsWith(".sharp") }) path else null
        }
        if (!file.name.endsWith(".sharp")) return null
        // the nearest folder above the file that is a project (a settings.json) or a program folder
        val programFolders = current(project)
        var folder = file.parent
        while (folder != null) {
            val here = ProgramFiles.normalise(folder.path)
            if (here in programFolders || folder.findChild(SharpProjectFiles.FILE_NAME) != null && SharpProjectFiles.isProject(here)) return here
            folder = folder.parent
        }
        return file.parent?.let { ProgramFiles.normalise(it.path) }
    }

    private fun relativeToProject(project: Project, path: String): String {
        val base = ProgramFiles.normalise(project.basePath ?: "")
        val clean = ProgramFiles.normalise(path)
        return when {
            base.isNotEmpty() && clean == base -> base
            base.isNotEmpty() && clean.startsWith("$base/") -> clean.substring(base.length + 1)
            else -> clean
        }
    }
}

/** Project view > right-click a folder > "Set as Sharp Program Folder" / "Unset Sharp Program Folder". */
class SetSharpProgramFolderAction : AnAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(event: AnActionEvent) {
        val project = event.project
        val folder = event.getData(CommonDataKeys.VIRTUAL_FILE)
        val shown = project != null && folder != null && folder.isDirectory && folder.isInLocalFileSystem
        event.presentation.isEnabledAndVisible = shown
        if (!shown) return
        val chosenNow = ProgramFiles.normalise(folder!!.path) in SharpProgramFolders.chosen(project!!)
        event.presentation.text = if (chosenNow) "Unset Sharp Program Folder" else "Set as Sharp Program Folder"
        event.presentation.description =
            if (chosenNow) "Back to automatic: the project folder is the program"
            else "The .sharp files of this folder are the program the IDE checks and runs"
    }

    override fun actionPerformed(event: AnActionEvent) {
        val project = event.project ?: return
        val folder = event.getData(CommonDataKeys.VIRTUAL_FILE) ?: return
        if (!folder.isDirectory) return
        val chosenNow = ProgramFiles.normalise(folder.path) in SharpProgramFolders.chosen(project)
        SharpProgramFolders.choose(project, if (chosenNow) null else folder)
    }
}

/**
 * Project view > right-click a folder > "New Sharp Project Here": the folder gets a settings.json
 * (name and output after the folder, `build` ignored), `src/main.sharp` and a first test of it
 * (`tests/main_tests.sharp`) when it holds no Sharp code yet, and `build/`; it becomes the program folder, and main.sharp is opened. Nothing that is there
 * already is overwritten; the entry is not offered for a folder that is a Sharp project.
 */
class NewSharpProjectAction : AnAction() {
    override fun getActionUpdateThread(): ActionUpdateThread = ActionUpdateThread.BGT

    override fun update(event: AnActionEvent) {
        val folder = event.getData(CommonDataKeys.VIRTUAL_FILE)
        event.presentation.isEnabledAndVisible = event.project != null && folder != null && folder.isDirectory &&
            folder.isInLocalFileSystem && folder.findChild(SharpProjectFiles.FILE_NAME) == null
    }

    override fun actionPerformed(event: AnActionEvent) {
        val project = event.project ?: return
        val folder = event.getData(CommonDataKeys.VIRTUAL_FILE) ?: return
        val main = create(folder) ?: return
        SharpProgramFolders.choose(project, folder)
        FileEditorManager.getInstance(project).openFile(main, true)
    }

    companion object {
        /** Makes `folder` a Sharp project; returns the program's first file (the new main.sharp, or null when the folder had Sharp code). */
        fun create(folder: VirtualFile): VirtualFile? {
            if (!folder.isDirectory || folder.findChild(SharpProjectFiles.FILE_NAME) != null) return null
            // a name the compiler takes as an output file name: letters, digits, _ and -
            val name = folder.name.map { if (it.isLetterOrDigit() || it == '_' || it == '-') it else '_' }.joinToString("").ifEmpty { "program" }
            var main: VirtualFile? = null
            runWriteAction {
                val hasCode = folder.children.any { !it.isDirectory && it.name.endsWith(".sharp") } ||
                    (folder.findChild("src")?.children?.any { it.name.endsWith(".sharp") } ?: false)
                VfsUtil.saveText(folder.createChildData(this, SharpProjectFiles.FILE_NAME), SharpProjectFiles.newProjectText(name))
                if (folder.findChild("build") == null) folder.createChildDirectory(this, "build")
                if (!hasCode) {
                    val source = folder.findChild("src") ?: folder.createChildDirectory(this, "src")
                    main = source.createChildData(this, "main.sharp")
                    VfsUtil.saveText(main!!, SharpProjectFiles.newMainText())
                    // ... and its first test, which the gutter's arrow beside it runs
                    if (folder.findChild("tests") == null) {
                        val tests = folder.createChildDirectory(this, "tests")
                        VfsUtil.saveText(tests.createChildData(this, "main_tests.sharp"), SharpProjectFiles.newTestsText())
                    }
                }
            }
            return main
        }
    }
}

/** A program folder or a Sharp project in the Project view: a module icon and a few words beside its name. */
class SharpProgramFolderDecorator : ProjectViewNodeDecorator {
    override fun decorate(node: ProjectViewNode<*>, data: PresentationData) {
        val project = node.project ?: return
        val folder = node.virtualFile ?: return
        if (!folder.isDirectory) return
        val path = ProgramFiles.normalise(folder.path)
        // (findChild looks in the IDE's own picture of the folder: no disk is touched for a folder without the file)
        val isProject = folder.findChild(SharpProjectFiles.FILE_NAME) != null && SharpProjectFiles.isProject(path)
        val chosen = path in SharpProgramFolders.chosen(project)
        val automatic = !chosen && path in SharpProgramFolders.current(project) &&
            (isProject || AnalysisService.getInstance(project).hasSharpProgram())
        val words = when {
            (chosen || automatic) && isProject -> "Sharp program · settings.json"
            chosen -> "Sharp program"
            automatic -> "Sharp program (automatic)"
            isProject -> "Sharp project"
            else -> return
        }
        data.setIcon(AllIcons.Nodes.Module)
        data.locationString = words
    }
}

/** "Run 'name'" in the right-click menu of a folder with Sharp files, or of a .sharp file. */
class SharpRunConfigurationProducer : LazyRunConfigurationProducer<SharpRunConfiguration>() {
    override fun getConfigurationFactory(): ConfigurationFactory = SharpRunConfigurationType.instance().factory

    override fun setupConfigurationFromContext(configuration: SharpRunConfiguration, context: ConfigurationContext, sourceElement: Ref<PsiElement>): Boolean {
        val file = context.location?.virtualFile ?: return false
        // (a project's tests are run as tests: sharp.ide.testing.SharpTestRunProducer)
        if (SharpProjectFiles.projectOfTest(file.path) != null) return false
        val folder = SharpProgramFolders.folderToRun(context.project, file) ?: return false
        configuration.fields.programFolder = folder
        configuration.name = SharpProjectFiles.read(folder)?.name ?: folder.substringAfterLast('/').ifEmpty { "Sharp program" }
        return true
    }

    override fun isConfigurationFromContext(configuration: SharpRunConfiguration, context: ConfigurationContext): Boolean {
        val file = context.location?.virtualFile ?: return false
        if (configuration.fields.projectTests || SharpProjectFiles.projectOfTest(file.path) != null) return false
        val folder = SharpProgramFolders.folderToRun(context.project, file) ?: return false
        return configuration.programFolderPath() == folder
    }
}
