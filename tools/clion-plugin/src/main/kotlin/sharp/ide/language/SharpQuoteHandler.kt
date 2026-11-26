package sharp.ide.language

import com.intellij.codeInsight.editorActions.SimpleTokenSetQuoteHandler
import sharp.ide.lexer.SharpTokenKinds

/**
 * QUOTE HANDLING
 *
 * Typing a double or single quote inserts the closing quote too, and typing the
 * closing quote over an existing one just steps past it. Strings and characters
 * are the two kinds of token that start with a quote. Registered as `quoteHandler`.
 */
class SharpQuoteHandler : SimpleTokenSetQuoteHandler(SharpTokenKinds.STRING_TEXT, SharpTokenKinds.CHAR)
