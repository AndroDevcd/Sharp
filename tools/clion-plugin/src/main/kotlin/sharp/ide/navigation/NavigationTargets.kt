/*
 * NavigationTargets - where does Ctrl+click on this name go?
 *
 * WHAT THIS IS FOR
 *   (doc/ide.md 6.10.) The compiler's table of uses says, for every resolved name, which declaration
 *   it refers to: `(file, node)` for fields, functions, constructors, types, enum values, and the index
 *   of the local for a variable. This turns that into a place: a file (by the compiler's path, also a
 *   library file) and an offset in the text the COMPILER read.
 *
 *   Moving that offset onto the text the person sees (the target file may be open and edited) is the
 *   platform side's job (SharpGotoDeclarationHandler); this class stays pure so the targets of a whole
 *   program are tested on strings:
 *
 *       names.size()      size  -> std/list.sharp, at the name `size` of `def size()`
 *       count.value       count -> the line `count := names.size();` of this file
 *       new book("x")     book  -> the class book (its constructor is written by the compiler)
 *
 *   `self`, `base` and names the compiler did not resolve have no target: the answer is null and
 *   nothing happens (no error, no popup).
 */
package sharp.ide.navigation

import sharp.ide.editor.FileView
import sharp.ide.editor.Symbol
import sharp.ide.editor.SymbolAt

/** A place in a file of the run: the compiler's path, the file's number, and offsets in the compiler's text. */
class NavTarget(val compilerPath: String, val file: Int, val from: Int, val to: Int, val name: String)

object NavigationTargets {

    fun at(view: FileView, offset: Int): NavTarget? {
        val symbol = SymbolAt.find(view, offset) as? Symbol.UseOf ?: return null
        val local = symbol.local
        if (local != null) {
            val file = view.file ?: return null
            return NavTarget(view.index.fileEntry(file)?.path ?: return null, file, local.declFrom, local.declTo, local.name)
        }
        val d = symbol.declaration ?: return null
        val path = view.index.fileEntry(d.file)?.path ?: return null
        return NavTarget(path, d.file, d.at.from, d.at.to, d.name)
    }
}
