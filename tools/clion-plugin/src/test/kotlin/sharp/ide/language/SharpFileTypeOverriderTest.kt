package sharp.ide.language

import com.intellij.openapi.application.runWriteAction
import com.intellij.openapi.fileTypes.ExtensionFileNameMatcher
import com.intellij.openapi.fileTypes.FileTypeManager
import com.intellij.openapi.fileTypes.PlainTextFileType
import com.intellij.testFramework.fixtures.BasePlatformTestCase

/**
 * A `.sharp` FILE IS A SHARP FILE WHATEVER THE SETTINGS SAY
 *
 * The owner's IDE had `*.sharp` assigned to another file type (a hand-made one), which
 * silently switched off every feature of the plugin. These tests put the settings into that
 * state and check the plugin still gets its files.
 */
class SharpFileTypeOverriderTest : BasePlatformTestCase() {

    private val matcher = ExtensionFileNameMatcher("sharp")

    /** What the owner's settings said: `*.sharp` taken away from the plugin's type and given to another. */
    private fun giveThePatternToAnotherType() {
        runWriteAction {
            val types = FileTypeManager.getInstance()
            types.removeAssociation(SharpFileType, matcher)
            types.associate(PlainTextFileType.INSTANCE, matcher)
        }
    }

    override fun tearDown() {
        try {
            runWriteAction {
                val types = FileTypeManager.getInstance()
                types.removeAssociation(PlainTextFileType.INSTANCE, matcher)
                types.associate(SharpFileType, matcher)
            }
        } finally {
            super.tearDown()
        }
    }

    fun testASharpFileKeepsItsTypeAndLanguageWhenThePatternBelongsToAnotherType() {
        giveThePatternToAnotherType()
        val file = myFixture.configureByText("taken.sharp", "mod app;\n\ndef main() { }\n")
        assertSame(SharpFileType, file.virtualFile.fileType)
        assertSame(SharpLanguage, file.language)            // what the error marks and completion are registered for
        assertTrue(file is SharpFile)
    }

    fun testOnlyTheNameDecides() {
        val overrider = SharpFileTypeOverrider()
        val sharp = myFixture.configureByText("a.sharp", "mod a;\n").virtualFile
        val upper = myFixture.configureByText("B.SHARP", "mod b;\n").virtualFile
        val other = myFixture.configureByText("notes.txt", "mod a;\n").virtualFile
        assertSame(SharpFileType, overrider.getOverriddenFileType(sharp))
        assertSame(SharpFileType, overrider.getOverriddenFileType(upper))
        assertNull(overrider.getOverriddenFileType(other))
        assertNull(overrider.getOverriddenFileType(sharp.parent))       // a folder
    }
}
