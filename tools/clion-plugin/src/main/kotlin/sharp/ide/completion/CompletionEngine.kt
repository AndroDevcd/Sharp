/*
 * CompletionEngine - decides WHAT to offer at the caret. No platform object in here.
 *
 * WHAT THIS IS FOR
 *   (doc/ide.md 6.8.) Given a FileView (the index, the text the compiler saw, the text now)
 *   and a caret offset it returns the list of Candidates - name, kind, type text, parameters,
 *   how to insert, how to rank - which SharpCompletionContributor turns into the platform's
 *   lookup items. Keeping the decision here means every situation of the design review is a test
 *   on plain strings.
 *
 *   The pieces: CompletionSite says what kind of place the caret is in; ReceiverResolver finds the
 *   type before a dot; TextLocals finds variables typed since the last compiler run; TypeLookup
 *   lists the members of a type with the type arguments put in.
 *
 *   RANKING (higher first): locals and parameters 100; members of the class itself 90; of a base
 *   class 80, one less per step up the chain; of an interface 75; globals of the file's own module
 *   65; globals of imported modules 60; types 55; keywords 40; classes of modules that are NOT
 *   imported 35 (choosing one adds the import); what `std#_object_` gives every class 30.
 *
 *   Everything degrades quietly: no index, no snapshot, a place the index does not know - fewer
 *   items, never an exception.
 */
package sharp.ide.completion

import sharp.ide.editor.FileView
import sharp.ide.editor.PlaceInfo
import sharp.ide.index.AliasInfo
import sharp.ide.index.ClassInfo
import sharp.ide.index.Declaration
import sharp.ide.index.EnumValueInfo
import sharp.ide.index.FieldInfo
import sharp.ide.index.FunctionInfo
import sharp.ide.index.MemberEntry
import sharp.ide.index.MemberFilter
import sharp.ide.index.RunIndex
import sharp.ide.index.Scope
import sharp.ide.index.SharpType
import sharp.ide.lexer.CodeTokens
import sharp.ide.lexer.SharpKeywords
import sharp.ide.lexer.SharpTokenKinds

enum class CandidateKind { LOCAL, PARAMETER, FIELD, PROPERTY, METHOD, CLASS, INTERFACE, ENUM, ENUM_VALUE, ALIAS, MODULE, KEYWORD, TYPE_WORD, TYPE_PARAMETER }

/** How choosing the item changes the text besides the name. */
enum class Insertion { NONE, CALL, NEW }

class Candidate(
    val name: String,
    val kind: CandidateKind,
    /** The member's type or a function's result, as text; null when unknown. */
    val typeText: String?,
    /** For a function or constructor: its parameters `(a: int, b: string)`; for a class not imported: its module. */
    val tailText: String?,
    val priority: Int,
    /** Declared in the class itself (shown in bold). */
    val bold: Boolean = false,
    val insertion: Insertion = Insertion.NONE,
    val hasParameters: Boolean = false,
    /** The module to add to the file's imports when this item is chosen. */
    val importModule: String? = null,
)

class CompletionResult(val site: CompletionSite, val candidates: List<Candidate>) {
    val prefix: String get() = site.prefix
}

object CompletionEngine {

    /** The keywords offered as plain words: every keyword (the type words are offered as types). */
    private val keywordList: List<String> by lazy { SharpKeywords.all.sorted() }

    fun complete(view: FileView?, text: String, caret: Int): CompletionResult {
        val tokens = CodeTokens.scan(text)
        val site = CompletionSite.of(text, caret, tokens) { name -> view?.index?.classesByName?.containsKey(name) == true }
        if (site.kind == SiteKind.NOWHERE) return CompletionResult(site, emptyList())
        if (view == null) {
            // no index yet: only the words of the language
            return CompletionResult(site, if (site.kind == SiteKind.PLAIN) keywords() else emptyList())
        }
        val place = view.placeAt(caret)
        val scope = view.lookup.scopeFor(view.file, text)
        val items = when (site.kind) {
            SiteKind.MODULE_NAME -> moduleNames(view)
            SiteKind.MODULE_MEMBER -> moduleMembers(view, site, scope)
            SiteKind.MEMBER -> members(view, site, place, scope, tokens)
            SiteKind.TYPE -> types(view, place, scope, classesOnly = false, text = text)
            SiteKind.TYPE_NEW -> types(view, place, scope, classesOnly = true, text = text)
            SiteKind.TYPE_OR_NAME -> types(view, place, scope, classesOnly = false, text = text) + plainNames(view, site, place, scope, tokens, text, caret)
            SiteKind.PLAIN -> plainNames(view, site, place, scope, tokens, text, caret)
            SiteKind.NOWHERE -> emptyList()
        }
        return CompletionResult(site, items)
    }

    /** A receiver reader for the place at `caret`: scope from the index, locals from the index and the text. */
    fun resolverAt(view: FileView, tokens: CodeTokens, caret: Int): ReceiverResolver {
        val place = view.placeAt(caret)
        val scope = view.lookup.scopeFor(view.file, view.text)
        return ReceiverResolver(view, place, localsFromText(view, place, scope, tokens.code, caret), scope)
    }

    // ------------------------------------------------------------------ module names

    private fun moduleNames(view: FileView): List<Candidate> =
        view.lookup.moduleNames().map { Candidate(it, CandidateKind.MODULE, null, null, 60) }

    private fun moduleMembers(view: FileView, site: CompletionSite, scope: Scope): List<Candidate> {
        val q = site.qualifier ?: return emptyList()
        val modules = scope.groups[q] ?: listOf(q)
        val result = ArrayList<Candidate>()
        for (m in modules) {
            val module = view.index.modules[m] ?: continue
            module.classes.filter { !it.hasFlag("private") }.forEach { result.add(classCandidate(it, 65, importModule = null, asNew = false)) }
            module.globals.filter { !it.hasFlag("private") }.forEach { result.add(fieldCandidate(it, it.type, 65, false)) }
            module.functions.filter { !it.hasFlag("private") }.forEach { result.add(functionCandidate(it, it.result, it.signature, 65, false)) }
        }
        return result
    }

    // ------------------------------------------------------------------ members after a dot

    private fun members(view: FileView, site: CompletionSite, place: PlaceInfo, scope: Scope, tokens: CodeTokens): List<Candidate> {
        val code = tokens.code
        val dotIndex = site.before.size - 1                     // `before` is a prefix of `code`
        val receiver = receiverBefore(view, place, scope, code, dotIndex, tokens, site) ?: return emptyList()
        val from = place.enclosingClass
        val entries = when {
            receiver.staticClass != null -> view.lookup.membersOfClass(receiver.staticClass, emptyList(), MemberFilter.STATIC, from)
            receiver.type != null -> view.lookup.membersOf(receiver.type, MemberFilter.INSTANCE, from)
            else -> emptyList()
        }
        return entries.mapNotNull { memberCandidate(it) }
    }

    private fun receiverBefore(
        view: FileView, place: PlaceInfo, scope: Scope, code: List<sharp.ide.lexer.Tok>, dotIndex: Int, tokens: CodeTokens, site: CompletionSite,
    ): Receiver? {
        val locals = localsFromText(view, place, scope, code, site.prefixStart)
        val resolver = ReceiverResolver(view, place, locals, scope)
        // `self->` and `base->`: the receiver is the word before the arrow
        val endOfReceiver = dotIndex
        resolver.resolveBefore(code, endOfReceiver)?.let { return it }
        return receiverFromUses(view, code, endOfReceiver)
    }

    /** Step 2 of the receiver reader: the compiler's own answer for the name before the dot, when it existed at the last run. */
    private fun receiverFromUses(view: FileView, code: List<sharp.ide.lexer.Tok>, endExclusive: Int): Receiver? {
        val bodies = view.bodies ?: return null
        var nameToken = code.getOrNull(endExclusive - 1) ?: return null
        if (nameToken.type == SharpTokenKinds.RPAREN) {
            val open = ChainParser.matchBack(code, endExclusive - 1) ?: return null
            nameToken = code.getOrNull(open - 1) ?: return null
        }
        if (!nameToken.isIdentifier) return null
        val old = view.toSnapshot(nameToken.start) ?: return null
        val use = bodies.useAt(old) ?: return null
        val type = use.type ?: return null
        return if (use.kind == 1) view.index.classOf(type)?.let { Receiver(null, it) } else Receiver(type, null)
    }

    private fun localsFromText(view: FileView, place: PlaceInfo, scope: Scope, code: List<sharp.ide.lexer.Tok>, caret: Int): Map<String, TextLocal> {
        val from = functionStart(view, place, code, caret)
        // the receiver reader used for the types of `name := expression` needs the locals found so far: a two-pass read
        var resolver: ReceiverResolver? = null
        val first = TextLocals.read(view, place, code, from, caret) { expression -> resolver?.typeOfExpression(expression) }
        resolver = ReceiverResolver(view, place, first, scope)
        return TextLocals.read(view, place, code, from, caret) { expression -> resolver.typeOfExpression(expression) }
    }

    /** Where the text of the function around the caret starts, in the CURRENT text. */
    private fun functionStart(view: FileView, place: PlaceInfo, code: List<sharp.ide.lexer.Tok>, caret: Int): Int {
        place.function?.let { f ->
            view.toCurrent(f.body.first)?.let { if (it <= caret) return it }
        }
        // no index for this place: the nearest `def` before the caret, or the start of the file
        return code.lastOrNull { it.end <= caret && it.isKeyword("def") }?.start ?: 0
    }

    private fun memberCandidate(entry: MemberEntry): Candidate? {
        val priority = memberPriority(entry)
        val bold = entry.distance == 0
        return when (val d = entry.declaration) {
            is FunctionInfo -> if (d.isConstructor || d.isOperator) null else functionCandidate(d, entry.type, entry.signature ?: d.signature, priority, bold)
            is FieldInfo -> fieldCandidate(d, entry.type, priority, bold)
            is EnumValueInfo -> Candidate(d.name, CandidateKind.ENUM_VALUE, entry.type?.text, null, priority, bold)
            is ClassInfo -> classCandidate(d, priority, null, false)
            else -> null
        }
    }

    private fun memberPriority(entry: MemberEntry): Int = when {
        entry.declaredIn.id == RunIndex.OBJECT_CLASS && entry.distance > 0 -> 30
        entry.distance == 0 -> 90
        entry.viaInterface -> 75
        else -> (80 - (entry.distance - 1)).coerceAtLeast(50)
    }

    // ------------------------------------------------------------------ types

    private fun types(view: FileView, place: PlaceInfo, scope: Scope, classesOnly: Boolean, text: String): List<Candidate> {
        val result = ArrayList<Candidate>()
        val seen = HashSet<String>()
        for (c in view.lookup.visibleClasses(scope)) {
            if (c.hasFlag("private")) continue
            if (classesOnly && c.kind != "class") continue
            if (seen.add(c.id)) result.add(classCandidate(c, 55, null, classesOnly))
        }
        if (!classesOnly) {
            view.lookup.visibleAliases(scope).forEach { result.add(Candidate(it.name, CandidateKind.ALIAS, it.target?.text, null, 55)) }
            view.lookup.builtInTypeWords.forEach { result.add(Candidate(it, CandidateKind.TYPE_WORD, null, null, 55)) }
            place.typeParameters.forEach { result.add(Candidate(it, CandidateKind.TYPE_PARAMETER, null, null, 58)) }
        }
        result.addAll(classesOfOtherModules(view, scope, classesOnly, seen))
        return result
    }

    /** Classes of modules the file does not import, each with the import that choosing it adds. */
    private fun classesOfOtherModules(view: FileView, scope: Scope, classesOnly: Boolean, seen: MutableSet<String>): List<Candidate> {
        val result = ArrayList<Candidate>()
        for ((name, module) in view.index.modules) {
            if (name in scope.modules || name.isEmpty()) continue
            for (c in module.classes) {
                if (c.hasFlag("private") || (classesOnly && c.kind != "class")) continue
                if (seen.add(c.id)) result.add(classCandidate(c, 35, importModule = name, asNew = classesOnly))
            }
        }
        return result
    }

    // ------------------------------------------------------------------ plain names

    private fun plainNames(
        view: FileView, site: CompletionSite, place: PlaceInfo, scope: Scope, tokens: CodeTokens, text: String, caret: Int,
    ): List<Candidate> {
        val result = ArrayList<Candidate>()
        val seen = HashSet<String>()
        fun add(c: Candidate) { if (seen.add(c.kind.name + ":" + c.name)) result.add(c) }

        // locals and parameters in scope (the index), then the ones typed since the last run
        for (l in place.locals.asReversed()) {
            val kind = if (l.kind == "parameter" || l.kind == "lambda_parameter") CandidateKind.PARAMETER else CandidateKind.LOCAL
            add(Candidate(l.name, kind, l.type?.text, null, 100))
        }
        val code = tokens.code
        val locals = localsFromText(view, place, scope, code, site.prefixStart)
        for (l in locals.values) {
            if (place.locals.any { it.name == l.name }) continue
            add(Candidate(l.name, CandidateKind.LOCAL, l.type?.text, null, 100))
        }

        // fields and functions of the class around the caret, inherited ones too
        place.enclosingClass?.let { c ->
            val entries = view.lookup.membersOfClass(c, emptyList(), MemberFilter.EVERYTHING, c)
            for (e in entries) {
                if (place.isStatic && !e.declaration.isStatic && e.declaration !is ClassInfo) continue
                memberCandidate(e)?.let { add(it) }
            }
        }

        // globals and functions of the file's module, then of the modules it imports
        for ((i, module) in scope.modules.withIndex()) {
            val m = view.index.modules[module] ?: continue
            val priority = if (i == 0) 65 else 60
            m.globals.filter { !it.hasFlag("private") || i == 0 }.forEach { add(fieldCandidate(it, it.type, priority, false)) }
            m.functions.filter { !it.hasFlag("private") || i == 0 }.forEach { add(functionCandidate(it, it.result, it.signature, priority, false)) }
        }

        // classes (for `Type.static_member` and for calls)
        for (c in view.lookup.visibleClasses(scope)) if (!c.hasFlag("private")) add(classCandidate(c, 55, null, false))
        view.lookup.visibleAliases(scope).forEach { add(Candidate(it.name, CandidateKind.ALIAS, it.target?.text, null, 55)) }
        view.lookup.builtInTypeWords.forEach { add(Candidate(it, CandidateKind.TYPE_WORD, null, null, 45)) }
        if (site.prefix.isNotEmpty()) {
            for (c in classesOfOtherModules(view, scope, false, HashSet())) add(c)
        }

        keywords().forEach { add(it) }
        return result
    }

    private fun keywords(): List<Candidate> = keywordList.map { Candidate(it, CandidateKind.KEYWORD, null, null, 40) }

    // ------------------------------------------------------------------ one item of each kind

    private fun fieldCandidate(f: FieldInfo, type: SharpType?, priority: Int, bold: Boolean) =
        Candidate(f.name, if (f.hasGetter || f.hasSetter) CandidateKind.PROPERTY else CandidateKind.FIELD, type?.text, null, priority, bold)

    private fun functionCandidate(f: FunctionInfo, result: SharpType?, signature: String, priority: Int, bold: Boolean) =
        Candidate(
            f.name, CandidateKind.METHOD, result?.text, parametersOf(signature), priority, bold,
            insertion = Insertion.CALL, hasParameters = f.parameters.isNotEmpty(),
        )

    private fun classCandidate(c: ClassInfo, priority: Int, importModule: String?, asNew: Boolean): Candidate {
        val kind = when (c.kind) { "interface" -> CandidateKind.INTERFACE; "enum" -> CandidateKind.ENUM; else -> CandidateKind.CLASS }
        return Candidate(
            c.name, kind, null, if (importModule != null) "  $importModule" else null, priority,
            insertion = if (asNew) Insertion.NEW else Insertion.NONE,
            hasParameters = asNew && c.constructors.any { it.parameters.isNotEmpty() },
            importModule = importModule,
        )
    }

    /** `at(index: int) : T` -> `(index: int)`. */
    fun parametersOf(signature: String): String {
        val open = signature.indexOf('(')
        if (open < 0) return "()"
        var depth = 0
        for (i in open until signature.length) {
            if (signature[i] == '(') depth++
            else if (signature[i] == ')') { depth--; if (depth == 0) return signature.substring(open, i + 1) }
        }
        return signature.substring(open)
    }
}
