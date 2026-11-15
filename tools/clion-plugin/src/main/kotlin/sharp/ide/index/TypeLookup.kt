/*
 * TypeLookup - the questions completion, hover and go-to ask of a RunIndex.
 *
 * WHAT THIS IS FOR
 *   (doc/ide.md 6.6.) Pure functions over an immutable RunIndex, so they are tested without
 *   the platform:
 *
 *     membersOf        what can be written after a dot on a value of this type
 *     scopeFor         which modules a file can see (its imports, plus the import lines
 *                      typed since the last compiler run)
 *     findClass        a class by the name a programmer writes: `list`, `std.io#file`, `outer.inner`
 *     resolveTypeName  a written type (`list<string>?`, `int[]`) as a SharpType
 *
 *   INHERITED MEMBERS ARE NOT COPIED INTO THE CLASS (4.6): the lookup follows `base` and
 *   `interfaces` itself, nearest first, and puts the receiver's type arguments in for the
 *   base's type parameters on the way up:
 *
 *       class list<T> ...           receiver  list<string>   ->  T = string
 *       class sorted<K> base list<K>  receiver  sorted<int>  ->  K = int, then T = K = int
 *
 *   A function overridden lower in the chain hides the one above (same name, same parameter
 *   types); a field or nested class hides by name.
 *
 *   Visibility: `private` only inside the same class (and classes nested in it), `protected`
 *   inside it and its subclasses. The compiler itself only WARNS about a private member used
 *   from outside, but offering it would be noise.
 */
package sharp.ide.index

/** How a member list is filtered: after a value, after a class name, or by plain name inside a class. */
enum class MemberFilter { INSTANCE, STATIC, EVERYTHING }

/** One member found on a type, as seen on that type (type arguments put in). */
class MemberEntry(
    val declaration: Declaration,
    /** The class the member is written in (a base class, an interface, or the class itself). */
    val declaredIn: ClassInfo,
    /** 0 for the class itself, 1 for its base or an interface, 2 for theirs ... */
    val distance: Int,
    val viaInterface: Boolean,
    /** The member's type as seen on the receiver: a field's type, a function's result. Null when unknown or none. */
    val type: SharpType?,
    /** For a function: the signature with the type arguments put in. */
    val signature: String?,
)

/** The modules a place in a file can see, in search order (its own first), and its import groups. */
class Scope(val modules: List<String>, val groups: Map<String, List<String>> = emptyMap()) {
    val ownModule: String? get() = modules.firstOrNull()
}

class TypeLookup(val index: RunIndex) {

    // ------------------------------------------------------------------ members

    /** The members visible on a value (INSTANCE) or on a class name (STATIC) of this type. Empty for arrays, numbers, functions. */
    fun membersOf(type: SharpType, filter: MemberFilter, from: ClassInfo?): List<MemberEntry> {
        if (type.isArray || type.isFunction || type.isParam) return emptyList()
        val c = index.classOf(type) ?: return emptyList()
        return membersOfClass(c, type.args, filter, from)
    }

    fun membersOfClass(start: ClassInfo, args: List<SharpType>, filter: MemberFilter, from: ClassInfo?): List<MemberEntry> {
        val result = ArrayList<MemberEntry>()
        val seen = HashSet<String>()
        val visited = HashSet<String>()
        class Step(val c: ClassInfo, val subst: Map<String, SharpType>, val distance: Int, val viaInterface: Boolean)
        val queue = ArrayDeque<Step>()
        queue.add(Step(start, substitutionFor(start, args), 0, false))
        while (queue.isNotEmpty() && visited.size < MAX_CLASSES) {
            val step = queue.removeFirst()
            if (!visited.add(step.c.id)) continue
            addOwnMembers(step.c, step.subst, step.distance, step.viaInterface, filter, from, seen, result)
            val baseType = step.c.base?.substitute(step.subst)
            val base = if (baseType != null) index.classOf(baseType) else index.baseOf(step.c)
            if (base != null) queue.add(Step(base, substitutionFor(base, baseType?.args ?: emptyList()), step.distance + 1, step.viaInterface))
            for (interfaceType in step.c.interfaces) {
                val t = interfaceType.substitute(step.subst)
                val c = index.classOf(t) ?: continue
                queue.add(Step(c, substitutionFor(c, t.args), step.distance + 1, true))
            }
        }
        return result
    }

    private fun addOwnMembers(
        c: ClassInfo, subst: Map<String, SharpType>, distance: Int, viaInterface: Boolean,
        filter: MemberFilter, from: ClassInfo?, seen: MutableSet<String>, out: MutableList<MemberEntry>,
    ) {
        fun take(key: String, declaration: Declaration, type: SharpType?, signature: String?) {
            if (!isVisible(declaration, c, from)) return
            if (!seen.add(key)) return
            out.add(MemberEntry(declaration, c, distance, viaInterface, type?.substitute(subst), signature))
        }
        fun filterAccepts(isStatic: Boolean) = when (filter) {
            MemberFilter.INSTANCE -> !isStatic
            MemberFilter.STATIC -> isStatic
            MemberFilter.EVERYTHING -> true
        }
        for (f in c.fields) if (filterAccepts(f.isStatic)) take("f:" + f.name, f, f.type, null)
        for (f in c.functions) {
            if (!filterAccepts(f.isStatic)) continue
            val key = "m:" + f.name + "(" + f.parameters.joinToString(",") { it.type?.substitute(subst)?.text ?: "?" } + ")"
            take(key, f, f.result, SignatureText.substituteWords(f.signature, subst.mapValues { it.value.text }))
        }
        if (filter != MemberFilter.INSTANCE) {
            for (v in c.values) take("f:" + v.name, v, SharpType(c.name, c.id), null)
            for (n in c.nested) take("c:" + n.name, n, null, null)
        }
    }

    private fun substitutionFor(c: ClassInfo, args: List<SharpType>): Map<String, SharpType> {
        if (args.isEmpty() || c.typeParameters.isEmpty()) return emptyMap()
        val map = HashMap<String, SharpType>()
        for ((i, p) in c.typeParameters.withIndex()) args.getOrNull(i)?.let { map[p.name] = it }
        return map
    }

    private fun isVisible(member: Declaration, declaredIn: ClassInfo, from: ClassInfo?): Boolean {
        val flags = member.flags
        if (flags.isEmpty()) return true
        if (member.hasFlag("private")) return from != null && sameOrInside(from, declaredIn)
        if (member.hasFlag("protected")) return from != null && (sameOrInside(from, declaredIn) || isSubclass(from, declaredIn))
        return true
    }

    private fun sameOrInside(c: ClassInfo, outer: ClassInfo): Boolean = c.id == outer.id || c.id.startsWith(outer.id + ".")

    /** Is `c` the class `ancestor` or derived from it (through bases or interfaces)? Guarded against cycles. */
    fun isSubclass(c: ClassInfo, ancestor: ClassInfo): Boolean {
        val seen = HashSet<String>()
        val todo = ArrayDeque<ClassInfo>()
        todo.add(c)
        while (todo.isNotEmpty() && seen.size < MAX_CLASSES) {
            val current = todo.removeFirst()
            if (!seen.add(current.id)) continue
            if (current.id == ancestor.id) return true
            index.baseOf(current)?.let { todo.add(it) }
            current.interfaces.mapNotNull { index.classOf(it) }.forEach { todo.add(it) }
        }
        return false
    }

    // ------------------------------------------------------------------ modules and names

    /** The modules a place can see: the manifest's imports for the file plus the import lines of the current text. */
    fun scopeFor(file: Int?, currentText: CharSequence): Scope {
        val modules = LinkedHashSet<String>()
        val groups = LinkedHashMap<String, List<String>>()
        val entry = file?.let { index.fileEntry(it) }
        val textual = ImportLines.read(currentText)
        val own = entry?.module ?: textual.module
        if (own != null) modules.add(own)
        if (entry != null) {
            modules.addAll(entry.imports)
            groups.putAll(entry.groups)
        }
        for (written in textual.imports) expandModuleName(written, modules)
        for ((name, members) in textual.groups) {
            val expanded = LinkedHashSet<String>()
            members.forEach { expandModuleName(it, expanded) }
            groups.putIfAbsent(name, expanded.toList())
        }
        return Scope(modules.toList(), groups)
    }

    /** `std.*` means every module whose name starts with `std.` (and `std` itself); a plain name is one module. */
    private fun expandModuleName(written: String, into: MutableSet<String>) {
        if (written.endsWith(".*")) {
            val prefix = written.removeSuffix(".*")
            into.add(prefix)
            index.modules.keys.filter { it.startsWith("$prefix.") }.forEach { into.add(it) }
        } else {
            into.add(written)
        }
    }

    fun moduleNames(): List<String> = index.modules.keys.filter { it.isNotEmpty() }.sorted()

    private val classNamesInModule: Map<String, Map<String, ClassInfo>> by lazy {
        index.modules.mapValues { (_, m) -> m.classes.associateBy { it.name } }
    }

    fun classInModule(module: String, name: String): ClassInfo? = classNamesInModule[module]?.get(name)

    /** The classes, interfaces and enums a place can name without qualifying them, own module first. */
    fun visibleClasses(scope: Scope): List<ClassInfo> = scope.modules.flatMap { index.modules[it]?.classes ?: emptyList() }

    /** The aliases a place can name. */
    fun visibleAliases(scope: Scope): List<AliasInfo> = scope.modules.flatMap { index.modules[it]?.aliases ?: emptyList() }

    /** A class by the name a programmer writes: `list`, `outer.inner`, `std.io#file`, `g#file` (a group). Null when not found. */
    fun findClass(written: String, scope: Scope): ClassInfo? {
        val hash = written.indexOf('#')
        val modules: List<String>
        val path: String
        if (hash >= 0) {
            val qualifier = written.substring(0, hash)
            modules = scope.groups[qualifier] ?: listOf(qualifier)
            path = written.substring(hash + 1)
        } else {
            modules = scope.modules
            path = written
        }
        val segments = path.split('.')
        var current: ClassInfo? = null
        for ((i, segment) in segments.withIndex()) {
            current = if (i == 0) modules.firstNotNullOfOrNull { classInModule(it, segment) ?: aliasTarget(it, segment) }
            else current?.nested?.firstOrNull { it.name == segment }
            if (current == null) return null
        }
        return current
    }

    private fun aliasTarget(module: String, name: String): ClassInfo? {
        val alias = index.modules[module]?.aliases?.firstOrNull { it.name == name } ?: return null
        return index.classOf(alias.target)
    }

    /** The type words that are not classes. */
    val builtInTypeWords = listOf("decimal", "object", "_int8", "_int16", "_int32", "_int64", "_uint8", "_uint16", "_uint32", "_uint64")

    /**
     * A written type as a SharpType: `list<string>?`, `std.io#file`, `int[]`, `T`, `_int64`. Null when
     * a class name in it is not found (a misspelt or not imported type).
     */
    fun resolveTypeName(text: String, scope: Scope, typeParameters: Set<String> = emptySet()): SharpType? {
        var t = text.trim()
        if (t.isEmpty()) return null
        var nullable = false
        while (t.endsWith("?")) { nullable = true; t = t.dropLast(1) }
        if (t.endsWith("[]")) {
            val element = resolveTypeName(t.removeSuffix("[]"), scope, typeParameters) ?: return null
            return SharpType(text.trim(), element.classId, element.args, isArray = true, element = element, nullable = nullable)
        }
        val open = t.indexOf('<')
        val name = if (open >= 0) t.substring(0, open) else t
        val args = if (open >= 0 && t.endsWith(">")) splitArguments(t.substring(open + 1, t.length - 1)).map { resolveTypeName(it, scope, typeParameters) ?: return null } else emptyList()
        if (name in typeParameters) return SharpType(text.trim(), isParam = true, nullable = nullable)
        if (name in builtInTypeWords) return SharpType(text.trim(), nullable = nullable)
        val c = findClass(name, scope) ?: return null
        return SharpType(text.trim(), c.id, args, nullable = nullable)
    }

    private fun splitArguments(text: String): List<String> {
        val parts = ArrayList<String>()
        var depth = 0
        val current = StringBuilder()
        for (ch in text) {
            when {
                ch == '<' || ch == '(' -> { depth++; current.append(ch) }
                ch == '>' || ch == ')' -> { depth--; current.append(ch) }
                ch == ',' && depth == 0 -> { parts.add(current.toString()); current.setLength(0) }
                else -> current.append(ch)
            }
        }
        if (current.isNotBlank()) parts.add(current.toString())
        return parts
    }

    /** A class used as a value: `circle` -> a SharpType of that class. */
    fun typeOfClass(c: ClassInfo): SharpType = SharpType(c.name, c.id)

    companion object {
        private const val MAX_CLASSES = 64
    }
}

/** What a file's text itself says about its module and imports (read with a regular expression over the top of the file). */
object ImportLines {
    class Read(val module: String?, val imports: List<String>, val groups: Map<String, List<String>>)

    private val modLine = Regex("(?m)^\\s*mod\\s+([A-Za-z_][A-Za-z_0-9.]*)\\s*;")
    private val importBlock = Regex("(?s)\\bimport\\s*\\(([^)]*)\\)(\\s*as\\s+([A-Za-z_][A-Za-z_0-9]*))?")

    fun read(text: CharSequence): Read {
        val s = text.toString()
        val module = modLine.find(s)?.groupValues?.get(1)
        val imports = ArrayList<String>()
        val groups = LinkedHashMap<String, List<String>>()
        for (m in importBlock.findAll(s)) {
            val names = m.groupValues[1].split(',', '\n').map { it.trim() }.filter { it.isNotEmpty() && !it.startsWith("//") }
            val group = m.groupValues[3]
            if (group.isNotEmpty()) groups[group] = names else imports.addAll(names)
        }
        return Read(module, imports, groups)
    }
}
