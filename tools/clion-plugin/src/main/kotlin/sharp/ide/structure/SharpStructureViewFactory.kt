/*
 * SharpStructureViewFactory - the platform's socket for the Structure tool window (Ctrl+F12).
 *
 * WHAT THIS IS FOR
 *   The platform asks the factory for a "builder" for a file; the builder makes a MODEL (the tree) that
 *   the tool window shows. The tree's data is StructureOutline (from the index); each node wraps the
 *   TOKEN at the declaration's name as its PSI element, which is all the platform needs to jump there
 *   when you click a row (the same trick as go-to-declaration, doc/ide.md 6.10).
 *
 *   Nothing in the index yet (no run finished, a new file): the tree is empty, no error.
 */
package sharp.ide.structure

import com.intellij.icons.AllIcons
import com.intellij.ide.structureView.StructureViewBuilder
import com.intellij.ide.structureView.StructureViewModel
import com.intellij.ide.structureView.StructureViewModelBase
import com.intellij.ide.structureView.StructureViewTreeElement
import com.intellij.ide.structureView.TreeBasedStructureViewBuilder
import com.intellij.ide.util.treeView.smartTree.Sorter
import com.intellij.lang.PsiStructureViewFactory
import com.intellij.openapi.editor.Editor
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import com.intellij.ide.structureView.impl.common.PsiTreeElementBase
import sharp.ide.editor.FileViews
import javax.swing.Icon

class SharpStructureViewFactory : PsiStructureViewFactory {
    override fun getStructureViewBuilder(psiFile: PsiFile): StructureViewBuilder =
        object : TreeBasedStructureViewBuilder() {
            override fun createStructureViewModel(editor: Editor?): StructureViewModel =
                SharpStructureViewModel(psiFile, editor)
        }
}

class SharpStructureViewModel(file: PsiFile, editor: Editor?) :
    StructureViewModelBase(file, editor, SharpFileElement(file)), StructureViewModel.ElementInfoProvider {
    init {
        withSorters(Sorter.ALPHA_SORTER)
    }

    override fun isAlwaysShowsPlus(element: StructureViewTreeElement?): Boolean = false
    override fun isAlwaysLeaf(element: StructureViewTreeElement?): Boolean = (element as? SharpOutlineElement)?.isLeaf == true
}

/** The root: the file; its children are the top-level declarations of the outline. */
class SharpFileElement(private val file: PsiFile) : PsiTreeElementBase<PsiFile>(file) {
    override fun getPresentableText(): String = file.name

    override fun getChildrenBase(): Collection<StructureViewTreeElement> {
        return try {
            val virtualFile = file.virtualFile ?: return emptyList()
            val text = file.viewProvider.document?.text ?: file.text
            val view = FileViews.forFile(file.project, virtualFile, text) ?: return emptyList()
            StructureOutline.of(view).mapNotNull { SharpOutlineElement.of(file, it) }
        } catch (e: com.intellij.openapi.progress.ProcessCanceledException) {
            throw e
        } catch (_: Throwable) {
            emptyList()
        }
    }
}

class SharpOutlineElement private constructor(
    leaf: PsiElement,
    private val node: OutlineNode,
    private val file: PsiFile,
) : PsiTreeElementBase<PsiElement>(leaf) {

    val isLeaf: Boolean get() = node.children.isEmpty()

    override fun getPresentableText(): String = node.label

    override fun getIcon(open: Boolean): Icon? = when (node.kind) {
        "class" -> AllIcons.Nodes.Class
        "interface" -> AllIcons.Nodes.Interface
        "enum" -> AllIcons.Nodes.Enum
        "function", "constructor" -> AllIcons.Nodes.Method
        "property" -> AllIcons.Nodes.Property
        "field", "value" -> AllIcons.Nodes.Field
        else -> AllIcons.Nodes.Type
    }

    override fun getChildrenBase(): Collection<StructureViewTreeElement> = node.children.mapNotNull { of(file, it) }

    companion object {
        fun of(file: PsiFile, node: OutlineNode): SharpOutlineElement? {
            val leaf = file.findElementAt(node.offset.coerceIn(0, maxOf(0, file.textLength - 1))) ?: return null
            return SharpOutlineElement(leaf, node, file)
        }
    }
}
