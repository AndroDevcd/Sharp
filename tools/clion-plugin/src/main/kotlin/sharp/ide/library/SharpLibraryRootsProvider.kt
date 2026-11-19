/*
 * SharpLibraryRootsProvider - makes the standard library show up under "External Libraries".
 *
 * WHAT THIS IS FOR
 *   The standard library usually lies OUTSIDE the project folder (the setting "Standard
 *   library" points at it). The IDE can open such a file - go to declaration does - but
 *   to the IDE it is then just "some file on the disk": the Project tool window has no
 *   place for it, so nothing appears under External Libraries, and the bullseye button
 *   ("Select Opened File") has nothing to select.
 *
 *   The platform's answer is a SYNTHETIC LIBRARY: a plugin tells the IDE "this folder is a
 *   library of this project". The IDE then lists it under External Libraries with every
 *   file in it, the bullseye finds library files there, and the files are indexed for
 *   "Find in Files" (scope: Project and Libraries).
 *
 *   Nothing is copied and nothing is checked differently: which files the COMPILER is given
 *   is still decided by ProgramFiles. This only tells the Project tool window about a folder.
 *
 * WHEN THERE IS NO LIBRARY NODE
 *   - no library folder is known (the setting is empty and the project has no
 *     lib/support/0.4.0), or the folder does not exist;
 *   - the library lies INSIDE the project (the Sharp repository itself): its files are in
 *     the project tree already, and listing them twice would only confuse.
 *
 * Registered in plugin.xml as `com.intellij.additionalLibraryRootsProvider`.
 * SharpConfigurable tells the IDE when the setting changes (libraryRootsChanged below).
 */
package sharp.ide.library

import com.intellij.navigation.ItemPresentation
import com.intellij.openapi.application.ApplicationManager
import com.intellij.openapi.project.Project
import com.intellij.openapi.roots.AdditionalLibraryRootsProvider
import com.intellij.openapi.roots.SyntheticLibrary
import com.intellij.openapi.roots.AdditionalLibraryRootsListener
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.openapi.vfs.VirtualFile
import sharp.ide.language.SharpIcons
import sharp.ide.paths.ProgramFiles
import sharp.ide.settings.SharpProjectSettings
import javax.swing.Icon

class SharpLibraryRootsProvider : AdditionalLibraryRootsProvider() {

    override fun getAdditionalProjectLibraries(project: Project): Collection<SyntheticLibrary> {
        val root = externalLibraryRoot(project) ?: return emptyList()
        return listOf(SharpStandardLibrary(root))
    }

    /** The IDE watches these folders for changes made outside it (a `git pull` of the library). */
    override fun getRootsToWatch(project: Project): Collection<VirtualFile> =
        listOfNotNull(externalLibraryRoot(project))

    companion object {
        /** The name of the node under External Libraries. */
        const val LIBRARY_NAME = "Sharp Standard Library"

        /**
         * The standard library folder of a project, when it is one the Project tool window
         * does not show already; null otherwise (see the top of this file).
         */
        fun externalLibraryRoot(project: Project): VirtualFile? {
            if (project.isDisposed) return null
            val settings = project.getService(SharpProjectSettings::class.java) ?: return null
            val path = ProgramFiles.layoutOf(settings.programSettings()).libraryRoot ?: return null

            val base = project.basePath?.replace('\\', '/')?.trimEnd('/')
            if (base != null && (path == base || path.startsWith("$base/"))) return null   // inside the project

            val folder = LocalFileSystem.getInstance().findFileByPath(path) ?: return null
            return if (folder.isValid && folder.isDirectory) folder else null
        }

        /**
         * Called after the "Standard library" setting was applied, with the library folder
         * before and after: makes the IDE ask this provider again, so the External Libraries
         * node appears, moves or goes at once instead of after the next restart.
         */
        fun libraryRootsChanged(project: Project, before: VirtualFile?, after: VirtualFile?) {
            if (before == after) return
            val application = ApplicationManager.getApplication()
            application.invokeLater({
                if (project.isDisposed) return@invokeLater
                application.runWriteAction {
                    // the platform's own way to say "what an AdditionalLibraryRootsProvider
                    // answers has changed": it re-indexes the difference and refreshes the
                    // Project tool window
                    AdditionalLibraryRootsListener.fireAdditionalLibraryChanged(
                        project, LIBRARY_NAME, listOfNotNull(before), listOfNotNull(after), LIBRARY_NAME,
                    )
                }
            }, project.disposed)
        }
    }
}

/**
 * The standard library as the IDE sees it: one folder of sources with a name and an icon.
 *
 * `equals` / `hashCode` matter: the platform compares the libraries it got before and after
 * a change to decide whether anything has to be re-indexed.
 */
class SharpStandardLibrary(private val root: VirtualFile) : SyntheticLibrary(), ItemPresentation {

    override fun getSourceRoots(): Collection<VirtualFile> = listOf(root)

    override fun getPresentableText(): String = SharpLibraryRootsProvider.LIBRARY_NAME

    /** Shown in grey after the name: where the library is. */
    override fun getLocationString(): String = root.presentableUrl

    override fun getIcon(unused: Boolean): Icon = SharpIcons.FILE

    override fun equals(other: Any?): Boolean = other is SharpStandardLibrary && other.root == root

    override fun hashCode(): Int = root.hashCode()
}
