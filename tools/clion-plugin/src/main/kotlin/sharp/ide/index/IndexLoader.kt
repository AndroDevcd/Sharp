/*
 * IndexLoader - reads a finished run folder into a RunIndex.
 *
 * WHAT THIS IS FOR
 *   After a run the folder holds ide.json, types.json, declarations.json and a bodies file
 *   for each open file (doc/ide.md section 4). This turns them into the immutable objects of
 *   DeclarationModel / FileBodies. The rules are the ones DiagnosticsReader follows: every
 *   file says `"format": 1` and carries the manifest's `run`, otherwise the whole run is
 *   refused with a reason in words (a leftover of an older run, or a compiler of another
 *   version).
 *
 *   The bodies files are read NOW, not on first use: the folder may be deleted by the time a
 *   feature asks (RunFolders prunes old runs), and only open files have one, so it is small.
 *
 *   A missing type (`-1`, an id out of range) is a null type, never an error: the compiler
 *   writes "unknown" that way. A file that is JSON but not in the expected shape is refused
 *   with a reason, like every other problem (`wellShaped` in DiagnosticsModel.kt).
 */
package sharp.ide.index

import com.google.gson.JsonArray
import com.google.gson.JsonElement
import com.google.gson.JsonObject
import com.google.gson.JsonParser
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path

object IndexLoader {

    /** Reads the index of `folder`, whose manifest was already read. Throws RunReadException when a file is wrong. */
    fun load(folder: Path, manifest: RunManifest): RunIndex {
        if (manifest.level == "syntax-errors") throw RunReadException("a run that stopped at syntax errors has no index")
        val types = TypeTable(wellShaped("types.json") { readTypes(folder.resolve("types.json"), manifest.run) })
        val declarations = wellShaped("declarations.json") { readDeclarations(folder.resolve("declarations.json"), manifest, types) }
        val bodies = HashMap<Int, FileBodies>()
        for ((number, file) in manifest.files.withIndex()) {
            val name = file.bodies ?: continue
            val path = folder.resolve(name)
            if (!Files.isRegularFile(path)) continue
            bodies[number] = wellShaped(name) { readBodies(path, manifest.run, number, types) }
        }
        return RunIndex(
            run = manifest.run, level = manifest.level, files = manifest.files, types = types,
            modules = declarations.modules, classesById = declarations.classes,
            declarations = declarations.byIdentity, bodies = bodies,
        )
    }

    // ------------------------------------------------------------------ types.json

    private fun readTypes(file: Path, run: String): List<TypeTable.TypeRow> {
        val root = readRoot(file, "types.json", run)
        return array(root, "types").map { element ->
            val t = element.asJsonObject
            TypeTable.TypeRow(
                text = t.get("text")?.asString ?: "?",
                classId = t.get("class")?.takeIf { it.isJsonPrimitive }?.asString,
                args = (t.get("args")?.takeIf { it.isJsonArray }?.asJsonArray ?: JsonArray()).map { it.asInt },
                isArray = flag(t, "array"),
                element = t.get("element")?.takeIf { it.isJsonPrimitive }?.asInt ?: -1,
                nullable = flag(t, "nullable"),
                isParam = flag(t, "param"),
                isFunction = flag(t, "function"),
            )
        }
    }

    // ------------------------------------------------------------------ declarations.json

    private class Declared(
        val modules: Map<String, ModuleInfo>,
        val classes: Map<String, ClassInfo>,
        val byIdentity: Map<Pair<Int, Int>, Declaration>,
    )

    /** The state of one reading of declarations.json: the maps being filled and the table to look types up in. */
    private class Reading(val types: TypeTable) {
        val classes = LinkedHashMap<String, ClassInfo>()
        val byIdentity = HashMap<Pair<Int, Int>, Declaration>()

        fun register(declaration: Declaration) {
            byIdentity.putIfAbsent(declaration.identity, declaration)
        }
    }

    private fun readDeclarations(file: Path, manifest: RunManifest, types: TypeTable): Declared {
        val root = readRoot(file, "declarations.json", manifest.run)
        val reading = Reading(types)
        val modules = LinkedHashMap<String, ModuleInfo>()
        for (element in array(root, "modules")) {
            val m = element.asJsonObject
            val name = m.get("name")?.asString ?: ""
            val module = ModuleInfo(name, ints(m, "files"))
            val defaultFile = module.files.firstOrNull() ?: -1
            for (c in array(m, "classes")) module.classes.add(readClass(c.asJsonObject, null, name, defaultFile, reading))
            for (f in array(m, "globals")) module.globals.add(readField(f.asJsonObject, null, name, defaultFile, reading))
            for (f in array(m, "functions")) module.functions.add(readFunction(f.asJsonObject, null, name, defaultFile, reading, false))
            for (a in array(m, "aliases")) module.aliases.add(readAlias(a.asJsonObject, null, name, defaultFile, reading))
            modules[name] = module
        }
        return Declared(modules, reading.classes, reading.byIdentity)
    }

    private fun readClass(c: JsonObject, owner: ClassInfo?, module: String, defaultFile: Int, r: Reading): ClassInfo {
        val file = c.get("file")?.asInt ?: defaultFile
        val node = c.get("node")?.asInt ?: -1
        val info = ClassInfo(
            id = c.get("id")?.asString ?: "?",
            name = c.get("name")?.asString ?: "?",
            kind = c.get("kind")?.asString ?: "class",
            flags = c.get("flags")?.asString ?: "",
            file = file, node = node, at = place(c.get("at")),
            owner = owner, module = module,
            body = c.get("body")?.takeIf { it.isJsonArray }?.asJsonArray?.let { it.get(0).asInt..it.get(1).asInt },
            typeParameters = array(c, "tparams").map { readTypeParameter(it.asJsonObject, null, module, file, r) },
            base = c.get("base")?.asInt?.let { r.types.typeOf(it) },
            interfaces = ints(c, "interfaces").mapNotNull { r.types.typeOf(it) },
        )
        r.classes.putIfAbsent(info.id, info)
        r.register(info)
        info.typeParameters.forEach(r::register)
        for (v in array(c, "values")) {
            val o = v.asJsonObject
            val value = EnumValueInfo(
                o.get("name")?.asString ?: "?", file, o.get("node")?.asInt ?: -1, place(o.get("at")), info, module,
                o.get("number")?.takeIf { it.isJsonPrimitive }?.asLong,
            )
            info.values.add(value)
            r.register(value)
        }
        for (f in array(c, "fields")) info.fields.add(readField(f.asJsonObject, info, module, file, r))
        for (f in array(c, "functions")) info.functions.add(readFunction(f.asJsonObject, info, module, file, r, false))
        for (f in array(c, "constructors")) info.constructors.add(readFunction(f.asJsonObject, info, module, file, r, true))
        for (n in array(c, "classes")) info.nested.add(readClass(n.asJsonObject, info, module, file, r))
        for (a in array(c, "aliases")) info.aliases.add(readAlias(a.asJsonObject, info, module, file, r))
        return info
    }

    private fun readTypeParameter(o: JsonObject, owner: ClassInfo?, module: String, file: Int, r: Reading) = TypeParameterInfo(
        o.get("name")?.asString ?: "?", file, o.get("node")?.asInt ?: -1, place(o.get("at")), owner, module,
        o.get("bound")?.takeIf { it.isJsonPrimitive }?.asInt?.let { r.types.typeOf(it) },
    )

    private fun readField(o: JsonObject, owner: ClassInfo?, module: String, defaultFile: Int, r: Reading): FieldInfo {
        val field = FieldInfo(
            name = o.get("name")?.asString ?: "?", flags = o.get("flags")?.asString ?: "",
            file = o.get("file")?.asInt ?: defaultFile, node = o.get("node")?.asInt ?: -1, at = place(o.get("at")),
            owner = owner, module = module,
            type = o.get("type")?.takeIf { it.isJsonPrimitive }?.asInt?.let { r.types.typeOf(it) },
            hasGetter = flag(o, "getter"), hasSetter = flag(o, "setter"),
        )
        r.register(field)
        return field
    }

    private fun readFunction(o: JsonObject, owner: ClassInfo?, module: String, defaultFile: Int, r: Reading, constructor: Boolean): FunctionInfo {
        val file = o.get("file")?.asInt ?: defaultFile
        val function = FunctionInfo(
            name = o.get("name")?.asString ?: "?", flags = o.get("flags")?.asString ?: "",
            file = file, node = o.get("node")?.asInt ?: -1, at = place(o.get("at")),
            owner = owner, module = module,
            signature = o.get("sig")?.asString ?: (o.get("name")?.asString ?: "?"),
            parameters = array(o, "params").map {
                val p = it.asJsonObject
                ParameterInfo(p.get("name")?.asString ?: "?", p.get("type")?.takeIf { t -> t.isJsonPrimitive }?.asInt?.let { t -> r.types.typeOf(t) })
            },
            result = o.get("result")?.takeIf { it.isJsonPrimitive }?.asInt?.let { r.types.typeOf(it) },
            typeParameters = array(o, "tparams").map { readTypeParameter(it.asJsonObject, owner, module, file, r) },
            isOperator = flag(o, "operator"), isExtension = flag(o, "extension"), isConstructor = constructor,
        )
        r.register(function)
        function.typeParameters.forEach(r::register)
        return function
    }

    private fun readAlias(o: JsonObject, owner: ClassInfo?, module: String, defaultFile: Int, r: Reading): AliasInfo {
        val alias = AliasInfo(
            o.get("name")?.asString ?: "?", o.get("file")?.asInt ?: defaultFile, o.get("node")?.asInt ?: -1,
            place(o.get("at")), owner, module, o.get("type")?.takeIf { it.isJsonPrimitive }?.asInt?.let { r.types.typeOf(it) },
        )
        r.register(alias)
        return alias
    }

    // ------------------------------------------------------------------ bodies/<n>.json

    private fun readBodies(file: Path, run: String, expectedFile: Int, types: TypeTable): FileBodies {
        val root = readRoot(file, file.fileName.toString(), run)
        val functions = array(root, "functions").map {
            val f = it.asJsonObject
            val body = f.get("body").asJsonArray
            BodyFunction(
                name = f.get("name")?.asString ?: "?", classId = f.get("class")?.takeIf { c -> c.isJsonPrimitive }?.asString,
                isStatic = flag(f, "static"), isLambda = flag(f, "lambda"), node = f.get("node")?.asInt ?: -1,
                body = body.get(0).asInt..body.get(1).asInt, enclosing = f.get("in")?.takeIf { c -> c.isJsonPrimitive }?.asInt ?: -1,
                instance = f.get("instance")?.takeIf { c -> c.isJsonPrimitive }?.asString,
            )
        }
        val locals = array(root, "locals").map {
            val l = it.asJsonObject
            val decl = l.get("decl").asJsonArray
            val visible = l.get("visible").asJsonArray
            LocalInfo(
                name = l.get("name")?.asString ?: "?", type = l.get("type")?.takeIf { t -> t.isJsonPrimitive }?.asInt?.let { t -> types.typeOf(t) },
                kind = l.get("kind")?.asString ?: "local", isConst = flag(l, "const"), function = l.get("function")?.asInt ?: -1,
                declFrom = decl.get(0).asInt, declTo = decl.get(1).asInt, visible = visible.get(0).asInt..visible.get(1).asInt,
            )
        }
        val rows = array(root, "uses").map { it.asJsonArray }.sortedBy { it.get(0).asInt }
        val n = rows.size
        val from = IntArray(n); val to = IntArray(n); val kind = IntArray(n); val fileNo = IntArray(n); val node = IntArray(n)
        val type = arrayOfNulls<SharpType>(n)
        for ((i, row) in rows.withIndex()) {
            from[i] = row.get(0).asInt; to[i] = row.get(1).asInt; kind[i] = row.get(2).asInt
            fileNo[i] = row.get(3).asInt; node[i] = row.get(4).asInt; type[i] = types.typeOf(row.get(5).asInt)
        }
        val lines = ints(root, "lines").toIntArray()
        return FileBodies(root.get("file")?.asInt ?: expectedFile, lines, functions, locals, from, to, kind, fileNo, node, type)
    }

    // ------------------------------------------------------------------ small helpers

    private fun readRoot(file: Path, what: String, run: String): JsonObject {
        if (!Files.isRegularFile(file)) throw RunReadException("the compiler wrote no $what in ${file.parent}")
        val text = try {
            String(Files.readAllBytes(file), StandardCharsets.UTF_8)
        } catch (e: java.io.IOException) {
            throw RunReadException("cannot read $what: ${e.message}")
        }
        val parsed: JsonElement = try {
            JsonParser.parseString(text)
        } catch (e: Exception) {
            throw RunReadException("$what is not valid JSON: ${e.message?.lineSequence()?.firstOrNull() ?: e.javaClass.simpleName}")
        }
        if (!parsed.isJsonObject) throw RunReadException("$what is not a JSON object")
        val root = parsed.asJsonObject
        val format = root.get("format")?.takeIf { it.isJsonPrimitive }?.asInt ?: throw RunReadException("$what has no format number")
        if (format != DiagnosticsReader.FORMAT) throw RunReadException("$what has format $format, this plugin reads format ${DiagnosticsReader.FORMAT}")
        val its = root.get("run")?.asString
        if (its != run) throw RunReadException("$what belongs to run $its, not to run $run (a leftover of an earlier run)")
        return root
    }

    private fun array(o: JsonObject, name: String): JsonArray = o.get(name)?.takeIf { it.isJsonArray }?.asJsonArray ?: JsonArray()

    private fun ints(o: JsonObject, name: String): List<Int> = array(o, name).map { it.asInt }

    private fun flag(o: JsonObject, name: String): Boolean = o.get(name)?.takeIf { it.isJsonPrimitive }?.asBoolean ?: false

    private fun place(e: JsonElement?): NamePlace {
        val a = e?.takeIf { it.isJsonArray }?.asJsonArray ?: return NamePlace(0, 0, 0, 0)
        return NamePlace(a.get(0).asInt, a.get(1).asInt, a.get(2).asInt, a.get(3).asInt)
    }
}
