/*
 * SharpSettingsSchema - completion, descriptions and checks inside a project's settings.json.
 *
 * WHAT THIS IS FOR
 *   settings.json has a fixed shape: the names the compiler reads at the top, and inside
 *   "native" only "sources" and "toolchain", inside "toolchain" only "compiler", "flags" ...
 *   The IDE's JSON support does everything that follows from a shape - Ctrl+Space offers the
 *   names that are possible WHERE THE CARET IS, hovering a name shows what it does, a name
 *   nobody reads or a value of the wrong kind is underlined - when it is handed the shape as
 *   a JSON Schema (resources/schemas/sharp-settings.schema.json). This file is the handing over:
 *   "for a Sharp project's settings.json, use that schema".
 *
 *   The schema is a description for the editor only. What a setting MEANS is decided by the
 *   compiler (compiler/configuration/settings_processor.sharp); when a setting is added
 *   there, add it to the schema file too. `SettingsSchemaTest` checks the two lists of names
 *   against each other.
 *
 * WHICH FILES
 *   `settings.json` is a common name (VS Code has one in every `.vscode` folder). The schema is
 *   given to a settings.json that is a Sharp project file already (SharpProjectFiles), or that
 *   lies in a folder with Sharp code - so a new, still empty one gets completion as well.
 *
 * This class is registered only when the IDE's JSON support is there (plugin.xml: an optional
 * dependency, sharp-json.xml), so the plugin still loads in an IDE without it.
 */
package sharp.ide.project

import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.VirtualFile
import com.jetbrains.jsonSchema.extension.JsonSchemaFileProvider
import com.jetbrains.jsonSchema.extension.JsonSchemaProviderFactory
import com.jetbrains.jsonSchema.extension.SchemaType
import java.nio.file.Files
import java.nio.file.Paths

class SharpSettingsSchemaProviderFactory : JsonSchemaProviderFactory {
    override fun getProviders(project: Project): List<JsonSchemaFileProvider> = listOf(SharpSettingsSchemaProvider())
}

class SharpSettingsSchemaProvider : JsonSchemaFileProvider {
    override fun getName(): String = "Sharp project settings"

    override fun getSchemaType(): SchemaType = SchemaType.embeddedSchema

    override fun getSchemaFile(): VirtualFile? =
        JsonSchemaProviderFactory.getResourceFile(SharpSettingsSchemaProvider::class.java, SharpSettingsSchema.RESOURCE)

    override fun isAvailable(file: VirtualFile): Boolean =
        file.name == SharpProjectFiles.FILE_NAME && SharpSettingsSchema.isSharpSettingsFolder(file.parent?.path)
}

object SharpSettingsSchema {
    const val RESOURCE = "/schemas/sharp-settings.schema.json"

    /** Is a settings.json in this folder a Sharp project's? (It is one already, or the folder holds Sharp code.) */
    fun isSharpSettingsFolder(folder: String?): Boolean {
        if (folder == null) return false
        val root = SharpProjectFiles.normalise(folder)
        if (root.isEmpty() || root.substringAfterLast('/') == ".vscode") return false
        if (SharpProjectFiles.isProject(root)) return true
        return listOf(root, "$root/src").any { holdsSharpFile(it) }
    }

    private fun holdsSharpFile(folder: String): Boolean {
        val path = Paths.get(folder)
        return Files.isDirectory(path) && try {
            Files.list(path).use { stream -> stream.anyMatch { it.fileName.toString().endsWith(".sharp") } }
        } catch (_: java.io.IOException) {
            false
        }
    }
}
