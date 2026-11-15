package sharp.ide.library

import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import sharp.ide.settings.SharpProjectSettings
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE STANDARD LIBRARY UNDER "EXTERNAL LIBRARIES"
 *
 * The provider answers "which folders are libraries of this project". What it answers
 * decides whether the Project tool window has an External Libraries node for the standard
 * library, and whether the bullseye can select a library file.
 */
class SharpLibraryRootsProviderTest : BasePlatformTestCase() {

    private lateinit var outside: Path

    override fun setUp() {
        super.setUp()
        outside = Files.createTempDirectory("sharp-library")
        Files.writeString(outside.resolve("string.sharp"), "mod std;\n")
        LocalFileSystem.getInstance().refreshAndFindFileByNioFile(outside)
    }

    override fun tearDown() {
        try {
            project.getService(SharpProjectSettings::class.java).resetToDefaults()
            outside.toFile().deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    private fun setLibraryPath(path: String) {
        project.getService(SharpProjectSettings::class.java).state.libraryPath = path
    }

    fun testALibraryOutsideTheProjectIsListedWithItsFolderAsSourceRoot() {
        setLibraryPath(outside.toString())
        val libraries = SharpLibraryRootsProvider().getAdditionalProjectLibraries(project)
        assertEquals(1, libraries.size)

        val library = libraries.first() as SharpStandardLibrary
        val root = library.sourceRoots.single()
        // (compared as files, not as text: the temp folder may be spelt with a short Windows name)
        assertEquals(LocalFileSystem.getInstance().findFileByNioFile(outside), root)
        assertEquals("Sharp Standard Library", library.presentableText)
        assertNotNull(root.findChild("string.sharp"))
    }

    fun testTheSameFolderGivesEqualLibrariesSoNothingIsIndexedAgain() {
        setLibraryPath(outside.toString())
        val provider = SharpLibraryRootsProvider()
        val first = provider.getAdditionalProjectLibraries(project).first()
        val second = provider.getAdditionalProjectLibraries(project).first()
        assertEquals(first, second)
        assertEquals(first.hashCode(), second.hashCode())
    }

    fun testNoLibraryIsListedWhenNoneIsSetOrTheFolderDoesNotExist() {
        setLibraryPath("")
        assertTrue(SharpLibraryRootsProvider().getAdditionalProjectLibraries(project).isEmpty())

        setLibraryPath(outside.resolve("not-there").toString())
        assertTrue(SharpLibraryRootsProvider().getAdditionalProjectLibraries(project).isEmpty())
    }

    fun testALibraryInsideTheProjectIsNotListedASecondTime() {
        val base = project.basePath ?: return
        setLibraryPath("$base/lib/support/0.4.0")
        assertTrue(SharpLibraryRootsProvider().getAdditionalProjectLibraries(project).isEmpty())
    }

    fun testTheWatchedRootsAreTheLibraryFolder() {
        setLibraryPath(outside.toString())
        val watched = SharpLibraryRootsProvider().getRootsToWatch(project)
        assertEquals(1, watched.size)
    }
}
