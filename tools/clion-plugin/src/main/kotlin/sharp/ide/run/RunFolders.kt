/*
 * RunFolders - one fresh folder for every compiler run.
 *
 * WHAT THIS IS FOR
 *   The compiler writes its answer (diagnostics.json, ...) into a folder the plugin
 *   gives it. A FRESH folder per run means the compiler needs no clean-up, never
 *   overwrites files the plugin is reading, and a run that was killed half way cannot
 *   leave files that look like another run's (doc/ide.md section 3.4).
 *
 *   Layout:  <IDE system folder>/sharp-ide/<project key>/run-<n>/
 *   The plugin keeps the newest folders and deletes the rest after each run.
 *   Nothing is ever written into the project.
 */
package sharp.ide.run

import java.io.IOException
import java.nio.file.Files
import java.nio.file.Path

object RunFolders {

    /** Creates `run-<n>` under `base`, `n` one more than the largest number already there. */
    fun create(base: Path): Path {
        Files.createDirectories(base)
        var next = numberedFolders(base).maxOfOrNull { it.first }?.plus(1) ?: 1
        while (true) {
            val folder = base.resolve("run-$next")
            try {
                return Files.createDirectory(folder)
            } catch (e: java.nio.file.FileAlreadyExistsException) {
                next++                                         // somebody (another IDE window) took it
            }
        }
    }

    /** Deletes every run folder under `base` except those in `keep`. Never fails: a locked folder stays until next time. */
    fun prune(base: Path, keep: Set<Path>) {
        for ((_, folder) in numberedFolders(base)) {
            if (folder in keep) continue
            try {
                deleteRecursively(folder)
            } catch (_: IOException) {
                // still in use; the next run's prune tries again
            }
        }
    }

    /** The `run-<n>` folders of `base` with their numbers, oldest first. */
    fun numberedFolders(base: Path): List<Pair<Long, Path>> {
        if (!Files.isDirectory(base)) return emptyList()
        Files.newDirectoryStream(base, "run-*").use { stream ->
            return stream.mapNotNull { folder ->
                val number = folder.fileName.toString().removePrefix("run-").toLongOrNull()
                if (number != null && Files.isDirectory(folder)) number to folder else null
            }.sortedBy { it.first }
        }
    }

    private fun deleteRecursively(folder: Path) {
        Files.walk(folder).use { walk ->
            walk.sorted(Comparator.reverseOrder()).forEach { Files.deleteIfExists(it) }
        }
    }
}
