/*
 * SettingsSchemaInTheIdeTest - a project's settings.json completes its settings BY WHERE THE CARET IS.
 *
 * Asks the IDE itself (not our code) for the completion list inside a real settings.json on disk:
 * at the top the compiler's settings, inside "native" only `sources` and `toolchain`, inside
 * "toolchain" only its six names. And a settings.json that is not a Sharp project's gets nothing
 * from us. Also checks the schema against the compiler's own list of settings, so the two cannot
 * drift apart unnoticed.
 */
package sharp.ide.project

import com.google.gson.JsonParser
import com.intellij.openapi.vfs.LocalFileSystem
import com.intellij.openapi.vfs.newvfs.impl.VfsRootAccess
import com.intellij.testFramework.fixtures.BasePlatformTestCase
import java.io.File
import java.nio.file.Files

class SettingsSchemaInTheIdeTest : BasePlatformTestCase() {
    private lateinit var folder: File

    override fun setUp() {
        super.setUp()
        folder = Files.createTempDirectory("sharp-settings-schema").toFile().canonicalFile
        VfsRootAccess.allowRootAccess(testRootDisposable, folder.path)
    }

    override fun tearDown() {
        try {
            folder.deleteRecursively()
        } finally {
            super.tearDown()
        }
    }

    /** The names the IDE offers where `|` stands in `text`, written as the settings.json of `project`. */
    private fun offeredAt(project: File, text: String): List<String> {
        val caret = text.indexOf('|')
        require(caret >= 0) { "no `|` in the text" }
        project.mkdirs()
        val file = File(project, "settings.json")
        file.writeText(text.replace("|", ""))
        val virtual = LocalFileSystem.getInstance().refreshAndFindFileByIoFile(file)!!
        myFixture.configureFromExistingVirtualFile(virtual)
        myFixture.editor.caretModel.moveToOffset(caret)
        myFixture.completeBasic()
        return (myFixture.lookupElementStrings ?: emptyList()).map { it.trim('"') }
    }

    private fun sharpProject(name: String): File {
        val project = File(folder, name)
        File(project, "src").mkdirs()
        File(project, "src/main.sharp").writeText("mod main;\ndef main() { }\n")
        return project
    }

    fun testTheTopLevelOffersTheCompilersSettings() {
        val offered = offeredAt(sharpProject("top"), "{\n  \"name\": \"app\",\n  \"|\"\n}\n")
        assertTrue("got $offered", offered.containsAll(listOf("output", "native", "ignore_folders", "optimize_level", "extern_lib")))
        assertFalse("`sources` belongs inside \"native\", not at the top", "sources" in offered)
        assertFalse("`compiler` belongs inside \"toolchain\"", "compiler" in offered)
    }

    fun testInsideNativeOnlyItsTwoSettingsAreOffered() {
        val offered = offeredAt(sharpProject("native"), "{\n  \"output\": \"app\",\n  \"native\": { \"|\" }\n}\n")
        assertEquals(listOf("sources", "toolchain"), offered.sorted())
    }

    fun testInsideToolchainItsSixSettingsAreOffered() {
        val offered = offeredAt(sharpProject("toolchain"),
            "{\n  \"output\": \"app\",\n  \"native\": { \"sources\": [ \"src/cpp\" ], \"toolchain\": { \"|\" } }\n}\n")
        assertEquals(listOf("compiler", "flags", "include", "libraries", "linux", "windows"), offered.sorted())
    }

    fun testInsideASystemsBlockTheFourToolchainValuesAreOffered() {
        val offered = offeredAt(sharpProject("system"),
            "{\n  \"output\": \"app\",\n  \"native\": { \"toolchain\": { \"windows\": { \"|\" } } }\n}\n")
        assertEquals(listOf("compiler", "flags", "include", "libraries"), offered.sorted())
    }

    fun testASettingsFileThatIsNotASharpProjectsGetsNothingFromUs() {
        // no Sharp code beside it, and nothing in it the compiler knows: somebody else's settings.json
        val other = File(folder, "elsewhere")
        val offered = offeredAt(other, "{\n  \"editor.fontSize\": 12,\n  \"|\"\n}\n")
        assertFalse("got $offered", "native" in offered)
        assertFalse(SharpSettingsSchema.isSharpSettingsFolder(File(folder, ".vscode").path))
    }

    fun testTheSchemaNamesExactlyTheSettingsTheCompilerReads() {
        val schema = JsonParser.parseString(
            SharpSettingsSchema::class.java.getResourceAsStream(SharpSettingsSchema.RESOURCE)!!.bufferedReader().readText()).asJsonObject
        val top = schema.getAsJsonObject("properties").keySet()
        assertEquals("the schema and the plugin's own list of settings", SharpProjectFiles.KNOWN_SETTINGS.sorted(), top.sorted())

        // the compiler's list: known_settings() in settings_processor.sharp (when the repository is around this plugin)
        val compilerFile = generateSequence(File("").absoluteFile) { it.parentFile }
            .map { File(it, "lib/compiler/configuration/settings_processor.sharp") }.firstOrNull { it.isFile }
        if (compilerFile != null) {
            val text = compilerFile.readText()
            val body = text.substring(text.indexOf("def known_settings()")).substringBefore("return names;")
            val compilerNames = Regex("names\\.add\\(\"([a-z_]+)\"\\)").findAll(body).map { it.groupValues[1] }.toList()
            assertTrue("known_settings() was read", compilerNames.size > 20)
            assertEquals("the schema and the compiler's known_settings()", compilerNames.sorted(), top.sorted())
        }
        val native = schema.getAsJsonObject("properties").getAsJsonObject("native").getAsJsonObject("properties")
        assertEquals(setOf("sources", "toolchain"), native.keySet())
        assertEquals(setOf("compiler", "flags", "include", "libraries", "windows", "linux"),
            native.getAsJsonObject("toolchain").getAsJsonObject("properties").keySet())
    }
}
