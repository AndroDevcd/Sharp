package sharp.ide.language

import com.intellij.lang.Language

/**
 * THE LANGUAGE "SHARP"
 *
 * In the IntelliJ Platform a language is only an identity: a unique name that
 * every feature (highlighting, completion, folding ...) is registered against in
 * plugin.xml (`language="Sharp"`). It holds no code. Making one object of this
 * class is what announces the language to the platform.
 *
 * Primer idea 2 in doc/ide.md section 2.
 */
object SharpLanguage : Language("Sharp") {
    // Language objects are singletons the platform may serialize by name.
    private fun readResolve(): Any = SharpLanguage

    override fun getDisplayName(): String = "Sharp"

    override fun isCaseSensitive(): Boolean = true
}
