package sharp.ide.run

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import sharp.ide.settings.SharpDefaults

/** The command template: quoting, placeholders, the launcher. A space, a quote and a dollar sign in a path must be harmless. */
class CommandTemplateTest {
    private val values = ScriptValues(
        vm = "\$HOME/sharp/vm",
        compiler = "\$HOME/out/sharpc",
        filesList = "/mnt/c/Users/Braxton Nunnally/run-1/files.txt",
        ideDirectory = "/mnt/c/it's \$here/run-1",
        extraArguments = listOf("-w", "a b"),
        timeoutSeconds = 90,
    )

    @Test fun quotingKeepsEveryCharacterLiteral() {
        assertEquals("'plain'", CommandTemplate.quote("plain"))
        assertEquals("'a b'", CommandTemplate.quote("a b"))
        assertEquals("'it'\\''s'", CommandTemplate.quote("it's"))
        assertEquals("'\$HOME'", CommandTemplate.quote("\$HOME"))
        assertEquals("''", CommandTemplate.quote(""))
    }

    @Test fun theDefaultScriptIsFilledIn() {
        val script = CommandTemplate.fillScript(SharpDefaults.SCRIPT, values)
        assertEquals(
            "mapfile -t FILES < '/mnt/c/Users/Braxton Nunnally/run-1/files.txt'\n" +
                "exec timeout 90 \$HOME/sharp/vm \$HOME/out/sharpc \"\${FILES[@]}\" -ide '/mnt/c/it'\\''s \$here/run-1' '-w' 'a b'\n",
            script,
        )
    }

    @Test fun onlyTheKnownPlaceholdersAreReplacedAndShellBracesSurvive() {
        val script = CommandTemplate.fillScript("echo {unknown} \${A[@]} {vm}\r\n", values)
        assertEquals("echo {unknown} \${A[@]} \$HOME/sharp/vm\n", script)
    }

    @Test fun aReplacementIsNotReplacedAgain() {
        val tricky = values.copy(ideDirectory = "/x/{vm}")
        assertTrue(CommandTemplate.fillScript("{ide_dir}", tricky).contains("'/x/{vm}'"))
    }

    @Test fun launcherLinesBecomeArguments() {
        assertEquals(
            listOf("wsl.exe", "-e", "bash", "/mnt/c/a b/run.sh"),
            CommandTemplate.launcherArguments("wsl.exe\n-e\n\n  bash \r\n{script}\n", "/mnt/c/a b/run.sh"),
        )
    }

    @Test fun extraArgumentsAreSplitOnSpaces() {
        assertEquals(listOf("-w", "-wlib"), CommandTemplate.splitExtraArguments("  -w   -wlib "))
        assertEquals(emptyList<String>(), CommandTemplate.splitExtraArguments("   "))
    }

    @Test fun wslIsRecognisedByTheLaunchersFirstArgument() {
        assertTrue(CommandTemplate.startsWsl("wsl.exe\n-e\nbash\n{script}"))
        assertTrue(CommandTemplate.startsWsl("C:\\Windows\\System32\\wsl.exe\n-e"))
        assertTrue(CommandTemplate.startsWsl("wsl\n-d\nUbuntu"))
        assertFalse(CommandTemplate.startsWsl("bash\n{script}"))
        assertFalse(CommandTemplate.startsWsl(""))
    }
}
