/*
 * SharpSymbolContributors - "Go to Class" (Ctrl+N) and "Go to Symbol" (Ctrl+Alt+Shift+N) for Sharp.
 *
 * WHAT THIS IS FOR
 *   The platform's search-everywhere pop-ups ask every contributor for NAMES and then for the ITEMS with
 *   a chosen name. The index already lists every class, function and global of the program (library
 *   included), so both contributors read it and answer from memory (doc/ide.md 6.10):
 *
 *     SharpGotoClassContributor     classes, interfaces and enums
 *     SharpGotoSymbolContributor    functions, globals, fields, enum values
 *
 *   An item is a NavigationItem: a name, a line of presentation (icon, `kind`, where it is) and
 *   `navigate`, which opens the file at the declaration (TargetPlaces moves the compiler's offset onto
 *   the text as it is now). Library items are offered only when the pop-up asks for non-project items
 *   (its "include library" switch).
 *
 *   Without an index the lists are empty.
 */
package sharp.ide.navigation

import com.intellij.icons.AllIcons
import com.intellij.navigation.GotoClassContributor
import com.intellij.navigation.ChooseByNameContributor
import com.intellij.navigation.ItemPresentation
import com.intellij.navigation.NavigationItem
import com.intellij.openapi.fileEditor.OpenFileDescriptor
import com.intellij.openapi.project.Project
import sharp.ide.index.AliasInfo
import sharp.ide.index.ClassInfo
import sharp.ide.index.Declaration
import sharp.ide.index.EnumValueInfo
import sharp.ide.index.FieldInfo
import sharp.ide.index.FunctionInfo
import sharp.ide.index.RunIndex
import sharp.ide.run.AnalysisService
import javax.swing.Icon

class SharpNavigationItem(
    private val project: Project,
    private val index: RunIndex,
    private val declaration: Declaration,
) : NavigationItem {

    val qualifiedName: String get() = when {
        declaration is ClassInfo -> declaration.id
        declaration.owner != null -> declaration.owner!!.id + "." + declaration.name
        declaration.module.isEmpty() -> declaration.name
        else -> declaration.module + "#" + declaration.name
    }

    override fun getName(): String = declaration.name

    override fun getPresentation(): ItemPresentation = object : ItemPresentation {
        override fun getPresentableText(): String = when (declaration) {
            is FunctionInfo -> declaration.signature
            else -> declaration.name
        }

        override fun getLocationString(): String = index.fileNameOf(declaration.file) + (declaration.owner?.let { " (${it.name})" } ?: "")

        override fun getIcon(unused: Boolean): Icon = when (declaration) {
            is ClassInfo -> if (declaration.isInterface) AllIcons.Nodes.Interface else if (declaration.isEnum) AllIcons.Nodes.Enum else AllIcons.Nodes.Class
            is FunctionInfo -> AllIcons.Nodes.Method
            is EnumValueInfo, is FieldInfo -> AllIcons.Nodes.Field
            else -> AllIcons.Nodes.Type
        }
    }

    override fun navigate(requestFocus: Boolean) {
        val place = TargetPlaces.resolve(project, index, declaration.file, declaration.at.from) ?: return
        OpenFileDescriptor(project, place.first, place.second).navigate(requestFocus)
    }

    override fun canNavigate(): Boolean = true

    override fun canNavigateToSource(): Boolean = true
}

class SharpGotoClassContributor : GotoClassContributor {

    override fun getNames(project: Project, includeNonProjectItems: Boolean): Array<String> {
        val index = AnalysisService.getInstance(project).index ?: return emptyArray()
        return index.allClasses.filter { included(index, it, includeNonProjectItems) }.map { it.name }.distinct().toTypedArray()
    }

    override fun getItemsByName(name: String, pattern: String, project: Project, includeNonProjectItems: Boolean): Array<NavigationItem> {
        val index = AnalysisService.getInstance(project).index ?: return emptyArray()
        return index.classesByName[name].orEmpty().filter { included(index, it, includeNonProjectItems) }
            .map { SharpNavigationItem(project, index, it) as NavigationItem }.toTypedArray()
    }

    override fun getQualifiedName(item: NavigationItem): String? = (item as? SharpNavigationItem)?.qualifiedName

    override fun getQualifiedNameSeparator(): String = "."

    private fun included(index: RunIndex, c: ClassInfo, includeLibrary: Boolean) =
        includeLibrary || index.fileEntry(c.file)?.library != true
}

class SharpGotoSymbolContributor : ChooseByNameContributor {

    override fun getNames(project: Project, includeNonProjectItems: Boolean): Array<String> {
        val index = AnalysisService.getInstance(project).index ?: return emptyArray()
        return symbols(index, includeNonProjectItems).map { it.name }.distinct().toTypedArray()
    }

    override fun getItemsByName(name: String, pattern: String, project: Project, includeNonProjectItems: Boolean): Array<NavigationItem> {
        val index = AnalysisService.getInstance(project).index ?: return emptyArray()
        return symbols(index, includeNonProjectItems).filter { it.name == name }
            .map { SharpNavigationItem(project, index, it) as NavigationItem }.toTypedArray()
    }

    private fun symbols(index: RunIndex, includeLibrary: Boolean): List<Declaration> =
        index.declarations.values.filter {
            (it is FunctionInfo && !it.isConstructor && !it.isOperator || it is FieldInfo || it is EnumValueInfo || it is AliasInfo) &&
                (includeLibrary || index.fileEntry(it.file)?.library != true)
        }
}
