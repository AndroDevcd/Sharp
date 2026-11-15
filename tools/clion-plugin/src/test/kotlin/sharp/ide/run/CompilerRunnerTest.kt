package sharp.ide.run

import com.intellij.openapi.progress.EmptyProgressIndicator
import com.intellij.openapi.progress.ProgressIndicator
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import sharp.ide.paths.PathTranslator
import sharp.ide.settings.SharpDefaults
import java.nio.file.Files
import java.nio.file.Path

/**
 * THE RUNNER WITH A FAKE PROCESS
 *
 * What lands in the run folder (the file list, the script) and how every way a process can end
 * is told back. No compiler is started.
 */
class CompilerRunnerTest {
    @get:Rule val temp = TemporaryFolder()

    /** Records the command it was asked to run and answers with a prepared result. */
    private class FakeLauncher(val answer: LaunchResult) : ProcessLauncher {
        var command: List<String> = emptyList()
        var timeout = 0L
        override fun launch(command: List<String>, timeoutMillis: Long, indicator: ProgressIndicator): LaunchResult {
            this.command = command
            this.timeout = timeoutMillis
            return answer
        }
    }

    private fun request(folder: Path, translator: PathTranslator?, launcher: String = SharpDefaults.LAUNCHER_WSL) = RunRequest(
        folder = folder,
        files = listOf("C:/Users/Braxton Nunnally/lib/std/a.sharp", "C:/Users/Braxton Nunnally/proj/main.sharp"),
        openFiles = listOf("C:/Users/Braxton Nunnally/proj/main.sharp"),
        launcher = launcher,
        script = SharpDefaults.SCRIPT,
        vm = "\$HOME/vm",
        compiler = "\$HOME/sharpc",
        extraArguments = listOf("-w"),
        timeoutSeconds = 30,
        translator = translator,
    )

    private fun lines(file: Path) = String(Files.readAllBytes(file), Charsets.UTF_8).lines().filter { it.isNotEmpty() }

    @Test fun theFolderGetsTheTranslatedFileListsAndTheScript() {
        val folder = temp.newFolder("run-1").toPath()
        val launcher = FakeLauncher(LaunchResult.Finished(0, "all fine"))
        val outcome = CompilerRunner(launcher).run(request(folder, PathTranslator("/mnt", "")), EmptyProgressIndicator())
        assertTrue(outcome is RunOutcome.Finished)
        assertEquals(listOf("/mnt/c/Users/Braxton Nunnally/lib/std/a.sharp", "/mnt/c/Users/Braxton Nunnally/proj/main.sharp"), lines(folder.resolve("files.txt")))
        assertEquals(listOf("/mnt/c/Users/Braxton Nunnally/proj/main.sharp"), lines(folder.resolve("bodies_for.txt")))
        val script = String(Files.readAllBytes(folder.resolve("run.sh")), Charsets.UTF_8)
        assertTrue(script, script.contains("exec timeout 30 \$HOME/vm \$HOME/sharpc \"\${FILES[@]}\" -ide '"))
        assertTrue(script, script.contains("-ide '/") && script.endsWith("'-w'\n"))
        assertTrue("no carriage returns", !script.contains('\r'))
    }

    @Test fun theLauncherIsStartedOnTheTranslatedScriptPath() {
        val folder = temp.newFolder("run 2").toPath()
        val launcher = FakeLauncher(LaunchResult.Finished(0, ""))
        CompilerRunner(launcher).run(request(folder, PathTranslator("/mnt", "")), EmptyProgressIndicator())
        assertEquals(listOf("wsl.exe", "-e", "bash"), launcher.command.take(3))
        val script = launcher.command[3]
        assertTrue(script, script.startsWith("/") && script.endsWith("/run 2/run.sh") || script.endsWith("/run 2/run.sh"))
        assertEquals((30 + 10) * 1000L, launcher.timeout)                       // the script's own limit plus ten seconds
    }

    @Test fun withoutATranslatorPathsGoThroughAsTheyAre() {
        val folder = temp.newFolder("run-3").toPath()
        val launcher = FakeLauncher(LaunchResult.Finished(0, ""))
        CompilerRunner(launcher).run(request(folder, null, SharpDefaults.LAUNCHER_NATIVE), EmptyProgressIndicator())
        assertEquals(listOf("C:/Users/Braxton Nunnally/lib/std/a.sharp", "C:/Users/Braxton Nunnally/proj/main.sharp"), lines(folder.resolve("files.txt")))
        assertEquals("bash", launcher.command[0])
    }

    private fun failure(answer: LaunchResult): RunFailure {
        val folder = temp.newFolder().toPath()
        val outcome = CompilerRunner(FakeLauncher(answer)).run(request(folder, null), EmptyProgressIndicator())
        return (outcome as RunOutcome.Failed).failure
    }

    @Test fun aProcessThatDoesNotStartIsAFailureWithTheCommand() {
        val f = failure(LaunchResult.CouldNotStart("Cannot run program \"wsl.exe\""))
        assertTrue(f.reason, f.reason.startsWith("the compiler could not be started"))
        assertTrue(f.command.startsWith("wsl.exe -e bash "))
    }

    @Test fun aTimeoutIsAFailureWithTheOutputTail() {
        val output = (1..30).joinToString("\n") { "line $it" }
        val f = failure(LaunchResult.TimedOut(output))
        assertTrue(f.reason, f.reason.contains("within 30 seconds"))
        assertEquals(20, f.outputTail.lines().size)
        assertTrue(f.outputTail.endsWith("line 30"))
    }

    @Test fun exitCode124MeansTheScriptsTimeoutEndedTheCompiler() {
        val f = failure(LaunchResult.Finished(124, "partial output"))
        assertTrue(f.reason, f.reason.contains("within 30 seconds"))
        assertEquals("partial output", f.outputTail)
    }

    @Test fun anyOtherExitCodeIsFinishedBecauseTheFolderDecidesIfTheRunWorked() {
        val folder = temp.newFolder().toPath()
        val outcome = CompilerRunner(FakeLauncher(LaunchResult.Finished(1, "boom"))).run(request(folder, null), EmptyProgressIndicator())
        assertEquals(1, (outcome as RunOutcome.Finished).exitCode)
        assertEquals("boom", outcome.outputTail)
    }

    @Test fun cancellationIsNotAFailure() {
        val folder = temp.newFolder().toPath()
        assertTrue(CompilerRunner(FakeLauncher(LaunchResult.Cancelled)).run(request(folder, null), EmptyProgressIndicator()) is RunOutcome.Cancelled)
    }

    @Test fun anUnwritableFolderIsAFailure() {
        val file = temp.newFile("not-a-folder").toPath()                          // a file where the folder should be
        val outcome = CompilerRunner(FakeLauncher(LaunchResult.Finished(0, ""))).run(request(file.resolve("run-1"), null), EmptyProgressIndicator())
        assertTrue((outcome as RunOutcome.Failed).failure.reason.startsWith("could not prepare the run folder"))
    }

    @Test fun theFailureTrackerReportsEachReasonOnceUntilARunSucceeds() {
        val tracker = FailureTracker()
        assertTrue(tracker.shouldReport("wsl.exe missing"))
        assertTrue(!tracker.shouldReport("wsl.exe missing"))
        assertTrue(!tracker.shouldReport("wsl.exe missing"))
        assertTrue(tracker.shouldReport("timed out"))
        tracker.clear()
        assertTrue(tracker.shouldReport("timed out"))
    }

    @Test fun runFoldersCountUpAndPruneKeepsTheNamedOnes() {
        val base = temp.newFolder("base").toPath()
        val a = RunFolders.create(base); val b = RunFolders.create(base); val c = RunFolders.create(base)
        assertEquals(listOf("run-1", "run-2", "run-3"), listOf(a, b, c).map { it.fileName.toString() })
        Files.write(a.resolve("x.txt"), byteArrayOf(1))
        RunFolders.prune(base, setOf(c))
        assertEquals(listOf("run-3"), RunFolders.numberedFolders(base).map { it.second.fileName.toString() })
        assertEquals("run-4", RunFolders.create(base).fileName.toString())
    }
}
