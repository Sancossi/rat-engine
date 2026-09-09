using System.Numerics;

namespace Rat.Expedition.Core;

// Finite solid boxes only. Feet Y is explicit; this adapter never chooses a floor.
public sealed class BodyCollisionWorld
{
    public const float Epsilon = 0.00001f;
    private readonly WorldBox[] solids;

    public BodyCollisionWorld(IEnumerable<WorldBox> solids) => this.solids = solids.ToArray();

    public bool HasClearance(Vector3 feet, float radius, float height) => !solids.Any(box =>
        feet.X + radius > box.Min.X + Epsilon && feet.X - radius < box.Max.X - Epsilon &&
        feet.Z + radius > box.Min.Z + Epsilon && feet.Z - radius < box.Max.Z - Epsilon &&
        feet.Y + height > box.Min.Y + Epsilon && feet.Y < box.Max.Y - Epsilon);

    // One complete supporting box is deliberately conservative at seams; no corner-only test.
    public bool HasSupport(Vector3 feet, float radius) => solids.Any(box =>
        Math.Abs(feet.Y - box.Max.Y) <= Epsilon &&
        feet.X - radius >= box.Min.X - Epsilon && feet.X + radius <= box.Max.X + Epsilon &&
        feet.Z - radius >= box.Min.Z - Epsilon && feet.Z + radius <= box.Max.Z + Epsilon);

    public bool CanStand(Vector3 feet, float radius, float height) =>
        HasSupport(feet, radius) && HasClearance(feet, radius, height);

    public bool IsSegmentClear(Vector3 from, Vector3 to, float radius, float height) =>
        HasClearance(from, radius, height) && FirstHit(from, to, radius, height) >= 1;

    public Vector3 MoveGrounded(Vector3 start, Vector2 delta, float radius, float height)
    {
        if (!CanStand(start, radius, height)) return start;
        return SweepAxis(SweepAxis(start, delta.X, true, radius, height), delta.Y, false, radius, height);
    }

    private Vector3 SweepAxis(Vector3 start, float delta, bool xAxis, float radius, float height)
    {
        if (delta == 0) return start;
        float from = xAxis ? start.X : start.Z;
        float other = xAxis ? start.Z : start.X;
        var intervals = solids.Where(box => Math.Abs(start.Y - box.Max.Y) <= Epsilon &&
            other - radius >= (xAxis ? box.Min.Z : box.Min.X) - Epsilon &&
            other + radius <= (xAxis ? box.Max.Z : box.Max.X) + Epsilon)
            .Select(box => (Min: (xAxis ? box.Min.X : box.Min.Z) + radius, Max: (xAxis ? box.Max.X : box.Max.Z) - radius))
            .Where(interval => interval.Min <= interval.Max).OrderBy(interval => interval.Min).ToArray();
        // Merge overlapping centre intervals so a long step cannot skip a support gap.
        float low = from, high = from;
        bool found = false;
        foreach (var interval in intervals)
        {
            if (!found && from >= interval.Min - Epsilon && from <= interval.Max + Epsilon)
            { low = interval.Min; high = interval.Max; found = true; }
            else if (found && interval.Min <= high + Epsilon) high = Math.Max(high, interval.Max);
        }
        if (!found) return start;
        float to = Math.Clamp(from + delta, low, high);
        var target = xAxis ? new Vector3(to, start.Y, start.Z) : new Vector3(start.X, start.Y, to);
        return Vector3.Lerp(start, target, FirstHit(start, target, radius, height));
    }

    private float FirstHit(Vector3 from, Vector3 to, float radius, float height)
    {
        float first = 1;
        var delta = to - from;
        foreach (var box in solids)
        {
            var min = new Vector3(box.Min.X - radius + Epsilon, box.Min.Y - height + Epsilon, box.Min.Z - radius + Epsilon);
            var max = new Vector3(box.Max.X + radius - Epsilon, box.Max.Y - Epsilon, box.Max.Z + radius - Epsilon);
            float enter = 0, leave = 1;
            bool intersects = true;
            for (int axis = 0; axis < 3; axis++)
            {
                if (delta[axis] == 0)
                { if (from[axis] <= min[axis] || from[axis] >= max[axis]) { intersects = false; break; } }
                else
                {
                    float a = (min[axis] - from[axis]) / delta[axis], b = (max[axis] - from[axis]) / delta[axis];
                    enter = Math.Max(enter, Math.Min(a, b)); leave = Math.Min(leave, Math.Max(a, b));
                    if (enter >= leave) { intersects = false; break; }
                }
            }
            if (intersects && leave > 0 && enter < first) first = Math.Max(0, enter);
        }
        return first;
    }
}
