/*
 * BUILD AND RUN, without an IDE: the installed Sharp is found, the script is right, the console's
 * places are found - and, on a machine where Sharp is installed, a real program is built and run
 * exactly the way the Run button does it.
 */
package sharp.ide.execution

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import sharp.ide.paths.ProgramFiles
import sharp.ide.paths.ProgramSettings
import sharp.ide.toolchain.SharpToolchain
import sharp.ide.toolchain.SharpToolchains
import java.io.File
import java.nio.charset.StandardCharsets
import java.nio.file.Files
import java.util.concurrent.TimeUnit

class BuildAndRunTest {
    @get:Rule
    val temp = TemporaryFolder()

    private fun touch(base: File, relative: String, text: String = "") {
        val file = File(base, relative)
        file.parentFile.mkdirs()
        file.writeText(text)
    }

    // ---------------------------------------------------------------- the installed Sharp

    @Test
    fun anInstallationIsFoundWithItsNewestCompiler() {
        val home = temp.newFolder("Sharp home")
        touch(home, "bin/sharp.exe")
        touch(home, "lib/std/list.sharp")
        touch(home, "compiler/sharpc-v0.4.0")
        touch(home, "compiler/sharpc-unoptimized-v0.4.0")
        touch(home, "compiler/sharpc-v0.10.1")
        touch(home, "compiler/sharpc-unoptimized-v0.10.1")
        touch(home, "compiler/notes.txt")
        val found = SharpToolchains.at(home.path)!!
        val root = ProgramFiles.normalise(home.path)
        assertEquals("0.10.1", found.version)                       // 0.10.1 is newer than 0.4.0: numbers, not text
        assertEquals("$root/bin/sharp.exe", found.vm)
        assertEquals("$root/compiler/sharpc-v0.10.1", found.compiler)
        assertEquals("$root/compiler/sharpc-unoptimized-v0.10.1", found.unoptimizedCompiler)
        assertEquals("$root/lib", found.library)
    }

    @Test
    fun aFolderThatIsNotACompleteInstallationIsNotOne() {
        val home = temp.newFolder("half")
        assertNull(SharpToolchains.at(home.path))
        touch(home, "bin/sharp")
        touch(home, "compiler/sharpc-v0.4.0")
        assertNull("no library", SharpToolchains.at(home.path))
        touch(home, "lib/std/list.sharp")
        val found = SharpToolchains.at(home.path)
        assertNotNull(found)
        assertNull("no unoptimized compiler beside it", found!!.unoptimizedCompiler)
        File(home, "compiler/sharpc-v0.4.0").delete()
        assertNull("no compiler", SharpToolchains.at(home.path))
    }

    @Test
    fun theDefaultPlaceFollowsTheSystem() {
        val none: (String) -> String? = { null }
        assertEquals("C:/Users/me/AppData/Local/Sharp", SharpToolchains.defaultHome({ if (it == "LOCALAPPDATA") "C:\\Users\\me\\AppData\\Local" else null }, "C:\\Users\\me", true))
        assertEquals("/home/me/.sharp", SharpToolchains.defaultHome(none, "/home/me", false))
        assertEquals("/opt/sharp", SharpToolchains.defaultHome({ if (it == "SHARP_HOME") "/opt/sharp/" else null }, "/home/me", false))
    }

    @Test
    fun theInstalledLibraryIsUsedWhenTheProjectNamesNoneAndHasNone() {
        val base = temp.newFolder("project")
        val installed = temp.newFolder("installed lib")
        touch(base, "main.sharp")
        touch(installed, "std/list.sharp")
        val plain = ProgramSettings(base.path, emptyList(), emptyList(), "", installed.path)
        assertEquals(ProgramFiles.normalise(installed.path), ProgramFiles.layoutOf(plain).libraryRoot)
        // the project's own library (the Sharp repository) comes first; a folder set by hand before both
        touch(base, "stdlib/std/list.sharp")
        assertEquals(ProgramFiles.normalise(base.path) + "/stdlib", ProgramFiles.layoutOf(plain).libraryRoot)
        val other = temp.newFolder("other lib")
        assertEquals(ProgramFiles.normalise(other.path), ProgramFiles.layoutOf(plain.copy(libraryPath = other.path)).libraryRoot)
    }

    // ---------------------------------------------------------------- the script

    private val values = RunScriptValues(
        title = "shapes",
        vm = "C:/Users/A B/Sharp/bin/sharp.exe",
        compiler = "C:/Users/A B/Sharp/compiler/sharpc-v0.4.0",
        compileArguments = listOf(
            CompileArgument.option("-files"), CompileArgument.path("C:/work/my shapes/build/.sharp-run/files.txt"),
            CompileArgument.option("-o"), CompileArgument.path("C:/work/my shapes/build/shapes"),
        ),
        output = "C:/work/my shapes/build/shapes",
        compileLog = "C:/work/my shapes/build/.sharp-run/compile.txt",
        compilerOptions = listOf("-O"),
        programArguments = listOf("one", "two words", "50%"),
        workingDirectory = "C:/work/my shapes",
        buildOnly = false,
    )

    @Test
    fun theWindowsScriptBuildsThenRuns() {
        val script = RunScript.windows(values)
        val lines = script.split("\r\n")
        assertEquals("@echo off", lines[0])
        assertTrue(script.contains("\"C:\\Users\\A B\\Sharp\\bin\\sharp.exe\" \"C:\\Users\\A B\\Sharp\\compiler\\sharpc-v0.4.0\" -files " +
            "\"C:\\work\\my shapes\\build\\.sharp-run\\files.txt\" -o \"C:\\work\\my shapes\\build\\shapes\" -O > " +
            "\"C:\\work\\my shapes\\build\\.sharp-run\\compile.txt\" 2>&1"))
        assertTrue(script.contains("if errorlevel 1 goto failed"))
        assertTrue(script.contains("cd /d \"C:\\work\\my shapes\""))
        // a space is quoted, a % is doubled (cmd.exe would read it as a variable)
        assertTrue(script.contains("\"C:\\Users\\A B\\Sharp\\bin\\sharp.exe\" \"C:\\work\\my shapes\\build\\shapes\" one \"two words\" 50%%"))
        assertTrue(script.endsWith(":failed\r\ntype \"C:\\work\\my shapes\\build\\.sharp-run\\compile.txt\"\r\nexit /b 1\r\n"))
        val buildOnly = RunScript.windows(values.copy(buildOnly = true))
        assertTrue(buildOnly.contains("echo Built C:\\work\\my shapes\\build\\shapes"))
        assertTrue(!buildOnly.contains("cd /d"))
    }

    @Test
    fun theUnixScriptBuildsThenRuns() {
        val v = values.copy(
            vm = "/home/a b/.sharp/bin/sharp", compiler = "/home/a b/.sharp/compiler/sharpc-v0.4.0",
            compileArguments = listOf(
                CompileArgument.option("-files"), CompileArgument.path("/w/it's/build/.sharp-run/files.txt"),
                CompileArgument.option("-o"), CompileArgument.path("/w/it's/build/shapes"),
            ),
            output = "/w/it's/build/shapes",
            compileLog = "/w/it's/build/.sharp-run/compile.txt", workingDirectory = "/w/it's",
        )
        val script = RunScript.unix(v)
        assertTrue(script.startsWith("#!/bin/sh\n"))
        assertTrue(script.contains("'/home/a b/.sharp/bin/sharp' '/home/a b/.sharp/compiler/sharpc-v0.4.0' -files '/w/it'\\''s/build/.sharp-run/files.txt' " +
            "-o '/w/it'\\''s/build/shapes' '-O' > '/w/it'\\''s/build/.sharp-run/compile.txt' 2>&1"))
        assertTrue(script.contains("exec '/home/a b/.sharp/bin/sharp' '/w/it'\\''s/build/shapes' 'one' 'two words' '50%'\n"))
        assertTrue(!script.contains("\r"))
    }

    // ---------------------------------------------------------------- the console

    @Test
    fun theCompilersPlacesAreFoundInConsoleLines() {
        val place = SharpConsoleFilter.placeIn("  --> C:/work/my shapes/main.sharp:12:5\n")!!
        assertEquals("C:/work/my shapes/main.sharp", place.file)
        assertEquals(12, place.line)
        assertEquals(5, place.column)
        assertEquals("C:/work/my shapes/main.sharp:12:5", "  --> C:/work/my shapes/main.sharp:12:5".substring(place.start, place.end))
        assertEquals("/home/me/a.sharp", SharpConsoleFilter.placeIn("  ::: /home/me/a.sharp:3:9")!!.file)
        assertNull(SharpConsoleFilter.placeIn("error[E0401]: `printn` is not known here"))
        assertNull(SharpConsoleFilter.placeIn("12 |     printn(1);"))
        assertNull(SharpConsoleFilter.placeIn("time: 12:30:05"))
        // where a test of std.testing failed, said in words
        val failed = "        at C:/work/my shapes/tests.sharp, line 12"
        val test = SharpConsoleFilter.placeIn(failed)!!
        assertEquals("C:/work/my shapes/tests.sharp", test.file)
        assertEquals(12, test.line)
        assertEquals("C:/work/my shapes/tests.sharp, line 12", failed.substring(test.start, test.end))
        assertNull(SharpConsoleFilter.placeIn("at the end, line 3 was read"))
    }

    @Test
    fun aCppCompilersErrorLineIsAPlaceToo() {
        // what g++ prints when a project's C++ (the native functions) does not build
        val line = "C:/work/my app/src/cpp/foo.cpp:4:1: error: expected unqualified-id before 'this'"
        val place = SharpConsoleFilter.placeIn(line)!!
        assertEquals("C:/work/my app/src/cpp/foo.cpp", place.file)
        assertEquals(4, place.line)
        assertEquals(1, place.column)
        assertEquals("C:/work/my app/src/cpp/foo.cpp:4:1", line.substring(place.start, place.end))
        assertEquals("/home/me/x.h", SharpConsoleFilter.placeIn("/home/me/x.h:10:3: note: declared here")!!.file)
        assertNull(SharpConsoleFilter.placeIn("native library: C:/work/app/build/outputs/foo.dll built from 1 C++ file(s)"))
    }

    @Test
    fun aProjectsCppFilesArePartOfWhatABuildDependsOn() {
        val folder = Files.createTempDirectory("sharp-native-project")
        Files.createDirectories(folder.resolve("src/cpp/inner"))
        Files.createDirectories(folder.resolve("native/arm"))
        Files.write(folder.resolve("native/arm/fast.cpp"), "// arm only\n".toByteArray())
        Files.write(folder.resolve("settings.json"), ("{ \"output\": \"app\", \"native\": { \"sources\": [ \"src/cpp\" ], " +
            "\"toolchain\": { \"processor\": { \"arm\": { \"sources\": [ \"native/arm\" ] } } } } }").toByteArray())
        Files.write(folder.resolve("src/main.sharp"), "mod main;\ndef main() { }\n".toByteArray())
        Files.write(folder.resolve("src/cpp/foo.cpp"), "// c++\n".toByteArray())
        Files.write(folder.resolve("src/cpp/inner/foo.h"), "// header\n".toByteArray())
        Files.write(folder.resolve("src/cpp/notes.txt"), "not C++\n".toByteArray())

        val project = sharp.ide.project.SharpProjectFiles.read(folder.toString())!!
        assertEquals("the sources of a processor's block count too", listOf("src/cpp", "native/arm"), project.nativeSources)
        val names = project.nativeSourceFiles().map { it.substringAfterLast('/') }
        assertEquals(listOf("fast.cpp", "foo.cpp", "foo.h"), names.sorted())
    }

    // ---------------------------------------------------------------- the plan, and a real run

    private fun fakeToolchain(): SharpToolchain {
        val home = temp.newFolder("toolchain")
        touch(home, "bin/sharp.exe")
        touch(home, "lib/std/list.sharp")
        touch(home, "compiler/sharpc-v0.4.0")
        return SharpToolchains.at(home.path)!!
    }

    @Test
    fun thePlanListsTheLibraryAndTheProgramFolderOnly() {
        val base = temp.newFolder("project")
        touch(base, "app/main.sharp")
        touch(base, "app/parts/a.sharp")
        touch(base, "app/tests/t.sharp")
        touch(base, "app/build/old.sharp")
        touch(base, "tools/other.sharp")
        val toolchain = fakeToolchain()
        val plan = SharpRunState.plan(toolchain, File(base, "app").path, base.path, listOf("tests", "build"), "", listOf("-O"), listOf("x"), "", false, false, true)
        val root = ProgramFiles.normalise(base.path)
        assertEquals(listOf("${toolchain.library}/std/list.sharp", "$root/app/main.sharp", "$root/app/parts/a.sharp"), plan.files)
        assertEquals(listOf("cmd.exe", "/d", "/c", plan.scriptFile.toString()), plan.command)
        assertTrue(ProgramFiles.normalise(plan.scriptFile.toString()).endsWith("/app/build/.sharp-run/run.cmd"))
        assertTrue(plan.scriptText.contains("\\app\\build\\app\""))                 // the program is build/<folder name>
        val unix = SharpRunState.plan(toolchain, File(base, "app").path, base.path, listOf("tests", "build"), "", emptyList(), emptyList(), "", true, false, false)
        assertEquals("sh", unix.command[0])
        assertTrue(unix.scriptText.contains("echo 'Built "))
    }

    @Test
    fun aDebugPlanBuildsWithDbgAndStartsTheDebuggerOnThePort() {
        val base = temp.newFolder("project")
        touch(base, "app/main.sharp")
        val home = temp.newFolder("with debugger")
        touch(home, "bin/sharp.exe")
        touch(home, "bin/sdb.exe")
        touch(home, "lib/std/list.sharp")
        touch(home, "compiler/sharpc-v0.5.6")
        val toolchain = SharpToolchains.at(home.path)!!
        assertEquals("${ProgramFiles.normalise(home.path)}/bin/sdb.exe", toolchain.debugger)

        // "build only" is not kept: a debug run always runs
        val windows = SharpRunState.plan(toolchain, File(base, "app").path, base.path, emptyList(), "", listOf("-O"), listOf("one"), "", true, false, true, debugPort = 51234)
        assertTrue(windows.scriptText, windows.scriptText.contains(" -O -dbg > "))                      // -dbg is added to what was asked for
        assertTrue(windows.scriptText, windows.scriptText.contains("\\bin\\sdb.exe\" -machine 51234 \""))
        assertTrue(windows.scriptText, windows.scriptText.contains("\\app\\build\\app\" one"))
        val unix = SharpRunState.plan(toolchain, File(base, "app").path, base.path, emptyList(), "", listOf("-dbg"), emptyList(), "", false, false, false, debugPort = 9)
        assertTrue(unix.scriptText, unix.scriptText.contains("/bin/sdb.exe' '-machine' '9' '"))
        assertTrue(unix.scriptText, !unix.scriptText.contains("-dbg' '-dbg"))                           // ... and not twice

        // a plain run of the same folder is the VM alone
        val plain = SharpRunState.plan(toolchain, File(base, "app").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true)
        assertTrue(!plain.scriptText.contains("sdb") && !plain.scriptText.contains("-dbg"))

        // an installation without sdb says so
        val message = try {
            SharpRunState.plan(fakeToolchain(), File(base, "app").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true, debugPort = 1)
            ""
        } catch (e: com.intellij.execution.ExecutionException) { e.message ?: "" }
        assertTrue(message, message.contains("has no debugger"))
    }

    @Test
    fun aPlanForAProjectsTestsBuildsWithTestsAndRunsTheTestsProgram() {
        val base = temp.newFolder("workspace")
        touch(base, "shop/settings.json", "{ \"name\": \"shop\", \"output\": \"shop\" }")
        touch(base, "shop/src/main.sharp", "mod main;\n")
        touch(base, "shop/tests/shop_tests.sharp", "mod shop_tests;\n")
        val home = temp.newFolder("new enough")
        touch(home, "bin/sharp.exe")
        touch(home, "lib/std/list.sharp")
        touch(home, "compiler/sharpc-v0.5.7")
        val toolchain = SharpToolchains.at(home.path)!!
        val folder = File(base, "shop").path
        fun messageOf(block: () -> Unit): String = try { block(); "" } catch (e: com.intellij.execution.ExecutionException) { e.message ?: "" }

        val tests = SharpRunState.plan(toolchain, folder, base.path, emptyList(), "", emptyList(), listOf("-ide-tests"), "", false, false, true, tests = true)
        assertTrue(tests.scriptText, tests.scriptText.contains(" -tests > "))
        assertTrue(tests.scriptText, tests.scriptText.contains("\\build\\outputs\\shop_tests\" -ide-tests"))
        assertTrue(tests.scriptText, tests.scriptText.contains("echo Compiling shop tests ..."))
        assertTrue("a build of its own", ProgramFiles.normalise(tests.runFolder.toString()).endsWith("/shop/build/.sharp-run/tests"))

        // the program of the same project is as before
        val program = SharpRunState.plan(toolchain, folder, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true)
        assertTrue(!program.scriptText.contains("-tests") && program.scriptText.contains("\\build\\outputs\\shop\""))
        assertTrue(ProgramFiles.normalise(program.runFolder.toString()).endsWith("/shop/build/.sharp-run"))

        // what cannot work says why: an older Sharp, no tests folder, a folder that is no project
        assertTrue(messageOf { SharpRunState.plan(fakeToolchain(), folder, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true, tests = true) }.contains("need Sharp 0.5.7"))
        File(base, "shop/tests").deleteRecursively()
        assertTrue(messageOf { SharpRunState.plan(toolchain, folder, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true, tests = true) }.contains("has no tests"))
        touch(base, "plain/main.sharp", "mod main;\n")
        assertTrue(messageOf { SharpRunState.plan(toolchain, File(base, "plain").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true, tests = true) }.contains("not a Sharp project"))
    }

    @Test
    fun aPlanThatCannotWorkSaysWhy() {
        val base = temp.newFolder("project")
        val toolchain = fakeToolchain()
        fun messageOf(block: () -> Unit): String = try { block(); "" } catch (e: com.intellij.execution.ExecutionException) { e.message ?: "" }
        assertTrue(messageOf { SharpRunState.plan(toolchain, "", base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true) }.contains("No program folder"))
        assertTrue(messageOf { SharpRunState.plan(toolchain, File(base, "nowhere").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true) }.contains("does not exist"))
        File(base, "empty").mkdirs()
        assertTrue(messageOf { SharpRunState.plan(toolchain, File(base, "empty").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true) }.contains("no .sharp file"))
        touch(base, "empty/main.sharp")
        assertTrue(messageOf { SharpRunState.plan(toolchain, File(base, "empty").path, base.path, emptyList(), "", emptyList(), emptyList(), "", false, true, true) }.contains("no unoptimized compiler"))
    }

    /** Runs a plan the way SharpRunState starts it, and gives what the console would show. */
    private fun runPlan(plan: RunPlan): Pair<Int, String> {
        SharpRunState.write(plan)
        val process = ProcessBuilder(plan.command).directory(File(plan.workingDirectory)).redirectErrorStream(true).start()
        val output = process.inputStream.bufferedReader(StandardCharsets.UTF_8).readText()
        assertTrue("the run did not end in three minutes", process.waitFor(180, TimeUnit.SECONDS))
        return process.exitValue() to output.replace("\r\n", "\n")
    }

    @Test
    fun aRealProgramIsBuiltAndRunWithTheInstalledSharp() {
        // (the tests run with SHARP_HOME pointing at an empty place - build.gradle.kts; this one test wants the real one)
        val toolchain = SharpToolchains.at(SharpToolchains.defaultHome({ if (it == "SHARP_HOME") null else System.getenv(it) }))
        assumeTrue("Sharp is not installed on this machine: nothing to run", toolchain != null)
        val windows = System.getProperty("os.name", "").lowercase().contains("win")
        val base = temp.newFolder("my program")
        touch(base, "greeter/main.sharp", """
            mod main;

            def main(args: string[]) {
                println(greeting() + " with " + sizeof(args) + " argument(s): " + args[0] + ", " + args[1]);
            }
        """.trimIndent())
        touch(base, "greeter/words/greeting.sharp", """
            mod main;

            def greeting() : string {
                words := new list<string>();
                words.add("hello");
                words.add("from the IDE");
                return words.at(0) + " " + words.at(1);
            }
        """.trimIndent())
        touch(base, "greeter/tests/broken.sharp", "this is not Sharp")          // an excluded folder: never compiled
        val folder = File(base, "greeter").path
        val plan = SharpRunState.plan(toolchain!!, folder, base.path, listOf("tests", "build"), "", listOf("-O"), listOf("one", "two words"), "", false, false, windows)
        val (exit, output) = runPlan(plan)
        assertEquals("Compiling greeter ...\nhello from the IDE with 2 argument(s): one, two words\n", output)
        assertEquals(0, exit)
        assertTrue(File(base, "greeter/build/greeter").isFile)

        // the unoptimized compiler builds the same program
        val (exitU, outputU) = runPlan(SharpRunState.plan(toolchain, folder, base.path, listOf("tests", "build"), "", emptyList(), listOf("a", "b"), "", false, true, windows))
        assertEquals("Compiling greeter ...\nhello from the IDE with 2 argument(s): a, b\n", outputU)
        assertEquals(0, exitU)

        // a mistake: the compiler's message with its place, a failing exit code, and no program run
        touch(base, "greeter/words/greeting.sharp", "mod main;\n\ndef greeting() : string {\n    return helo();\n}\n")
        val (exitBad, outputBad) = runPlan(SharpRunState.plan(toolchain, folder, base.path, listOf("tests", "build"), "", emptyList(), listOf("a", "b"), "", false, false, windows))
        assertEquals(1, exitBad)
        assertTrue(outputBad, outputBad.startsWith("Compiling greeter ...\n"))
        assertTrue(outputBad, outputBad.contains("error["))
        val place = outputBad.lines().mapNotNull { SharpConsoleFilter.placeIn(it) }.firstOrNull()
        assertNotNull("the error names its place: $outputBad", place)
        assertTrue(place!!.file.replace('\\', '/').endsWith("/greeter/words/greeting.sharp"))
        assertEquals(4, place.line)
        assertTrue(!outputBad.contains("argument(s)"))
    }

    /** The check after a save, the direct way: the installed compiler is started with -files and -ide, and its answer is read. */
    @Test
    fun theInstalledCompilerChecksAProgramDirectlyAndItsAnswerIsRead() {
        val found = SharpToolchains.at(SharpToolchains.defaultHome({ if (it == "SHARP_HOME") null else System.getenv(it) }))
        assumeTrue("Sharp is not installed on this machine: nothing to run", found != null)
        val toolchain = found!!
        val windowsHere = System.getProperty("os.name", "").lowercase().contains("win")
        val base = temp.newFolder("checked program")
        touch(base, "main.sharp", listOf("mod main;", "", "def main(args: string[]) {", "    printn(1);", "}", "").joinToString("\n"))
        val settings = ProgramSettings(base.path, emptyList(), emptyList(), "", toolchain.library)
        val files = ProgramFiles.list(settings)
        val folder = temp.newFolder("run folder").toPath()
        // a launcher that starts the process plainly (the IDE's own needs a running IDE)
        val launcher = object : sharp.ide.run.ProcessLauncher {
            override fun launch(command: List<String>, timeoutMillis: Long, indicator: com.intellij.openapi.progress.ProgressIndicator): sharp.ide.run.LaunchResult {
                val process = ProcessBuilder(command).redirectErrorStream(true).start()
                val output = process.inputStream.bufferedReader(StandardCharsets.UTF_8).readText()
                process.waitFor(180, TimeUnit.SECONDS)
                return sharp.ide.run.LaunchResult.Finished(process.exitValue(), output)
            }
        }
        val request = sharp.ide.run.RunRequest(
            folder = folder, files = files, openFiles = files, launcher = "", script = "",
            vm = toolchain.vm, compiler = toolchain.compiler, extraArguments = emptyList(), timeoutSeconds = 120,
            translator = if (windowsHere) sharp.ide.paths.PathTranslator.forNativeWindows() else null, direct = true,
        )
        // (the runner only hands the indicator on to the launcher, and this launcher never asks it: an empty stand-in
        //  will do - the platform's own indicators need a running IDE)
        val indicator = java.lang.reflect.Proxy.newProxyInstance(
            javaClass.classLoader, arrayOf(com.intellij.openapi.progress.ProgressIndicator::class.java),
        ) { _, _, _ -> null } as com.intellij.openapi.progress.ProgressIndicator
        val outcome = sharp.ide.run.CompilerRunner(launcher).run(request, indicator)
        assertTrue(outcome is sharp.ide.run.RunOutcome.Finished)
        val run = sharp.ide.index.DiagnosticsReader.readFolder(folder)
        val mine = run.diagnostics.filter { it.place.file.replace('\\', '/').endsWith("/checked program/main.sharp") }
        assertTrue("the mistake in main.sharp is reported: ${run.diagnostics.size} diagnostic(s)", mine.isNotEmpty())
        // the path the compiler names is the path the plugin gave it: the editor finds its file by it
        val asTheEditorNamesIt = if (windowsHere) sharp.ide.paths.PathTranslator.forNativeWindows().toEditorPath(mine[0].place.file) else mine[0].place.file
        assertEquals(ProgramFiles.normalise(File(base, "main.sharp").path), asTheEditorNamesIt)
        assertTrue("nothing but the editor's files is written: no executable", !File(folder.toFile(), "out").exists())
    }

    // ---------------------------------------------------------------- up to date; projects

    @Test
    fun theScriptOfAnUpToDateProgramStartsNoCompiler() {
        val script = RunScript.windows(values.copy(upToDate = true, newStamp = "C:/w/stamp.new.txt", keptStamp = "C:/w/stamp.txt"))
        assertTrue(script.contains("echo shapes is up to date"))
        assertTrue(!script.contains("sharpc-v0.4.0"))                      // the compiler is not named at all
        assertTrue(!script.contains("del /q"))                             // and the program is not deleted
        assertTrue(script.contains("\"C:\\Users\\A B\\Sharp\\bin\\sharp.exe\" \"C:\\work\\my shapes\\build\\shapes\" one"))
        val building = RunScript.windows(values.copy(newStamp = "C:/w/stamp.new.txt", keptStamp = "C:/w/stamp.txt"))
        assertTrue(building.contains("copy /y \"C:\\w\\stamp.new.txt\" \"C:\\w\\stamp.txt\" >nul"))
        val unix = RunScript.unix(values.copy(upToDate = true))
        assertTrue(unix.contains("echo 'shapes is up to date'"))
        assertTrue(!unix.contains("rm -f"))
    }

    @Test
    fun aSettingsFileMakesAFolderASharpProject() {
        val base = temp.newFolder("projects")
        // a JSON file of another tool is not Sharp's
        touch(base, "web/settings.json", "{ \"name\": \"site\", \"port\": 8080 }")
        assertNull(sharp.ide.project.SharpProjectFiles.read(File(base, "web").path))
        // not JSON at all
        touch(base, "broken/settings.json", "{ this is not json")
        assertNull(sharp.ide.project.SharpProjectFiles.read(File(base, "broken").path))
        // no file
        File(base, "plain").mkdirs()
        assertNull(sharp.ide.project.SharpProjectFiles.read(File(base, "plain").path))
        // an empty one counts where there is Sharp code
        touch(base, "empty/settings.json", "{ }")
        assertNull(sharp.ide.project.SharpProjectFiles.read(File(base, "empty").path))
        touch(base, "empty2/settings.json", "{ }")
        touch(base, "empty2/src/main.sharp", "mod main;\n")
        assertEquals("out", sharp.ide.project.SharpProjectFiles.read(File(base, "empty2").path)!!.output)
        // the real thing
        touch(base, "demo/settings.json", """
            { "name": "demo", "output": "demo_app", "extern_lib": [ "libs/shapes", "D:/else/where" ],
              "ignore_folders": [ "build", "src/generated", "D:/abs" ], "optimize_level": 1 }
        """.trimIndent())
        File(base, "demo/libs/shapes").mkdirs()
        val demo = sharp.ide.project.SharpProjectFiles.read(File(base, "demo").path)!!
        val root = ProgramFiles.normalise(File(base, "demo").path)
        assertEquals("demo", demo.name)
        assertEquals("$root/build/outputs/demo_app", demo.outputPath)
        assertEquals(listOf("$root/libs/shapes", "D:/else/where"), demo.externLibraryFolders())
        assertEquals(listOf("build", "src/generated"), demo.ignoredInsideProject())
        // the file changed: it is read again
        Thread.sleep(20)
        touch(base, "demo/settings.json", "{ \"output\": \"other\" }")
        assertEquals("other", sharp.ide.project.SharpProjectFiles.read(File(base, "demo").path)!!.output)
    }

    @Test
    fun aProjectIsPlannedAsTheCompilersProjectMode() {
        val base = temp.newFolder("project mode")
        touch(base, "demo/settings.json", "{ \"name\": \"demo\", \"output\": \"demo_app\", \"ignore_folders\": [ \"build\", \"old\" ] }")
        touch(base, "demo/src/main.sharp")
        touch(base, "demo/old/unused.sharp")
        touch(base, "demo/tests/t.sharp")
        val toolchain = fakeToolchain()
        val folder = File(base, "demo").path
        val plan = SharpRunState.plan(toolchain, folder, base.path, listOf("tests", "build"), "", listOf("-O"), emptyList(), "", false, false, true)
        val root = ProgramFiles.normalise(folder)
        assertTrue(plan.projectMode)
        val line = plan.scriptText.lines().first { it.contains("sharpc-v0.4.0") }
        assertTrue(line, line.contains(" -P \"" + root.replace('/', '\\') + "\" -L \"" + toolchain.library.replace('/', '\\') + "\" -O > "))
        assertTrue(!line.contains("-files"))
        assertTrue(plan.scriptText.contains("\\build\\outputs\\demo_app\""))
        // settings.json decides what is left out: `old` is, `tests` (the plugin's own exclusion) is not
        assertTrue(plan.files.contains("$root/tests/t.sharp"))
        assertTrue(!plan.files.contains("$root/old/unused.sharp"))
        assertTrue(plan.stampText.contains("$root/settings.json "))

        // a project that names its own library gets no -L
        touch(base, "demo/settings.json", "{ \"output\": \"demo_app\", \"extern_lib\": [ \"mylib\" ] }")
        touch(base, "demo/mylib/std/x.sharp")
        val own = SharpRunState.plan(toolchain, folder, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, true)
        assertTrue(!own.scriptText.lines().first { it.contains("sharpc-v0.4.0") }.contains(" -L "))
    }

    private fun realToolchain(): SharpToolchain? =
        SharpToolchains.at(SharpToolchains.defaultHome({ if (it == "SHARP_HOME") null else System.getenv(it) }))

    @Test
    fun aProgramThatDidNotChangeIsNotCompiledAgain() {
        val toolchain = realToolchain()
        assumeTrue("Sharp is not installed on this machine: nothing to run", toolchain != null)
        val windows = System.getProperty("os.name", "").lowercase().contains("win")
        val base = temp.newFolder("up to date")
        touch(base, "app/main.sharp", "mod main;\n\ndef main(args: string[]) {\n    println(\"one\");\n}\n")
        val folder = File(base, "app").path
        fun plan(options: List<String> = emptyList(), always: Boolean = false) =
            SharpRunState.plan(toolchain!!, folder, base.path, listOf("build"), "", options, emptyList(), "", false, false, windows, always)

        val first = plan()
        assertTrue(!first.upToDate)
        assertEquals(0 to "Compiling app ...\none\n", runPlan(first))

        val second = plan()                                                 // nothing changed
        assertTrue(second.upToDate)
        assertEquals(0 to "app is up to date\none\n", runPlan(second))

        assertTrue("asked to rebuild every time", !plan(always = true).upToDate)
        assertTrue("another option", !plan(listOf("-O")).upToDate)

        Thread.sleep(1100)                                                  // (file times have one-second steps on some disks)
        touch(base, "app/main.sharp", "mod main;\n\ndef main(args: string[]) {\n    println(\"two\");\n}\n")
        val third = plan()                                                  // a file changed
        assertTrue(!third.upToDate)
        assertEquals(0 to "Compiling app ...\ntwo\n", runPlan(third))
        assertTrue(plan().upToDate)

        touch(base, "app/more.sharp", "mod main;\n\ndef extra() : _int64 { return 1; }\n")
        assertTrue("a file was added", !plan().upToDate)
        runPlan(plan())
        assertTrue(plan().upToDate)

        File(base, "app/build/app").delete()                                // the program is gone
        assertTrue(!plan().upToDate)

        // a build that fails keeps no stamp: the next run compiles again
        touch(base, "app/main.sharp", "mod main;\n\ndef main(args: string[]) {\n    nothing_here();\n}\n")
        assertEquals(1, runPlan(plan()).first)
        assertTrue(!plan().upToDate)
    }

    @Test
    fun aRealProjectIsBuiltByTheCompilersProjectMode() {
        val toolchain = realToolchain()
        assumeTrue("Sharp is not installed on this machine: nothing to run", toolchain != null)
        val windows = System.getProperty("os.name", "").lowercase().contains("win")
        val base = temp.newFolder("real project")
        touch(base, "demo/settings.json", "{ \"name\": \"demo\", \"output\": \"demo_app\", \"ignore_folders\": [ \"build\", \"old\" ] }")
        touch(base, "demo/src/main.sharp", "mod main;\n\ndef main(args: string[]) {\n    println(\"project \" + answer());\n}\n")
        touch(base, "demo/src/answer.sharp", "mod main;\n\ndef answer() : _int64 { return 42; }\n")
        touch(base, "demo/old/broken.sharp", "this is not Sharp")           // ignored by settings.json
        File(base, "demo/build").mkdirs()
        File(base, "demo/old").mkdirs()
        val folder = File(base, "demo").path
        fun plan() = SharpRunState.plan(toolchain!!, folder, base.path, emptyList(), "", emptyList(), emptyList(), "", false, false, windows)

        val (exit, output) = runPlan(plan())
        assertEquals("Compiling demo ...\nproject 42\n", output)
        assertEquals(0, exit)
        assertTrue("the compiler wrote the program where settings.json says", File(base, "demo/build/outputs/demo_app").isFile)
        assertEquals(0 to "demo is up to date\nproject 42\n", runPlan(plan()))

        Thread.sleep(1100)
        touch(base, "demo/settings.json", "{ \"name\": \"demo\", \"output\": \"renamed\", \"ignore_folders\": [ \"build\", \"old\" ] }")
        val changed = plan()                                                // settings.json changed: built again, under the new name
        assertTrue(!changed.upToDate)
        assertEquals(0 to "Compiling demo ...\nproject 42\n", runPlan(changed))
        assertTrue(File(base, "demo/build/outputs/renamed").isFile)
    }
}
