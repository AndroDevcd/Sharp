/*
 * SharpFileTypeOverrider - a `.sharp` file is a Sharp file, whatever the IDE's settings say.
 *
 * WHAT THIS IS FOR
 *   Which "file type" a file has decides everything else: the lexer that colours it, and
 *   whether this plugin's error marks, completion, hover and go-to are asked at all (they are
 *   registered for the LANGUAGE Sharp, which only the file type Sharp has).
 *
 *   Normally the IDE picks the type from a table of name patterns (Settings > Editor > File
 *   Types), and the plugin registers `*.sharp` there. But that table belongs to the user and
 *   can say otherwise. It did on the owner's machine: a hand-made file type ("Sharp File",
 *   keyword lists for colours, made before this plugin existed) had taken `*.sharp`, and the
 *   IDE remembered "the pattern was removed from the plugin's type". Result: the files were
 *   coloured by the keyword lists, the compiler ran and found errors, and NOTHING showed -
 *   no squiggle, no completion - with no hint why. Deleting the hand-made type helped until
 *   the next restart brought it back.
 *
 *   A FileTypeOverrider is asked BEFORE the pattern table. This one answers "Sharp" for every
 *   file whose name ends in `.sharp`, so the plugin works without anybody touching a setting.
 *
 * Registered in plugin.xml as `com.intellij.fileTypeOverrider`.
 */
package sharp.ide.language

import com.intellij.openapi.fileTypes.FileType
import com.intellij.openapi.fileTypes.impl.FileTypeOverrider
import com.intellij.openapi.vfs.VirtualFile

class SharpFileTypeOverrider : FileTypeOverrider {

    /** The type of `file` when this plugin decides it; null = let the IDE's own rules decide. */
    override fun getOverriddenFileType(file: VirtualFile): FileType? {
        // called for every file the IDE looks at, on any thread: only the name is looked at
        if (file.isDirectory) return null
        return if (file.name.endsWith(".sharp", ignoreCase = true)) SharpFileType else null
    }
}
