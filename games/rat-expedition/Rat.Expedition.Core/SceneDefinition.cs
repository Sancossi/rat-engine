using System.Numerics;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Rat.Expedition.Core;

public sealed record Point3(float X, float Y, float Z)
{
    [JsonIgnore] public Vector3 Vector => new(X,Y,Z);
    [JsonIgnore] public bool IsFinite => float.IsFinite(X) && float.IsFinite(Y) && float.IsFinite(Z);
}
public sealed record Point2(float X,float Z)
{
    [JsonIgnore] public Vector2 Vector => new(X,Z);
}
public sealed record WorldBox(string Id,Point3 Min,Point3 Max)
{
    public bool OverlapsFootprint(Vector3 p,float radius) => p.X+radius>Min.X && p.X-radius<Max.X && p.Z+radius>Min.Z && p.Z-radius<Max.Z;
}
public sealed record LadderDefinition(string Id,Point3 Bottom,Point3 Top,Point3 BottomEntry,Point3 TopEntry,Point3 BottomExit,Point3 TopExit);
public sealed record RampDefinition(string Id,Point2 Min,Point2 Max,RampAxis Axis,float StartY,float EndY,float Thickness)
{
    public WorldRamp ToWorld()
    {
        if(Min is null || Max is null) throw new InvalidDataException($"Ramp '{Id}' needs bounds.");
        var ramp=new WorldRamp(Id,Min.Vector,Max.Vector,Axis,StartY,EndY,Thickness); ramp.Validate(); return ramp;
    }
}
public sealed record NamedSpawn(string Id,Point3 Position);
public sealed record PortalDefinition(string Id,Point3 Min,Point3 Max,Point3 Anchor,string TargetScene,string TargetSpawn)
{
    public bool Contains(Vector3 p) => p.X>=Min.X && p.X<=Max.X && p.Y>=Min.Y && p.Y<=Max.Y && p.Z>=Min.Z && p.Z<=Max.Z;
}
public sealed record OccluderGroup(string Id,string[] Members,string? HideWith=null);

internal static class StrictJson
{
    internal static readonly JsonSerializerOptions Options=new()
    {
        PropertyNameCaseInsensitive=true, UnmappedMemberHandling=JsonUnmappedMemberHandling.Disallow,
        RespectRequiredConstructorParameters=true, Converters={new JsonStringEnumConverter()}
    };
    internal static T Read<T>(string path)
    {
        try
        {
            string text=File.ReadAllText(path);
            if(typeof(T)==typeof(SceneDefinition))
            {
                using var document=JsonDocument.Parse(text);
                if(document.RootElement.ValueKind==JsonValueKind.Object&&document.RootElement.EnumerateObject().Any(p=>p.Name.Equals("spawn",StringComparison.OrdinalIgnoreCase)))
                    throw new InvalidDataException("Scene schema 3 uses named spawns; obsolete 'spawn' is forbidden.");
            }
            return JsonSerializer.Deserialize<T>(text,Options) ?? throw new InvalidDataException($"Null document '{path}'.");
        }
        catch(Exception e) when(e is IOException or JsonException or ArgumentException)
        {throw new InvalidDataException($"Cannot load '{path}': {e.Message}",e);}
    }
}

[method:JsonConstructor]
public sealed record SceneDefinition(int SchemaVersion,string Id,WorldBox Floor,NamedSpawn[] Spawns,
    WorldBox[] Walls,WorldBox[] Structures,LadderDefinition[] Ladders,RampDefinition[] Ramps,
    PortalDefinition[] Portals,float RecoveryThreshold,OccluderGroup[] OcclusionGroups,WorldBox[] Decorations)
{
    // Convenience for pure-code fixtures. Serialized schema 3 always requires all fields.
    public SceneDefinition(int schemaVersion,string id,WorldBox floor,Point3 spawn,WorldBox[] walls,WorldBox[] structures,LadderDefinition[] ladders)
        :this(schemaVersion,id,floor,[new("entry",spawn)],walls,structures,ladders,[],[],-2,[],[]) {}
    public string DefaultSpawnId {get;init;}="entry";
    [JsonIgnore] public IReadOnlyDictionary<string,string> DisplayNames {get;init;}=new Dictionary<string,string>();
    [JsonIgnore] public Point3 Spawn {get=>GetSpawn(DefaultSpawnId); init=>Spawns=[new(DefaultSpawnId,value)];}
    [JsonIgnore] public IEnumerable<WorldBox> AllSolids=>Walls.Concat(Structures).Prepend(Floor);
    public LayeredCollisionWorld CreateWorld()=>new(AllSolids,Ramps.Select(r=>r.ToWorld()));
    public Point3 GetSpawn(string id)=>Spawns.SingleOrDefault(s=>s.Id==id)?.Position ?? throw new InvalidDataException($"Scene '{Id}' has no spawn '{id}'.");
    public static SceneDefinition Load(string path) {var scene=StrictJson.Read<SceneDefinition>(path);scene.Validate();return scene;}
    public bool IsFree(Vector3 p) {var world=CreateWorld();return world.HasSupport(p,TraversalMotor.Radius)&&world.HasClearance(p,TraversalMotor.Radius,TraversalMotor.StandingHeight);}

    public void Validate()
    {
        if(SchemaVersion!=3 || string.IsNullOrWhiteSpace(Id)) throw new InvalidDataException("Scene requires schemaVersion 3 and id.");
        if(Floor is null || Spawns is null || Walls is null || Structures is null || Ladders is null || Ramps is null ||
            Portals is null || OcclusionGroups is null || Decorations is null || Ramps.Any(r=>r is null))
            throw new InvalidDataException($"Scene '{Id}' is missing required collections/geometry.");
        var world=CreateWorld();
        var ids=AllSolids.Select(b=>b.Id).Concat(Ramps.Select(r=>r.Id)).ToHashSet(StringComparer.Ordinal);
        foreach(var wall in Walls) if(wall.Min.Y!=Floor.Max.Y) throw new InvalidDataException("Walls must start at floor level; use finite structures otherwise.");
        if(!float.IsFinite(RecoveryThreshold) || RecoveryThreshold>=Floor.Min.Y)
            throw new InvalidDataException("Recovery threshold must be finite and below the floor bottom.");
        void UniqueId(string? id) {if(string.IsNullOrWhiteSpace(id)||!ids.Add(id))throw new InvalidDataException($"Scene '{Id}' duplicate/empty id '{id}'.");}
        bool Safe(Point3? p)=>p is not null && p.IsFinite && world.HasSupport(p.Vector,TraversalMotor.Radius)&&world.HasClearance(p.Vector,TraversalMotor.Radius,TraversalMotor.StandingHeight);
        foreach(var spawn in Spawns)
        {
            if(spawn is null)throw new InvalidDataException("Null spawn."); UniqueId(spawn.Id);
            if(!Safe(spawn.Position)||spawn.Position.Y<=RecoveryThreshold)
                throw new InvalidDataException($"Spawn '{spawn.Id}' needs standing support/clearance above the recovery threshold.");
        }
        _=GetSpawn(DefaultSpawnId);
        foreach(var ladder in Ladders)
        {
            if(ladder is null)throw new InvalidDataException("Null ladder."); UniqueId(ladder.Id);
            Point3[] points=[ladder.Bottom,ladder.Top,ladder.BottomEntry,ladder.TopEntry,ladder.BottomExit,ladder.TopExit];
            if(points.Any(p=>p is null||!p.IsFinite)||ladder.Bottom.X!=ladder.Top.X||ladder.Bottom.Z!=ladder.Top.Z||
                ladder.Top.Y<=ladder.Bottom.Y||!float.IsFinite(ladder.Top.Y-ladder.Bottom.Y)||
                ladder.BottomEntry.Y!=ladder.Bottom.Y||ladder.BottomExit.Y!=ladder.Bottom.Y||ladder.TopEntry.Y!=ladder.Top.Y||ladder.TopExit.Y!=ladder.Top.Y)
                throw new InvalidDataException($"Ladder '{ladder.Id}' needs finite vertical endpoints and matching entry/exit heights.");
            foreach(var p in new[]{ladder.BottomEntry,ladder.TopEntry,ladder.BottomExit,ladder.TopExit})
                if(!Safe(p))throw new InvalidDataException($"Ladder '{ladder.Id}' entry/exit needs standing support/clearance.");
            foreach(var (from,to) in new[]{(ladder.Bottom,ladder.Top),(ladder.BottomEntry,ladder.Bottom),(ladder.TopEntry,ladder.Top),(ladder.Bottom,ladder.BottomExit),(ladder.Top,ladder.TopExit)})
                if(Vector3.Distance(from.Vector,to.Vector)>(from.Y==to.Y?1.5f:10f)||world.SweepFraction(from.Vector,to.Vector,TraversalMotor.Radius,TraversalMotor.StandingHeight)<1)
                    throw new InvalidDataException($"Ladder '{ladder.Id}' corridor/entry/exit is blocked or too long.");
        }
        foreach(var portal in Portals)
        {
            if(portal is null)throw new InvalidDataException("Null portal."); UniqueId(portal.Id);
            if(portal.Min is null||portal.Max is null||!portal.Min.IsFinite||!portal.Max.IsFinite||
                portal.Min.X>=portal.Max.X||portal.Min.Y>=portal.Max.Y||portal.Min.Z>=portal.Max.Z||
                !float.IsFinite(portal.Max.X-portal.Min.X)||!float.IsFinite(portal.Max.Y-portal.Min.Y)||!float.IsFinite(portal.Max.Z-portal.Min.Z)||
                string.IsNullOrWhiteSpace(portal.TargetScene)||string.IsNullOrWhiteSpace(portal.TargetSpawn)||
                !Safe(portal.Anchor)||!portal.Contains(portal.Anchor.Vector))
                throw new InvalidDataException($"Portal '{portal.Id}' needs finite bounds, safe anchor and target.");
            if(Spawns.Any(s=>portal.Contains(s.Position.Vector)))throw new InvalidDataException($"Spawn lies inside portal '{portal.Id}'.");
        }
        _=new LayeredCollisionWorld(Decorations,[]);
        foreach(var box in Decorations)UniqueId(box.Id);
        var geometry=AllSolids.Select(b=>b.Id).Concat(Ramps.Select(r=>r.Id)).Concat(Decorations.Select(b=>b.Id)).ToHashSet(StringComparer.Ordinal);
        var members=new HashSet<string>(StringComparer.Ordinal);
        foreach(var group in OcclusionGroups)
        {
            if(group is null)throw new InvalidDataException("Null occluder group."); UniqueId(group.Id);
            if(group.Members is null||group.Members.Length==0||group.Members.Any(m=>!geometry.Contains(m)||!members.Add(m)))
                throw new InvalidDataException($"Occluder group '{group.Id}' needs existing exclusive members.");
        }
        var groups=OcclusionGroups.ToDictionary(g=>g.Id,StringComparer.Ordinal);
        foreach(var group in OcclusionGroups)
        {
            var visited=new HashSet<string>(StringComparer.Ordinal){group.Id};var current=group;
            while(current.HideWith is string parent)
            {
                if(!groups.TryGetValue(parent,out current!)||!visited.Add(parent))
                    throw new InvalidDataException($"Occluder group '{group.Id}' has a missing/cyclic HideWith dependency.");
            }
        }
    }
}
