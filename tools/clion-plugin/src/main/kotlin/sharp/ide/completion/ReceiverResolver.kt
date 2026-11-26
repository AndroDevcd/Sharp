/*
 * ReceiverResolver - "what is before this dot?" - the one genuinely hard part of completion.
 *
 * WHAT THIS IS FOR
 *   After `names.` the list must show the members of whatever `names` is. The difficulty
 *   (doc/ide.md 6.8): the text before a freshly typed dot is, nearly always, ITSELF freshly
 *   typed, so the compiler's table of uses has no entry for it. So the answer is worked out in
 *   this order:
 *
 *   1. READ THE CHAIN FROM THE TEXT and look the NAMES up in the index:
 *        a.b.c(1).d.      ->  segments  a  b  c(...)  d
 *      The first segment is `self`, `base`, a local or parameter in scope (index, then locals
 *      read from the text), a field or function of the class around the caret (inherited ones
 *      included), a global of the module and its imports, a class name (then what follows is
 *      STATIC access), `new foo(...)`, `foo(...)` for a class, `module#name`, or a string
 *      literal. Every further segment is a member lookup on the type so far: a field gives its
 *      type, a called function its result, `[...]` the item type or the result of `operator[]`.
 *      Type arguments are put in at every step (`names.at(0).` on a `list<string>`: `at` returns
 *      `T`, and `T` is `string`).
 *   2. ASK THE TABLE OF USES, when step 1 has no answer and the name before the dot existed at the
 *      last run: the compiler's own answer, which also covers `:=` from a long expression and an
 *      overload chosen by argument types.
 *   3. NOTHING: an empty list rather than a wrong one.
 *
 *      A chain may BEGIN with a group in parentheses: `(apple()).fruit.eat().`,
 *      `(new book("a")).title.`, `((owner)).`. What is inside the parentheses is read the same
 *      way - a chain, or `value as type`, which gives the type - and the chain goes on from it.
 *   NOT covered, so the answer is nothing: an operator expression `(a + b).`, a lambda parameter
 *   whose type is not written, a call through a function value.
 */
package sharp.ide.completion

import sharp.ide.editor.FileView
import sharp.ide.editor.PlaceInfo
import sharp.ide.index.ClassInfo
import sharp.ide.index.FieldInfo
import sharp.ide.index.FunctionInfo
import sharp.ide.index.MemberEntry
import sharp.ide.index.MemberFilter
import sharp.ide.index.Scope
import sharp.ide.index.SharpType
import sharp.ide.index.SignatureText
import sharp.ide.lexer.SharpTokenKinds
import sharp.ide.lexer.Tok

/** One link of a chain: `c(1, 2)` is a called segment with two arguments. */
class Segment(
    val name: String,
    val called: Boolean,
    val argumentCount: Int,
    /** How many `[ ]` follow it: `a[0][1]` is 2. */
    val indexed: Int,
    /** Written after `new`. */
    val isNew: Boolean,
    /** The text of `<string>` between a class name and its `(`, when written. */
    val genericText: String?,
    /** For `std.io#file`: the module (or group) before the `#`. */
    val qualifier: String?,
    val isStringLiteral: Boolean,
    /** For a chain that begins with `( ... )`: the tokens between the parentheses. */
    val group: List<Tok>? = null,
)

/** A function a call may mean, with its signature as seen on the receiver (type arguments put in). */
class Callee(val function: FunctionInfo, val signature: String)

/** What a chain stands for: a value of a type, or a class (static access). */
class Receiver(val type: SharpType?, val staticClass: ClassInfo?)

object ChainParser {

    /** A chain, and the index of its first token in the code it was read from. */
    class Parsed(val segments: List<Segment>, val start: Int)

    /** Reads the chain that ENDS at `code[endExclusive - 1]`, going backwards. Null when the tokens are not a plain chain. */
    fun parse(code: List<Tok>, endExclusive: Int): List<Segment>? = parseWithStart(code, endExclusive)?.segments

    /** The chain when `code` is ONE chain from its first token to its last (`a.b(1).c`, not `x + a.b`); else null. */
    fun parseWhole(code: List<Tok>): List<Segment>? = parseWithStart(code, code.size)?.takeIf { it.start == 0 }?.segments

    fun parseWithStart(code: List<Tok>, endExclusive: Int): Parsed? {
        var start = endExclusive
        var pos = endExclusive - 1
        val reversed = ArrayList<Segment>()
        var pendingIndexes = 0
        while (pos >= 0) {
            val t = code[pos]
            var name: String
            var called = false
            var argumentCount = 0
            var generic: String? = null
            when {
                t.isMark("!!") -> { pos--; continue }
                t.type == SharpTokenKinds.RBRACKET -> {
                    val open = matchBack(code, pos) ?: return null
                    pendingIndexes++
                    pos = open - 1
                    continue
                }
                t.type == SharpTokenKinds.RPAREN -> {
                    val open = matchBack(code, pos) ?: return null
                    argumentCount = countArguments(code, open, pos)
                    var nameAt = open - 1
                    if (nameAt >= 0 && (code[nameAt].isMark(">") || code[nameAt].isMark(">>"))) {
                        val lt = matchAngleBack(code, nameAt)
                        if (lt != null && lt >= 1 && code[lt - 1].isIdentifier) {
                            generic = code.subList(lt, nameAt + 1).joinToString("") { it.text }
                            nameAt = lt - 1
                        }
                    }
                    if (nameAt < 0 || !code[nameAt].isIdentifier) {
                        // no name before the `(`: these are the parentheses of a GROUP, `(apple()).fruit` -
                        // the chain begins here, with whatever stands between them
                        if (pos == open + 1) return null                        // `()`: nothing inside
                        reversed.add(Segment("", false, 0, pendingIndexes, false, null, null, false, code.subList(open + 1, pos)))
                        return Parsed(reversed.reversed(), open)
                    }
                    name = code[nameAt].text
                    called = true
                    pos = nameAt
                }
                t.isIdentifier || t.isKeyword("self") || t.isKeyword("base") -> name = t.text
                t.type == SharpTokenKinds.STRING_TEXT && t.text.endsWith("\"") -> {
                    reversed.add(Segment("", false, 0, pendingIndexes, false, null, null, true))
                    return Parsed(reversed.reversed(), pos)
                }
                else -> return null
            }
            // `pos` is at the token of the name; look at what stands before it
            var isNew = false
            var qualifier: String? = null
            var before = pos - 1
            var continues = before >= 0 && code[before].isDot
            if (before >= 0 && code[before].isKeyword("new")) { isNew = true; before-- }
            else if (before >= 0 && code[before].isMark("#")) {
                val parts = ArrayList<String>()
                var j = before - 1
                while (j >= 0 && (code[j].isIdentifier || code[j].isDot)) { parts.add(code[j].text); j-- }
                qualifier = parts.reversed().joinToString("")
                before = j
                continues = false
            }
            reversed.add(Segment(name, called, argumentCount, pendingIndexes, isNew, generic, qualifier, false))
            pendingIndexes = 0
            start = before + 1                                  // the first token of this segment (`new`, the module, or the name)
            if (continues && !isNew && qualifier == null) { pos = before - 1; continue }
            if (qualifier != null && before >= 0 && code[before].isDot) { pos = before - 1; continue }
            break
        }
        return if (reversed.isEmpty()) null else Parsed(reversed.reversed(), start)
    }

    /** The index of the bracket that opens the one closed at `closeAt`. */
    fun matchBack(code: List<Tok>, closeAt: Int): Int? {
        var depth = 0
        var i = closeAt
        while (i >= 0) {
            val t = code[i]
            when (t.type) {
                SharpTokenKinds.RPAREN, SharpTokenKinds.RBRACKET, SharpTokenKinds.RBRACE -> depth++
                SharpTokenKinds.LPAREN, SharpTokenKinds.LBRACKET, SharpTokenKinds.LBRACE -> { depth--; if (depth == 0) return i }
            }
            i--
        }
        return null
    }

    private fun matchAngleBack(code: List<Tok>, closeAt: Int): Int? {
        var depth = 0
        var i = closeAt
        while (i >= 0) {
            val t = code[i]
            if (t.isMark(">")) depth++
            else if (t.isMark(">>")) depth += 2
            else if (t.isMark("<")) { depth--; if (depth == 0) return i }
            else if (t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.LBRACE) return null
            i--
        }
        return null
    }

    /** The number of arguments between `open` and `close`: commas at depth zero plus one; none for `()`. */
    fun countArguments(code: List<Tok>, open: Int, close: Int): Int {
        if (close == open + 1) return 0
        var depth = 0
        var commas = 0
        for (i in open + 1 until close) {
            val t = code[i]
            when (t.type) {
                SharpTokenKinds.LPAREN, SharpTokenKinds.LBRACKET, SharpTokenKinds.LBRACE -> depth++
                SharpTokenKinds.RPAREN, SharpTokenKinds.RBRACKET, SharpTokenKinds.RBRACE -> depth--
                SharpTokenKinds.COMMA -> if (depth == 0) commas++
            }
        }
        return commas + 1
    }
}

class ReceiverResolver(
    private val view: FileView,
    private val place: PlaceInfo,
    /** The locals read from the text (TextLocals), by name. */
    private val textLocals: Map<String, TextLocal>,
    private val scope: Scope,
) {
    private val lookup = view.lookup
    private val index = view.index

    /** A value of a type, or a class name. */
    private sealed class Value {
        class Instance(val type: SharpType) : Value()
        class StaticOf(val c: ClassInfo) : Value()
    }

    /** What stands before the dot whose token is `code[dotIndex]`; null when nothing sure can be said. */
    fun resolveBefore(code: List<Tok>, dotIndex: Int): Receiver? = resolveChain(code, dotIndex)

    /** The type of the expression `code[from until to]` when it is a plain chain; null otherwise. */
    fun typeOfExpression(expression: List<Tok>): SharpType? = resolveChain(expression, expression.size)?.type

    private fun resolveChain(code: List<Tok>, endExclusive: Int): Receiver? {
        val segments = ChainParser.parse(code, endExclusive) ?: return null
        val value = evaluate(segments) ?: return null
        return when (value) {
            is Value.Instance -> Receiver(value.type, null)
            is Value.StaticOf -> Receiver(null, value.c)
        }
    }

    private fun evaluate(segments: List<Segment>, depth: Int = 0): Value? {
        var value = first(segments[0], depth) ?: return null
        for (segment in segments.drop(1)) value = step(value, segment) ?: return null
        return value
    }

    /**
     * The functions a call may mean, for parameter info: `code[nameIndex]` is the name before the `(`.
     * `new book(` gives the constructors of `book`; `names.add(` the overloads of `add` on the type of
     * `names` with the type arguments put in; a plain `make_book(` the functions of that name in the class
     * around the caret and in the modules in scope.
     */
    fun calleesOf(code: List<Tok>, nameIndex: Int): List<Callee> {
        val segments = ChainParser.parse(code, nameIndex + 1) ?: return emptyList()
        val last = segments.last()
        if (last.isNew || (segments.size == 1 && last.qualifier == null && memberNamed(last.name) == null && lookup.findClass(last.name, scope) != null && !isFunctionInScope(last.name))) {
            val c = lookup.findClass(last.name, scope) ?: return emptyList()
            // `new holder<string>(`: the written type arguments go in for the class's type parameters
            val written = typeArgumentsAfter(code, nameIndex)
            val arguments = if (written != null) lookup.resolveTypeName(last.name + written, scope, place.typeParameters)?.args.orEmpty() else emptyList()
            val substitution = c.typeParameters.zip(arguments).associate { (parameter, type) -> parameter.name to type.text }
            return c.constructors.map { Callee(it, SignatureText.substituteWords(it.signature, substitution)) }
        }
        if (last.qualifier != null) {
            val modules = scope.groups[last.qualifier] ?: listOf(last.qualifier)
            return modules.flatMap { m -> index.modules[m]?.functions?.filter { it.name == last.name } ?: emptyList() }.map { Callee(it, it.signature) }
        }
        val entries: List<MemberEntry>
        if (segments.size == 1) {
            val fromClass = place.enclosingClass?.let { c -> lookup.membersOfClass(c, emptyList(), MemberFilter.EVERYTHING, c) } ?: emptyList()
            val own = fromClass.filter { it.declaration is FunctionInfo && it.declaration.name == last.name }
            if (own.isNotEmpty()) return own.map { Callee(it.declaration as FunctionInfo, it.signature ?: (it.declaration as FunctionInfo).signature) }
            return scope.modules.flatMap { m -> index.modules[m]?.functions?.filter { it.name == last.name } ?: emptyList() }.map { Callee(it, it.signature) }
        }
        val receiver = evaluate(segments.dropLast(1)) ?: return emptyList()
        entries = when (receiver) {
            is Value.Instance -> lookup.membersOf(receiver.type, MemberFilter.INSTANCE, place.enclosingClass)
            is Value.StaticOf -> lookup.membersOfClass(receiver.c, emptyList(), MemberFilter.STATIC, place.enclosingClass)
        }
        return entries.filter { it.declaration is FunctionInfo && it.declaration.name == last.name }
            .map { Callee(it.declaration as FunctionInfo, it.signature ?: (it.declaration as FunctionInfo).signature) }
    }

    /** `<string, int>` written right after the name at `nameIndex`, as text; null when there is none. */
    private fun typeArgumentsAfter(code: List<Tok>, nameIndex: Int): String? {
        if (code.getOrNull(nameIndex + 1)?.isMark("<") != true) return null
        var depth = 0
        for (i in nameIndex + 1 until code.size) {
            val t = code[i]
            if (t.isMark("<")) depth++ else if (t.isMark(">")) depth-- else if (t.isMark(">>")) depth -= 2
            else if (t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.LBRACE) return null
            if (depth <= 0) return code.subList(nameIndex + 1, i + 1).joinToString("") { it.text }
        }
        return null
    }

    private fun isFunctionInScope(name: String) = scope.modules.any { m -> index.modules[m]?.functions?.any { it.name == name } == true }

    // ------------------------------------------------------------------ the first segment

    /**
     * What a group in parentheses stands for: `(apple())`, `(owner.title)`, `((owner))`, `(owner as item)`,
     * `(maybe!!)`. A chain inside is evaluated like any chain; `value as type` IS the type. Anything
     * else (`a + b`, a condition) has no answer here.
     */
    private fun groupValue(tokens: List<Tok>, depth: Int): Value? {
        if (depth > 16) return null
        var inner = tokens
        while (inner.isNotEmpty() && inner.last().isMark("!!")) inner = inner.subList(0, inner.size - 1)
        if (inner.isEmpty()) return null
        // `value as type` at the top level of the group: the type after the last `as`
        var level = 0
        var castAt = -1
        for (i in inner.indices) {
            when (inner[i].type) {
                SharpTokenKinds.LPAREN, SharpTokenKinds.LBRACKET, SharpTokenKinds.LBRACE -> level++
                SharpTokenKinds.RPAREN, SharpTokenKinds.RBRACKET, SharpTokenKinds.RBRACE -> level--
            }
            if (level == 0 && inner[i].isKeyword("as")) castAt = i
        }
        if (castAt >= 0) {
            val written = inner.subList(castAt + 1, inner.size).joinToString("") { it.text }.trimEnd('?', '!')
            if (written.isEmpty()) return null
            return lookup.resolveTypeName(written, scope, place.typeParameters)?.let { Value.Instance(it) }
        }
        val segments = ChainParser.parseWhole(inner) ?: return null
        return evaluate(segments, depth + 1)
    }

    private fun first(s: Segment, depth: Int = 0): Value? {
        if (s.group != null) return groupValue(s.group, depth)?.let { applyIndexes(it, s.indexed) }
        // `new outer.inner()`: the name after `new` is where a class is looked for; `inner(...)` then makes the object
        if (s.isNew && !s.called) return lookup.findClass(s.name, scope)?.let { Value.StaticOf(it) }
        if (s.isStringLiteral) return index.classesById["std#string"]?.let { applyIndexes(Value.Instance(lookup.typeOfClass(it)), s.indexed) }
        if (s.qualifier != null) return firstInModule(s)
        if (!s.called && !s.isNew) {
            when (s.name) {
                "self" -> return place.enclosingClass?.takeIf { !place.isStatic }?.let { applyIndexes(Value.Instance(lookup.typeOfClass(it)), s.indexed) }
                "base" -> return place.enclosingClass?.let { index.baseOf(it) }?.let { applyIndexes(Value.Instance(lookup.typeOfClass(it)), s.indexed) }
            }
            localType(s.name)?.let { return applyIndexes(Value.Instance(it), s.indexed) }
            if (s.name in textLocals) return null                               // a local with no known type: no guessing
            memberOfEnclosingClass(s)?.let { return applyIndexes(it, s.indexed) }
            globalField(s.name)?.let { return applyIndexes(Value.Instance(it), s.indexed) }
            lookup.findClass(s.name, scope)?.let { return Value.StaticOf(it) }
            return null
        }
        if (s.isNew || isClassCall(s)) {
            val written = s.name + (s.genericText ?: "")
            val type = lookup.resolveTypeName(written, scope, place.typeParameters) ?: return null
            if (index.classOf(type) == null) return null
            return applyIndexes(Value.Instance(type), s.indexed)
        }
        memberOfEnclosingClass(s)?.let { return applyIndexes(it, s.indexed) }
        globalFunctionResult(s.name, s.argumentCount)?.let { return applyIndexes(Value.Instance(it), s.indexed) }
        return null
    }

    private fun isClassCall(s: Segment) = s.called && memberNamed(s.name) == null && lookup.findClass(s.name, scope) != null

    private fun localType(name: String): SharpType? {
        place.locals.lastOrNull { it.name == name }?.let { return it.type ?: textLocals[name]?.type }
        return textLocals[name]?.type
    }

    private fun memberNamed(name: String): MemberEntry? {
        val c = place.enclosingClass ?: return null
        return lookup.membersOfClass(c, emptyList(), MemberFilter.EVERYTHING, c).firstOrNull { it.declaration.name == name }
    }

    private fun memberOfEnclosingClass(s: Segment): Value? {
        val c = place.enclosingClass ?: return null
        val all = lookup.membersOfClass(c, emptyList(), MemberFilter.EVERYTHING, c)
        return valueOfMember(all, s)
    }

    private fun globalField(name: String): SharpType? {
        for (m in scope.modules) index.modules[m]?.globals?.firstOrNull { it.name == name }?.let { return it.type }
        return null
    }

    private fun globalFunctionResult(name: String, argumentCount: Int): SharpType? {
        for (m in scope.modules) {
            val candidates = index.modules[m]?.functions?.filter { it.name == name } ?: continue
            if (candidates.isEmpty()) continue
            return (candidates.firstOrNull { it.parameters.size == argumentCount } ?: candidates.first()).result
        }
        return null
    }

    private fun firstInModule(s: Segment): Value? {
        val modules = scope.groups[s.qualifier] ?: listOf(s.qualifier!!)
        if (!s.called) {
            for (m in modules) {
                lookup.classInModule(m, s.name)?.let { return Value.StaticOf(it) }
                index.modules[m]?.globals?.firstOrNull { it.name == s.name }?.type?.let { return applyIndexes(Value.Instance(it), s.indexed) }
            }
            return null
        }
        for (m in modules) {
            val candidates = index.modules[m]?.functions?.filter { it.name == s.name } ?: continue
            if (candidates.isNotEmpty()) {
                val f = candidates.firstOrNull { it.parameters.size == s.argumentCount } ?: candidates.first()
                return f.result?.let { applyIndexes(Value.Instance(it), s.indexed) }
            }
            lookup.classInModule(m, s.name)?.let { return applyIndexes(Value.Instance(lookup.typeOfClass(it)), s.indexed) }
        }
        return null
    }

    // ------------------------------------------------------------------ the next segments

    private fun step(current: Value, s: Segment): Value? {
        val entries = when (current) {
            is Value.Instance -> lookup.membersOf(current.type, MemberFilter.INSTANCE, place.enclosingClass)
            is Value.StaticOf -> lookup.membersOfClass(current.c, emptyList(), MemberFilter.STATIC, place.enclosingClass)
        }
        return valueOfMember(entries, s)?.let { applyIndexes(it, s.indexed) }
    }

    /** The value of the member the segment names, among `entries`. */
    private fun valueOfMember(entries: List<MemberEntry>, s: Segment): Value? {
        val named = entries.filter { it.declaration.name == s.name }
        if (named.isEmpty()) return null
        if (s.called) {
            val functions = named.filter { it.declaration is FunctionInfo }
            val chosen = functions.firstOrNull { (it.declaration as FunctionInfo).parameters.size == s.argumentCount } ?: functions.firstOrNull()
            if (chosen == null) {
                // `outer.inner(...)`: no function of that name, but a class declared inside - an object of it
                val made = named.firstOrNull { it.declaration is ClassInfo } ?: return null
                return Value.Instance(lookup.typeOfClass(made.declaration as ClassInfo))
            }
            return chosen.type?.let { Value.Instance(it) }
        }
        val entry = named.firstOrNull { it.declaration is FieldInfo || it.declaration is sharp.ide.index.EnumValueInfo }
        if (entry != null) return entry.type?.let { Value.Instance(it) }
        val nested = named.firstOrNull { it.declaration is ClassInfo }
        if (nested != null) return Value.StaticOf(nested.declaration as ClassInfo)
        return null
    }

    private fun applyIndexes(value: Value, count: Int): Value? {
        var v: Value = value
        repeat(count) {
            val type = (v as? Value.Instance)?.type ?: return null
            v = if (type.isArray) Value.Instance(type.element ?: return null)
            else {
                val operator = lookup.membersOf(type, MemberFilter.INSTANCE, place.enclosingClass)
                    .firstOrNull { it.declaration.name == "operator[]" } ?: return null
                Value.Instance(operator.type ?: return null)
            }
        }
        return v
    }
}
