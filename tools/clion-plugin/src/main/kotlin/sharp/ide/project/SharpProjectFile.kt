/*
 * SETTINGS.JSON: a folder that is a Sharp PROJECT
 *
 * The compiler has a project mode: `sharpc -P <folder>` compiles every .sharp file of the folder
 * with the options written in the folder's `settings.json`, into `<folder>/build/outputs/<output>`
 * (lib/compiler/configuration/settings_processor.sharp lists every setting):
 *
 *     {
 *       "name": "demo",
 *       "output": "demo",
 *       "extern_lib": [ "libs/shapes" ],
 *       "ignore_folders": [ "build" ],
 *       "optimize_level": 1
 *     }
 *
 * The plugin uses that mode wherever a folder has such a file: a build IS `sharpc -P`, so what the
 * IDE builds is what the command line builds; the checks after a save leave out the same folders
 * and add the same libraries; and the folder is marked in the Project view.
 *
 * `settings.json` is a common name. A file counts as Sharp's when it is a JSON object with at least
 * one setting the compiler knows - or an empty `{ }` (which the compiler accepts as "a project with
 * no settings") in a folder that holds Sharp code.
 *
 * Reading is cached by the file's size and time, since the Project view asks for every folder it shows.
 */
package sharp.ide.project

import com.google.gson.JsonObject
import com.google.gson.JsonParser
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.util.concurrent.ConcurrentHashMap

/** What the plugin needs out of one settings.json. */
data class SharpProjectFile(
    /** The folder the file is in: forward slashes, no trailing slash. */
    val folder: String,
    val name: String?,
    /** The executable's name in build/outputs (the compiler's default when the file names none: `out`). */
    val output: String,
    /** As written: paths inside the project, or elsewhere. */
    val ignoreFolders: List<String>,
    val externLibraries: List<String>,
    /**
     * `"native": { "sources": [...] }`: the C++ files and folders the compiler builds into the
     * project's native libraries (the Sharp Native Interface). As written.
     */
    val nativeSources: List<String> = emptyList(),
) {
    /** Where `sharpc -P` writes the program. */
    val outputPath: String get() = "$folder/build/outputs/$output"

    /** The libraries the file names, as absolute paths: one inside the project when that exists (the compiler's rule), else as written. */
    fun externLibraryFolders(): List<String> = externLibraries.map { resolve(it) }

    /** The ignored folders that lie inside the project, relative to it (`build`, `src/generated`). */
    fun ignoredInsideProject(): List<String> =
        ignoreFolders.map { SharpProjectFiles.normalise(it) }.filter { it.isNotEmpty() && !isAbsolute(it) }

    /**
     * Every C++ file the project's native libraries are built from (sources and headers), as absolute
     * paths, sorted. A change to one of them means the project has to be built again, though no
     * .sharp file changed.
     */
    fun nativeSourceFiles(): List<String> {
        val found = sortedSetOf<String>()
        for (written in nativeSources) {
            val path = Paths.get(resolve(written))
            try {
                if (Files.isRegularFile(path)) {
                    found.add(SharpProjectFiles.normalise(path.toString()))
                } else if (Files.isDirectory(path)) {
                    Files.walk(path).use { stream ->
                        stream.filter { Files.isRegularFile(it) && isCxxFile(it.fileName.toString()) }
                            .forEach { found.add(SharpProjectFiles.normalise(it.toString())) }
                    }
                }
            } catch (_: java.io.IOException) {
                // a folder that cannot be read adds nothing; the compiler says what is wrong when it builds
            }
        }
        return found.toList()
    }

    private fun isCxxFile(name: String): Boolean {
        val lower = name.lowercase()
        return CXX_ENDINGS.any { lower.endsWith(it) }
    }

    private fun resolve(written: String): String {
        val clean = SharpProjectFiles.normalise(written)
        val inside = "$folder/$clean"
        return if (!isAbsolute(clean) && Files.exists(Paths.get(inside))) inside else clean
    }

    private fun isAbsolute(path: String): Boolean = path.startsWith("/") || (path.length >= 2 && path[1] == ':')
}

/** The endings of the files a C++ build reads: sources and headers. */
private val CXX_ENDINGS = listOf(".cpp", ".cc", ".cxx", ".c", ".h", ".hpp", ".hh", ".hxx", ".inl")

object SharpProjectFiles {
    const val FILE_NAME = "settings.json"

    /** Every setting the compiler reads: one of them makes a settings.json Sharp's. */
    val KNOWN_SETTINGS = setOf(
        "name", "version", "output", "output_file", "compile_only", "show_all_errors", "debug", "release", "release_build",
        "obfuscate", "optimize_level", "cache_files", "target", "target_api", "warnings", "enable_warnings", "disable_warnings",
        "warnings_as_errors", "error_limit", "green_mode", "magic_mode", "compiler_debug_mode", "ignore_files", "ignore_folders",
        "extern_lib", "extern_libs", "ignore_extern_files", "ignore_extern_folders", "native",
    )
    /** `name` and `version` alone are in every package.json and the like: they do not decide. */
    private val TELLING = KNOWN_SETTINGS - setOf("name", "version", "debug", "target", "warnings", "release")

    private class Kept(val size: Long, val time: Long, val project: SharpProjectFile?)
    private val kept = ConcurrentHashMap<String, Kept>()

    /** The Sharp project file of `folder`, or null: no settings.json there, or one that is not Sharp's. */
    fun read(folder: String): SharpProjectFile? {
        val root = normalise(folder)
        if (root.isEmpty()) return null
        val file = Paths.get("$root/$FILE_NAME")
        val size: Long
        val time: Long
        try {
            if (!Files.isRegularFile(file)) {
                kept.remove(root)
                return null
            }
            size = Files.size(file)
            time = Files.getLastModifiedTime(file).toMillis()
        } catch (_: java.io.IOException) {
            return null
        }
        kept[root]?.let { if (it.size == size && it.time == time) return it.project }
        val project = if (size > 1_000_000) null else parse(root, file)
        kept[root] = Kept(size, time, project)
        return project
    }

    fun isProject(folder: String): Boolean = read(folder) != null

    private fun parse(root: String, file: Path): SharpProjectFile? {
        val json: JsonObject = try {
            val element = JsonParser.parseString(String(Files.readAllBytes(file), StandardCharsets.UTF_8))
            if (!element.isJsonObject) return null
            element.asJsonObject
        } catch (_: Exception) {
            return null                     // not JSON (the compiler will say so when it is asked to build)
        }
        val names = json.keySet()
        val telling = names.any { it in TELLING }
        val plain = names.all { it in KNOWN_SETTINGS }
        if (!telling && !(plain && holdsSharpCode(root))) return null
        return SharpProjectFile(
            folder = root,
            name = text(json, "name"),
            output = text(json, "output") ?: text(json, "output_file") ?: "out",
            ignoreFolders = texts(json, "ignore_folders"),
            externLibraries = texts(json, "extern_lib") + texts(json, "extern_libs"),
            nativeSources = json.get("native")?.takeIf { it.isJsonObject }?.asJsonObject?.let { everySources(it) } ?: emptyList(),
        )
    }

    /** A .sharp file in the folder itself or in its `src`: enough to take a plain settings.json for Sharp's. */
    private fun holdsSharpCode(root: String): Boolean =
        listOf(root, "$root/src").any { folder ->
            val path = Paths.get(folder)
            Files.isDirectory(path) && try {
                Files.list(path).use { stream -> stream.anyMatch { it.fileName.toString().endsWith(".sharp") } }
            } catch (_: java.io.IOException) {
                false
            }
        }

    private fun text(json: JsonObject, name: String): String? {
        val value = json.get(name) ?: return null
        if (!value.isJsonPrimitive || !value.asJsonPrimitive.isString) return null
        return value.asString.trim().takeIf { it.isNotEmpty() }
    }

    /**
     * Every "sources" list at or below `json`: the one of "native" itself and the ones written for a
     * system or a processor (`toolchain.linux.sources`, `toolchain.processor.arm.sources` ...). All of
     * them, whatever machine this is: a change to C++ that another machine builds is still a change
     * of the project, and building once too often is the harmless mistake.
     */
    private fun everySources(json: JsonObject): List<String> {
        val found = ArrayList<String>()
        found.addAll(texts(json, "sources"))
        for ((_, value) in json.entrySet()) {
            if (value.isJsonObject) found.addAll(everySources(value.asJsonObject))
        }
        return found.distinct()
    }

    private fun texts(json: JsonObject, name: String): List<String> {
        val value = json.get(name) ?: return emptyList()
        if (!value.isJsonArray) return emptyList()
        return value.asJsonArray.filter { it.isJsonPrimitive && it.asJsonPrimitive.isString }.map { it.asString.trim() }.filter { it.isNotEmpty() }
    }

    /** The settings.json a new project starts with (the compiler's own `--new-project` writes the same three settings). */
    fun newProjectText(name: String): String =
        "{\n  \"name\": \"$name\",\n  \"output\": \"$name\",\n  \"ignore_folders\": [\n    \"build\"\n  ]\n}\n"

    /** The first program of a new project. */
    fun newMainText(): String =
        "mod main;\n\ndef main(args: string[]) {\n    println(\"Hello from Sharp!\");\n}\n"

    fun normalise(path: String): String {
        val slashes = path.trim().replace('\\', '/')
        return if (slashes.length > 1) slashes.trimEnd('/') else slashes
    }
}
