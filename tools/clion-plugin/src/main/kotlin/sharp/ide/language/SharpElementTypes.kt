package sharp.ide.language

import com.intellij.psi.tree.IElementType
import com.intellij.psi.tree.IFileElementType

/**
 * THE KINDS OF NODE IN THE (FLAT) SYNTAX TREE
 *
 * Tokens have their kinds in SharpTokenKinds. These are the two kinds that are
 * not tokens: the file as a whole, and a BLOCK, the braces pair the parser
 * groups together (the one piece of structure that is cheap to find without a
 * real parser, and what folding and "which block am I in" need).
 */
object SharpElementTypes {
    @JvmField val FILE = IFileElementType(SharpLanguage)
    @JvmField val BLOCK: IElementType = IElementType("BLOCK", SharpLanguage)
}
