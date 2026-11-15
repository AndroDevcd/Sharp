/*
 * PathTranslator - file paths between Windows and WSL.
 *
 * WHAT THIS IS FOR
 *   On this machine the IDE is a Windows program and the Sharp compiler is a Linux
 *   program that runs inside WSL (Windows Subsystem for Linux). The two name the same
 *   file differently:
 *
 *       Windows (what the editor says)         WSL (what the compiler says)
 *       C:\Users\me\my proj\a.sharp            /mnt/c/Users/me/my proj/a.sharp
 *       \\wsl.localhost\Ubuntu\home\me\a.sharp /home/me/a.sharp
 *
 *   Every path the plugin hands to the compiler is translated Windows -> WSL, and every
 *   path the compiler writes back (in diagnostics.json) is translated WSL -> Windows,
 *   so the plugin can compare "the file in this editor" with "the file this error is
 *   in". See doc/ide.md section 3.3.
 *
 * THE TWO WINDOWS FORMS
 *   The editor's own form (`VirtualFile.getPath()`) uses forward slashes: `C:/Users/me/a.sharp`.
 *   A Windows program such as wsl.exe is given backslashes: `C:\Users\me\a.sharp`.
 *   This class reads both and writes either one (`toWindows(path, forwardSlashes)`).
 *
 * WHAT IT DOES NOT DO
 *   It never touches the disk and never starts a process: it is plain text work, so it
 *   can be tested with a table (PathTranslatorTest).
 *
 * C++ comparison: none; the C++ compiler never needs this, it runs where the files are.
 */
package sharp.ide.paths

/**
 * Translates paths. `mountRoot` is where WSL mounts the Windows drives ("/mnt" unless the
 * distribution's /etc/wsl.conf says otherwise). `distro` is the name of the WSL
 * distribution ("Ubuntu"), needed only to turn a Linux path such as /home/me/a.sharp
 * back into `\\wsl.localhost\Ubuntu\home\me\a.sharp`; when it is empty such a path has
 * no Windows form and `toWindows` answers null.
 */
class PathTranslator(mountRoot: String = "/mnt", private val distro: String = "") {

    /** The mount root without a trailing slash and with exactly one leading slash. */
    private val mount: String = "/" + mountRoot.trim().trim('/', '\\').replace('\\', '/')

    /**
     * Windows (or already-Linux) path -> the path the compiler inside WSL uses.
     *
     * - `C:\a b\c` or `C:/a b/c`  ->  `/mnt/c/a b/c` (the drive letter is lowercased)
     * - `\\wsl.localhost\Ubuntu\home\me` or `\\wsl$\Ubuntu\home\me`  ->  `/home/me`
     * - a path that already starts with `/` is returned as it is (only slashes tidied)
     * - anything else (a relative path, an empty string) is returned unchanged except for
     *   the slashes, since there is nothing sensible to do with it.
     */
    fun toWsl(path: String): String {
        val slashes = path.replace('\\', '/')
        val unc = parseWslUnc(slashes)
        if (unc != null) return unc.second
        if (slashes.length >= 2 && slashes[1] == ':' && slashes[0].isLetter()) {
            val rest = slashes.substring(2).trimEnd('/')
            return mount + "/" + slashes[0].lowercaseChar() + (if (rest.startsWith("/")) rest else if (rest.isEmpty()) "" else "/$rest")
        }
        return stripTrailingSlash(slashes)
    }

    /**
     * WSL path -> the Windows path of the same file, or null when it has none.
     *
     * - `/mnt/c/a b/c`  ->  `C:\a b\c` (or `C:/a b/c` when `forwardSlashes` is true)
     * - `/home/me/a`    ->  `\\wsl.localhost\<distro>\home\me\a`, only when the distribution's
     *   name is known; otherwise null
     * - a path that is not absolute (does not start with `/`) is returned unchanged: it is
     *   not a WSL path, so it is probably a Windows path already.
     */
    fun toWindows(path: String, forwardSlashes: Boolean = false): String? {
        if (!path.startsWith("/")) return path
        val clean = stripTrailingSlash(path)
        val windows: String =
            if (clean == mount || clean.startsWith("$mount/")) {
                val afterMount = clean.removePrefix(mount).trimStart('/')
                val drive = afterMount.substringBefore('/')
                if (drive.length == 1 && drive[0].isLetter()) {
                    val rest = afterMount.substring(1)               // "" or "/x/y"
                    drive.uppercase() + ":" + (if (rest.isEmpty()) "/" else rest)
                } else {
                    return viaDistro(clean, forwardSlashes)
                }
            } else {
                return viaDistro(clean, forwardSlashes)
            }
        return if (forwardSlashes) windows else windows.replace('/', '\\')
    }

    /** The editor's form of a compiler path: forward slashes, as `VirtualFile.getPath()` says it. */
    fun toEditorPath(compilerPath: String): String? = toWindows(compilerPath, forwardSlashes = true)

    /** The distribution name seen in the first `\\wsl...` path, so a Linux path can be turned back. */
    fun withDistroFrom(path: String): PathTranslator {
        val found = parseWslUnc(path.replace('\\', '/'))?.first ?: return this
        return if (found == distro) this else PathTranslator(mount, found)
    }

    private fun viaDistro(linuxPath: String, forwardSlashes: Boolean): String? {
        if (distro.isEmpty()) return null
        val unc = "//wsl.localhost/$distro$linuxPath"
        return if (forwardSlashes) unc else unc.replace('/', '\\')
    }

    /** `//wsl.localhost/Ubuntu/home/me` or `//wsl$/Ubuntu/home/me` -> (`Ubuntu`, `/home/me`), else null. */
    private fun parseWslUnc(slashes: String): Pair<String, String>? {
        val lower = slashes.lowercase()
        val prefix = when {
            lower.startsWith("//wsl.localhost/") -> "//wsl.localhost/"
            lower.startsWith("//wsl$/") -> "//wsl$/"
            else -> return null
        }
        val afterHost = slashes.substring(prefix.length)
        val name = afterHost.substringBefore('/')
        if (name.isEmpty()) return null
        val rest = afterHost.substring(name.length)
        return name to (if (rest.isEmpty()) "/" else stripTrailingSlash(rest))
    }

    private fun stripTrailingSlash(text: String): String =
        if (text.length > 1 && text.endsWith("/")) text.trimEnd('/').ifEmpty { "/" } else text
}
