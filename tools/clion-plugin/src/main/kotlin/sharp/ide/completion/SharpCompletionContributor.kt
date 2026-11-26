/*
 * SharpCompletionContributor - the platform's socket for "Ctrl+Space" and the pop-up while typing.
 *
 * WHAT THIS IS FOR
 *   A completion contributor is called when the list is built. It adds items to a "result set".
 *   Everything that decides WHAT to offer lives in CompletionEngine (pure, tested on strings);
 *   this class only
 *     - finds the file's FileView (the index, the text the compiler saw, the text now),
 *     - asks the engine,
 *     - turns each Candidate into a LookupElement: icon, bold, type text on the right, parameters
 *       after the name, the insert handler (parentheses, the import),
 *     - and tells the result set which prefix the person has typed (`names.fo|` -> `fo`; in an
 *       import list `std.i`, because there the dots belong to the name).
 *
 *   It reads the CURRENT text of the editor and the caret (`parameters.editor`), not the copy of
 *   the file the platform makes for completion with a dummy word typed into it.
 *
 *   It never throws: any failure means "no items" (the platform would show an error balloon),
 *   except the platform's own cancellation, which must pass through.
 */
package sharp.ide.completion

import com.intellij.codeInsight.completion.CompletionContributor
import com.intellij.codeInsight.completion.CompletionParameters
import com.intellij.codeInsight.completion.CompletionProvider
import com.intellij.codeInsight.completion.CompletionResultSet
import com.intellij.codeInsight.completion.CompletionType
import com.intellij.codeInsight.completion.InsertHandler
import com.intellij.codeInsight.completion.InsertionContext
import com.intellij.codeInsight.completion.PrioritizedLookupElement
import com.intellij.codeInsight.lookup.LookupElement
import com.intellij.codeInsight.lookup.LookupElementBuilder
import com.intellij.icons.AllIcons
import com.intellij.openapi.diagnostic.Logger
import com.intellij.openapi.progress.ProcessCanceledException
import com.intellij.patterns.PlatformPatterns
import com.intellij.util.ProcessingContext
import sharp.ide.editor.FileViews
import javax.swing.Icon

class SharpCompletionContributor : CompletionContributor() {
    init {
        extend(CompletionType.BASIC, PlatformPatterns.psiElement(), SharpCompletionProvider())
    }
}

class SharpCompletionProvider : CompletionProvider<CompletionParameters>() {

    override fun addCompletions(parameters: CompletionParameters, context: ProcessingContext, result: CompletionResultSet) {
        try {
            val file = parameters.originalFile.virtualFile
            val text = parameters.editor.document.text
            val caret = parameters.offset
            val view = file?.let { FileViews.forFile(parameters.editor.project ?: parameters.originalFile.project, it, text) }
            val answer = CompletionEngine.complete(view, text, caret)
            if (answer.candidates.isEmpty()) return
            val matching = result.withPrefixMatcher(answer.prefix)
            for (candidate in answer.candidates) matching.addElement(lookupElementFor(candidate))
        } catch (e: ProcessCanceledException) {
            throw e
        } catch (e: Throwable) {
            LOG.debug("Sharp completion failed quietly", e)
        }
    }

    companion object {
        private val LOG = Logger.getInstance(SharpCompletionProvider::class.java)

        fun lookupElementFor(c: Candidate): LookupElement {
            var builder = LookupElementBuilder.create(c.name).withIcon(iconFor(c.kind)).withBoldness(c.bold)
            c.typeText?.let { builder = builder.withTypeText(it, true) }
            c.tailText?.let { builder = builder.withTailText(it, true) }
            if (c.insertion != Insertion.NONE || c.importModule != null) builder = builder.withInsertHandler(SharpInsertHandler(c))
            return PrioritizedLookupElement.withPriority(builder, c.priority.toDouble())
        }

        fun iconFor(kind: CandidateKind): Icon? = when (kind) {
            CandidateKind.LOCAL -> AllIcons.Nodes.Variable
            CandidateKind.PARAMETER -> AllIcons.Nodes.Parameter
            CandidateKind.FIELD -> AllIcons.Nodes.Field
            CandidateKind.PROPERTY -> AllIcons.Nodes.Property
            CandidateKind.METHOD -> AllIcons.Nodes.Method
            CandidateKind.CLASS -> AllIcons.Nodes.Class
            CandidateKind.INTERFACE -> AllIcons.Nodes.Interface
            CandidateKind.ENUM -> AllIcons.Nodes.Enum
            CandidateKind.ENUM_VALUE -> AllIcons.Nodes.Field
            CandidateKind.ALIAS -> AllIcons.Nodes.Type
            CandidateKind.MODULE -> AllIcons.Nodes.Package
            CandidateKind.TYPE_WORD -> AllIcons.Nodes.Type
            CandidateKind.TYPE_PARAMETER -> AllIcons.Nodes.Parameter
            CandidateKind.KEYWORD -> null
        }
    }
}

/**
 * What choosing an item does besides inserting its name: a function or a class after `new` gets `()` (the caret
 * goes inside when there are parameters), and a class of a module that is not imported also adds the import.
 */
class SharpInsertHandler(private val candidate: Candidate) : InsertHandler<LookupElement> {

    override fun handleInsert(context: InsertionContext, item: LookupElement) {
        val document = context.document
        val editor = context.editor
        if (candidate.insertion != Insertion.NONE) {
            val tail = context.tailOffset
            val alreadyCalled = tail < document.textLength && document.charsSequence[tail] == '('
            if (!alreadyCalled) {
                document.insertString(tail, "()")
                editor.caretModel.moveToOffset(if (candidate.hasParameters) tail + 1 else tail + 2)
            } else {
                editor.caretModel.moveToOffset(tail + 1)
            }
        }
        candidate.importModule?.let { module ->
            // above the caret: the platform's offsets and the caret follow the edit
            val plan = ImportEdit.plan(document.text, module)
            if (plan != null) document.insertString(plan.offset, plan.text)
        }
    }
}
