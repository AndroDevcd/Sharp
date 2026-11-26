/*
 * "SHARP" IN THE IDE'S NEW PROJECT WINDOW
 *
 * File > New > Project lists the kinds of project the IDE can start. This adds "Sharp": choose a
 * folder, press Create, and the folder is a Sharp project, opened and ready to run:
 *
 *     <folder>/settings.json      the project's file: its name and output after the folder, `build` left out
 *     <folder>/src/main.sharp     a first program, opened in the editor
 *     <folder>/build/             where the compiler writes the program
 *
 * and a "Sharp Program" run configuration for the folder, selected - so the Run and Debug buttons
 * work at once.
 *
 * It is the same project "New Sharp Project Here" makes in a folder of an open project
 * (NewSharpProjectAction: the files are made there), so nothing that is in the folder already is
 * overwritten: a folder that holds Sharp code keeps it and gets the settings file.
 *
 * The platform's name for this is a DIRECTORY PROJECT GENERATOR: the IDE makes and opens the folder,
 * then hands it here to be filled. CLion's New Project window shows every generator; IntelliJ IDEA's
 * shows them under "More" in some versions and not at all in others (there the right-click action
 * does the same in an empty project).
 */
package sharp.ide.project

import com.intellij.execution.RunManager
import com.intellij.facet.ui.ValidationResult
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.fileEditor.FileEditorManager
import com.intellij.openapi.module.Module
import com.intellij.openapi.project.Project
import com.intellij.openapi.startup.StartupManager
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.platform.DirectoryProjectGeneratorBase
import sharp.ide.execution.NewSharpProjectAction
import sharp.ide.execution.SharpRunConfiguration
import sharp.ide.execution.SharpRunConfigurationType
import sharp.ide.language.SharpIcons
import sharp.ide.paths.ProgramFiles
import javax.swing.Icon

class SharpProjectGenerator : DirectoryProjectGeneratorBase<Any>() {
    override fun getName(): String = "Sharp"
    override fun getDescription(): String = "A Sharp program: settings.json, src/main.sharp and a run configuration"
    override fun getLogo(): Icon = SharpIcons.FILE

    /** Any folder will do: what is in it already is kept. */
    override fun validate(baseDirPath: String): ValidationResult = ValidationResult.OK

    override fun generateProject(project: Project, baseDir: VirtualFile, settings: Any, module: Module) {
        val main = fill(project, baseDir)
        if (main != null) {
            // the editor can take a file once the project's window is there
            StartupManager.getInstance(project).runAfterOpened {
                ApplicationManager.getApplication().invokeLater({
                    if (main.isValid) FileEditorManager.getInstance(project).openFile(main, true)
                }, project.disposed)
            }
        }
    }

    companion object {
        /**
         * Makes `folder` a Sharp project and gives `project` a run configuration for it, selected.
         * Returns the new main.sharp, or null when the folder already held Sharp code.
         */
        fun fill(project: Project, folder: VirtualFile): VirtualFile? {
            val main = NewSharpProjectAction.create(folder)
            val path = ProgramFiles.normalise(folder.path)
            val runs = RunManager.getInstance(project)
            val already = runs.allSettings.any { (it.configuration as? SharpRunConfiguration)?.programFolderPath() == path }
            if (!already) {
                val name = SharpProjectFiles.read(path)?.name ?: folder.name
                val made = runs.createConfiguration(name, SharpRunConfigurationType.instance().factory)
                (made.configuration as SharpRunConfiguration).fields.programFolder = path
                runs.addConfiguration(made)
                runs.selectedConfiguration = made
            }
            return main
        }
    }
}
