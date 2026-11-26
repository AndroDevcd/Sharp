package sharp.ide.language

import com.intellij.lang.ASTNode
import com.intellij.lang.folding.FoldingBuilderEx
import com.intellij.lang.folding.FoldingDescriptor
import com.intellij.openapi.editor.Document
import com.intellij.openapi.project.DumbAware
import com.intellij.psi.PsiElement
import sharp.ide.lexer.SharpTokenKinds

/**
 * CODE FOLDING: THE LITTLE MINUS SIGN IN THE GUTTER
 *
 * Two kinds of region fold: a curly-brace block that spans several lines (it
 * shows as `{...}`) and a block comment that spans several lines. Both are single
 * nodes of the flat tree, so this only walks the tree once.
 * Registered as `lang.foldingBuilder`; DumbAware = works while the IDE indexes.
 */
class SharpFoldingBuilder : FoldingBuilderEx(), DumbAware {
    override fun buildFoldRegions(root: PsiElement, document: Document, quick: Boolean): Array<FoldingDescriptor> {
        val regions = ArrayList<FoldingDescriptor>()
        collect(root.node, document, regions)
        return regions.toTypedArray()
    }

    private fun collect(node: ASTNode, document: Document, into: MutableList<FoldingDescriptor>) {
        val type = node.elementType
        if (type == SharpElementTypes.BLOCK || type == SharpTokenKinds.BLOCK_COMMENT || type == SharpTokenKinds.DOC_COMMENT) {
            val range = node.textRange
            if (spansSeveralLines(document, range.startOffset, range.endOffset)) into.add(FoldingDescriptor(node, range))
        }
        var child = node.firstChildNode
        while (child != null) {
            collect(child, document, into)
            child = child.treeNext
        }
    }

    private fun spansSeveralLines(document: Document, start: Int, end: Int): Boolean =
        end <= document.textLength && document.getLineNumber(start) != document.getLineNumber(end)

    override fun getPlaceholderText(node: ASTNode): String = when (node.elementType) {
        SharpElementTypes.BLOCK -> "{...}"
        SharpTokenKinds.DOC_COMMENT -> "/**...*/"
        else -> "/*...*/"
    }

    override fun isCollapsedByDefault(node: ASTNode): Boolean = false
}
