/*
 * SharpDefaults - the settings a fresh installation starts with.
 *
 * WHAT THIS IS FOR
 *   The plugin has to work on the machine it was built for without the owner
 *   filling in a settings page: here the IDE is a Windows program and the Sharp VM
 *   is a Linux build in WSL. So the defaults describe exactly that, and they are
 *   kept in this one file (not scattered over the settings classes) so that
 *   "Reset to defaults" and the tests can ask for them.
 *
 *   The command is two texts (doc/ide.md section 3.3): a LAUNCHER (one argument
 *   per line) that starts a shell on a script file, and the SCRIPT itself. The
 *   plugin fills in the placeholders, writes the script into the run folder and
 *   starts the launcher on it. Because the long file list travels in a file and
 *   only ONE path crosses from Windows into WSL, nothing ever has to be quoted
 *   for two shells at once.
 *
 *   The default compiler is a DEVELOPMENT default: it is the build that has the
 *   editor support (`-ide`). When the finished compiler has a permanent home the
 *   owner changes the one text field "compiler".
 */
package sharp.ide.settings

import com.intellij.openapi.util.SystemInfo

object SharpDefaults {
    /** Starts bash inside WSL on the script. `{script}` is replaced by the script's path (translated for WSL). */
    const val LAUNCHER_WSL = "wsl.exe\n-e\nbash\n{script}"

    /** Starts bash directly: for a Linux or macOS IDE, where the VM runs natively. */
    const val LAUNCHER_NATIVE = "bash\n{script}"

    /**
     * The script. `mapfile` reads the file list (one path per line) into an array, which keeps a path
     * with a space in one piece. `timeout` ends the compiler from inside WSL when it runs too long
     * (killing wsl.exe from Windows does not always end the Linux process it started).
     */
    const val SCRIPT = "mapfile -t FILES < {files_list}\nexec timeout {timeout} {vm} {compiler} \"\${FILES[@]}\" -ide {ide_dir} {extra}\n"

    const val VM = "\$HOME/sharp_work/sharp_cl/sharp"
    const val COMPILER = "\$HOME/sharp_work/out/boot_ide/sharpc_boot"
    const val TIMEOUT_SECONDS = 120
    const val DEBOUNCE_MILLIS = 1500
    /** With the wait after a save, a check starts 4.5 seconds after the last key press. */
    const val TYPING_PAUSE_MILLIS = 3000
    const val MOUNT_ROOT = "/mnt"

    val EXCLUDED_FOLDERS: List<String> = listOf("tests", "build", ".git", "out", "node_modules")

    fun launcher(): String = if (SystemInfo.isWindows) LAUNCHER_WSL else LAUNCHER_NATIVE
}
