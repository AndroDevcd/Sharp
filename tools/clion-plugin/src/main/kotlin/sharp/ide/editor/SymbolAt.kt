/*
 * SymbolAt - "what is under this offset?" for hover and go-to.
 *
 * WHAT THIS IS FOR
 *   Both answer from the compiler's TABLE OF USES (doc/ide.md 4.7): every name in the code that the
 *   compiler resolved, with what it refers to and the type of the value. The offset is first moved back
 *   into the text the compiler read (OffsetMap): if the person is editing right there, the answer is
 *   "nothing" - never a guess about different text.
 *
 *     a use         `size` in `names.size()`          -> the function `std#list.size`, type `int`
 *     a local       `count` in `count.value`          -> the local variable, with its declaration place
 *     self / base   `self`                            -> no declaration, only the type
 *     a declaration the name in `def label() : ...`   -> the declaration itself (hover shows it)
 */
package sharp.ide.editor

import sharp.ide.index.BodyFunction
import sharp.ide.index.Declaration
import sharp.ide.index.LocalInfo
import sharp.ide.index.SharpType
import sharp.ide.index.Use

sealed class Symbol {
    /** A name in code: what the compiler bound it to. `declaration` is null when the index has no record of the target. */
    class UseOf(
        val use: Use,
        val declaration: Declaration?,
        val local: LocalInfo?,
        /** The function whose code the use is in. */
        val inFunction: BodyFunction?,
        /** The range of the name in the SNAPSHOT text. */
        val from: Int,
        val to: Int,
    ) : Symbol() {
        val type: SharpType? get() = use.type
    }

    /** A declaration's own name. */
    class DeclarationHere(val declaration: Declaration) : Symbol()
}

object SymbolAt {

    /** The symbol at `offset` of the CURRENT text, or null when there is none or the text there was edited. */
    fun find(view: FileView, offset: Int): Symbol? {
        val file = view.file ?: return null
        val old = view.toSnapshot(offset) ?: return null
        val bodies = view.index.bodiesFor(file)
        val use = bodies?.useAt(old)
        if (use != null && !wordIsIntact(view, use.from, use.to)) return null
        if (bodies != null && use != null) {
            val local = if (use.kind == 6) bodies.locals.getOrNull(use.node) else null
            val declaration = if (use.kind == 6 || use.kind == 7) null else view.index.declarations[use.file to use.node]
            val function = bodies.functions.getOrNull(bodies.functionAt(old))
            return Symbol.UseOf(use, declaration, local, function, use.from, use.to)
        }
        return view.index.declarationAt(file, old)?.let { Symbol.DeclarationHere(it) }
    }

    /**
     * The name at [from, to) of the compiler's text is still the same word in the current text: the same letters,
     * and no identifier character typed right against either end (`size` became `sizeXX`: it is another name now).
     */
    private fun wordIsIntact(view: FileView, from: Int, to: Int): Boolean {
        val old = view.snapshot ?: return false
        val now = view.toCurrent(from) ?: return false
        val word = old.substring(from.coerceIn(0, old.length), to.coerceIn(0, old.length))
        if (!view.text.regionMatches(now, word, 0, word.length)) return false
        fun isIdentifierChar(c: Char) = c == '_' || c.isLetterOrDigit()
        if (word.isNotEmpty() && isIdentifierChar(word.first()) && now > 0 && isIdentifierChar(view.text[now - 1])) return false
        if (word.isNotEmpty() && isIdentifierChar(word.last()) && now + word.length < view.text.length && isIdentifierChar(view.text[now + word.length])) return false
        return true
    }
}
