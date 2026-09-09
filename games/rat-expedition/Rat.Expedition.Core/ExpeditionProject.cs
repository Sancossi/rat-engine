using System.Collections.ObjectModel;

namespace Rat.Expedition.Core;

public sealed record SceneAsset(string Id,string Path);
public sealed record ProjectDefinition(int SchemaVersion,string StartScene,SceneAsset[] Scenes);

public sealed class ExpeditionProject
{
    private readonly Dictionary<string,SceneDefinition> scenes;
    private readonly Dictionary<string,string>? paths;
    public string StartScene {get;}
    public IReadOnlyDictionary<string,SceneDefinition> Scenes {get;}
    public ExpeditionProject(string startScene,IEnumerable<SceneDefinition> definitions)
    {
        scenes=new(StringComparer.Ordinal);
        foreach(var scene in definitions)
        {
            if(scene is null)throw new InvalidDataException("Null project scene.");
            scene.Validate(); if(!scenes.TryAdd(scene.Id,scene))throw new InvalidDataException($"Duplicate scene '{scene.Id}'.");
        }
        if(string.IsNullOrWhiteSpace(startScene)||!scenes.ContainsKey(startScene))throw new InvalidDataException("Project start scene is missing.");
        StartScene=startScene; Scenes=new ReadOnlyDictionary<string,SceneDefinition>(scenes);
        foreach(var scene in scenes.Values)ValidateReferences(scene);
    }
    private ExpeditionProject(string startScene,IEnumerable<SceneDefinition> definitions,Dictionary<string,string> paths):this(startScene,definitions)=>this.paths=paths;
    public static ExpeditionProject Load(string contentDirectory)
    {
        string root=Path.GetFullPath(contentDirectory);
        var document=StrictJson.Read<ProjectDefinition>(ContentPath(root,"project.json"));
        if(document.SchemaVersion!=1 || document.Scenes is null || document.Scenes.Length==0)throw new InvalidDataException("Project requires schemaVersion 1 and scenes.");
        var paths=new Dictionary<string,string>(StringComparer.Ordinal); var scenes=new List<SceneDefinition>();
        foreach(var entry in document.Scenes)
        {
            if(entry is null||string.IsNullOrWhiteSpace(entry.Id))throw new InvalidDataException("Scene asset needs id/path.");
            string path=ContentPath(root,entry.Path);
            if(!paths.TryAdd(entry.Id,path))throw new InvalidDataException($"Duplicate project scene '{entry.Id}'.");
            var scene=SceneDefinition.Load(path);
            if(scene.Id!=entry.Id)throw new InvalidDataException($"Scene id in '{entry.Path}' does not match project.");
            scenes.Add(scene);
        }
        return new(document.StartScene,scenes,paths);
    }
    public SceneDefinition LoadCandidate(string id)
    {
        if(!scenes.TryGetValue(id,out var scene))throw new InvalidDataException($"Unknown scene '{id}'.");
        if(paths is not null)scene=SceneDefinition.Load(paths[id]);
        if(scene.Id!=id)throw new InvalidDataException($"Candidate scene id changed: '{id}'.");
        scene.Validate(); ValidateReferences(scene); return scene;
    }
    private void ValidateReferences(SceneDefinition scene)
    {
        foreach(var portal in scene.Portals)
        {
            if(!scenes.TryGetValue(portal.TargetScene,out var target))throw new InvalidDataException($"Portal '{portal.Id}' targets missing scene '{portal.TargetScene}'.");
            _=(portal.TargetScene==scene.Id?scene:target).GetSpawn(portal.TargetSpawn);
        }
    }
    private static string ContentPath(string root,string relative)
    {
        if(string.IsNullOrWhiteSpace(relative)||Path.IsPathRooted(relative)||relative.Contains(':'))throw new InvalidDataException("Content path must be relative.");
        var parts=relative.Replace('\\','/').Split('/');
        if(parts.Any(p=>p is "" or "." or ".." || p.EndsWith('.') || p.EndsWith(' ')))
            throw new InvalidDataException("Content path must not escape or use Windows-normalized directory names.");
        string current=root;
        foreach(var part in parts)
        {
            current=Path.Combine(current,part);
            if((File.Exists(current)||Directory.Exists(current))&&(File.GetAttributes(current)&FileAttributes.ReparsePoint)!=0)
                throw new InvalidDataException($"Content path uses a symbolic link/reparse point: '{relative}'.");
        }
        return current;
    }
}
