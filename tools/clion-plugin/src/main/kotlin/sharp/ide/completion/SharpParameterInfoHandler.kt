/*
 * SharpParameterInfoHandler - the hint that shows a call's parameters while its arguments are typed.
 *
 * WHAT THIS IS FOR
 *   After `names.add(` the editor shows `element: string` and puts the parameter being typed in bold;
 *   a comma moves the bold on. For an overloaded function every overload is shown. The platform drives
 *   a handler in a fixed order (doc/ide.md 6.8, "parameter info"):
 *
 *     findElementForParameterInfo     the caret is in a call? answer with a PSI element at its `(`
 *                                     and put the overloads in `itemsToShow`
 *     showParameterInfo               show the hint at that `(`
 *     findElementForUpdatingParameterInfo / updateParameterInfo   after every keystroke: still the
 *                                     same call? which argument is the caret in?
 *     updateUI                        the text of one overload, with the current parameter highlighted
 *
 *   Which call and which overloads is worked out from the TOKENS and the index, the same way
 *   completion finds a receiver (CallSite, ReceiverResolver.calleesOf). The PSI is flat, so the
 *   "owner" element is the token `(`.
 *
 *   Never throws: nothing found means no hint.
 */
package sharp.ide.completion

import com.intellij.lang.parameterInfo.CreateParameterInfoContext
import com.intellij.lang.parameterInfo.ParameterInfoHandler
import com.intellij.lang.parameterInfo.ParameterInfoUIContext
import com.intellij.lang.parameterInfo.UpdateParameterInfoContext
import com.intellij.openapi.diagnostic.Logger
import com.intellij.openapi.progress.ProcessCanceledException
import com.intellij.psi.PsiElement
import com.intellij.psi.PsiFile
import sharp.ide.editor.FileViews
import sharp.ide.lexer.CodeTokens

/** One overload as the hint shows it. */
class ParameterHint(val parameters: List<String>, val isConstructor: Boolean)

object ParameterHints {

    /** The overloads of the call at `caret` in `text`, or empty; also the call site. */
    fun overloads(view: sharp.ide.editor.FileView, text: String, caret: Int): Pair<CallSiteInfo, List<ParameterHint>>? {
        val tokens = CodeTokens.scan(text)
        val site = CallSite.at(tokens, caret) ?: return null
        val callees = CompletionEngine.resolverAt(view, tokens, caret).calleesOf(tokens.code, site.nameIndex)
        if (callees.isEmpty()) return null
        return site to callees.distinctBy { it.signature }.map { callee ->
            ParameterHint(parametersOf(callee.signature), callee.function.isConstructor)
        }
    }

    /** `add(index: int, element: string) : x` -> [`index: int`, `element: string`]. */
    fun parametersOf(signature: String): List<String> {
        val inner = CompletionEngine.parametersOf(signature).removePrefix("(").removeSuffix(")")
        if (inner.isBlank()) return emptyList()
        val parts = ArrayList<String>()
        var depth = 0
        val current = StringBuilder()
        for (ch in inner) {
            when {
                ch == '(' || ch == '<' || ch == '[' -> { depth++; current.append(ch) }
                ch == ')' || ch == '>' || ch == ']' -> { depth--; current.append(ch) }
                ch == ',' && depth == 0 -> { parts.add(current.toString().trim()); current.setLength(0) }
                else -> current.append(ch)
            }
        }
        if (current.isNotBlank()) parts.add(current.toString().trim())
        return parts
    }

    /** The text of the hint and the range to highlight for the argument number `current`. */
    fun render(hint: ParameterHint, current: Int): Triple<String, Int, Int> {
        if (hint.parameters.isEmpty()) return Triple("<no parameters>", -1, -1)
        val text = hint.parameters.joinToString(", ")
        if (current < 0 || current >= hint.parameters.size) return Triple(text, -1, -1)
        val start = hint.parameters.take(current).sumOf { it.length + 2 }
        return Triple(text, start, start + hint.parameters[current].length)
    }
}

class SharpParameterInfoHandler : ParameterInfoHandler<PsiElement, ParameterHint> {

    override fun findElementForParameterInfo(context: CreateParameterInfoContext): PsiElement? {
        val found = find(context.file, context.offset, context.editor.document.text) ?: return null
        context.itemsToShow = found.second.toTypedArray()
        return found.first
    }

    override fun showParameterInfo(element: PsiElement, context: CreateParameterInfoContext) {
        context.showHint(element, element.textRange.startOffset, this)
    }

    override fun findElementForUpdatingParameterInfo(context: UpdateParameterInfoContext): PsiElement? =
        find(context.file, context.offset, context.editor.document.text)?.first

    override fun updateParameterInfo(parameterOwner: PsiElement, context: UpdateParameterInfoContext) {
        try {
            val tokens = CodeTokens.scan(context.editor.document.text)
            val site = CallSite.at(tokens, context.offset)
            if (site == null || site.parenOffset != parameterOwner.textRange.startOffset) {
                context.removeHint()
                return
            }
            context.setCurrentParameter(site.argumentIndex)
        } catch (e: ProcessCanceledException) {
            throw e
        } catch (e: Throwable) {
            LOG.debug("Sharp parameter info failed quietly", e)
        }
    }

    override fun updateUI(p: ParameterHint?, context: ParameterInfoUIContext) {
        if (p == null) return
        val (text, from, to) = ParameterHints.render(p, context.currentParameterIndex)
        context.setupUIComponentPresentation(text, from, to, !context.isUIComponentEnabled, false, false, context.defaultParameterColor)
    }

    private fun find(file: PsiFile, caret: Int, text: String): Pair<PsiElement, List<ParameterHint>>? {
        try {
            val virtualFile = file.virtualFile ?: return null
            val view = FileViews.forFile(file.project, virtualFile, text) ?: return null
            val (site, hints) = ParameterHints.overloads(view, text, caret) ?: return null
            val owner = file.findElementAt(site.parenOffset) ?: return null
            return owner to hints
        } catch (e: ProcessCanceledException) {
            throw e
        } catch (e: Throwable) {
            LOG.debug("Sharp parameter info failed quietly", e)
            return null
        }
    }

    private companion object {
        val LOG = Logger.getInstance(SharpParameterInfoHandler::class.java)
    }
}
