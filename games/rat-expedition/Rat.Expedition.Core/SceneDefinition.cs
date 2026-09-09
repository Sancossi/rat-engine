using System.Numerics;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Rat.Expedition.Core;

public sealed record Point3(float X, float Y, float Z)
{
    [JsonIgnore] public Vector3 Vector => new(X, Y, Z);
    [JsonIgnore] public bool IsFinite => float.IsFinite(X) && float.IsFinite(Y) && float.IsFinite(Z);
}

public sealed record WorldBox(string Id, Point3 Min, Point3 Max)
{
    public bool OverlapsFootprint(Vector3 position, float radius) =>
        position.X + radius > Min.X && position.X - radius < Max.X &&
        position.Z + radius > Min.Z && position.Z - radius < Max.Z;
}

public sealed record LadderDefinition(string Id, Point3 Bottom, Point3 Top, Point3 BottomEntry, Point3 TopEntry, Point3 BottomExit, Point3 TopExit);

public sealed record SceneDefinition(int SchemaVersion, string Id, WorldBox Floor, Point3 Spawn, WorldBox[] Walls, WorldBox[] Structures, LadderDefinition[] Ladders)
{
    [JsonIgnore] public IEnumerable<WorldBox> AllSolids => Walls.Concat(Structures).Prepend(Floor);
    public static SceneDefinition Load(string path)
    {
        try
        {
            var scene = JsonSerializer.Deserialize<SceneDefinition>(File.ReadAllText(path),
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true, UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow,
                    RespectRequiredConstructorParameters = true });
            if (scene is null) throw new InvalidDataException("Scene document is null.");
            scene.Validate();
            return scene;
        }
        catch (Exception error) when (error is IOException or JsonException or ArgumentException)
        {
            throw new InvalidDataException($"Cannot load courtyard '{path}': {error.Message}", error);
        }
    }

    public void Validate()
    {
        if (SchemaVersion != 2 || string.IsNullOrWhiteSpace(Id))
            throw new InvalidDataException("Scene requires schemaVersion 2 and an id.");
        if (Floor is null || Spawn is null || Walls is null || Structures is null || Ladders is null || !Spawn.IsFinite)
            throw new InvalidDataException("Scene requires floor, finite spawn, walls, structures and ladders.");
        var ids = new HashSet<string>(StringComparer.Ordinal);
        foreach (var box in AllSolids)
        {
            if (box is null || string.IsNullOrWhiteSpace(box.Id) || !ids.Add(box.Id) ||
                box.Min is null || box.Max is null || !box.Min.IsFinite || !box.Max.IsFinite ||
                box.Min.X >= box.Max.X || box.Min.Y >= box.Max.Y || box.Min.Z >= box.Max.Z ||
                !float.IsFinite(box.Max.X - box.Min.X) || !float.IsFinite(box.Max.Y - box.Min.Y) || !float.IsFinite(box.Max.Z - box.Min.Z))
                throw new InvalidDataException("Geometry requires unique ids and finite min < max bounds.");
        }
        if (Floor.Max.X - Floor.Min.X <= TraversalMotor.Radius * 2 ||
            Floor.Max.Z - Floor.Min.Z <= TraversalMotor.Radius * 2 ||
            !IsFree(Spawn.Vector))
            throw new InvalidDataException("Spawn needs stable support and full standing clearance.");
        foreach (var wall in Walls)
        {
            if (wall.Min.Y != Floor.Max.Y)
                throw new InvalidDataException("Walls start at floor level; use structures for finite elevated solids.");
        }
        var world = new BodyCollisionWorld(AllSolids);
        foreach (var ladder in Ladders)
        {
            if (ladder is null || string.IsNullOrWhiteSpace(ladder.Id) || !ids.Add(ladder.Id))
                throw new InvalidDataException("Ladder ids must be unique across scene geometry.");
            Point3[] points = [ladder.Bottom,ladder.Top,ladder.BottomEntry,ladder.TopEntry,ladder.BottomExit,ladder.TopExit];
            if (points.Any(p => p is null || !p.IsFinite) || ladder.Bottom.X != ladder.Top.X || ladder.Bottom.Z != ladder.Top.Z ||
                ladder.Top.Y <= ladder.Bottom.Y || !float.IsFinite(ladder.Top.Y-ladder.Bottom.Y) ||
                ladder.BottomEntry.Y != ladder.Bottom.Y || ladder.BottomExit.Y != ladder.Bottom.Y ||
                ladder.TopEntry.Y != ladder.Top.Y || ladder.TopExit.Y != ladder.Top.Y)
                throw new InvalidDataException($"Ladder {ladder.Id} needs finite vertical endpoints and matching entry/exit heights.");
            foreach (var p in new[] {ladder.BottomEntry,ladder.TopEntry,ladder.BottomExit,ladder.TopExit})
                if (!world.CanStand(p.Vector,TraversalMotor.Radius,TraversalMotor.StandingHeight))
                    throw new InvalidDataException($"Ladder {ladder.Id} entry/exit needs full standing support and clearance.");
            foreach (var (from,to) in new[] {(ladder.Bottom,ladder.Top),(ladder.BottomEntry,ladder.Bottom),(ladder.TopEntry,ladder.Top),(ladder.Bottom,ladder.BottomExit),(ladder.Top,ladder.TopExit)})
                if (Vector3.Distance(from.Vector,to.Vector)> (from.Y==to.Y ? 1.5f : 10f) ||
                    !world.IsSegmentClear(from.Vector,to.Vector,TraversalMotor.Radius,TraversalMotor.StandingHeight))
                    throw new InvalidDataException($"Ladder {ladder.Id} corridor or entry/exit segment is blocked or too long.");
        }
    }

    public bool IsFree(Vector3 position) => new BodyCollisionWorld(AllSolids).CanStand(position,TraversalMotor.Radius,TraversalMotor.StandingHeight);
}
