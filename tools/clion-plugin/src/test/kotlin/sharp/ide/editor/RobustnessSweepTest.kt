package sharp.ide.editor

import org.junit.After
import org.junit.Before
import org.junit.Test
import sharp.ide.completion.CompletionEngine
import sharp.ide.documentation.HoverModel
import sharp.ide.index.IndexFixture
import sharp.ide.navigation.NavigationTargets
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE QUIET-DEGRADATION RULE, SWEPT: no caret position may throw
 *
 * The design says completion, hover and go-to must answer "nothing" rather than fail when the place is
 * unknown to the index. This asks every feature at EVERY offset of the real sample, first with the text
 * the compiler saw and then with text that was edited since (a line removed from the middle, a word cut
 * in half, an unfinished expression at the end). A thrown exception here would be a popup for the owner.
 */
class RobustnessSweepTest {
    private lateinit var temp: Path
    private lateinit var shop: IndexFixture.Loaded

    @Before
    fun setUp() {
        temp = Files.createTempDirectory("sharp-sweep")
        shop = IndexFixture("completion").load(temp, "/work/shop.sharp")
    }

    @After
    fun tearDown() {
        temp.toFile().deleteRecursively()
    }

    private fun sweep(text: String) {
        for (offset in 0..text.length) {
            val view = shop.view(text)
            try {
                CompletionEngine.complete(view, text, offset)
                HoverModel.at(view, offset)
                NavigationTargets.at(view, offset)
            } catch (e: Throwable) {
                throw AssertionError("offset $offset near `${text.substring(maxOf(0, offset - 15), minOf(text.length, offset + 15))}`", e)
            }
        }
    }

    @Test
    fun everyOffsetOfTheSavedText() = sweep(shop.fixture.savedText)

    @Test
    fun everyOffsetAfterALineWasRemoved() {
        val t = shop.fixture.savedText
        val at = t.indexOf("    box := ")
        sweep(t.substring(0, at) + t.substring(t.indexOf('\n', at) + 1))
    }

    @Test
    fun everyOffsetWithAnUnfinishedExpressionAtTheEnd() {
        val t = shop.fixture.savedText
        sweep(t + "\ndef broken() { x := names.\n  y := (a + \n  z := new ")
    }

    @Test
    fun everyOffsetWithAnUnclosedStringAndComment() {
        val t = shop.fixture.savedText
        sweep(t.replace("\"done: \"", "\"done: ").replace("// The program", "/* The program"))
    }
}
