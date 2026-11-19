package sharp.ide.language

import com.intellij.openapi.util.IconLoader
import javax.swing.Icon

/**
 * THE ICONS OF THE PLUGIN
 *
 * One icon for now: the file icon (a sharp sign). It lives in
 * src/main/resources/icons/sharp.svg; the platform picks `sharp_dark.svg`
 * by itself when the IDE theme is dark.
 */
object SharpIcons {
    @JvmField
    val FILE: Icon = IconLoader.getIcon("/icons/sharp.svg", SharpIcons::class.java)
}
