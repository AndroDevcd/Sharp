/*
 * THE COMPILER'S PLACES IN THE RUN CONSOLE ARE LINKS
 *
 * When a build fails, the console shows the compiler's messages. Each one names its place on a line
 * of its own:
 *
 *     error[E0401]: `printn` is not known here
 *       --> C:/work/shapes/main.sharp:12:5
 *       ::: C:/work/shapes/area.sharp:3:9          (a related place)
 *
 * This filter turns `<file>:<line>:<column>` after `-->` or `:::` into a link that opens the file
 * there. A place whose file does not exist (any more) is left as plain text.
 *
 * A failed test of std.testing names the line of the check that failed in words, and that is a link too:
 *
 *         at C:/work/shapes/tests.sharp, line 12
 *
 * `placeIn` is the part without IDE classes: the test asks it directly.
 */
package sharp.ide.execution

import com.intellij.execution.filters.Filter
import com.intellij.execution.filters.OpenFileHyperlinkInfo
import com.intellij.openapi.project.Project
import com.intellij.openapi.vfs.LocalFileSystem

/** A place found in one console line: where its text is in the line, and what it names. */
data class ConsolePlace(val start: Int, val end: Int, val file: String, val line: Int, val column: Int)

class SharpConsoleFilter(private val project: Project) : Filter {
    override fun applyFilter(line: String, entireLength: Int): Filter.Result? {
        val place = placeIn(line) ?: return null
        val file = LocalFileSystem.getInstance().findFileByPath(place.file.replace('\\', '/')) ?: return null
        val lineStart = entireLength - line.length
        val link = OpenFileHyperlinkInfo(project, file, place.line - 1, place.column - 1)
        return Filter.Result(lineStart + place.start, lineStart + place.end, link)
    }

    companion object {
        // the file may hold colons (C:/...) and spaces; the two numbers at the end of the line decide
        private val PLACE = Regex("""^\s*(?:-->|:::)\s+(.+?):(\d+):(\d+)\s*$""")

        // what g++ and clang++ print when a project's C++ does not build:
        //     C:/work/app/src/cpp/foo.cpp:4:1: error: expected ...
        private val CXX_PLACE = Regex("""^(.+?\.(?:cpp|cc|cxx|c|h|hpp|hh|hxx|inl)):(\d+):(\d+)(?=:)""", RegexOption.IGNORE_CASE)

        // where a test failed, as std.testing says it:     at C:/work/app/tests.sharp, line 12
        private val TEST_PLACE = Regex("""^\s*at (.+\.sharp), line (\d+)\s*$""")

        /** The place this line of compiler output names, or null. */
        fun placeIn(line: String): ConsolePlace? {
            val text = line.trimEnd('\n', '\r')
            TEST_PLACE.find(text)?.let { test ->
                val row = test.groupValues[2].toIntOrNull() ?: return null
                if (row < 1) return null
                return ConsolePlace(test.groups[1]!!.range.first, test.groups[2]!!.range.last + 1, test.groupValues[1], row, 1)
            }
            val match = PLACE.find(text) ?: CXX_PLACE.find(text) ?: return null
            val file = match.groups[1] ?: return null
            val row = match.groupValues[2].toIntOrNull() ?: return null
            val column = match.groupValues[3].toIntOrNull() ?: return null
            if (row < 1 || column < 1) return null
            val end = match.groups[3]!!.range.last + 1
            return ConsolePlace(file.range.first, end, file.value, row, column)
        }
    }
}
