/*
 * DiagnosticHtml - the tooltip text for one diagnostic.
 *
 * WHAT THIS IS FOR
 *   The compiler's messages are written to be read whole: a message, a label under the
 *   marked piece, the rule that was broken (notes), how to fix it (helps) and other
 *   places that matter ("declared here"). When the mouse rests on a red squiggle the
 *   editor shows this as a tooltip. The layout:
 *
 *       E0400  there is no variable, field or type named `missing_name` here
 *       not declared in this function, in module `x` or in anything this file imports
 *       note: ...                      (the rule, one line per note)
 *       help: declare it above ...     (the fix; the text in `backticks` is shown as code)
 *       see also: the first one is here  (shop.sharp:3)       <- a link that jumps there
 *
 *   Everything that comes from the compiler is HTML-escaped, then text between
 *   backticks is wrapped in <code>, so a `<` in a message cannot break the tooltip.
 *
 *   Pure text work: no platform classes, so it is tested with plain strings.
 *   doc/ide.md section 6.7.
 */
package sharp.ide.annotator

import sharp.ide.index.Place
import sharp.ide.index.SharpDiagnostic

object DiagnosticHtml {

    /** The one-line text for the status bar and the Problems view: `E0400: there is no ...`. */
    fun shortMessage(diagnostic: SharpDiagnostic): String = "${diagnostic.code}: ${diagnostic.message}"

    /**
     * The tooltip. `linkFor` gives the href of a related place (null: show it as plain text);
     * `fileLabel` the text that names the place's file (the file name and the line).
     */
    fun tooltip(
        diagnostic: SharpDiagnostic,
        linkFor: (Place) -> String?,
        fileLabel: (Place) -> String,
    ): String {
        val html = StringBuilder("<html><body>")
        html.append("<b>").append(escape(diagnostic.code)).append("</b>&nbsp; ")
            .append(withCode(diagnostic.message))
        if (diagnostic.label.isNotEmpty()) html.append("<br/><i>").append(withCode(diagnostic.label)).append("</i>")
        for (note in diagnostic.notes) html.append("<br/><b>note:</b> ").append(withCode(note))
        for (help in diagnostic.helps) html.append("<br/><b>help:</b> ").append(withCode(help))
        for (place in diagnostic.related) {
            html.append("<br/><b>see also:</b> ")
            val text = (if (place.label.isEmpty()) "" else withCode(place.label) + " &nbsp;") + "(" + escape(fileLabel(place)) + ")"
            val href = linkFor(place)
            if (href == null) html.append(text) else html.append("<a href=\"").append(escape(href)).append("\">").append(text).append("</a>")
        }
        return html.append("</body></html>").toString()
    }

    /** Escapes `& < > "` and turns line ends into <br/>. */
    fun escape(text: String): String {
        val out = StringBuilder(text.length + 8)
        for (c in text) {
            when (c) {
                '&' -> out.append("&amp;")
                '<' -> out.append("&lt;")
                '>' -> out.append("&gt;")
                '"' -> out.append("&quot;")
                '\n' -> out.append("<br/>")
                '\r' -> {}
                else -> out.append(c)
            }
        }
        return out.toString()
    }

    private val backticks = Regex("`([^`]*)`")

    /** Escaped text in which each `backticked` piece is shown as code. */
    fun withCode(text: String): String = backticks.replace(escape(text)) { "<code>" + it.groupValues[1] + "</code>" }
}
