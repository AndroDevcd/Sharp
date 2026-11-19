package sharp.ide.paths

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File

/** Which files make up the program: roots, exclusions, the library, the automatic defaults. */
class ProgramFilesTest {
    @get:Rule val temp = TemporaryFolder()

    private fun touch(base: File, relative: String) {
        val file = File(base, relative)
        file.parentFile.mkdirs()
        file.writeText("// $relative\n")
    }

    private fun relative(base: File, files: List<String>): List<String> {
        val prefix = ProgramFiles.normalise(base.path) + "/"
        return files.map { it.removePrefix(prefix) }
    }

    @Test fun libraryComesFirstThenSourcesEachSorted() {
        val base = temp.newFolder("proj")
        listOf("lib/support/0.4.0/std/b.sharp", "lib/support/0.4.0/std/a.sharp", "z.sharp", "src/a.sharp", "src/notes.txt").forEach { touch(base, it) }
        val files = ProgramFiles.list(ProgramSettings(base.path, emptyList(), listOf("tests"), ""))
        assertEquals(
            listOf("lib/support/0.4.0/std/a.sharp", "lib/support/0.4.0/std/b.sharp", "src/a.sharp", "z.sharp"),
            relative(base, files),
        )
    }

    @Test fun theLibraryIsCountedOnceEvenBelowASourceRoot() {
        val base = temp.newFolder("proj")
        touch(base, "lib/support/0.4.0/std/a.sharp")
        touch(base, "main.sharp")
        val files = ProgramFiles.list(ProgramSettings(base.path, emptyList(), emptyList(), ""))
        assertEquals(1, files.count { it.endsWith("std/a.sharp") })
    }

    @Test fun excludedFoldersByNameAndByFragmentAreSkippedBelowTheRootOnly() {
        val base = temp.newFolder("tests")                       // the root itself is called tests: it is NOT excluded
        listOf("a.sharp", "tests/b.sharp", "lib/old/c.sharp", "lib/new/d.sharp", "build/e.sharp").forEach { touch(base, it) }
        val settings = ProgramSettings(base.path, listOf(base.path), listOf("tests", "lib/old", "build"), "")
        assertEquals(listOf("a.sharp", "lib/new/d.sharp"), relative(base, ProgramFiles.list(settings)))
    }

    @Test fun relativeRootsAreResolvedAgainstTheProjectFolder() {
        val base = temp.newFolder("proj")
        touch(base, "app/a.sharp"); touch(base, "other/b.sharp")
        val files = ProgramFiles.list(ProgramSettings(base.path, listOf("app"), emptyList(), ""))
        assertEquals(listOf("app/a.sharp"), relative(base, files))
    }

    @Test fun theSharpRepositoryDefaultsToTheBootstrapCompiler() {
        val base = temp.newFolder("repo")
        touch(base, "lib/compiler/main.sharp")
        touch(base, "lib/compiler/backend/a.sharp")
        touch(base, "lib/compiler/tests/skipped.sharp")
        touch(base, "tests/suite/case.sharp")
        touch(base, "lib/support/0.4.0/std/a.sharp")
        val layout = ProgramFiles.layoutOf(ProgramSettings(base.path, emptyList(), listOf("tests"), ""))
        assertEquals(listOf(ProgramFiles.normalise(base.path) + "/lib/compiler"), layout.sourceRoots)
        assertEquals(ProgramFiles.normalise(base.path) + "/lib/support/0.4.0", layout.libraryRoot)
        val files = relative(base, ProgramFiles.list(ProgramSettings(base.path, emptyList(), listOf("tests"), "")))
        assertEquals(listOf("lib/support/0.4.0/std/a.sharp", "lib/compiler/backend/a.sharp", "lib/compiler/main.sharp"), files)
    }

    @Test fun noLibraryFolderMeansNoLibraryRoot() {
        val base = temp.newFolder("proj")
        touch(base, "a.sharp")
        assertNull(ProgramFiles.layoutOf(ProgramSettings(base.path, emptyList(), emptyList(), "")).libraryRoot)
    }

    @Test fun anExplicitLibraryPathWins() {
        val base = temp.newFolder("proj")
        val library = temp.newFolder("elsewhere")
        touch(library, "std/a.sharp")
        touch(base, "a.sharp")
        val files = ProgramFiles.list(ProgramSettings(base.path, emptyList(), emptyList(), library.path))
        assertEquals(ProgramFiles.normalise(library.path) + "/std/a.sharp", files.first())
    }

    @Test fun containsIsAStringTestThatAgreesWithList() {
        val base = temp.newFolder("proj")
        val names = listOf("src/a.sharp", "src/tests/b.sharp", "lib/support/0.4.0/std/c.sharp", "outside/d.sharp")
        names.forEach { touch(base, it) }
        val settings = ProgramSettings(base.path, listOf("src"), listOf("tests"), "")
        val layout = ProgramFiles.layoutOf(settings)
        val listed = ProgramFiles.list(settings).toSet()
        for (name in names) {
            val path = ProgramFiles.normalise(File(base, name).path)
            assertEquals(name, path in listed, ProgramFiles.contains(layout, path))
        }
        assertFalse(ProgramFiles.contains(layout, ProgramFiles.normalise(File(base, "src/readme.txt").path)))
        assertTrue(ProgramFiles.contains(layout, ProgramFiles.normalise(File(base, "src/new.sharp").path)))   // not on disk yet
    }
}
