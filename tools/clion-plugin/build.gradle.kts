// Build script of the Sharp plugin for CLion / IntelliJ-based IDEs.
//
// The IntelliJ Platform Gradle Plugin (2.x) downloads the target IDE (CLion
// 2025.2), compiles against it, runs the tests inside it and zips the plugin.
// Only the platform module is used, never CLion's C++ modules, so the zip also
// loads in IntelliJ IDEA (see doc/ide.md section 7.1).
import org.jetbrains.intellij.platform.gradle.IntelliJPlatformType
import org.jetbrains.intellij.platform.gradle.TestFrameworkType

plugins {
    kotlin("jvm") version "2.1.20"
    id("org.jetbrains.intellij.platform") version "2.20.1"
}

group = "sharp"
version = providers.gradleProperty("pluginVersion").get()

repositories {
    mavenCentral()
    intellijPlatform { defaultRepositories() }
}

dependencies {
    intellijPlatform {
        clion("2025.2")
        testFramework(TestFrameworkType.Platform)
    }
    testImplementation("junit:junit:4.13.2")
    testImplementation("org.opentest4j:opentest4j:1.3.0")
}

kotlin { jvmToolchain(21) }

intellijPlatform {
    // `buildSearchableOptions` starts the IDE headlessly to index the texts of settings pages (so the Settings
    // search finds them). CLion 2025.2 crashes while doing that, in its own colour page (GeneralColorsPage,
    // "Class initialization must not depend on services"), before any code of this plugin runs. Nothing but
    // the Settings search box depends on it: the Sharp page is still in Settings > Languages & Frameworks.
    buildSearchableOptions = false

    pluginConfiguration {
        id = "sharp.language.support"
        name = "Sharp"
        version = providers.gradleProperty("pluginVersion")
        ideaVersion {
            sinceBuild = "252"
            untilBuild = provider { null }
        }
    }
    // `verifyPlugin` (the Plugin Verifier) checks the plugin against the IDE it
    // targets: it finds calls to platform API that does not exist or was removed.
    pluginVerification {
        ides {
            create(IntelliJPlatformType.CLion, "2025.2")
            // IntelliJ IDEA 2025.3.1 as installed on this machine (no download): the plugin depends on the
            // platform module only, so it must load there as well. Skipped when the folder is not there.
            val installedIdea = file("C:/Program Files/JetBrains/IntelliJ IDEA 2025.3.1")
            if (installedIdea.isDirectory) local(installedIdea)
        }
    }
}

tasks.test {
    // The tests read the standard library and the suite programs from the repository.
    systemProperty("sharp.repo", rootProject.projectDir.resolve("../..").canonicalPath)
    // The older tests are about the SCRIPT way of starting the compiler (WSL). On a machine where Sharp is
    // installed the plugin would start that one directly instead, and those tests would test something else
    // there than on a machine without it. SHARP_HOME names a folder that holds no Sharp, so "the installed
    // Sharp" is nothing in every test - except BuildAndRunTest, which looks in the real place on purpose.
    environment("SHARP_HOME", layout.buildDirectory.dir("no-sharp-installed-here").get().asFile.path)
    // `gradlew test -PrequireRealCompiler`: the end-to-end test (EndToEndWithRealCompilerTest) then FAILS on a
    // machine where the real compiler cannot be run, instead of skipping silently.
    systemProperty("sharp.requireRealCompiler", providers.gradleProperty("requireRealCompiler").isPresent.toString())
    // The tests depend on things Gradle cannot see (the compiler in WSL, the repository's library), so a result
    // of an earlier run is never reused: `gradlew test` always runs them.
    outputs.upToDateWhen { false }
    outputs.cacheIf { false }
}

// KEEP THE DOWNLOADED IDE UNTOUCHED.
// The tests and `runIde` run inside the CLion that Gradle downloaded and unpacked into its own cache
// (~/.gradle/caches/<version>/transforms/...). Gradle treats that folder as read-only and checks it when a
// new Gradle daemon first uses it: one extra file in it and EVERY task fails with "The contents of the
// immutable workspace ... have been modified". Two things used to write there:
//   - on Windows the platform unpacks two small helper programs for stopping processes (winp...dll,
//     sendctrlc...exe) next to its own jars the first time a process is started (our tests start some);
//   - a Java runtime that crashes or runs out of memory writes hs_err_pid<n>.log / replay_pid<n>.log into
//     the folder it was started in.
// Both are sent to this project's build folder instead. (If the message ever appears all the same:
// doc/ide.md 13.7 says what to delete.)
val helperFolder = layout.buildDirectory.dir("ide-helpers")
fun JavaForkOptions.keepTheIdeFolderClean() {
    val folder = helperFolder.get().asFile
    systemProperty("winp.folder.preferred", folder.absolutePath)
    jvmArgs(
        "-XX:ErrorFile=" + folder.resolve("hs_err_pid%p.log").absolutePath,
        "-XX:ReplayDataFile=" + folder.resolve("replay_pid%p.log").absolutePath,
    )
}
tasks.withType<Test>().configureEach {                 // `test`
    doFirst { helperFolder.get().asFile.mkdirs() }
    keepTheIdeFolderClean()
}
tasks.withType<JavaExec>().configureEach {             // `runIde` (and any other task that starts the IDE)
    doFirst { helperFolder.get().asFile.mkdirs() }
    keepTheIdeFolderClean()
}
