package sharp.ide.lexer

import org.junit.Assert.assertEquals
import org.junit.Test

/**
 * mock<...> AND THE TESTING WORDS ARE MARKED WHERE THEY ARE USED - AND ONLY THERE
 *
 * They are ordinary names (a program may call a variable `test`), so each check
 * comes in pairs: the place where the word is the framework speaking, and a
 * place where it is just a name.
 */
class SharpBuiltInWordsTest {
    @Test
    fun mockFollowedByAnAngleBracketIsABuiltInCall() {
        assertEquals(
            listOf("BUILT_IN_CALL[mock]", "OPERATOR[<]", "IDENTIFIER[note_store]", "OPERATOR[>]", "LPAREN[(]", "RPAREN[)]"),
            kinds("mock<note_store>()"),
        )
    }

    @Test
    fun mockAsAClassNameIsAPlainName() {
        assertEquals(
            listOf("KEYWORD_DECLARATION[class]", "IDENTIFIER[fake]", "KEYWORD_VALUE[base]", "IDENTIFIER[mock]"),
            kinds("class fake base mock"),
        )
        assertEquals(listOf("IDENTIFIER[mock]", "OPERATOR[=]", "INTEGER[1]"), kinds("mock = 1"))
    }

    @Test
    fun theTestingWordsAreMarkedWhenCalled() {
        assertEquals("TESTING_WORD[tests]", kinds("tests(\"a\") { }").first())
        assertEquals("TESTING_WORD[test]", kinds("test(\"a\") { }").first())
        assertEquals("TESTING_WORD[expect]", kinds("expect(x)").first())
        assertEquals("TESTING_WORD[given]", kinds("given { store.load(); }").first())
        assertEquals("TESTING_WORD[verify]", kinds("verify { store.save(1); }").first())
        assertEquals("TESTING_WORD[provide]", kinds("provide<clock>() { }").first())
        assertEquals("TESTING_WORD[expect_throws]", kinds("expect_throws<e>() { }").first())
    }

    @Test
    fun aTestingWordThatIsNotCalledIsAPlainName() {
        assertEquals(listOf("IDENTIFIER[test]", "OPERATOR[=]", "INTEGER[1]"), kinds("test = 1"))
        assertEquals("IDENTIFIER[given]", kinds("given : _int64").first())
    }

    @Test
    fun aMemberOrADeclarationOfThatNameIsAPlainName() {
        assertEquals(listOf("IDENTIFIER[x]", "DOT[.]", "IDENTIFIER[verify]", "LPAREN[(]", "RPAREN[)]"), kinds("x.verify()"))
        assertEquals(
            listOf("KEYWORD_DECLARATION[def]", "IDENTIFIER[test]", "LPAREN[(]", "RPAREN[)]"),
            kinds("def test()"),
        )
    }

    @Test
    fun theCheckAfterExpectIsAnOrdinaryMember() {
        assertEquals(
            listOf("TESTING_WORD[expect]", "LPAREN[(]", "IDENTIFIER[x]", "RPAREN[)]", "DOT[.]", "IDENTIFIER[should_be]", "LPAREN[(]", "INTEGER[1]", "RPAREN[)]"),
            kinds("expect(x).should_be(1)"),
        )
    }
}
