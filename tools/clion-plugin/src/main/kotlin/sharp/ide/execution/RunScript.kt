/*
 * BUILD, THEN RUN: the small script a "Sharp Program" run configuration starts
 *
 * Running a Sharp program is two steps - the compiler writes the program, the VM runs it - and the
 * Run console is the console of ONE process. So the run configuration starts one small script that
 * does both (sharp.ide.execution.SharpRunState writes it next to the program and starts it):
 *
 *     Compiling shapes ...
 *     (the compiler, its output kept in a file)
 *         it failed, or wrote no program  ->  the file is printed: the errors, with their places
 *                                             (SharpConsoleFilter makes them links), and the run ends
 *         it worked                       ->  nothing of it is shown: the console is the program's;
 *                                             the build's stamp is kept (RunStamp.kt)
 *     (the VM on the program, with the program's arguments, in the working folder)
 *
 * When nothing the build depends on has changed since the last good build (RunStamp.kt), the
 * compiler is not started at all: the script says `shapes is up to date` and runs the program.
 *
 * The script is a `.cmd` file on Windows and a `sh` script elsewhere; both are written by this file
 * from the same values, as plain text, so they are tested without starting anything.
 */
package sharp.ide.execution

/** Everything the script needs. Paths may use either slash; each script writes them its own way. */
data class RunScriptValues(
    /** What the first line calls the program: `shapes`. */
    val title: String,
    val vm: String,
    val compiler: String,
    /**
     * What follows the compiler on its command line, each a path or a plain word:
     *   a folder of files     `-files <list> -o <output>`      (the files are in a list file: a command
     *                                                            line has a length limit a few hundred paths pass)
     *   a project             `-P <folder> -L <library>`       (settings.json decides the rest)
     */
    val compileArguments: List<CompileArgument>,
    /** The program the compiler writes and the VM runs. */
    val output: String,
    /** Where the compiler's own output is kept. */
    val compileLog: String,
    /** Added to the compiler's command line, as typed: `-O`. */
    val compilerOptions: List<String>,
    val programArguments: List<String>,
    val workingDirectory: String,
    /** true: stop after the compiler (a "build" without a run). */
    val buildOnly: Boolean,
    /** true: the last good build is still right - the compiler is not started. */
    val upToDate: Boolean = false,
    /** The stamp of THIS build, written before the script starts ... */
    val newStamp: String = "",
    /** ... and where it is copied to when the build worked ("" for neither: no stamp is kept). */
    val keptStamp: String = "",
    /**
     * What runs the program, when it is not the VM alone: the debugger and its own options
     * (`sdb -machine 51234`). Empty: the VM. The program and its arguments follow either way.
     */
    val runner: List<String> = emptyList(),
)

/** One word of the compiler's command line: an option as it is, or a path (written the system's way and quoted). */
data class CompileArgument(val text: String, val isPath: Boolean) {
    companion object {
        fun option(text: String) = CompileArgument(text, false)
        fun path(text: String) = CompileArgument(text, true)
    }
}

object RunScript {
    /** The script for cmd.exe. Line ends are `\r\n`. */
    fun windows(v: RunScriptValues): String {
        val lines = ArrayList<String>()
        lines += "@echo off"
        lines += "chcp 65001 >nul"
        if (v.upToDate) {
            lines += "echo ${echoText(v.title)} is up to date"
        } else {
            lines += "echo Compiling ${echoText(v.title)} ..."
            lines += "if exist ${win(v.output)} del /q ${win(v.output)}"
            lines += (listOf(win(v.vm), win(v.compiler)) + v.compileArguments.map { if (it.isPath) win(it.text) else it.text } +
                v.compilerOptions.map(::winArgument)).joinToString(" ") + " > ${win(v.compileLog)} 2>&1"
            lines += "if errorlevel 1 goto failed"
            lines += "if not exist ${win(v.output)} goto failed"
            if (v.keptStamp.isNotEmpty()) lines += "copy /y ${win(v.newStamp)} ${win(v.keptStamp)} >nul"
        }
        if (v.buildOnly) {
            if (!v.upToDate) lines += "echo Built ${echoText(backslashes(v.output))}"
            lines += "exit /b 0"
        } else {
            lines += "cd /d ${win(v.workingDirectory)}"
            val runner = if (v.runner.isEmpty()) listOf(win(v.vm)) else listOf(win(v.runner[0])) + v.runner.drop(1).map(::winArgument)
            lines += (runner + win(v.output) + v.programArguments.map(::winArgument)).joinToString(" ")
            lines += "exit /b %ERRORLEVEL%"
        }
        lines += ":failed"
        lines += "type ${win(v.compileLog)}"
        lines += "exit /b 1"
        return lines.joinToString("\r\n") + "\r\n"
    }

    /** The script for `sh`. Line ends are `\n`. */
    fun unix(v: RunScriptValues): String {
        val lines = ArrayList<String>()
        lines += "#!/bin/sh"
        if (v.upToDate) {
            lines += "echo ${sh("${v.title} is up to date")}"
        } else {
            lines += "echo ${sh("Compiling ${v.title} ...")}"
            lines += "rm -f ${sh(v.output)}"
            lines += (listOf(sh(v.vm), sh(v.compiler)) + v.compileArguments.map { if (it.isPath) sh(it.text) else it.text } +
                v.compilerOptions.map(::sh)).joinToString(" ") + " > ${sh(v.compileLog)} 2>&1"
            lines += "if [ \$? -ne 0 ] || [ ! -f ${sh(v.output)} ]; then"
            lines += "    cat ${sh(v.compileLog)}"
            lines += "    exit 1"
            lines += "fi"
            if (v.keptStamp.isNotEmpty()) lines += "cp ${sh(v.newStamp)} ${sh(v.keptStamp)}"
        }
        if (v.buildOnly) {
            if (!v.upToDate) lines += "echo ${sh("Built ${v.output}")}"
        } else {
            lines += "cd ${sh(v.workingDirectory)} || exit 1"
            val runner = if (v.runner.isEmpty()) listOf(sh(v.vm)) else v.runner.map(::sh)
            lines += "exec " + (runner + sh(v.output) + v.programArguments.map(::sh)).joinToString(" ")
        }
        return lines.joinToString("\n") + "\n"
    }

    /** A path as cmd.exe wants it: backslashes, in double quotes. */
    private fun win(path: String): String = "\"" + backslashes(path).replace("%", "%%") + "\""

    private fun backslashes(path: String): String = path.replace('/', '\\')

    /**
     * One argument for a `.cmd` line: `%` doubled (it starts a variable there), and double quotes around
     * it when it has a space or one of the characters cmd.exe gives a meaning to.
     */
    private fun winArgument(argument: String): String {
        val text = argument.replace("%", "%%").replace("\"", "\\\"")
        return if (text.isEmpty() || text.any { it in " \t&|<>^()" }) "\"$text\"" else text
    }

    /** Text after `echo`: the characters cmd.exe would act on are escaped with `^`. */
    private fun echoText(text: String): String =
        buildString { for (c in text) { if (c in "&|<>^()") append('^'); if (c == '%') append('%'); append(c) } }

    /** `text` as one shell word that means exactly `text`. */
    private fun sh(text: String): String = "'" + text.replace("'", "'\\''") + "'"
}
