package sharp.ide.language

import com.intellij.openapi.fileTypes.LanguageFileType
import com.intellij.openapi.vfs.VirtualFile
import javax.swing.Icon

/**
 * THE FILE TYPE ".sharp"
 *
 * Says "files that end in .sharp are Sharp files": the editor then opens them
 * with the Sharp language (colours, folding, ...). Registered in plugin.xml with
 * `fieldName="INSTANCE"`, which is the static field Kotlin makes for an `object`.
 *
 * Primer idea 2 in doc/ide.md section 2.
 */
object SharpFileType : LanguageFileType(SharpLanguage) {
    override fun getName(): String = "Sharp"

    override fun getDescription(): String = "Sharp source file"

    override fun getDefaultExtension(): String = "sharp"

    override fun getIcon(): Icon = SharpIcons.FILE

    /**
     * The compiler reads the bytes of a file as UTF-8 whatever the IDE's
     * encoding setting says. The character offsets the compiler writes for the
     * editor (doc/ide.md section 4.1) only agree with the editor when the editor
     * decodes the file the same way, so the file type forces UTF-8.
     */
    override fun getCharset(file: VirtualFile, content: ByteArray): String = "UTF-8"
}
