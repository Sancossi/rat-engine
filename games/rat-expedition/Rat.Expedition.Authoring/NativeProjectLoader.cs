using Rat.Expedition.Core;
using Stride.Core;
using Stride.Core.Serialization.Contents;
using Stride.Engine;

namespace Rat.Expedition.Authoring;

public sealed class NativeProjectLoader(IServiceRegistry services,string projectAsset="ExpeditionProject")
{
    public ExpeditionProject Load()
    {
        var initial=Read();
        return ExpeditionProject.FromSource(initial.StartScene,initial.Scenes.Values,id=>Read().LoadCandidate(id));
    }
    // A fresh manager on every candidate avoids serving a previously accepted object
    // from ContentManager's object cache. No native entity/GPU resource escapes Read.
    private ExpeditionProject Read()
    {
        var content=new ContentManager(services);var loaded=new List<Scene>();
        try
        {
            var manifest=content.Load<Scene>(projectAsset);loaded.Add(manifest);
            var roots=NativeSceneAdapter.Entities(manifest,"ExpeditionProject").Where(e=>e.Get<TraversalProjectComponent>() is not null).ToArray();
            if(roots.Length!=1)throw new InvalidDataException("ExpeditionProject requires exactly one project component.");
            var settings=roots[0].Get<TraversalProjectComponent>();
            if(settings.StartScene is null||settings.Scenes is null||settings.Scenes.Count==0)throw new InvalidDataException("ExpeditionProject requires StartScene and Scenes references.");
            var scenes=new Dictionary<string,Scene>(StringComparer.Ordinal);
            foreach(var reference in settings.Scenes)
            {
                if(reference is null||string.IsNullOrWhiteSpace(reference.Url)||scenes.ContainsKey(reference.Url))throw new InvalidDataException("ExpeditionProject has empty/duplicate scene reference.");
                var scene=content.Load<Scene>(reference.Url);loaded.Add(scene);scenes.Add(reference.Url,scene);
            }
            if(!scenes.ContainsKey(settings.StartScene.Url))throw new InvalidDataException("ExpeditionProject StartScene is absent from Scenes.");
            var definitions=scenes.ToDictionary(p=>p.Key,p=>NativeSceneAdapter.Convert(p.Value,p.Key,scenes));
            return new(definitions[settings.StartScene.Url].Id,definitions.Values);
        }
        finally {foreach(var scene in loaded.AsEnumerable().Reverse())content.Unload(scene);}
    }
}
