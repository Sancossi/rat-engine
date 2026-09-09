using System.Numerics;

namespace Rat.Expedition.Core;

public enum RampAxis { X, Z }

// A finite slab between parallel planes, with vertical end/side faces.
// StartY/EndY are the top heights at the minimum/maximum coordinate of Axis.
public sealed record WorldRamp(string Id, Vector2 Min, Vector2 Max, RampAxis Axis,
    float StartY, float EndY, float Thickness)
{
    public void Validate()
    {
        float length = Axis == RampAxis.X ? Max.X - Min.X : Max.Y - Min.Y;
        if (string.IsNullOrWhiteSpace(Id) || !Enum.IsDefined(Axis) ||
            !float.IsFinite(Min.X) || !float.IsFinite(Min.Y) || !float.IsFinite(Max.X) || !float.IsFinite(Max.Y) ||
            !float.IsFinite(StartY) || !float.IsFinite(EndY) || !float.IsFinite(Thickness) ||
            Max.X - Min.X <= BodyCollisionWorld.Epsilon || Max.Y - Min.Y <= BodyCollisionWorld.Epsilon ||
            !float.IsFinite(Max.X - Min.X) || !float.IsFinite(Max.Y - Min.Y) ||
            Thickness <= BodyCollisionWorld.Epsilon || !float.IsFinite(Math.Min(StartY, EndY) - Thickness) ||
            Math.Abs((double)EndY - StartY) <= BodyCollisionWorld.Epsilon || Math.Abs((double)EndY - StartY) > length)
            throw new InvalidDataException($"Ramp '{Id}' requires finite bounds, positive thickness and nonzero slope of at most 1.");
    }

    public double Slope => ((double)EndY - StartY) / (Axis == RampAxis.X ? (double)Max.X - Min.X : (double)Max.Y - Min.Y);
    public double TopAt(double x, double z) => StartY + Slope * (Axis == RampAxis.X ? x - Min.X : z - Min.Y);
}
