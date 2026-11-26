/*
 * DiagnosticsReader - reads a finished run folder (ide.json + diagnostics.json).
 *
 * WHAT THIS IS FOR
 *   After the compiler has run, the plugin looks at the folder it wrote. The rules
 *   (doc/ide.md 3.5): the manifest `ide.json` is written LAST, so
 *     - no manifest means the run failed (and `failed.txt` may say why);
 *     - every file must say `"format": 1` and carry the manifest's `run` id, otherwise it
 *       is a leftover from another run, or from a compiler of another version.
 *   Any such problem is a RunReadException whose message is shown to the owner.
 *
 *   JSON is read with Gson's tree parser (Gson ships with the platform) rather than into
 *   classes: the files are written by a hand-made writer, so every field is checked here
 *   and a missing one gets a clear message instead of a crash.
 */
package sharp.ide.index

import com.google.gson.JsonArray
import com.google.gson.JsonElement
import com.google.gson.JsonObject
import com.google.gson.JsonParser
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path

object DiagnosticsReader {

    /** The format number of the files this plugin understands. */
    const val FORMAT = 1

    /** Reads `folder`. Throws RunReadException with a message fit for the owner when the run did not succeed. */
    fun readFolder(folder: Path): DiagnosticsRun {
        val manifestFile = folder.resolve("ide.json")
        if (!Files.isRegularFile(manifestFile)) throw RunReadException(missingManifestReason(folder))
        val manifest = parseManifest(readText(manifestFile, "ide.json"))
        val diagnosticsFile = folder.resolve("diagnostics.json")
        if (!Files.isRegularFile(diagnosticsFile)) {
            throw RunReadException("the compiler wrote ide.json but no diagnostics.json in $folder")
        }
        return parseDiagnostics(readText(diagnosticsFile, "diagnostics.json"), manifest)
    }

    /** The reason to show when there is no manifest: the compiler's own `failed.txt` when it wrote one. */
    fun missingManifestReason(folder: Path): String {
        val failed = folder.resolve("failed.txt")
        if (Files.isRegularFile(failed)) {
            val first = try {
                String(Files.readAllBytes(failed), StandardCharsets.UTF_8).lines().firstOrNull { it.isNotBlank() }
            } catch (_: java.io.IOException) {
                null
            }
            if (first != null) return "the compiler could not write its answer: ${first.trim()}"
        }
        return "the compiler wrote no ide.json (it did not finish, or it is a build without the -ide option)"
    }

    fun parseManifest(text: String): RunManifest = wellShaped("ide.json") { manifestOf(text) }

    private fun manifestOf(text: String): RunManifest {
        val root = parseObject(text, "ide.json")
        checkFormat(root, "ide.json")
        val files = root.get("files")?.takeIf { it.isJsonArray }?.asJsonArray
            ?: throw RunReadException("ide.json has no list of files")
        return RunManifest(
            run = string(root, "run", "ide.json"),
            level = string(root, "level", "ide.json"),
            errors = int(root, "errors"),
            warnings = int(root, "warnings"),
            files = files.map { element ->
                val file = element.asJsonObject
                ManifestFile(
                    path = string(file, "path", "ide.json"),
                    hash = file.get("hash")?.takeIf { it.isJsonPrimitive }?.asString,
                    size = file.get("size")?.takeIf { it.isJsonPrimitive }?.asLong ?: -1,
                    module = file.get("module")?.takeIf { it.isJsonPrimitive }?.asString,
                    library = file.get("library")?.takeIf { it.isJsonPrimitive }?.asBoolean ?: false,
                    imports = strings(file, "imports"),
                    groups = (file.get("groups")?.takeIf { it.isJsonArray }?.asJsonArray ?: JsonArray()).associate {
                        val group = it.asJsonObject
                        (group.get("name")?.asString ?: "") to strings(group, "modules")
                    },
                    bodies = file.get("bodies")?.takeIf { it.isJsonPrimitive }?.asString,
                )
            },
        )
    }

    fun parseDiagnostics(text: String, manifest: RunManifest): DiagnosticsRun =
        wellShaped("diagnostics.json") { diagnosticsOf(text, manifest) }

    private fun diagnosticsOf(text: String, manifest: RunManifest): DiagnosticsRun {
        val root = parseObject(text, "diagnostics.json")
        checkFormat(root, "diagnostics.json")
        val run = string(root, "run", "diagnostics.json")
        if (run != manifest.run) {
            throw RunReadException("diagnostics.json belongs to run $run, not to run ${manifest.run} (a leftover of an earlier run)")
        }
        val list = root.get("diagnostics")?.takeIf { it.isJsonArray }?.asJsonArray
            ?: throw RunReadException("diagnostics.json has no list of diagnostics")
        val unreadable = LinkedHashMap<String, String>()
        root.get("unreadable")?.takeIf { it.isJsonArray }?.asJsonArray?.forEach {
            val entry = it.asJsonObject
            unreadable[string(entry, "file", "diagnostics.json")] = entry.get("problem")?.asString ?: ""
        }
        return DiagnosticsRun(
            manifest = manifest,
            diagnostics = list.map { parseDiagnostic(it.asJsonObject) },
            truncated = root.get("truncated")?.asBoolean ?: false,
            unreadable = unreadable,
        )
    }

    private fun parseDiagnostic(entry: JsonObject): SharpDiagnostic = SharpDiagnostic(
        severity = if (string(entry, "severity", "a diagnostic") == "warning") DiagnosticSeverity.WARNING else DiagnosticSeverity.ERROR,
        code = string(entry, "code", "a diagnostic"),
        message = string(entry, "message", "a diagnostic"),
        place = parsePlace(entry, label = ""),
        label = entry.get("label")?.asString ?: "",
        notes = strings(entry, "notes"),
        helps = strings(entry, "helps"),
        related = (entry.get("related")?.takeIf { it.isJsonArray }?.asJsonArray ?: JsonArray())
            .map { parsePlace(it.asJsonObject, label = it.asJsonObject.get("label")?.asString ?: "") },
    )

    private fun parsePlace(entry: JsonObject, label: String): Place {
        val from = int(entry, "from")
        val to = int(entry, "to")
        return Place(
            file = string(entry, "file", "a place"),
            from = from,
            to = maxOf(from, to),
            line = int(entry, "line"),
            column = int(entry, "column"),
            endLine = int(entry, "end_line"),
            endColumn = int(entry, "end_column"),
            label = label,
        )
    }

    // ---- small helpers: every field is checked, so a malformed file gives a message, not a crash ----

    private fun parseObject(text: String, what: String): JsonObject {
        val parsed: JsonElement = try {
            JsonParser.parseString(text)
        } catch (e: Exception) {
            throw RunReadException("$what is not valid JSON: ${e.message?.lineSequence()?.firstOrNull() ?: e.javaClass.simpleName}")
        }
        if (!parsed.isJsonObject) throw RunReadException("$what is not a JSON object")
        return parsed.asJsonObject
    }

    private fun checkFormat(root: JsonObject, what: String) {
        val format = root.get("format")?.takeIf { it.isJsonPrimitive }?.asInt
            ?: throw RunReadException("$what has no format number")
        if (format != FORMAT) {
            val which = if (format > FORMAT) "the plugin is older than the compiler: update the plugin"
            else "the compiler is older than the plugin: update the compiler"
            throw RunReadException("$what has format $format, this plugin reads format $FORMAT ($which)")
        }
    }

    private fun string(entry: JsonObject, name: String, what: String): String =
        entry.get(name)?.takeIf { it.isJsonPrimitive }?.asString ?: throw RunReadException("$what has no `$name`")

    private fun int(entry: JsonObject, name: String): Int =
        entry.get(name)?.takeIf { it.isJsonPrimitive }?.asInt ?: 0

    private fun strings(entry: JsonObject, name: String): List<String> =
        entry.get(name)?.takeIf { it.isJsonArray }?.asJsonArray?.map { it.asString } ?: emptyList()

    private fun readText(file: Path, what: String): String =
        try {
            String(Files.readAllBytes(file), StandardCharsets.UTF_8)
        } catch (e: java.io.IOException) {
            throw RunReadException("cannot read $what: ${e.message}")
        }
}
