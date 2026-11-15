package sharp.ide.language

import com.intellij.lang.BracePair
import com.intellij.lang.PairedBraceMatcher
import com.intellij.psi.PsiFile
import com.intellij.psi.tree.IElementType
import sharp.ide.lexer.SharpTokenKinds

/**
 * BRACE MATCHING
 *
 * With the caret next to a parenthesis, bracket or curly brace the editor
 * highlights the partner, and typing the opener can insert the closer. The
 * dollar-brace pair of a string interpolation is a pair too.
 * Registered as `lang.braceMatcher`.
 */
class SharpBraceMatcher : PairedBraceMatcher {
    override fun getPairs(): Array<BracePair> = PAIRS

    // A closer may be typed in front of any token, whatever it is.
    override fun isPairedBracesAllowedBeforeType(lbraceType: IElementType, contextType: IElementType?): Boolean = true

    override fun getCodeConstructStart(file: PsiFile?, openingBraceOffset: Int): Int = openingBraceOffset

    private companion object {
        val PAIRS = arrayOf(
            BracePair(SharpTokenKinds.LBRACE, SharpTokenKinds.RBRACE, true),
            BracePair(SharpTokenKinds.LPAREN, SharpTokenKinds.RPAREN, false),
            BracePair(SharpTokenKinds.LBRACKET, SharpTokenKinds.RBRACKET, false),
            BracePair(SharpTokenKinds.INTERP_OPEN, SharpTokenKinds.INTERP_CLOSE, false),
        )
    }
}
