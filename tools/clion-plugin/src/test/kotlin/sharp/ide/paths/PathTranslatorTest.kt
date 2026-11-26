package sharp.ide.paths

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/** Path translation Windows <-> WSL, both ways, with the awkward cases (a space, a UNC path, no distribution). */
class PathTranslatorTest {
    private val translator = PathTranslator("/mnt", "Ubuntu")

    @Test fun driveLetterPathsGoUnderMnt() {
        assertEquals("/mnt/c/Users/me/x y/a.sharp", translator.toWsl("C:\\Users\\me\\x y\\a.sharp"))
        assertEquals("/mnt/c/Users/me/a.sharp", translator.toWsl("C:/Users/me/a.sharp"))
        assertEquals("/mnt/d/proj", translator.toWsl("D:\\proj\\"))
    }

    @Test fun aDriveRootIsTheMountPointOfTheDrive() {
        assertEquals("/mnt/c", translator.toWsl("C:\\"))
        assertEquals("/mnt/c", translator.toWsl("c:/"))
    }

    @Test fun theUserNameWithASpaceSurvives() {
        assertEquals(
            "/mnt/c/Users/Braxton Nunnally/Documents/Github/Sharp",
            translator.toWsl("C:\\Users\\Braxton Nunnally\\Documents\\Github\\Sharp"),
        )
    }

    @Test fun wslUncPathsLoseTheirHost() {
        assertEquals("/home/me/a.sharp", translator.toWsl("\\\\wsl.localhost\\Ubuntu\\home\\me\\a.sharp"))
        assertEquals("/home/me/a.sharp", translator.toWsl("\\\\wsl\$\\Ubuntu\\home\\me\\a.sharp"))
        assertEquals("/", translator.toWsl("//wsl.localhost/Ubuntu"))
    }

    @Test fun aLinuxPathIsLeftAlone() {
        assertEquals("/home/me/a.sharp", translator.toWsl("/home/me/a.sharp"))
        assertEquals("/home/me", translator.toWsl("/home/me/"))
        assertEquals("/", translator.toWsl("/"))
    }

    @Test fun backToWindowsBothSpellings() {
        assertEquals("C:\\Users\\me\\x y\\a.sharp", translator.toWindows("/mnt/c/Users/me/x y/a.sharp"))
        assertEquals("C:/Users/me/x y/a.sharp", translator.toWindows("/mnt/c/Users/me/x y/a.sharp", forwardSlashes = true))
        assertEquals("C:/Users/me/x y/a.sharp", translator.toEditorPath("/mnt/c/Users/me/x y/a.sharp"))
        assertEquals("C:\\", translator.toWindows("/mnt/c"))
    }

    @Test fun aLinuxOnlyPathNeedsTheDistribution() {
        assertEquals("\\\\wsl.localhost\\Ubuntu\\home\\me\\a.sharp", translator.toWindows("/home/me/a.sharp"))
        assertEquals("//wsl.localhost/Ubuntu/home/me/a.sharp", translator.toEditorPath("/home/me/a.sharp"))
        assertNull(PathTranslator("/mnt", "").toWindows("/home/me/a.sharp"))
    }

    @Test fun theDistributionIsLearnedFromAnUncPath() {
        val learned = PathTranslator("/mnt", "").withDistroFrom("//wsl.localhost/Debian/home/me/proj")
        assertEquals("//wsl.localhost/Debian/home/me/a.sharp", learned.toEditorPath("/home/me/a.sharp"))
        // a path that says nothing leaves the translator as it was
        assertEquals("C:/x", PathTranslator("/mnt", "").withDistroFrom("C:/x").toEditorPath("/mnt/c/x"))
    }

    @Test fun anotherMountRoot() {
        val other = PathTranslator("/c", "")
        assertEquals("/c/c/Users/me", other.toWsl("C:\\Users\\me"))
        assertEquals("C:\\Users\\me", other.toWindows("/c/c/Users/me"))
    }

    @Test fun roundTripOfEveryDriveAndUncForm() {
        for (path in listOf("C:/Users/Braxton Nunnally/a.sharp", "D:/x", "E:/a b/c d/e.sharp")) {
            assertEquals(path, translator.toEditorPath(translator.toWsl(path)))
        }
        val unc = "//wsl.localhost/Ubuntu/home/me/a.sharp"
        assertEquals(unc, translator.toEditorPath(translator.toWsl(unc)))
    }

    @Test fun aPathThatIsNotAbsoluteIsNotTouchedGoingBack() {
        assertEquals("rel/a.sharp", translator.toWindows("rel/a.sharp"))
    }
}
