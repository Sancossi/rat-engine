using System.Numerics;

namespace Rat.Expedition.Core;

public sealed class TraversalMotor
{
    public const float Radius = 0.2f;
    public const float Speed = 3;
    public const double StepSeconds = 1.0 / 120;
    public const int MaximumCatchUpSteps = 12;
    public static readonly Vector2 CameraRight = Vector2.Normalize(new(1, -1));
    public static readonly Vector2 CameraForward = Vector2.Normalize(new(-1, -1));
    private readonly SceneDefinition scene;
    private double accumulator;
    public Vector3 Position { get; private set; }
    public long Ticks { get; private set; }

    public TraversalMotor(SceneDefinition scene)
    {
        scene.Validate();
        this.scene = scene;
        Position = scene.Spawn.Vector;
    }

    public void Advance(double elapsed, Vector2 screenInput, bool active = true)
    {
        if (!double.IsFinite(elapsed) || elapsed < 0 || !float.IsFinite(screenInput.X) || !float.IsFinite(screenInput.Y))
            throw new ArgumentOutOfRangeException(nameof(elapsed), "Time and input must be finite, time nonnegative.");
        if (!active) { accumulator = 0; return; }
        accumulator = Math.Min(accumulator + elapsed, StepSeconds * MaximumCatchUpSteps);
        while (accumulator + 1e-12 >= StepSeconds)
        {
            Step(screenInput);
            accumulator -= StepSeconds;
        }
    }

    private void Step(Vector2 screenInput)
    {
        if (screenInput.LengthSquared() > 1) screenInput = Vector2.Normalize(screenInput);
        var delta = (CameraRight * screenInput.X + CameraForward * screenInput.Y) * (Speed * (float)StepSeconds);
        Position = SweepAxis(Position, delta.X, true);
        Position = SweepAxis(Position, delta.Y, false);
        Ticks++;
    }

    private Vector3 SweepAxis(Vector3 start, float delta, bool xAxis)
    {
        float from = xAxis ? start.X : start.Z;
        float other = xAxis ? start.Z : start.X;
        float minFloor = (xAxis ? scene.Floor.Min.X : scene.Floor.Min.Z) + Radius;
        float maxFloor = (xAxis ? scene.Floor.Max.X : scene.Floor.Max.Z) - Radius;
        float to = Math.Clamp(from + delta, minFloor, maxFloor);
        foreach (var wall in scene.Walls)
        {
            float otherMin = (xAxis ? wall.Min.Z : wall.Min.X) - Radius;
            float otherMax = (xAxis ? wall.Max.Z : wall.Max.X) + Radius;
            if (other <= otherMin || other >= otherMax) continue;
            float wallMin = (xAxis ? wall.Min.X : wall.Min.Z) - Radius;
            float wallMax = (xAxis ? wall.Max.X : wall.Max.Z) + Radius;
            if (delta > 0 && from <= wallMin && to > wallMin) to = wallMin;
            if (delta < 0 && from >= wallMax && to < wallMax) to = wallMax;
        }
        return xAxis ? new(to, start.Y, start.Z) : new(start.X, start.Y, to);
    }
}
