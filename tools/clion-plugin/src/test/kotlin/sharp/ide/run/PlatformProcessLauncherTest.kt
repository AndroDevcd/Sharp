package sharp.ide.run

import com.intellij.openapi.progress.EmptyProgressIndicator
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Test
import java.io.File

/**
 * THE REAL PROCESS LAUNCHER ON REAL PROCESSES
 *
 * The launcher is what runs wsl.exe on the owner's machine, so it is exercised on real
 * programs: the JVM that runs this test (always there), and, on Windows with WSL installed, the
 * default command shape with a script path that contains a space.
 */
class PlatformProcessLauncherTest {
    private val launcher = PlatformProcessLauncher()
    private val javaExe: String = File(System.getProperty("java.home"), "bin/java" + if (System.getProperty("os.name").startsWith("Windows")) ".exe" else "").path

    @Test fun aFinishedProcessGivesItsExitCodeAndOutput() {
        val result = launcher.launch(listOf(javaExe, "-version"), 30_000, EmptyProgressIndicator())
        assertTrue(result is LaunchResult.Finished)
        result as LaunchResult.Finished
        assertEquals(0, result.exitCode)
        assertTrue(result.output, result.output.contains("version"))               // java -version prints on stderr
    }

    @Test fun aProgramThatIsNotThereCouldNotStart() {
        val result = launcher.launch(listOf("definitely-not-a-program-xyz"), 5_000, EmptyProgressIndicator())
        assertTrue(result.toString(), result is LaunchResult.CouldNotStart)
    }

    @Test fun aProcessThatRunsTooLongIsKilledAndReportedAsTimedOut() {
        val folder = java.nio.file.Files.createTempDirectory("sleeper").toFile()
        val sleeper = File(folder, "Sleeper.java")
        try {
            sleeper.writeText("public class Sleeper { public static void main(String[] a) throws Exception { System.out.println(\"started\"); Thread.sleep(60000); } }")
            val started = System.nanoTime()
            val result = launcher.launch(listOf(javaExe, sleeper.path), 1_500, EmptyProgressIndicator())
            assertTrue(result.toString(), result is LaunchResult.TimedOut)
            assertTrue("it must not wait the full minute", (System.nanoTime() - started) / 1_000_000 < 30_000)
        } finally {
            folder.deleteRecursively()
        }
    }

    /** The default command on this machine: wsl.exe -e bash "<path with a space>/run.sh", started from a Windows process. */
    @Test fun theDefaultWslCommandRunsAScriptWhosePathHasASpace() {
        assumeTrue(System.getProperty("os.name").startsWith("Windows"))
        val wsl = File(System.getenv("SystemRoot") ?: "C:\\Windows", "System32/wsl.exe")
        assumeTrue("wsl.exe is not installed", wsl.exists())
        val folder = File(System.getProperty("java.io.tmpdir"), "sharp launcher test " + System.nanoTime())
        folder.mkdirs()
        try {
            File(folder, "run.sh").writeBytes("echo hello from the script\n".toByteArray())
            val translated = sharp.ide.paths.PathTranslator().toWsl(File(folder, "run.sh").canonicalPath)
            assertTrue(translated, translated.contains(" "))
            val result = launcher.launch(listOf("wsl.exe", "-e", "bash", translated), 60_000, EmptyProgressIndicator())
            assertTrue(result.toString(), result is LaunchResult.Finished)
            result as LaunchResult.Finished
            assertEquals(result.output, 0, result.exitCode)
            assertTrue(result.output, result.output.contains("hello from the script"))
        } finally {
            folder.deleteRecursively()
        }
    }
}
