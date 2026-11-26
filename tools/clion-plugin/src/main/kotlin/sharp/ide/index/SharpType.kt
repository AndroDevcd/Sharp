/*
 * SharpType - a type as the editor features see it, and the table the files point into.
 *
 * WHAT THIS IS FOR
 *   `types.json` (doc/ide.md 4.5) lists every type once; every other file says "type 7"
 *   meaning entry 7. Inside the plugin a type is a VALUE (6.6): putting `string` in for
 *   `T` in `list<T>` makes a new type the compiler never wrote down, so the features
 *   cannot work with ids alone. `TypeTable.typeOf(id)` builds the value.
 *
 *     entry { "text": "list<string>?", "class": "std#list", "args": [0], "nullable": true }
 *       -> SharpType(text = "list<string>?", classId = "std#list", args = [string], nullable = true)
 *
 *   For an ARRAY the entry's `class` is the class of the ITEMS (`string[]` says `std#string`),
 *   so a feature must look at `isArray` first: an array itself has no members.
 *
 *   C++ comparison: this is the job of the compiler's `sharp_type` plus `resolved_type`,
 *   reduced to what a tooltip and a member list need.
 */
package sharp.ide.index

class SharpType(
    /** The type as a programmer writes it: `list<string>?`, `int[]`, `T`. */
    val text: String,
    /** The id (`module#outer.inner`) of the class, interface or enum; for an array the class of its items. */
    val classId: String? = null,
    val args: List<SharpType> = emptyList(),
    val isArray: Boolean = false,
    val element: SharpType? = null,
    val nullable: Boolean = false,
    /** True for a type parameter such as `T` (then `text` is its name, with a `?` when nullable). */
    val isParam: Boolean = false,
    val isFunction: Boolean = false,
) {
    /** The name of a type parameter: `T` for `T` and for `T?`. */
    val paramName: String get() = text.trimEnd('?')

    /** The same type with its type parameters replaced; `T -> string` turns `list<T>` into `list<string>`. */
    fun substitute(by: Map<String, SharpType>): SharpType {
        if (by.isEmpty()) return this
        if (isParam) {
            val replacement = by[paramName] ?: return this
            return if (nullable && !replacement.nullable) replacement.asNullable() else replacement
        }
        if (args.isEmpty() && element == null) return this
        val newArgs = args.map { it.substitute(by) }
        val newElement = element?.substitute(by)
        return SharpType(
            text = SignatureText.substituteWords(text, by.mapValues { it.value.text }),
            classId = classId, args = newArgs, isArray = isArray, element = newElement,
            nullable = nullable, isParam = false, isFunction = isFunction,
        )
    }

    fun asNullable(): SharpType =
        if (nullable) this else SharpType(text + "?", classId, args, isArray, element, true, isParam, isFunction)

    /** The text without a trailing `?`: how a type is named in "is a `list<string>`". */
    val plainText: String get() = text.trimEnd('?')

    override fun toString(): String = text
}

/** Replacing whole words in display text: `at(index: int) : T` with `T -> string`. */
object SignatureText {
    private val word = Regex("[A-Za-z_][A-Za-z_0-9]*")

    fun substituteWords(text: String, by: Map<String, String>): String {
        if (by.isEmpty()) return text
        return word.replace(text) { by[it.value] ?: it.value }
    }
}

/** The raw rows of `types.json`, turned into values on demand and remembered. */
class TypeTable(private val rows: List<TypeRow>) {

    class TypeRow(
        val text: String,
        val classId: String?,
        val args: List<Int>,
        val isArray: Boolean,
        val element: Int,
        val nullable: Boolean,
        val isParam: Boolean,
        val isFunction: Boolean,
    )

    private val made = arrayOfNulls<SharpType>(rows.size)

    val size: Int get() = rows.size

    /** The type with this id; null for `-1`, for an id out of range, and for a cycle (which a compiler never writes). */
    fun typeOf(id: Int): SharpType? = typeOf(id, 0)

    private fun typeOf(id: Int, depth: Int): SharpType? {
        if (id < 0 || id >= rows.size || depth > 32) return null
        made[id]?.let { return it }
        val row = rows[id]
        val type = SharpType(
            text = row.text,
            classId = row.classId,
            args = row.args.mapNotNull { typeOf(it, depth + 1) },
            isArray = row.isArray,
            element = if (row.isArray) typeOf(row.element, depth + 1) else null,
            nullable = row.nullable,
            // the compiler marks `T[]` as a parameter too (it looks through to `T`); the array itself is not one
            isParam = row.isParam && !row.isArray,
            isFunction = row.isFunction,
        )
        made[id] = type
        return type
    }
}
