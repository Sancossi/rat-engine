using System.Numerics;
using System.Text.Json;
using System.Text.Json.Nodes;
using Rat.Expedition.Core;

int passed = 0, failed = 0;
void Check(string name, Action test)
{
    try { test(); passed++; Console.WriteLine($"PASS {name}"); }
    catch (Exception error) { failed++; Console.Error.WriteLine($"FAIL {name}: {error}"); }
}
void Require(bool condition, string message) { if (!condition) throw new Exception(message); }
void Reject(Action action)
{
    try { action(); } catch (InvalidDataException) { return; }
    throw new Exception("Invalid scene was accepted");
}
WorldBox Box(string id, float x1, float z1, float x2, float z2) => new(id, new(x1, 0, z1), new(x2, 2, z2));
SceneDefinition Scene(params WorldBox[] walls) => new(1, "test", new("floor", new(-10, -1, -10), new(10, 0, 10)), new(0, 0, 3), walls);
void Drive(TraversalMotor motor, Vector2 input, int frames) { for (int i = 0; i < frames; i++) motor.Advance(1.0 / 60, input); }

Check("camera-relative W moves screen-up on XZ at 3 units per second", () => {
    var motor = new TraversalMotor(Scene()); Drive(motor, new(0, 1), 60);
    Require(Math.Abs(Vector3.Distance(motor.Position, new(0, 0, 3)) - 3) < 0.001, "Wrong walk speed");
    Require(motor.Position.X < 0 && motor.Position.Z < 3 && motor.Position.Y == 0, "Wrong camera basis or floor height");
});
Check("diagonal speed equals cardinal speed", () => {
    var a = new TraversalMotor(Scene()); var b = new TraversalMotor(Scene());
    Drive(a, new(1, 0), 60); Drive(b, new(1, 1), 60);
    Require(Math.Abs(Vector3.Distance(a.Position, new(0, 0, 3)) - Vector3.Distance(b.Position, new(0, 0, 3))) < 0.001, "Diagonal speed boost");
});
Check("thin wall sweep stops footprint and permits sliding", () => {
    var scene = Scene(Box("thin-wall", -9, -0.001f, 9, 0.001f)); var motor = new TraversalMotor(scene);
    Drive(motor, new(0, 1), 180);
    Require(motor.Position.Z >= 0.201f - 0.0001f, "Passed through wall");
    Require(motor.Position.X < -3 && scene.IsFree(motor.Position), "No wall sliding or footprint overlaps");
});
Check("all floor edges retain complete footprint", () => {
    var scene = Scene(); var motor = new TraversalMotor(scene);
    foreach (var direction in new[] { new Vector2(1,1), new(-1,-1), new(1,-1), new(-1,1) }) {
        Drive(motor, direction, 1000); Require(scene.IsFree(motor.Position), "Left floor bounds");
    }
});
Check("long frame caps catchup and cannot tunnel", () => {
    var scene = Scene(Box("wall", -9, 2, 9, 2.01f)); var motor = new TraversalMotor(scene);
    motor.Advance(20, new(1,1));
    Require(motor.Ticks == TraversalMotor.MaximumCatchUpSteps, "Unbounded catchup");
    Require(scene.IsFree(motor.Position) && motor.Position.Z > 2.21f, "Tunneled");
});
Check("focus loss clears pending fractional tick", () => {
    var motor = new TraversalMotor(Scene()); motor.Advance(TraversalMotor.StepSeconds * 0.75, new(1,0));
    motor.Advance(20, new(1,0), false); var paused = motor.Position;
    motor.Advance(TraversalMotor.StepSeconds * 0.5, Vector2.Zero);
    Require(motor.Ticks == 0 && motor.Position == paused, "Focus regain executed stale motion");
});
Check("spawn inside wall rejected", () => Reject(() => Scene(Box("wall", -1, 2, 1, 4)).Validate()));
Check("spawn on floor edge lacking footprint rejected", () => Reject(() => (Scene() with { Spawn = new(10,0,0) }).Validate()));
Check("invalid or nonfinite geometry rejected", () => {
    Reject(() => Scene(Box("inverted", 2, 0, 1, 1)).Validate());
    Reject(() => Scene(Box("nan", float.NaN, 0, 1, 1)).Validate());
    Reject(() => Scene(Box("inf", 0, 0, float.PositiveInfinity, 1)).Validate());
    Reject(() => Scene(Box("overflow-span", -float.MaxValue, 0, float.MaxValue, 1)).Validate());
    Reject(() => (Scene() with { Spawn = new(0,1,0) }).Validate());
});
Check("duplicate ids and unsupported raised wall rejected", () => {
    Reject(() => Scene(Box("same", 2,2,3,3), Box("same", 4,4,5,5)).Validate());
    Reject(() => Scene(new WorldBox("raised", new(2,1,2), new(3,2,3))).Validate());
});
Check("malformed missing and unknown JSON fields rejected", () => {
    string path = Path.Combine(Path.GetTempPath(), "rat-scene-" + Guid.NewGuid() + ".json");
    try {
        foreach (string json in new[] { "{broken", "{}", "null", "{\"unexpected\":true}" }) {
            File.WriteAllText(path, json); Reject(() => SceneDefinition.Load(path));
        }
        File.Delete(path); Reject(() => SceneDefinition.Load(path));
    } finally { if (File.Exists(path)) File.Delete(path); }
});
Check("missing nested spawn and bounds coordinates rejected", () => {
    string path = Path.Combine(Path.GetTempPath(), "rat-scene-required-" + Guid.NewGuid() + ".json");
    try {
        foreach (var member in new[] { "X", "Y", "Z" }) {
            foreach (var location in new[] { "Spawn", "FloorMin", "WallMax" }) {
                var json = JsonSerializer.SerializeToNode(Scene(Box("wall", 2,2,3,3)))!;
                var point = location == "Spawn" ? json["Spawn"] : location == "FloorMin" ? json["Floor"]!["Min"] : json["Walls"]![0]!["Max"];
                point!.AsObject().Remove(member);
                File.WriteAllText(path, json.ToJsonString()); Reject(() => SceneDefinition.Load(path));
            }
        }
    } finally { if (File.Exists(path)) File.Delete(path); }
});
BodyCollisionTests.Run(Check, Require);
Console.WriteLine($"Core scenarios: {passed} passed, {failed} failed.");
return failed == 0 ? 0 : 1;
