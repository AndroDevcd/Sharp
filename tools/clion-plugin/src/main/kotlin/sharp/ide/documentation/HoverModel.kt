/*
 * HoverModel - what the quick documentation says about the name under the mouse or the caret.
 *
 * WHAT THIS IS FOR
 *   (doc/ide.md 6.9.) The declaration and the type of what is under the offset, its owner, its module
 *   and where it is declared:
 *
 *       def area(scale: decimal) : decimal          <- the signature, in code style
 *       Type: decimal
 *       Member of: shapes#circle
 *       Module: shapes
 *       Declared in: shapes.sharp:6
 *
 *   The answer is plain data (HoverInfo) built from the index with no platform object, and
 *   `HoverHtml` writes it as the small HTML the platform shows. Generic code is described for ONE
 *   instance of the class (the compiler's limit 5): then a note says whose types are shown.
 *
 *   Nothing is guessed: an offset inside edited text, in a comment, or on a name the compiler did not
 *   resolve gives null - the platform then shows nothing.
 */
package sharp.ide.documentation

import sharp.ide.editor.FileView
import sharp.ide.editor.Symbol
import sharp.ide.editor.SymbolAt
import sharp.ide.index.AliasInfo
import sharp.ide.index.ClassInfo
import sharp.ide.index.Declaration
import sharp.ide.index.EnumValueInfo
import sharp.ide.index.FieldInfo
import sharp.ide.index.FunctionInfo
import sharp.ide.index.TypeParameterInfo

/** What the hover shows. */
class HoverInfo(
    /** The kind of thing, as a word: `function`, `local variable`, `class` ... */
    val kind: String,
    val name: String,
    /** The declaration as code: `def area(scale: decimal) : decimal`. */
    val definition: String,
    /** The type of the value at this place; null when none or unknown. */
    val type: String?,
    /** `shapes#circle`: the class a member belongs to. */
    val memberOf: String?,
    val module: String?,
    /** `shapes.sharp:6`. */
    val declaredIn: String?,
    /** A note under the rest, e.g. which generic instance the types are of. */
    val note: String?,
) {
    /** One line for the Ctrl+hover hint: `function area(scale: decimal) : decimal`. */
    val hint: String get() = "$kind $name" + (type?.let { " : $it" } ?: "")
}

object HoverModel {

    fun at(view: FileView, offset: Int): HoverInfo? {
        val symbol = SymbolAt.find(view, offset) ?: return null
        return when (symbol) {
            is Symbol.DeclarationHere -> describe(view, symbol.declaration, valueType = null, note = null)
            is Symbol.UseOf -> describeUse(view, symbol)
        }
    }

    private fun describeUse(view: FileView, s: Symbol.UseOf): HoverInfo? {
        val note = s.inFunction?.instance?.let { "The types are those of `$it` (the code is worked out for one instance of the generic)." }
        val typeText = s.type?.text
        if (s.local != null) {
            val l = s.local
            val line = view.bodies?.lineOf(l.declFrom)
            val where = view.file?.let { "${view.index.fileNameOf(it)}:$line" }
            val kindWord = when (l.kind) {
                "parameter" -> "parameter"
                "lambda_parameter" -> "lambda parameter"
                "foreach" -> "loop variable"
                "catch" -> "caught exception"
                else -> if (l.isConst) "constant" else "local variable"
            }
            val ownerFunction = view.bodies?.functions?.getOrNull(l.function)?.takeIf { !it.isLambda }?.name
            return HoverInfo(
                kind = kindWord, name = l.name,
                definition = (if (l.isConst) "const " else "") + l.name + " : " + (l.type?.text ?: "unknown"),
                type = l.type?.text, memberOf = ownerFunction?.let { "function `$it`" }, module = view.file?.let { view.index.moduleOfFile(it) },
                declaredIn = where, note = note,
            )
        }
        if (s.use.kind == 7) {
            val text = view.snapshot?.let { it.substring(s.from.coerceIn(0, it.length), s.to.coerceIn(0, it.length)) }?.ifEmpty { null } ?: "self"
            return HoverInfo("keyword", text, text + (typeText?.let { " : $it" } ?: ""), typeText, null, null, null, note)
        }
        val declaration = s.declaration
        if (declaration == null) {
            // the compiler resolved the name, the index has no record of the target (an alias in a block ...)
            return if (typeText != null) HoverInfo("value", "", "", typeText, null, null, null, note) else null
        }
        return describe(view, declaration, typeText, note)
    }

    private fun describe(view: FileView, d: Declaration, valueType: String?, note: String?): HoverInfo {
        val declaredIn = view.index.fileNameOf(d.file) + ":" + d.at.line
        val owner = d.owner?.id
        return when (d) {
            is FunctionInfo -> HoverInfo(
                kind = if (d.isConstructor) "constructor" else if (d.isOperator) "operator" else if (d.isExtension) "extension function" else "function",
                name = d.name,
                definition = flagsPrefix(d) + (if (d.isConstructor) "" else "def ") + d.signature,
                type = valueType ?: d.result?.text, memberOf = owner, module = d.module, declaredIn = declaredIn, note = note,
            )
            is FieldInfo -> HoverInfo(
                kind = if (d.owner == null) "global variable" else if (d.hasGetter || d.hasSetter) "property" else "field",
                name = d.name, definition = flagsPrefix(d) + d.name + " : " + (d.type?.text ?: valueType ?: "unknown"),
                type = valueType ?: d.type?.text, memberOf = owner, module = d.module, declaredIn = declaredIn, note = note,
            )
            is EnumValueInfo -> HoverInfo(
                "enum value", d.name, d.name + (d.number?.let { " = $it" } ?: ""), d.owner?.name, owner, d.module, declaredIn, note,
            )
            is ClassInfo -> HoverInfo(
                kind = d.kind, name = d.name, definition = classDefinition(d), type = valueType,
                memberOf = owner, module = d.module, declaredIn = declaredIn,
                note = listOfNotNull(note, memberSummary(d)).joinToString("\n").ifEmpty { null },
            )
            is AliasInfo -> HoverInfo("alias", d.name, "alias ${d.target?.text ?: "?"} as ${d.name}", d.target?.text, owner, d.module, declaredIn, note)
            is TypeParameterInfo -> HoverInfo("type parameter", d.name, d.name, valueType, owner, d.module, declaredIn, note)
        }
    }

    /** The modifiers as written, without the default `public`. */
    private fun flagsPrefix(d: Declaration): String = shownFlags(d.flags).let { if (it.isEmpty()) "" else "$it " }

    private fun shownFlags(flags: String): String = flags.split(' ').filter { it.isNotEmpty() && it != "public" }.joinToString(" ")

    /** `public class circle<T> base shape : printable, other` */
    private fun classDefinition(c: ClassInfo): String {
        val sb = StringBuilder()
        shownFlags(c.flags).let { if (it.isNotEmpty()) sb.append(it).append(' ') }
        sb.append(c.kind).append(' ').append(c.name)
        if (c.typeParameters.isNotEmpty()) sb.append('<').append(c.typeParameters.joinToString(", ") { it.name }).append('>')
        c.base?.let { sb.append(" base ").append(it.text) }
        if (c.interfaces.isNotEmpty()) sb.append(" : ").append(c.interfaces.joinToString(", ") { it.text })
        return sb.toString()
    }

    private fun memberSummary(c: ClassInfo): String? {
        val parts = ArrayList<String>()
        if (c.fields.isNotEmpty()) parts.add("${c.fields.size} field(s)")
        if (c.functions.isNotEmpty()) parts.add("${c.functions.size} function(s)")
        if (c.values.isNotEmpty()) parts.add("${c.values.size} value(s)")
        return if (parts.isEmpty()) null else "Has " + parts.joinToString(", ") + "."
    }
}

/** The HTML the platform shows; every text is escaped. */
object HoverHtml {
    fun page(info: HoverInfo): String {
        val sb = StringBuilder()
        if (info.definition.isNotEmpty()) sb.append("<div class='definition'><pre>").append(escape(info.definition)).append("</pre></div>")
        sb.append("<div class='content'>")
        fun row(label: String, value: String?) { if (!value.isNullOrEmpty()) sb.append("<p><b>").append(label).append(":</b> <code>").append(escape(value)).append("</code></p>") }
        row("Kind", info.kind)
        row("Type", info.type)
        row("Member of", info.memberOf)
        row("Module", info.module)
        row("Declared in", info.declaredIn)
        info.note?.let { sb.append("<p>").append(escape(it)).append("</p>") }
        sb.append("</div>")
        return sb.toString()
    }

    fun escape(text: String): String =
        text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace("\"", "&quot;")
}
