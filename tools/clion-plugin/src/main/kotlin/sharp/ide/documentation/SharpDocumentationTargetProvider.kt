/*
 * SharpDocumentationTargetProvider - the platform's socket for "quick documentation" (hover, Ctrl+Q).
 *
 * WHAT THIS IS FOR
 *   Since 2023.1 the platform asks a DocumentationTargetProvider "which documentation targets are at this
 *   offset of this file?". It needs no PSI structure behind the offset - exactly right for Sharp, whose
 *   PSI is flat because the compiler is the parser (doc/ide.md 6.1). The answer is built by HoverModel
 *   from the index; a target only holds the finished texts (strings), so its "pointer" can simply be the
 *   target itself and never dangles.
 *
 *   A target has three jobs: a presentation (the title and icon of the popup), a hint (one line, shown
 *   on Ctrl+hover) and the documentation page (HTML).
 *
 *   Never throws: no index, no snapshot, an offset the index does not know -> an empty list.
 */
package sharp.ide.documentation

import com.intellij.icons.AllIcons
import com.intellij.model.Pointer
import com.intellij.openapi.diagnostic.Logger
import com.intellij.openapi.progress.ProcessCanceledException
import com.intellij.platform.backend.documentation.DocumentationResult
import com.intellij.platform.backend.documentation.DocumentationTarget
import com.intellij.platform.backend.documentation.DocumentationTargetProvider
import com.intellij.platform.backend.presentation.TargetPresentation
import com.intellij.psi.PsiFile
import sharp.ide.editor.FileViews

class SharpDocumentationTargetProvider : DocumentationTargetProvider {

    override fun documentationTargets(file: PsiFile, offset: Int): List<DocumentationTarget> {
        try {
            val virtualFile = file.virtualFile ?: return emptyList()
            val text = file.viewProvider.document?.text ?: file.text
            val view = FileViews.forFile(file.project, virtualFile, text) ?: return emptyList()
            val info = HoverModel.at(view, offset) ?: return emptyList()
            return listOf(SharpDocumentationTarget(info))
        } catch (e: ProcessCanceledException) {
            throw e
        } catch (e: Throwable) {
            LOG.debug("Sharp hover failed quietly", e)
            return emptyList()
        }
    }

    private companion object {
        val LOG = Logger.getInstance(SharpDocumentationTargetProvider::class.java)
    }
}

class SharpDocumentationTarget(private val info: HoverInfo) : DocumentationTarget {

    override fun createPointer(): Pointer<out DocumentationTarget> = Pointer.hardPointer(this)

    override fun computePresentation(): TargetPresentation =
        TargetPresentation.builder(info.name.ifEmpty { info.kind })
            .icon(iconFor(info.kind))
            .locationText(info.declaredIn)
            .presentation()

    private fun iconFor(kind: String) = when {
        kind == "class" || kind == "interface" || kind == "enum" -> AllIcons.Nodes.Class
        kind.contains("function") || kind == "constructor" || kind == "operator" -> AllIcons.Nodes.Method
        kind == "field" || kind == "property" || kind == "global variable" || kind == "enum value" -> AllIcons.Nodes.Field
        else -> AllIcons.Nodes.Variable
    }

    override fun computeDocumentationHint(): String = info.hint

    override fun computeDocumentation(): DocumentationResult = DocumentationResult.documentation(HoverHtml.page(info))
}
