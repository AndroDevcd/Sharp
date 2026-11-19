/*
 * SharpProjectSettings - the settings that belong to ONE project: which files make
 * up its Sharp program, and whether to check on save.
 *
 * WHAT THIS IS FOR
 *   Saved in the project's own `.idea/sharp.xml` (so a team can share them). An empty
 *   list of source roots and an empty library path mean "work it out" (ProgramFiles
 *   describes the automatic rules). doc/ide.md sections 3.2 and 6.4.
 */
package sharp.ide.settings

import com.intellij.openapi.components.PersistentStateComponent
import com.intellij.openapi.components.Service
import com.intellij.openapi.components.State
import com.intellij.openapi.components.Storage
import com.intellij.openapi.project.Project
import com.intellij.util.xmlb.XmlSerializerUtil
import sharp.ide.paths.ProgramSettings

@Service(Service.Level.PROJECT)
@State(name = "SharpProjectSettings", storages = [Storage("sharp.xml")])
class SharpProjectSettings(private val project: Project) : PersistentStateComponent<SharpProjectSettings.State> {

    class State {
        /** The switch: check the program when a file is saved. A "Run Sharp check now" works even when off. */
        var enabled: Boolean = true
        /** Folders (absolute, or relative to the project folder); empty = automatic. */
        var sourceRoots: MutableList<String> = mutableListOf()
        var excludedFolders: MutableList<String> = SharpDefaults.EXCLUDED_FOLDERS.toMutableList()
        /** The standard library folder; empty = `lib/support/0.4.0` of the project when it exists. */
        var libraryPath: String = ""
        /**
         * Hover and go-to INSIDE a file need that file's function bodies in the index.
         * true (default): asked for every file of the program, the standard library included,
         * on every run - moving between library files never waits for another run.
         * false: only for the files open in an editor (each run is shorter; opening a file
         * asks for one more run, and a closed file is forgotten at the next run).
         */
        var bodiesForEveryFile: Boolean = true
        /** Added to the compiler's command line, separated by spaces. */
        var extraArguments: String = ""
    }

    private var state = State()

    override fun getState(): State = state

    override fun loadState(loaded: State) {
        XmlSerializerUtil.copyBean(loaded, state)
    }

    fun resetToDefaults() {
        loadState(State())
    }

    /**
     * A plain copy of what ProgramFiles needs. Where a program folder is a Sharp PROJECT (it has a
     * settings.json: sharp.ide.project), the file's ignored folders are left out as well and the
     * libraries it names are part of the program - so the editor checks what `sharpc -P` builds.
     */
    fun programSettings(): ProgramSettings {
        val plain = ProgramSettings(
            baseDirectory = project.basePath ?: "",
            sourceRoots = state.sourceRoots.toList(),
            excludedFolders = state.excludedFolders.toList(),
            libraryPath = state.libraryPath,
            installedLibrary = SharpApplicationSettings.getInstance().toolchain()?.library ?: "",
        )
        val roots = sharp.ide.paths.ProgramFiles.layoutOf(plain).sourceRoots
        val projects = roots.mapNotNull { sharp.ide.project.SharpProjectFiles.read(it) }
        if (projects.isEmpty()) return plain
        val libraries = projects.flatMap { it.externLibraryFolders() }.filter { it !in roots }
        val ignored = projects.flatMap { it.ignoredInsideProject() }.filter { it !in plain.excludedFolders }
        if (libraries.isEmpty() && ignored.isEmpty()) return plain
        return plain.copy(sourceRoots = roots + libraries, excludedFolders = plain.excludedFolders + ignored)
    }
}
