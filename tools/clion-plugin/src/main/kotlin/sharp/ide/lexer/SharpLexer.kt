package sharp.ide.lexer

import com.intellij.lexer.LexerBase
import com.intellij.psi.tree.IElementType

/**
 * THE LEXER: CUTS SHARP TEXT INTO TOKENS FOR THE EDITOR
 *
 * A lexer turns characters into tokens (a keyword, a name, a string ...). The
 * editor runs it while you type, to colour the text; it must be fast and must
 * never fail. This one follows `scan()` in
 * lib/support/bootstrap/frontend/lexer/lexer.sharp rule for rule, so the editor
 * and the compiler agree on where a token begins and ends. Where the two differ
 * it is only to be gentle (an editor shows half-typed text all the time):
 *
 *  - the compiler reports an unterminated string or character as an error; here
 *    the text is still a string / character token, up to the line end;
 *  - the compiler cuts a string into finer pieces only at `$`; here escapes
 *    (`\n`, `$$`) are pieces too (see SharpTokenKinds);
 *  - comments and whitespace are tokens here (the compiler drops them).
 *
 * The rules, as the compiler has them:
 *  - identifiers: ASCII letters, digits and `_`; a word in SharpKeywords is a keyword;
 *  - numbers: `12`, `1_000`, `0xFF_ff`, `1.5` (a `.` joins the number only when
 *    a digit follows, so `5.to_string()` is a call), exponent `1e9`, `2.5E-3`;
 *  - a character `'a'`, `'\n'`: up to the closing quote on the same line;
 *  - `//` to the end of the line; `/* ... */` nests; `/** */` is a doc comment;
 *  - a string ends on the line it starts on. A backslash takes the next
 *    character; `$$` is one dollar; `$name` and `${ code }` are interpolations.
 *    Inside `${ }` braces are counted, so the `}` of a lambda does not end it,
 *    and a string inside may have its own `${ }` (the compiler allows 32 levels);
 *  - a line end inside `${ ` ends the interpolation (the compiler's recovery, so
 *    one mistake does not turn the rest of the file into "inside a string").
 *
 * RESTARTING. After an edit the platform does not lex the whole file again: it
 * restarts at an earlier token whose state was 0. `getState()` is therefore 0
 * only between tokens of ordinary code and 1 anywhere inside a string or an
 * interpolation. A restart then always begins before the string, and the stack
 * of open `${` (which has depth) never has to be squeezed into the state number.
 * Comments are one token however long, so they need no state.
 *
 * C++ comparison: the same hand-written switch-on-first-character scanner that
 * the C++ compiler's tokenizer is.
 */
class SharpLexer : LexerBase() {
    private var buffer: CharSequence = ""
    private var bufferEnd = 0

    // where the current token lies, and its kind (null = end of text)
    private var tokenStart = 0
    private var tokenEnd = 0
    private var tokenType: IElementType? = null
    /** the lexer state BEFORE the current token */
    private var tokenState = 0

    /** where scanning goes on: just after the current token */
    private var position = 0

    // ---- state that carries from one token to the next (see "RESTARTING")
    /** the next characters are string text (inside a string, between pieces) */
    private var inString = false
    /** brace depth inside each open `${`; the depth of `${ ... }` counts `{` seen since it opened */
    private val interpolationDepths = IntArray(MAX_INTERPOLATION_LEVELS)
    private var interpolationTop = 0

    override fun start(buffer: CharSequence, startOffset: Int, endOffset: Int, initialState: Int) {
        this.buffer = buffer
        this.bufferEnd = endOffset
        this.position = startOffset
        // The platform only restarts at state 0 (see "RESTARTING"), so there is
        // never an open string or interpolation to rebuild.
        this.inString = false
        this.interpolationTop = 0
        advance()
    }

    override fun getState(): Int = tokenState

    override fun getTokenType(): IElementType? = tokenType

    override fun getTokenStart(): Int = tokenStart

    override fun getTokenEnd(): Int = tokenEnd

    override fun getBufferSequence(): CharSequence = buffer

    override fun getBufferEnd(): Int = bufferEnd

    override fun advance() {
        tokenStart = position
        tokenState = if (inString || interpolationTop > 0) 1 else 0
        if (position >= bufferEnd) {
            tokenType = null
            tokenEnd = position
            return
        }
        // a string ends on the line it starts on: a line end in string text is
        // whitespace again, never part of the string
        if (inString && buffer[position] == '\n') inString = false
        tokenType = if (inString) scanStringPiece() else scanCode()
        tokenEnd = position
    }

    // ------------------------------------------------------------------ code

    private fun scanCode(): IElementType {
        val c = buffer[position]
        return when {
            isSpace(c) || c == '\n' -> scanWhitespace()
            isLetter(c) -> scanWord()
            isDigit(c) -> scanNumber()
            c == '"' -> {
                position++
                inString = true
                scanStringPiece(afterQuote = true)
            }
            c == '\'' -> scanCharacter()
            isPunctuation(c) -> scanPunctuation(c)
            else -> {
                // a character that starts nothing; a surrogate pair stays together
                position++
                if (Character.isHighSurrogate(c) && position < bufferEnd && Character.isLowSurrogate(buffer[position])) position++
                SharpTokenKinds.BAD_CHARACTER
            }
        }
    }

    private fun scanWhitespace(): IElementType {
        while (position < bufferEnd) {
            val c = buffer[position]
            if (c == '\n') {
                // A line end inside `${` ends the interpolation: the string can
                // not be finished any more (the compiler's recovery).
                interpolationTop = 0
            } else if (!isSpace(c)) {
                break
            }
            position++
        }
        return SharpTokenKinds.WHITE_SPACE
    }

    private fun scanWord(): IElementType {
        val start = position
        position++
        while (position < bufferEnd && (isLetter(buffer[position]) || isDigit(buffer[position]))) position++
        return SharpKeywords.kindOf(buffer.subSequence(start, position)) ?: SharpTokenKinds.IDENTIFIER
    }

    private fun scanNumber(): IElementType {
        var kind: IElementType = SharpTokenKinds.INTEGER
        val first = buffer[position]
        position++
        if (first == '0' && position < bufferEnd && (buffer[position] == 'x' || buffer[position] == 'X')) {
            kind = SharpTokenKinds.HEX
            position++
            while (position < bufferEnd && (isHexDigit(buffer[position]) || buffer[position] == '_')) position++
            return kind
        }
        while (position < bufferEnd && (isDigit(buffer[position]) || buffer[position] == '_')) position++
        // a fraction: a `.` that a digit follows (so `5.to_string()` stays a call)
        if (position + 1 < bufferEnd && buffer[position] == '.' && isDigit(buffer[position + 1])) {
            kind = SharpTokenKinds.DECIMAL
            position++
            while (position < bufferEnd && (isDigit(buffer[position]) || buffer[position] == '_')) position++
        }
        // an exponent, only when digits really follow it
        if (position < bufferEnd && (buffer[position] == 'e' || buffer[position] == 'E')) {
            var j = position + 1
            if (j < bufferEnd && (buffer[j] == '+' || buffer[j] == '-')) j++
            if (j < bufferEnd && isDigit(buffer[j])) {
                kind = SharpTokenKinds.DECIMAL
                position = j
                while (position < bufferEnd && isDigit(buffer[position])) position++
            }
        }
        return kind
    }

    /** 'c' or '\n': up to the closing quote, on the same line */
    private fun scanCharacter(): IElementType {
        position++
        while (position < bufferEnd && buffer[position] != '\n') {
            val c = buffer[position]
            if (c == '\'') {
                position++
                break
            }
            if (c == '\\') position++
            position++
        }
        if (position > bufferEnd) position = bufferEnd
        return SharpTokenKinds.CHAR
    }

    private fun scanPunctuation(c: Char): IElementType {
        position++
        val next = if (position < bufferEnd) buffer[position] else '\u0000'

        if (c == '/' && next == '/') {
            while (position < bufferEnd && buffer[position] != '\n') position++
            return SharpTokenKinds.LINE_COMMENT
        }
        if (c == '/' && next == '*') return scanBlockComment()

        // Inside `${ }` braces are counted; the `}` that brings the count back to
        // where the `${` started ends the interpolation.
        if (interpolationTop > 0) {
            if (c == '{') {
                interpolationDepths[interpolationTop - 1]++
            } else if (c == '}') {
                if (interpolationDepths[interpolationTop - 1] == 0) {
                    interpolationTop--
                    inString = true             // string text follows
                    return SharpTokenKinds.INTERP_CLOSE
                }
                interpolationDepths[interpolationTop - 1]--
            }
        }

        // two- and three-character operators: only their length matters for the
        // colours, and `?.` / `!!.` are dots
        var kind: IElementType = single(c)
        if (next == '=') {
            if (c in "<>=+-*/&|^%!:") { kind = SharpTokenKinds.OPERATOR; position++ }
        } else if (next == c) {
            if (c in "+-&|<>*") {
                kind = SharpTokenKinds.OPERATOR
                position++
            } else if (c == '!') {
                kind = SharpTokenKinds.OPERATOR
                position++
                if (position < bufferEnd && buffer[position] == '.') { kind = SharpTokenKinds.DOT; position++ }
            }
        } else if (c == '-' && next == '>') {
            kind = SharpTokenKinds.OPERATOR; position++
        } else if (c == '?' && next == '.') {
            kind = SharpTokenKinds.DOT; position++
        } else if (c == '?' && next == ':') {
            kind = SharpTokenKinds.OPERATOR; position++
        }
        return kind
    }

    /** `/* ... */`, nesting; one token however long. `position` is just after the `/`. */
    private fun scanBlockComment(): IElementType {
        val start = position - 1
        var depth = 1
        position++                                  // the `*`
        while (position < bufferEnd && depth > 0) {
            val c = buffer[position]
            if (c == '*' && position + 1 < bufferEnd && buffer[position + 1] == '/') {
                depth--
                position++
            } else if (c == '/' && position + 1 < bufferEnd && buffer[position + 1] == '*') {
                depth++
                position++
            }
            position++
        }
        // `/**/` is an empty comment, not a doc comment
        val isDoc = position - start > 4 && buffer[start + 2] == '*'
        return if (isDoc) SharpTokenKinds.DOC_COMMENT else SharpTokenKinds.BLOCK_COMMENT
    }

    private fun single(c: Char): IElementType = when (c) {
        '{' -> SharpTokenKinds.LBRACE
        '}' -> SharpTokenKinds.RBRACE
        '(' -> SharpTokenKinds.LPAREN
        ')' -> SharpTokenKinds.RPAREN
        '[' -> SharpTokenKinds.LBRACKET
        ']' -> SharpTokenKinds.RBRACKET
        ',' -> SharpTokenKinds.COMMA
        ';' -> SharpTokenKinds.SEMICOLON
        '.' -> SharpTokenKinds.DOT
        else -> SharpTokenKinds.OPERATOR
    }

    // --------------------------------------------------------------- strings

    /**
     * One piece of a string: plain text up to the next escape / interpolation /
     * closing quote, an escape, `$name`, or the `${` that opens code.
     * `inString` is true and `position` is at the piece's first character (or
     * just after the opening quote, when `afterQuote`).
     */
    private fun scanStringPiece(afterQuote: Boolean = false): IElementType {
        if (!afterQuote) {
            val c = buffer[position]
            val next = if (position + 1 < bufferEnd) buffer[position + 1] else '\u0000'
            if (c == '\\') {
                // a backslash takes the next character with it
                position = minOf(position + 2, bufferEnd)
                return SharpTokenKinds.STRING_ESCAPE
            }
            if (c == '$' && next == '$') {
                position += 2
                return SharpTokenKinds.STRING_ESCAPE
            }
            if (c == '$' && next == '{') {
                position += 2
                inString = false
                if (interpolationTop < MAX_INTERPOLATION_LEVELS) {
                    interpolationDepths[interpolationTop] = 0
                    interpolationTop++
                } else {
                    // 32 strings inside each other: the compiler gives up on this one
                    inString = true
                }
                return SharpTokenKinds.INTERP_OPEN
            }
            if (c == '$' && isLetter(next)) {
                position++
                while (position < bufferEnd && (isLetter(buffer[position]) || isDigit(buffer[position]))) position++
                return SharpTokenKinds.INTERP_NAME
            }
        }
        // plain text, up to whatever ends the piece
        while (position < bufferEnd) {
            val c = buffer[position]
            if (c == '\n') {
                // a string ends on the line it starts on
                inString = false
                return SharpTokenKinds.STRING_TEXT
            }
            if (c == '"') {
                position++
                inString = false
                return SharpTokenKinds.STRING_TEXT
            }
            if (c == '\\') return SharpTokenKinds.STRING_TEXT
            if (c == '$') {
                val next = if (position + 1 < bufferEnd) buffer[position + 1] else '\u0000'
                if (next == '$' || next == '{' || isLetter(next)) return SharpTokenKinds.STRING_TEXT
            }
            position++
        }
        inString = false                            // the file ends inside the string
        return SharpTokenKinds.STRING_TEXT
    }

    // ------------------------------------------------------ character classes

    /** the compiler's C_SPACE: blank, tab, carriage return, vertical tab, backspace */
    private fun isSpace(c: Char) = c == ' ' || c == '\t' || c == '\r' || c == '\u000B' || c == '\b'

    /** the compiler's C_LETTER: a-z A-Z _ */
    private fun isLetter(c: Char) = (c in 'a'..'z') || (c in 'A'..'Z') || c == '_'

    private fun isDigit(c: Char) = c in '0'..'9'

    private fun isHexDigit(c: Char) = isDigit(c) || (c in 'a'..'f') || (c in 'A'..'F')

    private fun isPunctuation(c: Char) = c in "(){}[]#.+-*/%:;,<>=&|^!?$@"

    private companion object {
        const val MAX_INTERPOLATION_LEVELS = 32
    }
}
