/*
 * DiagnosticsModel - what the compiler said about a run, as plain Kotlin values.
 *
 * WHAT THIS IS FOR
 *   The compiler writes `ide.json` (the manifest) and `diagnostics.json` into the run
 *   folder (doc/ide.md sections 4.3 and 4.4). DiagnosticsReader turns them into these
 *   classes. They are immutable, so any thread may hold one while a newer run replaces
 *   it. Every position is an OFFSET in UTF-16 code units of the file as the SAVED text
 *   the compiler read it (section 4.1); `line` and `column` are only for people.
 *
 *   Paths are exactly what the compiler wrote (its own form, `/mnt/c/...`).
 */
package sharp.ide.index

enum class DiagnosticSeverity { ERROR, WARNING }

/** A range in a file: the diagnostic's own, or one of its related places. */
data class Place(
    val file: String,
    val from: Int,
    val to: Int,
    val line: Int,
    val column: Int,
    val endLine: Int,
    val endColumn: Int,
    /** For a related place: the words that say what it is ("the first one is here"). */
    val label: String,
)

/** One error or warning, with everything the compiler said about it. */
data class SharpDiagnostic(
    val severity: DiagnosticSeverity,
    /** `E0400`, `W1022` ... */
    val code: String,
    /** One line; names the things involved in backticks. */
    val message: String,
    val place: Place,
    /** The few words under the marked piece; "" when none. */
    val label: String,
    /** The rule that was broken, one text each. */
    val notes: List<String>,
    /** What to do about it, one text each. */
    val helps: List<String>,
    val related: List<Place>,
)

/** One file as the manifest lists it: what the compiler read. */
data class ManifestFile(
    val path: String,
    /** FNV-1a (32 bit) of the bytes the compiler read, eight hex digits; null when it could not read the file. */
    val hash: String?,
    val size: Long,
    /** The module the file's `mod` line names ("" = the nameless module); null at level `syntax-errors`. */
    val module: String? = null,
    /** True for the standard library (`std`, `std.*`, `platform`, `platform.*`). */
    val library: Boolean = false,
    /** The modules the file can see, its own first (doc/ide.md 4.3). */
    val imports: List<String> = emptyList(),
    /** `import (a, b) as g` groups: group name -> modules. */
    val groups: Map<String, List<String>> = emptyMap(),
    /** The file's bodies file (`bodies/<n>.json`), or null. */
    val bodies: String? = null,
)

/** `ide.json`: what a run covers. Only the fields the diagnostics side needs; the index stage reads the rest. */
data class RunManifest(
    val run: String,
    /** `syntax-errors`, `declarations` or `full`. */
    val level: String,
    val errors: Int,
    val warnings: Int,
    val files: List<ManifestFile>,
) {
    val hashByPath: Map<String, String?> by lazy { files.associate { it.path to it.hash } }
}

/** The diagnostics of one run, grouped by the file they are in. */
class DiagnosticsRun(
    val manifest: RunManifest,
    val diagnostics: List<SharpDiagnostic>,
    /** True when the compiler cut the list at its limit (1000). */
    val truncated: Boolean,
    /** Files the compiler could not read, with the reason. */
    val unreadable: Map<String, String>,
) {
    val run: String get() = manifest.run
    val byFile: Map<String, List<SharpDiagnostic>> by lazy { diagnostics.groupBy { it.place.file } }

    fun forFile(compilerPath: String): List<SharpDiagnostic> = byFile[compilerPath] ?: emptyList()
}

/** The reason a run's folder could not be read; shown to the owner once. */
class RunReadException(message: String) : Exception(message)

/**
 * Runs `read` and turns "valid JSON of the wrong shape" into a reason in words. The files are checked
 * field by field, but a text where a list should be, a missing list or a number that is not one makes
 * the JSON library throw its own exceptions (IllegalStateException, NumberFormatException ...). Those
 * must never escape as a crash of the background task: they become a RunReadException naming the file,
 * exactly like broken JSON does.
 */
inline fun <T> wellShaped(what: String, read: () -> T): T =
    try {
        read()
    } catch (e: RunReadException) {
        throw e
    } catch (e: RuntimeException) {
        throw RunReadException("$what is not in the shape this plugin reads (${e.javaClass.simpleName}); the compiler and the plugin may be of different versions")
    }
