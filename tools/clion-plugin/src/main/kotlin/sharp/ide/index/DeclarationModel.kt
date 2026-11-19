/*
 * DeclarationModel - what `declarations.json` says, as plain Kotlin objects.
 *
 * WHAT THIS IS FOR
 *   One object per thing a program declares: modules, classes (also interfaces and
 *   enums), fields, functions, enum values, aliases, type parameters (doc/ide.md 4.6).
 *   Members know their owner class, so a hover can say "member of `shapes#circle`"
 *   without a search; the owner is passed in when the member is made.
 *
 *   Identity: a declaration is named by `(file, node)` - the manifest number of the file it
 *   is declared in and its node number in that file's syntax tree. A use in a bodies file
 *   points at exactly that pair. Both numbers mean something only inside ONE run.
 *
 *   Positions are offsets into the SAVED text of the file the compiler read (section 4.1).
 */
package sharp.ide.index

/** `[from, to, line, column]` of a name token: where go-to-declaration jumps to. */
class NamePlace(val from: Int, val to: Int, val line: Int, val column: Int)

/** Anything a use can point at. */
sealed class Declaration {
    abstract val name: String
    /** The manifest number of the file the declaration is written in. */
    abstract val file: Int
    abstract val node: Int
    abstract val at: NamePlace
    /** The modifiers as words: `public static stable`; "" when none. */
    abstract val flags: String
    /** The class it is a member of; null for a module-level declaration. */
    abstract val owner: ClassInfo?
    /** The name of the module it belongs to ("" for the nameless one). */
    abstract val module: String

    val isStatic: Boolean get() = hasFlag("static")
    fun hasFlag(word: String): Boolean = flags.split(' ').contains(word)
    val identity: Pair<Int, Int> get() = file to node
}

class TypeParameterInfo(
    override val name: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    /** The type id of the bound (`T base number`), kept as a value; null when it has none. */
    val bound: SharpType?,
) : Declaration() {
    override val flags: String get() = ""
}

class FieldInfo(
    override val name: String,
    override val flags: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    val type: SharpType?,
    val hasGetter: Boolean,
    val hasSetter: Boolean,
) : Declaration()

class ParameterInfo(val name: String, val type: SharpType?)

class FunctionInfo(
    override val name: String,
    override val flags: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    /** The signature as display text: `at(index: int) : T`. */
    val signature: String,
    val parameters: List<ParameterInfo>,
    /** The type of the result; null when it returns nothing or the compiler did not know it. */
    val result: SharpType?,
    val typeParameters: List<TypeParameterInfo>,
    val isOperator: Boolean,
    val isExtension: Boolean,
    val isConstructor: Boolean,
) : Declaration()

class EnumValueInfo(
    override val name: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    val number: Long?,
) : Declaration() {
    override val flags: String get() = "static"
}

class AliasInfo(
    override val name: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    val target: SharpType?,
) : Declaration() {
    override val flags: String get() = ""
}

class ClassInfo(
    /** `module#outer.inner`: the id types use. */
    val id: String,
    override val name: String,
    /** `class`, `interface` or `enum`. */
    val kind: String,
    override val flags: String,
    override val file: Int,
    override val node: Int,
    override val at: NamePlace,
    override val owner: ClassInfo?,
    override val module: String,
    /** `[from, to]` of the `{ ... }` body, braces included; null when unknown. */
    val body: IntRange?,
    val typeParameters: List<TypeParameterInfo>,
    /** The written base class; null when none is written (then `std#_object_` is the base). */
    val base: SharpType?,
    val interfaces: List<SharpType>,
) : Declaration() {
    val fields = ArrayList<FieldInfo>()
    val functions = ArrayList<FunctionInfo>()
    val constructors = ArrayList<FunctionInfo>()
    val values = ArrayList<EnumValueInfo>()
    val nested = ArrayList<ClassInfo>()
    val aliases = ArrayList<AliasInfo>()

    val isEnum: Boolean get() = kind == "enum"
    val isInterface: Boolean get() = kind == "interface"
}

/** A module and everything declared at its top level (nested classes are inside their classes). */
class ModuleInfo(val name: String, val files: List<Int>) {
    val classes = ArrayList<ClassInfo>()
    val globals = ArrayList<FieldInfo>()
    val functions = ArrayList<FunctionInfo>()
    val aliases = ArrayList<AliasInfo>()
}
