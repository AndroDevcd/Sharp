/*
 * THE "SHARP PROGRAM" RUN CONFIGURATION: build one folder of Sharp code and run it
 *
 * A run configuration is what the IDE's Run button starts (Run > Edit Configurations lists them).
 * This one says: THIS folder is a program - compile every .sharp file in it with the standard
 * library, then run what the compiler wrote, and show the output in the Run console.
 *
 *     program folder       the folder whose .sharp files are the program (excluded folders of the
 *                          project's settings - tests, build ... - are left out). It is one folder,
 *                          not "the whole project": a project can hold several programs.
 *     compiler options     added to the compiler's command line: -O
 *     program arguments    what the program's `main(args)` gets
 *     working directory    where the program runs (empty: the program folder)
 *     build only           compile and stop
 *     rebuild every time   off (default): the compiler runs only when a file, an option or the compiler
 *                          itself changed since the last good build (RunStamp.kt)
 *     unoptimized compiler use the copy of the compiler that was built without the optimizer
 *     test program         the program is tests written with std.testing: Run shows them as a tree
 *                          of groups and tests instead of plain output (sharp.ide.testing)
 *
 * The pieces, in the platform's terms:
 *
 *     SharpRunConfigurationType     the kind of configuration, with its name and icon
 *     SharpConfigurationFactory     makes new, empty ones
 *     SharpRunOptions               the stored fields (written into the project's workspace file)
 *     SharpRunConfiguration         one configuration: its fields, its editor, and `getState`, which
 *                                   hands the run to SharpRunState (SharpRunState.kt)
 *     SharpRunSettingsEditor        the form shown in Edit Configurations
 *
 * A program folder that has a settings.json is built as a Sharp PROJECT (`sharpc -P`, sharp.ide.project):
 * its program is `<folder>/build/outputs/<output>`. Any other folder's program is written to
 * `<program folder>/build/<folder name>`; `build` is one of the folders the
 * plugin leaves out of the program by default, so the output never becomes an input.
 */
package sharp.ide.execution

import com.intellij.execution.Executor
import com.intellij.execution.configurations.ConfigurationFactory
import com.intellij.execution.configurations.ConfigurationType
import com.intellij.execution.configurations.ConfigurationTypeBase
import com.intellij.execution.configurations.ConfigurationTypeUtil
import com.intellij.execution.configurations.RunConfiguration
import com.intellij.execution.configurations.RunConfigurationBase
import com.intellij.execution.configurations.RunConfigurationOptions
import com.intellij.execution.configurations.RunProfileState
import com.intellij.execution.configurations.RuntimeConfigurationError
import com.intellij.execution.runners.ExecutionEnvironment
import com.intellij.openapi.components.BaseState
import com.intellij.openapi.fileChooser.FileChooserDescriptorFactory
import com.intellij.openapi.options.SettingsEditor
import com.intellij.openapi.project.Project
import com.intellij.openapi.ui.TextFieldWithBrowseButton
import com.intellij.openapi.util.NotNullLazyValue
import com.intellij.ui.components.JBCheckBox
import com.intellij.ui.components.JBTextField
import com.intellij.util.ui.FormBuilder
import sharp.ide.language.SharpIcons
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpApplicationSettings
import sharp.ide.toolchain.SharpToolchains
import java.nio.file.Files
import java.nio.file.Paths
import javax.swing.JComponent

class SharpRunConfigurationType : ConfigurationTypeBase(
    ID,
    "Sharp Program",
    "Compile a folder of Sharp code and run the program",
    NotNullLazyValue.createValue { SharpIcons.FILE },
) {
    init {
        addFactory(SharpConfigurationFactory(this))
    }

    val factory: ConfigurationFactory get() = configurationFactories[0]

    companion object {
        const val ID = "SharpProgramRunConfiguration"
        fun instance(): SharpRunConfigurationType = ConfigurationTypeUtil.findConfigurationType(SharpRunConfigurationType::class.java)
    }
}

class SharpConfigurationFactory(type: ConfigurationType) : ConfigurationFactory(type) {
    override fun getId(): String = "Sharp Program"
    override fun createTemplateConfiguration(project: Project): RunConfiguration = SharpRunConfiguration(project, this, "Sharp Program")
    override fun getOptionsClass(): Class<out BaseState> = SharpRunOptions::class.java
}

/** The stored fields. (`string("")` and `property(false)` are the platform's "write this into the file" properties.) */
class SharpRunOptions : RunConfigurationOptions() {
    var programFolder by string("")
    var compilerOptions by string("")
    var programArguments by string("")
    var workingDirectory by string("")
    var buildOnly by property(false)
    var unoptimizedCompiler by property(false)
    var alwaysRebuild by property(false)
    var testProgram by property(false)
}

class SharpRunConfiguration(project: Project, factory: ConfigurationFactory, name: String) :
    RunConfigurationBase<SharpRunOptions>(project, factory, name) {

    val fields: SharpRunOptions get() = options as SharpRunOptions

    /** The program folder as an absolute path with forward slashes ("" when not set). A relative one is below the project. */
    fun programFolderPath(): String {
        val written = ProgramFiles.normalise(fields.programFolder ?: "")
        if (written.isEmpty()) return ""
        val absolute = written.startsWith("/") || (written.length >= 2 && written[1] == ':')
        val base = ProgramFiles.normalise(project.basePath ?: "")
        return if (absolute || base.isEmpty()) written else "$base/$written"
    }

    override fun getConfigurationEditor(): SettingsEditor<out RunConfiguration> = SharpRunSettingsEditor(project)

    override fun getState(executor: Executor, environment: ExecutionEnvironment): RunProfileState = SharpRunState(environment, this)

    /** Shown in red in Edit Configurations, and asked before a run. */
    override fun checkConfiguration() {
        val folder = programFolderPath()
        if (folder.isEmpty()) throw RuntimeConfigurationError("Choose the program folder: the folder with the program's .sharp files")
        if (!Files.isDirectory(Paths.get(folder))) throw RuntimeConfigurationError("The program folder does not exist: $folder")
        val app = SharpApplicationSettings.getInstance()
        if (SharpToolchains.find(app.state.sharpHome) == null) {
            throw RuntimeConfigurationError(SharpToolchains.notInstalledMessage(app.state.sharpHome))
        }
    }
}

/** The form of Edit Configurations: one row per field. */
class SharpRunSettingsEditor(private val project: Project) : SettingsEditor<SharpRunConfiguration>() {
    private val programFolder = TextFieldWithBrowseButton()
    private val compilerOptions = JBTextField()
    private val programArguments = JBTextField()
    private val workingDirectory = TextFieldWithBrowseButton()
    private val buildOnly = JBCheckBox("Build only: compile, do not run")
    private val unoptimized = JBCheckBox("Use the compiler that was built without the optimizer (when the normal one misbehaves)")
    private val alwaysRebuild = JBCheckBox("Rebuild every time (off: compile again only when something the program is built from changed)")
    private val testProgram = JBCheckBox("Test program (std.testing): show the groups and their tests as a tree")

    override fun createEditor(): JComponent {
        programFolder.addBrowseFolderListener(project, FileChooserDescriptorFactory.createSingleFolderDescriptor().withTitle("Sharp Program Folder"))
        workingDirectory.addBrowseFolderListener(project, FileChooserDescriptorFactory.createSingleFolderDescriptor().withTitle("Working Directory"))
        compilerOptions.emptyText.text = "for example: -O"
        workingDirectory.textField.let { (it as? JBTextField)?.emptyText?.text = "the program folder" }
        return FormBuilder.createFormBuilder()
            .addLabeledComponent("Program folder:", programFolder)
            .addTooltip("Every .sharp file in this folder (and below) is compiled together with the standard library. " +
                "A folder with a settings.json is built as a Sharp project: the file names the output, the libraries and what is left out.")
            .addLabeledComponent("Compiler options:", compilerOptions)
            .addLabeledComponent("Program arguments:", programArguments)
            .addLabeledComponent("Working directory:", workingDirectory)
            .addComponent(testProgram)
            .addComponent(buildOnly)
            .addComponent(alwaysRebuild)
            .addComponent(unoptimized)
            .panel
    }

    override fun resetEditorFrom(configuration: SharpRunConfiguration) {
        val fields = configuration.fields
        programFolder.text = fields.programFolder ?: ""
        compilerOptions.text = fields.compilerOptions ?: ""
        programArguments.text = fields.programArguments ?: ""
        workingDirectory.text = fields.workingDirectory ?: ""
        buildOnly.isSelected = fields.buildOnly
        unoptimized.isSelected = fields.unoptimizedCompiler
        alwaysRebuild.isSelected = fields.alwaysRebuild
        testProgram.isSelected = fields.testProgram
    }

    override fun applyEditorTo(configuration: SharpRunConfiguration) {
        val fields = configuration.fields
        fields.programFolder = programFolder.text.trim()
        fields.compilerOptions = compilerOptions.text.trim()
        fields.programArguments = programArguments.text.trim()
        fields.workingDirectory = workingDirectory.text.trim()
        fields.buildOnly = buildOnly.isSelected
        fields.unoptimizedCompiler = unoptimized.isSelected
        fields.alwaysRebuild = alwaysRebuild.isSelected
        fields.testProgram = testProgram.isSelected
    }
}
