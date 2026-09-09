using System.IO;

namespace Rat.StrideMcp.Adapter;

internal static class AssetPaths
{
    internal static bool Inside(string root,string path)
    {
        root=Path.GetFullPath(root);path=Path.GetFullPath(path);
        var relative=Path.GetRelativePath(root,path);
        if(Path.IsPathRooted(relative)||relative==".."||relative.StartsWith(".."+Path.DirectorySeparatorChar,StringComparison.Ordinal))return false;
        // Recheck each operation; a package or asset directory can be replaced
        // after session startup. Do not follow junctions outside the selected root.
        for(var current=path;current is not null;current=Path.GetDirectoryName(current))
        {
            if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)return false;
            if(string.Equals(current,root,StringComparison.OrdinalIgnoreCase))break;
        }
        return true;
    }
}
