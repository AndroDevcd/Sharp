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
        /**
         * true (default): the plugin starts the INSTALLED Sharp directly (sharp.ide.toolchain: the VM on the
         * compiler, no shell) whenever one is found in `sharpHome`. When none is installed - or with false -
         * the launcher and the script below are used instead (a development build inside WSL, say).
         */
        var useInstalledSharp: Boolean = true
        /** The folder Sharp is installed in; empty = the default place (%LOCALAPPDATA%\Sharp, ~/.sharp, or SHARP_HOME). */
        var sharpHome: String = ""
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

    /** The installed Sharp the plugin is to use, or null: not wanted, or none is installed there. */
    fun toolchain(): sharp.ide.toolchain.SharpToolchain? = toolchainOf(state)

    fun translateModeValue(): TranslateMode =
        TranslateMode.entries.firstOrNull { it.name == state.translateMode } ?: TranslateMode.AUTOMATIC

    companion object {
        fun getInstance(): SharpApplicationSettings = service()

        // The answer is asked for on every highlight of every file (is a path to be translated for WSL?),
        // and finding it looks at the disk: it is kept for a few seconds.
        private const val KEEP_MILLIS = 5_000L
        @Volatile private var keptFor: String? = null
        @Volatile private var keptAt = 0L
        @Volatile private var kept: sharp.ide.toolchain.SharpToolchain? = null

        /** The installed Sharp these settings ask for, or null (`useInstalledSharp` off, or nothing installed in `sharpHome`). */
        fun toolchainOf(state: State): sharp.ide.toolchain.SharpToolchain? {
            if (!state.useInstalledSharp) return null
            val home = state.sharpHome.trim()
            val now = System.currentTimeMillis()
            if (keptFor == home && now - keptAt < KEEP_MILLIS) return kept
            val found = sharp.ide.toolchain.SharpToolchains.find(home)
            kept = found
            keptFor = home
            keptAt = now
            return found
        }

        /** Forget what was found (the settings page changed, a test moved the installation). */
        fun forgetToolchain() {
            keptFor = null
        }
    }
}
