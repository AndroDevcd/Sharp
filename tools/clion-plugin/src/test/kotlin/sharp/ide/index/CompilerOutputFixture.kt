package sharp.ide.index

import java.nio.file.Files
import java.nio.file.Path

/**
 * REAL COMPILER OUTPUT FOR THE TESTS
 *
 * The folders under src/test/resources/compiler-output were made by the real compiler
 * (the sample's source is beside them as `source.sharp`; tests/ide/make_fixtures.sh makes
 * output like this on demand). The compiler wrote the paths of the machine it ran on;
 * `writeRunFolder` swaps that path for one the test controls, which changes nothing else
 * (offsets and hashes stay true).
 *
 *   analysis_errors   two errors and a warning in a program that parses
 *   syntax_error      the program does not parse: level syntax-errors
 *   related_places    a diagnostic with a related place and one with a note
 *   clean_program     a program with no diagnostics at all (level full)
 *   wide              CRLF line ends, a tab, non-ASCII text and an emoji before the error
 */
class CompilerOutputFixture(val name: String) {
    private fun resource(file: String): ByteArray =
        (javaClass.classLoader.getResourceAsStream("compiler-output/$name/$file") ?: error("missing fixture $name/$file")).use { it.readBytes() }

    /** The sample's source file, byte for byte. */
    val sourceBytes: ByteArray = resource("source.sharp")
    val manifestText: String = String(resource("ide.json"), Charsets.UTF_8)
    val diagnosticsText: String = String(resource("diagnostics.json"), Charsets.UTF_8)

    /** The path the compiler wrote for the sample's own file: the one in its first diagnostic, else the manifest's only non-library file. */
    val originalPath: String = (Regex("\"file\":\"([^\"]+)\"").find(diagnosticsText)
        ?: Regex("\"path\":\"([^\"]+)\",\"module\":\"[^\"]*\",\"library\":false").find(manifestText))!!.groupValues[1]

    /** Writes ide.json and diagnostics.json into `folder`, the sample's path replaced by `path`. */
    fun writeRunFolder(folder: Path, path: String): Path {
        Files.createDirectories(folder)
        Files.write(folder.resolve("ide.json"), manifestText.replace(originalPath, path).toByteArray(Charsets.UTF_8))
        Files.write(folder.resolve("diagnostics.json"), diagnosticsText.replace(originalPath, path).toByteArray(Charsets.UTF_8))
        return folder
    }
}
