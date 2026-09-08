using System.Numerics;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Rat.Expedition.Core;

public sealed record Point3(float X, float Y, float Z)
{
    public Vector3 Vector => new(X, Y, Z);
    [JsonIgnore] public bool IsFinite => float.IsFinite(X) && float.IsFinite(Y) && float.IsFinite(Z);
}

public sealed record WorldBox(string Id, Point3 Min, Point3 Max)
{
    public bool OverlapsFootprint(Vector3 position, float radius) =>
        position.X + radius > Min.X && position.X - radius < Max.X &&
        position.Z + radius > Min.Z && position.Z - radius < Max.Z;
}

public sealed record SceneDefinition(int SchemaVersion, string Id, WorldBox Floor, Point3 Spawn, WorldBox[] Walls)
{
    public static SceneDefinition Load(string path)
    {
        try
        {
            var scene = JsonSerializer.Deserialize<SceneDefinition>(File.ReadAllText(path),
                new JsonSerializerOptions { PropertyNameCaseInsensitive = true, UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow });
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
        if (SchemaVersion != 1 || string.IsNullOrWhiteSpace(Id))
            throw new InvalidDataException("Scene requires schemaVersion 1 and an id.");
        if (Floor is null || Spawn is null || Walls is null || !Spawn.IsFinite)
            throw new InvalidDataException("Scene requires floor, finite spawn and walls.");
        var ids = new HashSet<string>(StringComparer.Ordinal);
        foreach (var box in Walls.Prepend(Floor))
        {
            if (box is null || string.IsNullOrWhiteSpace(box.Id) || !ids.Add(box.Id) ||
                box.Min is null || box.Max is null || !box.Min.IsFinite || !box.Max.IsFinite ||
                box.Min.X >= box.Max.X || box.Min.Y >= box.Max.Y || box.Min.Z >= box.Max.Z ||
                !float.IsFinite(box.Max.X - box.Min.X) || !float.IsFinite(box.Max.Y - box.Min.Y) || !float.IsFinite(box.Max.Z - box.Min.Z))
                throw new InvalidDataException("Geometry requires unique ids and finite min < max bounds.");
        }
        if (Floor.Max.X - Floor.Min.X <= TraversalMotor.Radius * 2 ||
            Floor.Max.Z - Floor.Min.Z <= TraversalMotor.Radius * 2 ||
            Spawn.Y != Floor.Max.Y || !IsFree(Spawn.Vector))
            throw new InvalidDataException("Spawn needs the floor surface and a free complete footprint.");
        foreach (var wall in Walls)
        {
            if (wall.Min.Y != Floor.Max.Y)
                throw new InvalidDataException("P1.1 supports walls standing on the single floor only.");
        }
    }

    public bool IsFree(Vector3 position) =>
        position.X - TraversalMotor.Radius >= Floor.Min.X && position.X + TraversalMotor.Radius <= Floor.Max.X &&
        position.Z - TraversalMotor.Radius >= Floor.Min.Z && position.Z + TraversalMotor.Radius <= Floor.Max.Z &&
        !Walls.Any(wall => wall.OverlapsFootprint(position, TraversalMotor.Radius));
}
