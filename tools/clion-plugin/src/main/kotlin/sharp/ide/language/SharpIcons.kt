package sharp.ide.language

import com.intellij.openapi.util.IconLoader
import javax.swing.Icon

/**
 * THE ICONS OF THE PLUGIN
 *
 * One icon for now: the file icon (the blade of the Sharp logo). It lives in
 * src/main/resources/icons/sharp.png (16 x 16); the platform picks
 * `sharp@2x.png` (32 x 32) by itself on a high-resolution screen. The blade
 * reads on a light and on a dark IDE theme alike, so there is no `_dark` file.
 */
object SharpIcons {
    @JvmField
    val FILE: Icon = IconLoader.getIcon("/icons/sharp.png", SharpIcons::class.java)
}
