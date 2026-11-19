/*
 * CommandTemplate - turns the two command settings into a real command.
 *
 * WHAT THIS IS FOR
 *   The owner's settings say HOW to run the compiler as two texts (doc/ide.md 3.3):
 *
 *     launcher   one argument per line, e.g.   wsl.exe / -e / bash / {script}
 *     script     a few lines of shell, e.g.    exec timeout {timeout} {vm} {compiler} ...
 *
 *   For every run the plugin fills in the placeholders of the script, writes it to
 *   `run.sh` in the run folder, and starts the launcher on that file. This class does
 *   the filling. It is pure text work, so it is tested with awkward paths (a space, a
 *   quote, a dollar sign).
 *
 *   Placeholders of the script:
 *     {vm}          the Sharp VM, pasted as written (so `$HOME/...` works)
 *     {compiler}    the compiled compiler, pasted as written
 *     {files_list}  path of files.txt, in single quotes
 *     {ide_dir}     the folder the compiler must write to, in single quotes
 *     {extra}       the extra arguments, each in single quotes
 *     {timeout}     the time limit in seconds
 *   Only these exact words are replaced; `${FILES[@]}` and every other `{` is left alone.
 *
 *   WHY SINGLE QUOTES: in a shell, text inside '...' is taken literally. The only
 *   character that cannot be inside is `'` itself, which is written as `'\''`
 *   (close the quote, an escaped quote, open again). That is all the quoting there is.
 */
package sharp.ide.run

/** What goes into the script's placeholders. The paths must already be in the compiler's form (WSL form behind wsl). */
data class ScriptValues(
    val vm: String,
    val compiler: String,
    val filesList: String,
    val ideDirectory: String,
    val extraArguments: List<String>,
    val timeoutSeconds: Int,
)

object CommandTemplate {

    /** `text` as one shell word that means exactly `text`. */
    fun quote(text: String): String = "'" + text.replace("'", "'\\''") + "'"

    /** The script with every placeholder filled in, line ends `\n`, and a final line end. */
    fun fillScript(template: String, values: ScriptValues): String {
        val replacements = linkedMapOf(
            "{vm}" to values.vm.trim(),
            "{compiler}" to values.compiler.trim(),
            "{files_list}" to quote(values.filesList),
            "{ide_dir}" to quote(values.ideDirectory),
            "{extra}" to values.extraArguments.joinToString(" ") { quote(it) },
            "{timeout}" to values.timeoutSeconds.toString(),
        )
        var script = template.replace("\r\n", "\n").replace('\r', '\n')
        // One pass over the text, so a replacement that itself contains `{vm}` is not replaced again.
        val pattern = Regex("""\{(vm|compiler|files_list|ide_dir|extra|timeout)}""")
        script = pattern.replace(script) { replacements.getValue(it.value) }
        return if (script.endsWith("\n")) script else script + "\n"
    }

    /** The launcher's lines as an argument list, with `{script}` replaced by the script's path. Blank lines are skipped. */
    fun launcherArguments(launcher: String, scriptPath: String): List<String> =
        launcher.replace("\r\n", "\n").split('\n')
            .map { it.trim() }
            .filter { it.isNotEmpty() }
            .map { it.replace("{script}", scriptPath) }

    /** The extra-arguments setting as a list: split on spaces (no quoting rules; one argument never contains a space). */
    fun splitExtraArguments(text: String): List<String> =
        text.trim().split(Regex("\\s+")).filter { it.isNotEmpty() }

    /** Does the launcher start wsl.exe? Then paths have to be translated (when the mode is automatic). */
    fun startsWsl(launcher: String): Boolean {
        val first = launcher.replace("\r\n", "\n").lineSequence().map { it.trim() }.firstOrNull { it.isNotEmpty() } ?: return false
        val name = first.replace('\\', '/').substringAfterLast('/').lowercase()
        return name == "wsl" || name == "wsl.exe"
    }
}
