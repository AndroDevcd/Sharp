/*
 * StructureOutline - the tree of a file's declarations: classes with their members, then the globals.
 *
 * WHAT THIS IS FOR
 *   The Structure tool window (Ctrl+F12) lists what a file declares. The compiler already wrote that
 *   down (`declarations.json`), so the outline is built from the index, not from a parser of our own
 *   (doc/ide.md 6.11):
 *
 *       class circle base shape
 *           radius : decimal
 *           area(scale: decimal) : decimal
 *       enum color
 *           red  green  blue
 *       def total(scale: decimal) : decimal
 *       string.shout() : string            <- an extension function written in this file
 *
 *   Positions are moved onto the CURRENT text (a declaration whose name was edited is left out until
 *   the next run). Pure data: the platform classes only wrap it.
 */
package sharp.ide.structure

import sharp.ide.editor.FileView
import sharp.ide.index.AliasInfo
import sharp.ide.index.ClassInfo
import sharp.ide.index.Declaration
import sharp.ide.index.EnumValueInfo
import sharp.ide.index.FieldInfo
import sharp.ide.index.FunctionInfo

class OutlineNode(
    val label: String,
    /** The word for the icon: class, interface, enum, function, field, property, value, alias. */
    val kind: String,
    /** Offset of the name in the text NOW. */
    val offset: Int,
    val children: List<OutlineNode>,
)

object StructureOutline {

    fun of(view: FileView): List<OutlineNode> {
        val file = view.file ?: return emptyList()
        if (!view.hasOffsets) return emptyList()
        val index = view.index
        val top = ArrayList<OutlineNode>()
        for (d in index.declarations.values.filter { it.file == file }) {
            when {
                d is ClassInfo && d.owner == null -> node(view, d)?.let { top.add(it) }
                d is FunctionInfo && d.owner == null && !d.isConstructor -> leaf(view, d)?.let { top.add(it) }
                d is FunctionInfo && d.isExtension -> leaf(view, d, prefix = d.owner?.name?.plus(".") ?: "")?.let { top.add(it) }
                d is FieldInfo && d.owner == null -> leaf(view, d)?.let { top.add(it) }
                d is AliasInfo && d.owner == null -> leaf(view, d)?.let { top.add(it) }
            }
        }
        return top.sortedBy { it.offset }
    }

    private fun node(view: FileView, c: ClassInfo): OutlineNode? {
        val offset = view.toCurrent(c.at.from) ?: return null
        val members = ArrayList<OutlineNode>()
        val file = c.file
        for (v in c.values) leaf(view, v)?.let { members.add(it) }
        for (f in c.fields.filter { it.file == file }) leaf(view, f)?.let { members.add(it) }
        for (f in c.constructors.filter { it.file == file }) leaf(view, f)?.let { members.add(it) }
        for (f in c.functions.filter { it.file == file && !it.isExtension }) leaf(view, f)?.let { members.add(it) }
        for (n in c.nested) node(view, n)?.let { members.add(it) }
        val header = buildString {
            append(c.name)
            if (c.typeParameters.isNotEmpty()) append('<').append(c.typeParameters.joinToString(", ") { it.name }).append('>')
            c.base?.let { append(" base ").append(it.text) }
        }
        return OutlineNode(header, c.kind, offset, if (c.isEnum) members else members.sortedBy { it.offset })
    }

    private fun leaf(view: FileView, d: Declaration, prefix: String = ""): OutlineNode? {
        val offset = view.toCurrent(d.at.from) ?: return null
        return when (d) {
            is FunctionInfo -> OutlineNode(prefix + d.signature, if (d.isConstructor) "constructor" else "function", offset, emptyList())
            is FieldInfo -> OutlineNode(d.name + (d.type?.let { " : ${it.text}" } ?: ""), if (d.hasGetter || d.hasSetter) "property" else "field", offset, emptyList())
            is EnumValueInfo -> OutlineNode(d.name, "value", offset, emptyList())
            is AliasInfo -> OutlineNode("${d.name} = ${d.target?.text ?: "?"}", "alias", offset, emptyList())
            else -> null
        }
    }
}
