/*
 * TextLocals - the variables written since the compiler last ran.
 *
 * WHAT THIS IS FOR
 *   "One save behind" must not mean "the variable I declared a second ago does not complete".
 *   The index knows the locals the compiler saw (FileBodies.locals). For the function the caret
 *   is in, this class ALSO reads the current text, from the start of the function to the caret,
 *   with the lexer's tokens, and collects what was declared (doc/ide.md 6.8):
 *
 *      name : type              parameter or typed variable      `n : shape;`   `(a: int, b: string)`
 *      name := new type(...)    a variable made from a class     `s := new circle(2);`
 *      name := type(...)        the same, where `type` is a class
 *      name := "text"           `string`;  a number: `int` (or `decimal`);  true/false: `bool`
 *      name := other.chain()    the type the receiver reader (ReceiverResolver) gives the expression
 *      foreach(name in expr)    the item type of `expr`
 *      catch(name : type)       typed like any `name : type`
 *
 *   A name without a known type is still offered as a plain name, a dot after it offers nothing.
 *   The scan knows nothing about blocks (a variable of a block that has ended is still offered):
 *   accepted, the next good compiler run corrects it.
 */
package sharp.ide.completion

import sharp.ide.editor.FileView
import sharp.ide.editor.PlaceInfo
import sharp.ide.index.SharpType
import sharp.ide.lexer.SharpTokenKinds
import sharp.ide.lexer.Tok

class TextLocal(val name: String, val type: SharpType?)

object TextLocals {

    /**
     * The locals declared in the current text between `from` and `caret`. `chainType` works out the type of
     * an expression written as tokens (the receiver reader); `code` are the code tokens of the whole text.
     */
    fun read(
        view: FileView, place: PlaceInfo, code: List<Tok>, from: Int, caret: Int,
        chainType: (List<Tok>) -> SharpType?,
    ): Map<String, TextLocal> {
        val scope = view.lookup.scopeFor(view.file, view.text)
        val found = LinkedHashMap<String, TextLocal>()
        val tokens = code.filter { it.start >= from && it.end <= caret }
        var i = 0
        while (i < tokens.size) {
            val t = tokens[i]
            if (t.isIdentifier && startsStatement(tokens, i)) {
                val next = tokens.getOrNull(i + 1)
                if (next != null && next.isMark(":=")) {
                    val expression = expressionAfter(tokens, i + 2)
                    found[t.text] = TextLocal(t.text, typeOfExpression(view, scope, expression, chainType))
                    i += 2
                    continue
                }
                if (next != null && next.isMark(":")) {
                    val typeTokens = typeTokensAfter(tokens, i + 2)
                    val text = typeTokens.joinToString("") { it.text }
                    found[t.text] = TextLocal(t.text, view.lookup.resolveTypeName(text, scope, place.typeParameters))
                    i += 2 + typeTokens.size
                    continue
                }
            }
            if (t.isKeyword("foreach") && tokens.getOrNull(i + 1)?.type == SharpTokenKinds.LPAREN) {
                val name = tokens.getOrNull(i + 2)
                if (name != null && name.isIdentifier && tokens.getOrNull(i + 3)?.isKeyword("in") == true) {
                    val expression = expressionAfter(tokens, i + 4, stopAtParen = true)
                    val collection = typeOfExpression(view, scope, expression, chainType)
                    found[name.text] = TextLocal(name.text, itemTypeOf(collection))
                    i += 4
                    continue
                }
            }
            i++
        }
        return found
    }

    /** `name` is at the start of a statement or parameter: after `;` `{` `}` `(` `,` or at the start of the scanned text. */
    private fun startsStatement(tokens: List<Tok>, i: Int): Boolean {
        if (i == 0) return true
        val p = tokens[i - 1]
        return p.type == SharpTokenKinds.SEMICOLON || p.type == SharpTokenKinds.LBRACE || p.type == SharpTokenKinds.RBRACE ||
            p.type == SharpTokenKinds.LPAREN || p.type == SharpTokenKinds.COMMA
    }

    /** The tokens of an expression: up to the `;` (or a `)` that closes the foreach), at depth zero. */
    private fun expressionAfter(tokens: List<Tok>, from: Int, stopAtParen: Boolean = false): List<Tok> {
        var depth = 0
        var i = from
        while (i < tokens.size) {
            val t = tokens[i]
            if (t.type == SharpTokenKinds.LPAREN || t.type == SharpTokenKinds.LBRACKET || t.type == SharpTokenKinds.LBRACE) depth++
            else if (t.type == SharpTokenKinds.RPAREN || t.type == SharpTokenKinds.RBRACKET || t.type == SharpTokenKinds.RBRACE) {
                if (depth == 0) break
                depth--
            } else if (t.type == SharpTokenKinds.SEMICOLON && depth == 0) break
            i++
        }
        return tokens.subList(from, i)
    }

    /** The tokens of a written type after a `:` - a name, dots, `#`, `<...>`, `[]`, `?` - up to `=` `;` `,` `)` `{`. */
    private fun typeTokensAfter(tokens: List<Tok>, from: Int): List<Tok> {
        var depth = 0
        var i = from
        while (i < tokens.size) {
            val t = tokens[i]
            if (t.isMark("<")) depth++
            else if (t.isMark(">")) depth--
            else if (t.isMark(">>")) depth -= 2
            else if (depth <= 0 && (t.isMark("=") || t.type == SharpTokenKinds.SEMICOLON || t.type == SharpTokenKinds.COMMA ||
                    t.type == SharpTokenKinds.RPAREN || t.type == SharpTokenKinds.LBRACE || t.isMark(":=") || t.isMark("->"))) break
            else if (t.type == SharpTokenKinds.LPAREN) break
            i++
        }
        return tokens.subList(from, i)
    }

    private fun typeOfExpression(view: FileView, scope: sharp.ide.index.Scope, expression: List<Tok>, chainType: (List<Tok>) -> SharpType?): SharpType? {
        val first = expression.firstOrNull() ?: return null
        val string = view.lookup.index.classesById["std#string"]
        when {
            first.type == SharpTokenKinds.STRING_TEXT -> return string?.let { view.lookup.typeOfClass(it) }
            first.type == SharpTokenKinds.INTEGER && expression.size == 1 -> return view.lookup.resolveTypeName("int", scope)
            first.type == SharpTokenKinds.DECIMAL && expression.size == 1 -> return SharpType("decimal")
            first.isKeyword("true") || first.isKeyword("false") -> return SharpType("bool")
        }
        return chainType(expression)
    }

    /** The item type of a collection: `T` of `list<T>`, the element of `T[]`. */
    fun itemTypeOf(collection: SharpType?): SharpType? {
        if (collection == null) return null
        if (collection.isArray) return collection.element
        return collection.args.firstOrNull()
    }
}
