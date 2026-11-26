/*
 * SharpJumpTarget - "this exact place in this file", in the form go-to-declaration wants.
 *
 * WHAT THIS IS FOR
 *   A GotoDeclarationHandler must answer with PSI elements (the platform's syntax-tree
 *   objects). The first version answered with the TOKEN at the target and left it to the
 *   platform to work out where that is. In the owner's CLion that opened the right file but
 *   at its TOP: how an IDE turns "a token of somebody's tree" into a caret position is the
 *   IDE's business, and it is not the same in every product and version.
 *
 *   So the answer is now an element of our own that says the place itself:
 *
 *     - `navigate` opens the file at the offset with an OpenFileDescriptor - the platform's
 *       plain "open this file at this character" - and nothing else decides the position;
 *     - for code that asks instead of calling `navigate`, the element reports that same
 *       offset as its text offset and the target file as its file.
 *
 *   It still looks like the token it stands for (its text and range), because the platform
 *   also uses the target to draw the Ctrl+hover preview.
 *
 *   C++ comparison: a small adapter object - the interface asks for an Element*, what we have
 *   is (file, offset), so this wraps the pair in the interface.
 */
package sharp.ide.navigation

import com.intellij.navigation.ItemPresentation
import com.intellij.openapi.fileEditor.OpenFileDescriptor
import com.intellij.openapi.util.TextRange
import com.intellij.openapi.vfs.VirtualFile
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import com.intellij.psi.impl.FakePsiElement
import sharp.ide.language.SharpIcons
import javax.swing.Icon

/**
 * @param token   the token at the target (what the element looks like: its text and range)
 * @param file    the file to open
 * @param offset  where in that file's text NOW the caret goes
 */
class SharpJumpTarget(
    private val token: PsiElement,
    private val file: VirtualFile,
    private val offset: Int,
) : FakePsiElement() {

    // ---- where it is: always the place we were given

    override fun navigate(requestFocus: Boolean) {
        if (!file.isValid) return
        OpenFileDescriptor(token.project, file, offset).navigate(requestFocus)
    }

    override fun canNavigate(): Boolean = file.isValid

    override fun canNavigateToSource(): Boolean = file.isValid

    override fun getTextOffset(): Int = offset

    override fun getNavigationElement(): PsiElement = this

    // ---- what it looks like: the token

    override fun getParent(): PsiElement = token.parent ?: token.containingFile

    override fun getContainingFile(): PsiFile = token.containingFile

    override fun getTextRange(): TextRange = token.textRange

    override fun getText(): String = token.text

    override fun getTextLength(): Int = token.textLength

    override fun getStartOffsetInParent(): Int = token.startOffsetInParent

    override fun getName(): String = token.text

    override fun isValid(): Boolean = token.isValid && file.isValid

    override fun getPresentation(): ItemPresentation = this

    override fun getPresentableText(): String = token.text

    override fun getLocationString(): String = file.name

    override fun getIcon(open: Boolean): Icon = SharpIcons.FILE

    // two targets for the same place are the same target (the platform removes duplicates)
    override fun equals(other: Any?): Boolean = other is SharpJumpTarget && other.file == file && other.offset == offset

    override fun hashCode(): Int = 31 * file.hashCode() + offset
}
