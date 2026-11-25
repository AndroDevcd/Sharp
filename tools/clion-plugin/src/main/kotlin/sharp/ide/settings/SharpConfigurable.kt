/*
 * SharpConfigurable - the settings page (Settings > Languages & Frameworks > Sharp).
 *
 * WHAT THIS IS FOR
 *   One page that edits both kinds of settings: the per-project ones (is checking on, which
 *   folders are the program, where the library is) and the per-machine ones (how the compiler is
 *   started). The page is described with the platform's "Kotlin UI DSL": `panel { row { ... } }`,
 *   where every field is bound to a property, so reading, writing, "is it modified?" and "Reset"
 *   need no code of ours. A new run is requested when the page is applied, so a changed
 *   path shows its effect without a save.
 *
 *   Lists (source roots, excluded folders) are text areas with one entry per line.
 *   The launcher and the script are under "Advanced": normally they are left alone.
 */
package sharp.ide.settings

import com.intellij.openapi.options.BoundSearchableConfigurable
import com.intellij.openapi.project.Project
import com.intellij.openapi.ui.DialogPanel
import com.intellij.ui.dsl.builder.COLUMNS_LARGE
import com.intellij.ui.dsl.builder.COLUMNS_MEDIUM
import com.intellij.ui.dsl.builder.bindIntText
import com.intellij.ui.dsl.builder.bindItem
import com.intellij.ui.dsl.builder.bindSelected
import com.intellij.ui.dsl.builder.bindText
import com.intellij.ui.dsl.builder.columns
import com.intellij.ui.dsl.builder.rows
import com.intellij.ui.dsl.builder.panel
import sharp.ide.library.SharpLibraryRootsProvider
import sharp.ide.run.AnalysisService
import sharp.ide.run.RunReason

class SharpConfigurable(private val project: Project) :
    BoundSearchableConfigurable("Sharp", "sharp.settings", "sharp.settings") {

    private val app = SharpApplicationSettings.getInstance()
    private val projectSettings = project.getService(SharpProjectSettings::class.java)

    override fun createPanel(): DialogPanel {
        val appState = app.state
        val projectState = projectSettings.state
        return panel {
            group("This project") {
                row {
                    checkBox("Check the program with the compiler when a file is saved")
                        .bindSelected(projectState::enabled)
                }
                row("Program folder:") {
                    textArea()
                        .rows(3).columns(COLUMNS_LARGE)
                        .comment("The folder whose .sharp files are the program (one per line for more than one; absolute or relative to the project folder). " +
                            "Also set by right-clicking a folder in the Project view: Set as Sharp Program Folder. " +
                            "Empty: automatic (the project folder, or compiler in the Sharp repository).")
                        .bindText(
                            { projectState.sourceRoots.joinToString("\n") },
                            { projectState.sourceRoots = splitLines(it) },
                        )
                }
                row("Excluded folders:") {
                    textArea()
                        .rows(3).columns(COLUMNS_LARGE)
                        .comment("A folder name (tests) or a path fragment (lib/old), one per line, looked for below the source folders.")
                        .bindText(
                            { projectState.excludedFolders.joinToString("\n") },
                            { projectState.excludedFolders = splitLines(it) },
                        )
                }
                row("Standard library:") {
                    textField()
                        .columns(COLUMNS_LARGE)
                        .comment("The folder of the standard library. Empty: the one installed with Sharp (in the Sharp repository: stdlib of the project).")
                        .bindText(projectState::libraryPath)
                }
                row {
                    checkBox("Analyse every file for hover and go-to, not only the open ones")
                        .comment("On: moving between files, the standard library included, never waits for another check. " +
                            "Off: each check is about a quarter shorter, but opening a file starts one more check.")
                        .bindSelected(projectState::bodiesForEveryFile)
                }
                row("Extra compiler arguments:") {
                    textField()
                        .columns(COLUMNS_LARGE)
                        .comment("Added to the compiler's command line, separated by spaces.")
                        .bindText(projectState::extraArguments)
                }
            }
            group("Sharp on this machine") {
                row {
                    checkBox("Use the installed Sharp")
                        .comment(installedComment(appState.sharpHome) +
                            " When it is not installed (or this is off), the launcher and script under Advanced are used instead.")
                        .bindSelected(appState::useInstalledSharp)
                }
                row("Sharp folder:") {
                    textField().columns(COLUMNS_LARGE)
                        .comment("Where Sharp is installed. Empty: the usual place (" + sharp.ide.toolchain.SharpToolchains.defaultHome() + ").")
                        .bindText(appState::sharpHome)
                }
                row("C++ compiler:") {
                    textField().columns(COLUMNS_LARGE)
                        .comment("For projects with native functions (\"native\" in settings.json): the g++ or clang++ that builds their C++ - " +
                            "a path, or a name on the PATH. A project's own settings.json wins over this. " +
                            "Empty: the compiler looks on the PATH and where MinGW is usually installed (CLion's own is found).")
                        .bindText(appState::cxxCompiler)
                }
                row("Time limit (seconds):") {
                    textField().columns(COLUMNS_MEDIUM).bindIntText(appState::timeoutSeconds)
                }
                row("Save after a pause in typing (milliseconds):") {
                    textField().columns(COLUMNS_MEDIUM)
                        .comment("An edited Sharp file is saved this long after the last key press, which starts a check. 0: only Ctrl+S and the IDE's own saves do.")
                        .bindIntText(appState::typingPauseMillis)
                }
                row("Wait after a save (milliseconds):") {
                    textField().columns(COLUMNS_MEDIUM)
                        .comment("Added to the pause above: several saves close together become one check.")
                        .bindIntText(appState::debounceMillis)
                }
            }
            collapsibleGroup("Advanced") {
                row("Sharp VM:") {
                    textField().columns(COLUMNS_LARGE)
                        .comment("For the script below ({vm}); not used with the installed Sharp.")
                        .bindText(appState::vm)
                }
                row("Compiler:") {
                    textField().columns(COLUMNS_LARGE)
                        .comment("For the script below ({compiler}); not used with the installed Sharp.")
                        .bindText(appState::compiler)
                }
                row("Launcher:") {
                    textArea().rows(4).columns(COLUMNS_LARGE)
                        .comment("One argument per line; {script} is the path of the script. Default on Windows starts bash inside WSL.")
                        .bindText(appState::launcher)
                }
                row("Script:") {
                    textArea().rows(4).columns(COLUMNS_LARGE)
                        .comment("Placeholders: {vm} {compiler} {files_list} {ide_dir} {extra} {timeout}.")
                        .bindText(appState::script)
                }
                row("Translate paths to WSL:") {
                    comboBox(TranslateMode.entries.map { it.name })
                        .bindItem({ appState.translateMode }, { appState.translateMode = it ?: TranslateMode.AUTOMATIC.name })
                }
                row("WSL mount folder:") {
                    textField().columns(COLUMNS_MEDIUM).bindText(appState::mountRoot)
                }
                row("WSL distribution:") {
                    textField().columns(COLUMNS_MEDIUM)
                        .comment("Only needed to open files that live inside WSL (\\\\wsl.localhost\\NAME\\...).")
                        .bindText(appState::wslDistro)
                }
                row {
                    button("Reset to defaults") {
                        app.resetToDefaults()
                        projectSettings.resetToDefaults()
                        reset()
                    }
                }
            }
        }
    }

    /** After the page is applied, check again: the new settings may change what the compiler says. */
    override fun apply() {
        val libraryBefore = SharpLibraryRootsProvider.externalLibraryRoot(project)
        super.apply()
        SharpApplicationSettings.forgetToolchain()      // the "Sharp folder" may have changed
        // the "Standard library" field may have changed: the External Libraries node follows it
        SharpLibraryRootsProvider.libraryRootsChanged(project, libraryBefore, SharpLibraryRootsProvider.externalLibraryRoot(project))
        AnalysisService.getInstance(project).requestRun(RunReason.SETTINGS)
    }

    /** What the page says about the installation it would use right now. */
    private fun installedComment(home: String): String {
        val found = sharp.ide.toolchain.SharpToolchains.find(home)
        return if (found != null) "Found: Sharp ${found.version} in ${found.home}."
        else "Not found in ${home.trim().ifEmpty { sharp.ide.toolchain.SharpToolchains.defaultHome() }}."
    }

    private fun splitLines(text: String): MutableList<String> =
        text.lines().map { it.trim() }.filter { it.isNotEmpty() }.toMutableList()
}
