/*
 * ImportEdit - the text to add when a class of a module the file does not import is chosen.
 *
 * WHAT THIS IS FOR
 *   Completion offers classes of modules the file does not import yet (doc/ide.md 6.8). Choosing
 *   one must also make the class visible, so the module goes into the file's import list:
 *
 *       import ( std.io )                 ->   import ( std.io, shop )
 *       import (                               import (
 *           platform,                              platform,
 *           platform.kernel                        platform.kernel,
 *       )                                          shop
 *                                              )
 *       (no import list)                  ->   `import ( shop )` below the `mod` line
 *
 *   `plan` is a pure function from the text to "insert THIS at THAT offset" (or nothing, when
 *   the module is already imported), so the rule is tested on strings.
 */
package sharp.ide.completion

/** Insert `text` at `offset`. */
class TextInsertion(val offset: Int, val text: String)

object ImportEdit {
    private val importBlock = Regex("(?s)\\bimport\\s*\\(([^)]*)\\)(\\s*as\\s+[A-Za-z_][A-Za-z_0-9]*)?")
    private val modLine = Regex("(?m)^[ \\t]*mod\\s+[A-Za-z_][A-Za-z_0-9.]*\\s*;")

    fun plan(text: String, module: String): TextInsertion? {
        for (m in importBlock.findAll(text)) {
            if (m.groups[2] != null) continue                                   // an `as g` group is not the plain list
            val body = m.groups[1]!!
            val names = body.value.split(',', '\n').map { it.trim() }.filter { it.isNotEmpty() }
            if (module in names) return null
            // insert right after the last name; an empty list gets the name between its parentheses
            val lastName = body.value.indexOfLast { !it.isWhitespace() }
            if (lastName < 0) return TextInsertion(body.range.first, " $module ")
            val at = body.range.first + lastName + 1
            val multiLine = body.value.contains('\n')
            if (!multiLine) return TextInsertion(at, ", $module")
            val lineStart = text.lastIndexOf('\n', at - 1) + 1
            val indent = text.substring(lineStart, at).takeWhile { it == ' ' || it == '\t' }
            return TextInsertion(at, ",\n$indent$module")
        }
        val mod = modLine.find(text)
        if (mod != null) return TextInsertion(mod.range.last + 1, "\n\nimport ( $module )")
        return TextInsertion(0, "import ( $module )\n\n")
    }
}
