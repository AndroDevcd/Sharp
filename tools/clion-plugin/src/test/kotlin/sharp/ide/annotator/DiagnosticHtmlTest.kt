package sharp.ide.annotator

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import sharp.ide.index.DiagnosticSeverity
import sharp.ide.index.Place
import sharp.ide.index.SharpDiagnostic

/** The tooltip: every part of the compiler's message, escaped, with code shown as code and related places as links. */
class DiagnosticHtmlTest {
    private fun place(file: String, from: Int, line: Int, label: String = "") = Place(file, from, from + 3, line, 1, line, 4, label)

    private val diagnostic = SharpDiagnostic(
        severity = DiagnosticSeverity.ERROR,
        code = "E0302",
        message = "`thing` is declared twice in module `rel` <&>",
        place = place("/a.sharp", 49, 7),
        label = "a second class with this name",
        notes = listOf("the rule: names are unique"),
        helps = listOf("rename one of them: `thing2`"),
        related = listOf(place("/a.sharp", 16, 3, "the first one is here"), place("/lib/b.sharp", 5, 1, "the other")),
    )

    @Test fun theShortMessageCarriesTheCode() {
        assertEquals("E0302: `thing` is declared twice in module `rel` <&>", DiagnosticHtml.shortMessage(diagnostic))
    }

    @Test fun everyPartIsThereInOrder() {
        val html = DiagnosticHtml.tooltip(diagnostic, linkFor = { "#sharp/r/${it.from}/x" }, fileLabel = { it.file.substringAfterLast('/') + ":" + it.line })
        val order = listOf("E0302", "declared twice", "a second class with this name", "note:", "the rule", "help:", "rename one of them", "see also:", "the first one is here", "a.sharp:3", "b.sharp:1")
        var at = -1
        for (part in order) {
            val found = html.indexOf(part, at + 1)
            assertTrue("'$part' missing or out of order in $html", found > at)
            at = found
        }
    }

    @Test fun backtickedTextBecomesCodeAndNothingBreaksTheHtml() {
        val html = DiagnosticHtml.tooltip(diagnostic, { null }, { "" })
        assertTrue(html.contains("<code>thing</code> is declared twice in module <code>rel</code> &lt;&amp;&gt;"))
        assertTrue(html.contains("rename one of them: <code>thing2</code>"))
        assertFalse(html.contains("<&>"))
    }

    @Test fun aRelatedPlaceIsALinkWhenThereIsAnHrefAndPlainTextWhenNot() {
        val withLinks = DiagnosticHtml.tooltip(diagnostic, { if (it.file == "/a.sharp") "#sharp/r/${it.from}/x" else null }, { "f:" + it.line })
        assertTrue(withLinks.contains("<a href=\"#sharp/r/16/x\">"))
        assertEquals(1, Regex("<a href").findAll(withLinks).count())     // the library place has no link
        assertTrue(withLinks.contains("the other"))
    }

    @Test fun emptyLabelNotesAndHelpsAddNothing() {
        val bare = diagnostic.copy(label = "", notes = emptyList(), helps = emptyList(), related = emptyList())
        val html = DiagnosticHtml.tooltip(bare, { null }, { "" })
        assertFalse(html.contains("note:")); assertFalse(html.contains("help:")); assertFalse(html.contains("see also:")); assertFalse(html.contains("<i>"))
    }

    @Test fun newlinesBecomeLineBreaks() {
        assertEquals("a<br/>b", DiagnosticHtml.escape("a\r\nb"))
    }

    @Test fun linksRoundTripWithAPathThatNeedsEncoding() {
        val place = place("/mnt/c/Users/Braxton Nunnally/a&b.sharp", 123, 4)
        val href = TooltipLinks.hrefFor("17a3", place)
        assertTrue(href.startsWith("#sharp/17a3/123/"))
        assertFalse(href.contains(" "))
        val parsed = TooltipLinks.parse(href.removePrefix(TooltipLinks.PREFIX))
        assertNotNull(parsed)
        assertEquals(Triple("17a3", 123, "/mnt/c/Users/Braxton Nunnally/a&b.sharp"), parsed)
    }

    @Test fun aLinkThatIsNotOursIsRefused() {
        assertNull(TooltipLinks.parse("/only/two"))
        assertNull(TooltipLinks.parse("/run/notanumber/path"))
    }
}
