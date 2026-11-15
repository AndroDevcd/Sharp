/*
 * "NOTHING CHANGED: DO NOT COMPILE AGAIN" - the stamp of a build
 *
 * A build's result depends on exactly these things: the compiler, the options, and every file it
 * reads. The STAMP of a build is a text that names them all -
 *
 *     compiler C:/.../sharpc-v0.4.0 4294407 1763226000000
 *     arguments -P C:/work/shapes -L C:/.../Sharp/lib | -O
 *     C:/.../Sharp/lib/std/list.sharp 9120 1763220000000        (every file: size and time of last change)
 *     C:/work/shapes/settings.json 96 1763226100000
 *     C:/work/shapes/src/main.sharp 412 1763226200000
 *
 * - and it is kept beside the program when a build worked (the run script copies it: RunScript.kt).
 * Before the next run the stamp is made again; when it is the same text and the program file is
 * still there, the compiler is not started. Change a file, add or remove one, change an option,
 * install another compiler, edit settings.json, delete the program: the texts differ, and it builds.
 *
 * Size and time are what `make` and every build tool since compare. They are read without opening
 * the files, so the check costs a few milliseconds for hundreds of files.
 *
 * A file the stamp lists but the build does not read (one inside a folder settings.json ignores in a
 * way the plugin does not see) only causes a build that was not needed - never a missed one.
 */
package sharp.ide.execution

import java.nio.file.Files
import java.nio.file.Paths

object RunStamp {
    /** The stamp for a build by `compiler` with these words on its command line, reading these files. */
    fun of(compiler: String, commandLine: List<String>, files: List<String>): String {
        val text = StringBuilder()
        text.append("compiler ").append(describe(compiler)).append('\n')
        text.append("arguments ").append(commandLine.joinToString(" ")).append('\n')
        for (file in files.sorted()) text.append(describe(file)).append('\n')
        return text.toString()
    }

    /** Is the kept stamp (null: there is none) the stamp of the build about to be made, with the program still there? */
    fun isUpToDate(newStamp: String, keptStampFile: String, output: String): Boolean {
        val kept = try {
            val path = Paths.get(keptStampFile)
            if (!Files.isRegularFile(path)) return false
            String(Files.readAllBytes(path), Charsets.UTF_8)
        } catch (_: java.io.IOException) {
            return false
        }
        return kept == newStamp && Files.isRegularFile(Paths.get(output))
    }

    /** `path size time`, or `path missing` for a file that is not there. */
    private fun describe(file: String): String = try {
        val path = Paths.get(file)
        "$file ${Files.size(path)} ${Files.getLastModifiedTime(path).toMillis()}"
    } catch (_: java.io.IOException) {
        "$file missing"
    }
}
