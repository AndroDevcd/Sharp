// Lets Gradle download the JDK the build asks for (`jvmToolchain(21)` in build.gradle.kts) on a machine that
// has none installed.
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}

rootProject.name = "sharp-clion-plugin"
