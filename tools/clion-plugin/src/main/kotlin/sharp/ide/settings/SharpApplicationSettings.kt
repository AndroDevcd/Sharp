/*
 * SharpApplicationSettings - the settings that belong to this MACHINE, not to a project.
 *
 * WHAT THIS IS FOR
 *   How the compiler is started depends on the machine (is it behind WSL? where is the
 *   VM?), not on the project, so these settings live once per IDE installation, in the
 *   file `sharp.xml` of the IDE's options folder. The platform saves and loads the
 *   `State` object by itself (a `PersistentStateComponent`): every public `var` with a
 *   default value is stored, and only values that differ from the default are written.
 *
 *   The per-project half (which files, on/off) is SharpProjectSettings.
 *   The page that edits both is SharpConfigurable. doc/ide.md section 6.4.
 */
package sharp.ide.settings

import com.intellij.openapi.components.PersistentStateComponent
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.State
import com.intellij.openapi.components.Storage
import com.intellij.openapi.components.service
import com.intellij.util.xmlb.XmlSerializerUtil

/** How the wsl translation of paths is decided. */
enum class TranslateMode { AUTOMATIC, ALWAYS, NEVER }

@Service(Service.Level.APP)
@State(name = "SharpApplicationSettings", storages = [Storage("sharp.xml")])
class SharpApplicationSettings : PersistentStateComponent<SharpApplicationSettings.State> {

    /** The stored values. Plain `var`s with defaults, which is what the platform's serializer needs. */
    class State {
        /** One argument per line; `{script}` is replaced by the script's path. */
        var launcher: String = SharpDefaults.launcher()
        var script: String = SharpDefaults.SCRIPT
        var vm: String = SharpDefaults.VM
        var compiler: String = SharpDefaults.COMPILER
        var timeoutSeconds: Int = SharpDefaults.TIMEOUT_SECONDS
        var debounceMillis: Int = SharpDefaults.DEBOUNCE_MILLIS
        /** How long after the last key press an edited Sharp file is saved by the plugin (TypingPauseSaver); 0 = never. */
        var typingPauseMillis: Int = SharpDefaults.TYPING_PAUSE_MILLIS
        var translateMode: String = TranslateMode.AUTOMATIC.name
        var mountRoot: String = SharpDefaults.MOUNT_ROOT
        var wslDistro: String = ""
    }

    private var state = State()

    override fun getState(): State = state

    override fun loadState(loaded: State) {
        XmlSerializerUtil.copyBean(loaded, state)
    }

    /** Back to what a fresh installation has. */
    fun resetToDefaults() {
        loadState(State())
    }

    fun translateModeValue(): TranslateMode =
        TranslateMode.entries.firstOrNull { it.name == state.translateMode } ?: TranslateMode.AUTOMATIC

    companion object {
        fun getInstance(): SharpApplicationSettings = service()
    }
}
