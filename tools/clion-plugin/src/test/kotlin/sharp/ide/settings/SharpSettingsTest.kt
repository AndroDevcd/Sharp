package sharp.ide.settings

import com.intellij.testFramework.fixtures.BasePlatformTestCase
import com.intellij.util.xmlb.XmlSerializer
import com.intellij.util.xmlb.XmlSerializerUtil

/**
 * THE SETTINGS: DEFAULTS, SAVING AND LOADING, AND THE PAGE
 *
 * The state classes are saved by the platform's XML serializer; a value that survives
 * serialize -> deserialize is a value that survives a restart of the IDE.
 */
class SharpSettingsTest : BasePlatformTestCase() {

    fun testTheDefaultsDescribeThisMachinesSetup() {
        val state = SharpApplicationSettings.State()
        assertTrue(state.script.contains("mapfile -t FILES < {files_list}"))
        assertTrue(state.script.contains("timeout {timeout} {vm} {compiler}"))
        assertEquals("\$HOME/sharp_work/sharp_cl/sharp", state.vm)
        assertEquals(120, state.timeoutSeconds)
        assertEquals(1500, state.debounceMillis)
        assertEquals(TranslateMode.AUTOMATIC.name, state.translateMode)
        val project = SharpProjectSettings.State()
        assertTrue(project.enabled)
        assertTrue(project.sourceRoots.isEmpty())
        assertEquals(listOf("tests", "build", ".git", "out", "node_modules"), project.excludedFolders)
    }

    fun testApplicationSettingsSurviveARestart() {
        val state = SharpApplicationSettings.State()
        state.launcher = "bash\n{script}"
        state.vm = "/opt/sharp/vm"
        state.timeoutSeconds = 7
        state.translateMode = TranslateMode.NEVER.name
        state.wslDistro = "Debian"
        val element = XmlSerializer.serialize(state)
        val loaded = XmlSerializer.deserialize(element, SharpApplicationSettings.State::class.java)
        assertEquals("bash\n{script}", loaded.launcher)
        assertEquals("/opt/sharp/vm", loaded.vm)
        assertEquals(7, loaded.timeoutSeconds)
        assertEquals(TranslateMode.NEVER.name, loaded.translateMode)
        assertEquals("Debian", loaded.wslDistro)
        assertEquals(SharpDefaults.COMPILER, loaded.compiler)             // an untouched value stays the default
    }

    fun testProjectSettingsSurviveARestartIncludingEmptiedLists() {
        val state = SharpProjectSettings.State()
        state.enabled = false
        state.sourceRoots = mutableListOf("src", "more src")
        state.excludedFolders = mutableListOf()                          // the person removed every exclusion
        state.libraryPath = "D:/sharp/lib"
        state.extraArguments = "-w -wlib"
        val loaded = XmlSerializer.deserialize(XmlSerializer.serialize(state), SharpProjectSettings.State::class.java)
        assertFalse(loaded.enabled)
        assertEquals(listOf("src", "more src"), loaded.sourceRoots)
        assertEquals(emptyList<String>(), loaded.excludedFolders)
        assertEquals("D:/sharp/lib", loaded.libraryPath)
        assertEquals("-w -wlib", loaded.extraArguments)
    }

    fun testLoadStateCopiesIntoTheLiveObject() {
        val settings = SharpApplicationSettings()
        val live = settings.state
        settings.loadState(SharpApplicationSettings.State().also { it.vm = "/x/vm" })
        assertSame(live, settings.state)
        assertEquals("/x/vm", settings.state.vm)
        settings.resetToDefaults()
        assertEquals(SharpDefaults.VM, settings.state.vm)
    }

    fun testThePageIsRegisteredAndEditsTheSettings() {
        val configurable = com.intellij.openapi.options.Configurable.PROJECT_CONFIGURABLE.getExtensions(project)
            .firstOrNull { it.instanceClass == SharpConfigurable::class.java.name }?.createConfigurable() as SharpConfigurable?
        assertNotNull("the settings page is registered in plugin.xml", configurable)
        val app = SharpApplicationSettings.getInstance().state
        val saved = SharpApplicationSettings.State().also { XmlSerializerUtil.copyBean(app, it) }
        try {
            configurable!!.createComponent()
            assertFalse(configurable.isModified)
            configurable.reset()
            assertFalse(configurable.isModified)
            configurable.disposeUIResources()
        } finally {
            XmlSerializerUtil.copyBean(saved, app)
        }
    }

    fun testWhetherTheLauncherStartsWslDecidesTheTranslation() {
        val wsl = SharpApplicationSettings.State().also { it.launcher = SharpDefaults.LAUNCHER_WSL }
        assertNotNull(sharp.ide.run.AnalysisService.translatorFor(wsl, null))
        val native = SharpApplicationSettings.State().also { it.launcher = SharpDefaults.LAUNCHER_NATIVE }
        assertNull(sharp.ide.run.AnalysisService.translatorFor(native, null))
        native.translateMode = TranslateMode.ALWAYS.name
        assertNotNull(sharp.ide.run.AnalysisService.translatorFor(native, null))
        wsl.translateMode = TranslateMode.NEVER.name
        assertNull(sharp.ide.run.AnalysisService.translatorFor(wsl, null))
    }
}
