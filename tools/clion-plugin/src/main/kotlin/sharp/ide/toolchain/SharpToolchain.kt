/*
 * THE INSTALLED SHARP: where the VM, the compiler and the standard library are on this machine
 *
 * A Sharp release installs into one folder (build-scripts/internal/zip-contents in the repository: win-install.cmd,
 * unix-install.sh):
 *
 *     <home>/bin/sharp(.exe)                    the VM
 *     <home>/bin/sdb(.exe)                      the debugger (sharp.ide.debug starts it)
 *     <home>/compiler/sharpc-v0.4.0             the compiler - a Sharp program the VM runs
 *     <home>/compiler/sharpc-unoptimized-v0.4.0 the same compiler built without the optimizer
 *     <home>/lib/                               the standard library
 *
 *     <home> is  %LOCALAPPDATA%\Sharp  on Windows and  ~/.sharp  elsewhere, or what SHARP_HOME names.
 *
 * The plugin starts the VM on the compiler DIRECTLY - no shell, no WSL, no script - both to check the
 * program after a save and to build and run it (sharp.ide.execution). When several compilers are in
 * the folder (an update installed over an older one) the highest version is taken.
 *
 * Everything here is plain file-system code with no IDE classes, so it is tested without an IDE.
 */
package sharp.ide.toolchain

import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths

/** One installed Sharp. Every path uses forward slashes. */
data class SharpToolchain(
    val home: String,
    val vm: String,
    val compiler: String,
    /** The compiler built without the optimizer, when the installation has one. */
    val unoptimizedCompiler: String?,
    val library: String,
    /** The language version, read from the compiler's file name: `0.4.0`. */
    val version: String,
    /** sdb, the debugger (`<home>/bin/sdb`), when the installation has one: releases before 0.5.6 have none. */
    val debugger: String? = null,
)

object SharpToolchains {
    private const val COMPILER_PREFIX = "sharpc-v"
    private const val UNOPTIMIZED_PREFIX = "sharpc-unoptimized-v"

    /** Where a release installs when nothing says otherwise. */
    fun defaultHome(
        environment: (String) -> String? = System::getenv,
        userHome: String = System.getProperty("user.home") ?: "",
        windows: Boolean = System.getProperty("os.name", "").lowercase().contains("win"),
    ): String {
        environment("SHARP_HOME")?.trim()?.takeIf { it.isNotEmpty() }?.let { return normalise(it) }
        if (windows) {
            val local = environment("LOCALAPPDATA")?.trim()?.takeIf { it.isNotEmpty() } ?: "$userHome/AppData/Local"
            return normalise("$local/Sharp")
        }
        return normalise("$userHome/.sharp")
    }

    /**
     * The installation in `configuredHome` (the setting; empty = the default place), or null when
     * that folder does not hold a complete one.
     */
    fun find(configuredHome: String): SharpToolchain? {
        val home = configuredHome.trim().ifEmpty { defaultHome() }
        return at(home)
    }

    /** The installation in exactly this folder, or null. */
    fun at(home: String): SharpToolchain? {
        val root = normalise(home)
        if (root.isEmpty()) return null
        val vm = listOf("$root/bin/sharp.exe", "$root/bin/sharp").firstOrNull { Files.isRegularFile(Paths.get(it)) } ?: return null
        val library = "$root/lib"
        if (!Files.isDirectory(Paths.get(library))) return null
        val compilers = Paths.get("$root/compiler")
        if (!Files.isDirectory(compilers)) return null
        val names = try {
            Files.list(compilers).use { stream -> stream.map { it.fileName.toString() }.toList() }
        } catch (_: java.io.IOException) {
            return null
        }
        val newest = names.filter { it.startsWith(COMPILER_PREFIX) }
            .maxWithOrNull { a, b -> compareVersions(a.removePrefix(COMPILER_PREFIX), b.removePrefix(COMPILER_PREFIX)) }
            ?: return null
        val version = newest.removePrefix(COMPILER_PREFIX)
        val unoptimized = "$UNOPTIMIZED_PREFIX$version".takeIf { it in names }
        return SharpToolchain(
            home = root,
            vm = vm,
            compiler = "$root/compiler/$newest",
            unoptimizedCompiler = unoptimized?.let { "$root/compiler/$it" },
            library = library,
            version = version,
            debugger = listOf("$root/bin/sdb.exe", "$root/bin/sdb").firstOrNull { Files.isRegularFile(Paths.get(it)) },
        )
    }

    /** The first Sharp whose compiler knows a project's tests (`-tests`, `-testdir`) and whose std.testing runs a part of them. */
    const val FIRST_WITH_PROJECT_TESTS = "0.5.7"

    /** `0.4.0` against `0.10.1`: number by number, a missing number counts as 0, anything that is not a number as -1. */
    fun compareVersions(a: String, b: String): Int {
        val left = a.split('.')
        val right = b.split('.')
        for (i in 0 until maxOf(left.size, right.size)) {
            val x = left.getOrNull(i)?.let { it.toIntOrNull() ?: -1 } ?: 0
            val y = right.getOrNull(i)?.let { it.toIntOrNull() ?: -1 } ?: 0
            if (x != y) return x.compareTo(y)
        }
        return 0
    }

    /** What to tell someone whose machine has no installation in `configuredHome`. */
    fun notInstalledMessage(configuredHome: String): String {
        val home = configuredHome.trim().ifEmpty { defaultHome() }
        return "Sharp is not installed in $home. Install it from the release archive (windows/win-install.cmd or " +
            "linux/unix-install.sh inside sharp-<version>.zip), or name the folder it is in under " +
            "Settings > Languages & Frameworks > Sharp."
    }

    private fun normalise(path: String): String {
        val slashes = path.trim().replace('\\', '/')
        return if (slashes.length > 1) slashes.trimEnd('/') else slashes
    }

    @Suppress("unused")
    private fun path(text: String): Path = Paths.get(text)
}
