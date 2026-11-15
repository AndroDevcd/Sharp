package sharp.ide.index

import sharp.ide.editor.FileView
import sharp.ide.run.SavedText
import java.nio.file.Files
import java.nio.file.Path
import java.util.zip.GZIPInputStream

/**
 * REAL COMPILER OUTPUT FOR THE INDEX TESTS
 *
 * The folders under src/test/resources/index-fixtures were written by the real compiler
 * (`scripts/make_index_fixture.sh`, packed by `scripts/pack_index_fixture.sh`): the sample
 * `source.sharp` compiled TOGETHER WITH THE WHOLE STANDARD LIBRARY with `-ide`, asking for the
 * sample's bodies file. So `declarations.json` really holds the library (1.1 MB, kept gzipped)
 * and a test can ask what `list<string>` offers after a dot.
 *
 *   completion   the program of the completion tests (shop: item, book, holder, an enum ...)
 *   shapes       the compiler's own sample of every kind of declaration
 *
 * The compiler wrote the path of the machine it ran on; `writeRunFolder` swaps the sample's path
 * for one the test controls, which changes nothing else (offsets and hashes stay true).
 */
class IndexFixture(val name: String) {

    private fun resource(file: String): ByteArray? =
        javaClass.classLoader.getResourceAsStream("index-fixtures/$name/$file")?.use { it.readBytes() }

    private fun text(file: String): String {
        resource(file)?.let { return String(it, Charsets.UTF_8) }
        val zipped = resource("$file.gz") ?: error("missing fixture $name/$file")
        return String(GZIPInputStream(zipped.inputStream()).use { it.readBytes() }, Charsets.UTF_8)
    }

    /** The sample's source file, byte for byte, and the text as the editor shows it. */
    val sourceBytes: ByteArray = resource("source.sharp")!!
    val savedText: String = SavedText.decode(sourceBytes)

    private val manifestText: String = text("ide.json")

    /** The path the compiler wrote for the sample. */
    val originalPath: String = Regex("\"path\":\"([^\"]+)\",\"module\":\"[^\"]*\",\"library\":false").find(manifestText)!!.groupValues[1]

    /** The id the compiler gave this run, and the folder it read the standard library from. */
    val originalRun: String = Regex("\"run\":\"([0-9a-f]+)\"").find(manifestText)!!.groupValues[1]
    val originalLibraryRoot: String = Regex("\"path\":\"([^\"]+)/std/list[.]sharp\"").find(manifestText)!!.groupValues[1]

    private fun rewrite(text: String, samplePath: String, runId: String?, libraryRoot: String?): String {
        var result = text.replace(originalPath, samplePath)
        if (libraryRoot != null) result = result.replace(originalLibraryRoot, libraryRoot)
        if (runId != null) result = result.replace(originalRun, runId)
        return result
    }

    /**
     * Writes the whole run into `folder`, the sample's path replaced by `samplePath`; optionally another run id
     * (a test that needs several different runs) and another folder for the standard library (a test that
     * needs a real library file to jump into). Returns the folder.
     */
    fun writeRunFolder(folder: Path, samplePath: String, runId: String? = null, libraryRoot: String? = null): Path {
        Files.createDirectories(folder.resolve("bodies"))
        Files.write(folder.resolve("ide.json"), rewrite(manifestText, samplePath, runId, libraryRoot).toByteArray(Charsets.UTF_8))
        for (file in listOf("types.json", "declarations.json", "diagnostics.json")) {
            Files.write(folder.resolve(file), rewrite(text(file), samplePath, runId, libraryRoot).toByteArray(Charsets.UTF_8))
        }
        for (m in Regex("\"bodies\":\"(bodies/[0-9]+\\.json)\"").findAll(manifestText)) {
            Files.write(folder.resolve(m.groupValues[1]), String(resource(m.groupValues[1])!!, Charsets.UTF_8).replace(originalRun, runId ?: originalRun).toByteArray(Charsets.UTF_8))
        }
        return folder
    }

    /** Reads the fixture through the real reader. `samplePath` is the path the test file will have. */
    fun load(tempDir: Path, samplePath: String = originalPath, runId: String? = null, libraryRoot: String? = null): Loaded {
        val folder = writeRunFolder(tempDir.resolve("run-$name" + (runId ?: "")), samplePath, runId, libraryRoot)
        val run = DiagnosticsReader.readFolder(folder)
        return Loaded(this, run, IndexLoader.load(folder, run.manifest), samplePath)
    }

    class Loaded(val fixture: IndexFixture, val run: DiagnosticsRun, val index: RunIndex, val samplePath: String) {
        /** The sample's number in the manifest. */
        val file: Int = index.fileByPath[samplePath]!!

        /** A view of the sample whose current text is `text` (the compiler saw `fixture.savedText`). */
        fun view(text: String = fixture.savedText) = FileView(index, file, fixture.savedText, text)

        /** The sample's saved text with `insert` put in front of the first occurrence of `before`. */
        fun withInsertBefore(before: String, insert: String): String {
            val at = fixture.savedText.indexOf(before)
            require(at >= 0) { "no `$before` in the sample" }
            return fixture.savedText.substring(0, at) + insert + fixture.savedText.substring(at)
        }
    }
}
