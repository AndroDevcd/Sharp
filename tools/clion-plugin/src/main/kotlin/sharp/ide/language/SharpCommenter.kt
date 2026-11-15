package sharp.ide.language

import com.intellij.lang.Commenter

/**
 * COMMENT / UNCOMMENT (Ctrl+/ and Ctrl+Shift+/)
 *
 * Sharp has `//` to the end of the line, and a block comment that nests.
 * Registered as `lang.commenter`.
 */
class SharpCommenter : Commenter {
    override fun getLineCommentPrefix(): String = "//"

    override fun getBlockCommentPrefix(): String = "/*"

    override fun getBlockCommentSuffix(): String = "*/"

    override fun getCommentedBlockCommentPrefix(): String? = null

    override fun getCommentedBlockCommentSuffix(): String? = null
}
