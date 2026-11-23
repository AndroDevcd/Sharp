#!/bin/bash
# Builds the CLion / IntelliJ plugin (tools/clion-plugin): the zip a release ships in its
# clion-plugin/ folder.   A PART OF release.sh (step 9).   (Git Bash)
#
#     bash build-scripts/windows/build_plugin.sh           builds the zip
#     bash build-scripts/windows/build_plugin.sh test      runs the plugin's tests first (a minute more)
#
# The plugin has its own version (tools/clion-plugin/gradle.properties, pluginVersion): change it
# by hand when the plugin changed. The zip is
#     tools/clion-plugin/build/distributions/sharp-clion-plugin-<pluginVersion>.zip
# and is installed in the IDE by hand: Settings > Plugins > the gear > Install Plugin from Disk.
#
# Gradle needs a Java (17 or newer) to start with. JAVA_HOME is used when it is set; otherwise the
# one an installed JetBrains IDE or Android Studio brings along.
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
PLUGIN="$ROOT/tools/clion-plugin"
if [ -z "$JAVA_HOME" ] || [ ! -x "$JAVA_HOME/bin/java" ]; then
    JAVA_HOME=""
    for candidate in "/c/Program Files/JetBrains"/*/jbr "/c/Program Files/Android/Android Studio/jbr"; do
        [ -x "$candidate/bin/java" ] && JAVA_HOME="$candidate"
    done
    [ -n "$JAVA_HOME" ] || { echo "no Java found: set JAVA_HOME to a JDK 17 or newer (a JetBrains IDE brings one in its jbr folder)"; exit 1; }
    export JAVA_HOME
fi
VERSION=$(grep '^pluginVersion=' "$PLUGIN/gradle.properties" | cut -d= -f2 | tr -d '\r')
[ -n "$VERSION" ] || { echo "pluginVersion was not found in $PLUGIN/gradle.properties"; exit 1; }
TASKS="buildPlugin"; [ "$1" = test ] && TASKS="test buildPlugin"
ZIP="$PLUGIN/build/distributions/sharp-clion-plugin-$VERSION.zip"
rm -f "$ZIP"
( cd "$PLUGIN" && ./gradlew.bat $TASKS --console=plain 2>&1 ) | grep -E "^e: |FAILED|tests completed|BUILD" | tail -30
[ -f "$ZIP" ] || { echo "the plugin did not build: run  ./gradlew.bat $TASKS  in $PLUGIN  to see why"; exit 1; }
echo "built: $ZIP   (plugin $VERSION, $(stat -c %s "$ZIP") bytes)"
