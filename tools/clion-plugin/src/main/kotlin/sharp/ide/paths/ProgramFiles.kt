/*
 * ProgramFiles - which .sharp files make up "the program" the compiler is asked about.
 *
 * WHAT THIS IS FOR
 *   The compiler analyses a whole program, not one file: a class in one file is used in
 *   another. So on every run the plugin names every file of the program on the command
 *   line: the standard library first, then the project's own sources.
 *   The rule is in doc/ide.md section 3.2:
 *
 *     source roots     folders; every *.sharp below them belongs to the program
 *     excluded         folder names (`tests`) or path fragments (`lib/old`) to skip,
 *                      looked at relative to the source root they are found under
 *     library path     the standard library; every *.sharp below it (nothing excluded)
 *
 *   A file below both the library and a source root is counted once, as library.
 *
 * AUTOMATIC SETTINGS
 *   An empty list of source roots means "the project's base folder", except for the
 *   Sharp repository itself, where the program is the self-hosted compiler
 *   (`lib/support/bootstrap`): the repository as a whole has many `main`s and is not
 *   one program. An empty library path means `lib/support/0.4.0` of the project when
 *   that folder exists.
 *
 * Paths here are in the EDITOR's form (forward slashes, `C:/x/y`), as
 * `VirtualFile.getPath()` gives them. Translation to the compiler's form is the
 * PathTranslator's job and happens later.
 *
 * C++ comparison: the C++ compiler takes its file list from the command line or from
 * `-P <project>`; this class is the plugin's own "project file".
 */
package sharp.ide.paths

import java.io.IOException
import java.nio.file.FileVisitResult
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import java.nio.file.SimpleFileVisitor
import java.nio.file.attribute.BasicFileAttributes

/** The settings that decide which files belong to the program (a plain copy, so it can be used on any thread). */
data class ProgramSettings(
    val baseDirectory: String,
    val sourceRoots: List<String>,
    val excludedFolders: List<String>,
    val libraryPath: String,
    /** The library of the installed Sharp (sharp.ide.toolchain), "" when there is none: used when nothing else names one. */
    val installedLibrary: String = "",
)

/** The program of a project: its library root and its source roots, after the automatic defaults are applied. */
data class ProgramLayout(val libraryRoot: String?, val sourceRoots: List<String>, val excluded: List<String>)

object ProgramFiles {

    private const val LIBRARY_FOLDER = "lib/support/0.4.0"
    private const val BOOTSTRAP_FOLDER = "lib/support/bootstrap"

    /** Applies the automatic defaults described at the top of this file. */
    fun layoutOf(settings: ProgramSettings): ProgramLayout {
        val base = normalise(settings.baseDirectory)
        val library = settings.libraryPath.trim().let {
            when {
                it.isNotEmpty() -> absolute(base, it)
                // the project holds the library's own source (the Sharp repository): that copy is the one being worked on
                base.isNotEmpty() && Files.isDirectory(Paths.get("$base/$LIBRARY_FOLDER")) -> "$base/$LIBRARY_FOLDER"
                // any other project: the library that was installed with Sharp
                settings.installedLibrary.isNotBlank() && Files.isDirectory(Paths.get(normalise(settings.installedLibrary))) ->
                    normalise(settings.installedLibrary)
                else -> null
            }
        }
        val explicitRoots = settings.sourceRoots.map { it.trim() }.filter { it.isNotEmpty() }
        val roots = when {
            explicitRoots.isNotEmpty() -> explicitRoots.map { absolute(base, it) }
            base.isNotEmpty() && Files.isRegularFile(Paths.get("$base/$BOOTSTRAP_FOLDER/main.sharp")) ->
                listOf("$base/$BOOTSTRAP_FOLDER")
            base.isNotEmpty() -> listOf(base)
            else -> emptyList()
        }
        val excluded = settings.excludedFolders.map { normalise(it).trim('/') }.filter { it.isNotEmpty() }
        return ProgramLayout(library, roots.distinct(), excluded)
    }

    /**
     * Every file of the program: library files first, then source files, each group sorted by
     * path, a file never twice. Walks the disk with plain `java.nio` (no read action needed).
     */
    fun list(settings: ProgramSettings): List<String> {
        val layout = layoutOf(settings)
        val seen = LinkedHashSet<String>()
        val library = ArrayList<String>()
        layout.libraryRoot?.let { root ->
            for (file in walk(root, emptyList())) if (seen.add(file)) library.add(file)
        }
        val sources = ArrayList<String>()
        for (root in layout.sourceRoots) {
            for (file in walk(root, layout.excluded)) if (seen.add(file)) sources.add(file)
        }
        library.sort()
        sources.sort()
        return library + sources
    }

    /**
     * Does this one path belong to the program? A string test only (the file watcher calls it
     * on the UI thread for every changed file, so it must not touch the disk).
     */
    fun contains(layout: ProgramLayout, path: String): Boolean {
        val file = normalise(path)
        if (!file.endsWith(".sharp")) return false
        layout.libraryRoot?.let { if (file.startsWith("$it/")) return true }
        for (root in layout.sourceRoots) {
            if (!file.startsWith("$root/")) continue
            if (!isExcluded(file.substring(root.length + 1), layout.excluded)) return true
        }
        return false
    }

    /** `relative` is a path below a source root, e.g. `tests/a/b.sharp`; excluded entries are matched against its folders. */
    private fun isExcluded(relative: String, excluded: List<String>): Boolean {
        val folders = relative.substringBeforeLast('/', "")
        if (folders.isEmpty()) return false
        val padded = "/$folders/"
        // `tests` matches any folder called tests; `lib/old` matches that sequence of folders.
        // Both are "the entry between two slashes somewhere in the folder path".
        return excluded.any { padded.contains("/$it/") }
    }

    private fun walk(root: String, excluded: List<String>): List<String> {
        val start = Paths.get(root)
        if (!Files.isDirectory(start)) return emptyList()
        val found = ArrayList<String>()
        Files.walkFileTree(start, object : SimpleFileVisitor<Path>() {
            override fun preVisitDirectory(dir: Path, attrs: BasicFileAttributes): FileVisitResult {
                if (dir != start) {
                    val relative = normalise(start.relativize(dir).toString()) + "/x"
                    if (isExcluded(relative, excluded)) return FileVisitResult.SKIP_SUBTREE
                }
                return FileVisitResult.CONTINUE
            }

            override fun visitFile(file: Path, attrs: BasicFileAttributes): FileVisitResult {
                if (file.fileName.toString().endsWith(".sharp")) found.add(normalise(file.toString()))
                return FileVisitResult.CONTINUE
            }

            override fun visitFileFailed(file: Path, exc: IOException): FileVisitResult = FileVisitResult.CONTINUE
        })
        return found
    }

    /** Forward slashes, no trailing slash. */
    fun normalise(path: String): String {
        val slashes = path.trim().replace('\\', '/')
        return if (slashes.length > 1) slashes.trimEnd('/') else slashes
    }

    private fun absolute(base: String, path: String): String {
        val clean = normalise(path)
        val isAbsolute = clean.startsWith("/") || (clean.length >= 2 && clean[1] == ':')
        return if (isAbsolute || base.isEmpty()) clean else "$base/$clean"
    }
}
