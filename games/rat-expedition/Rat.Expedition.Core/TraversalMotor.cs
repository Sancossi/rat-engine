using System.Numerics;

namespace Rat.Expedition.Core;

public sealed class TraversalMotor
{
    public const float Radius = 0.2f, StandingHeight = .8f, CrouchedHeight = .4f;
    public const float Speed = 3, CrouchSpeed = 1.5f, ClimbSpeed = 1;
    public const double StepSeconds = 1.0 / 120;
    public const int MaximumCatchUpSteps = 12;
    public static readonly Vector2 CameraRight = Vector2.Normalize(new(1, -1));
    public static readonly Vector2 CameraForward = Vector2.Normalize(new(-1, -1));
    private readonly SceneDefinition scene;
    private readonly BodyCollisionWorld world;
    private double accumulator;
    private bool interactWasHeld, pendingInteract;
    private LadderDefinition? ladder;
    private readonly Queue<Vector3> approach = new();
    private bool departing;
    public Vector3 Position { get; private set; }
    public long Ticks { get; private set; }
    public PlayerTraversalState State { get; private set; } = PlayerTraversalState.Standing;
    public BodyStance Stance => State == PlayerTraversalState.Crouched ? BodyStance.Crouched : BodyStance.Standing;
    public TraversalMode Mode => State == PlayerTraversalState.Climbing ? TraversalMode.Climbing : TraversalMode.Grounded;
    public float BodyHeight => Stance == BodyStance.Standing ? StandingHeight : CrouchedHeight;
    public bool StandBlocked { get; private set; }

    public TraversalMotor(SceneDefinition scene)
    {
        scene.Validate(); this.scene = scene;
        world = new(scene.AllSolids); Position = scene.Spawn.Vector;
    }

    public TraversalSnapshot Snapshot
    {
        get
        {
            var context = ladder is null ? FindLadder() : null;
            string hint = ladder is not null ? "W / S — вверх / вниз" : StandBlocked ? "Здесь нельзя встать" :
                context is not null ? "E / Enter — лестница" : "Ctrl — присесть";
            return new(scene.Id,Position,State,Stance,Mode,BodyHeight,StandBlocked,ladder?.Id ?? context?.Ladder.Id,hint);
        }
    }

    public void Advance(double elapsed, TraversalInput input, bool active = true)
    {
        if (!double.IsFinite(elapsed) || elapsed < 0 || !float.IsFinite(input.Move.X) || !float.IsFinite(input.Move.Y))
            throw new ArgumentOutOfRangeException(nameof(elapsed), "Time and input must be finite, time nonnegative.");
        if (!active) { accumulator = 0; pendingInteract = false; interactWasHeld = true; return; }
        pendingInteract |= input.InteractHeld && !interactWasHeld;
        interactWasHeld = input.InteractHeld;
        accumulator = Math.Min(accumulator + elapsed, StepSeconds * MaximumCatchUpSteps);
        while (accumulator + 1e-12 >= StepSeconds)
        {
            bool wasClimbing = State == PlayerTraversalState.Climbing;
            Step(input); accumulator -= StepSeconds;
            // A climb-direction input belongs to this action until the exit snapshot is emitted.
            if(wasClimbing && State != PlayerTraversalState.Climbing) {accumulator=0;break;}
        }
    }

    private void Step(TraversalInput input)
    {
        bool interact = pendingInteract; pendingInteract = false;
        if (State == PlayerTraversalState.Climbing) { Climb(input.Move.Y); Ticks++; return; }
        if (input.CrouchHeld) { State = PlayerTraversalState.Crouched; StandBlocked = false; }
        else TryStand();
        var target = interact ? FindLadder() : null;
        if (target is not null && TryBeginClimb(target)) { Climb(0); Ticks++; return; }
        var move = input.Move;
        if (move.LengthSquared() > 1) move = Vector2.Normalize(move);
        var delta = (CameraRight * move.X + CameraForward * move.Y) * ((Stance == BodyStance.Crouched ? CrouchSpeed : Speed) * (float)StepSeconds);
        Position = world.MoveGrounded(Position,delta,Radius,BodyHeight); Ticks++;
    }

    private sealed record LadderTarget(LadderDefinition Ladder, bool Top, float Distance);

    private bool TryStand()
    {
        if (State == PlayerTraversalState.Climbing) return false;
        StandBlocked = !world.HasClearance(Position,Radius,StandingHeight);
        State = StandBlocked ? PlayerTraversalState.Crouched : PlayerTraversalState.Standing;
        return !StandBlocked;
    }

    private bool TryBeginClimb(LadderTarget target)
    {
        if (State == PlayerTraversalState.Climbing || !world.CanStand(Position,Radius,StandingHeight)) return false;
        ladder = target.Ladder; State = PlayerTraversalState.Climbing; StandBlocked = false;
        approach.Enqueue(target.Top ? ladder.TopEntry.Vector : ladder.BottomEntry.Vector);
        approach.Enqueue(target.Top ? ladder.Top.Vector : ladder.Bottom.Vector);
        departing = false; return true;
    }

    private void FinishClimb()
    {
        if (State != PlayerTraversalState.Climbing || !world.CanStand(Position,Radius,StandingHeight))
            throw new InvalidOperationException("Validated ladder exit lost support or standing clearance.");
        State = PlayerTraversalState.Standing; StandBlocked = false; ladder = null; departing = false;
    }
    private LadderTarget? FindLadder()
    {
        if (!world.CanStand(Position,Radius,StandingHeight)) return null;
        return scene.Ladders.SelectMany(l => new[] {new LadderTarget(l,false,Vector3.Distance(Position,l.BottomEntry.Vector)),new LadderTarget(l,true,Vector3.Distance(Position,l.TopEntry.Vector))})
            .Where(t => t.Distance <= .55f && Math.Abs(Position.Y-(t.Top?t.Ladder.TopEntry.Y:t.Ladder.BottomEntry.Y)) <= BodyCollisionWorld.Epsilon)
            .Where(t =>
            {
                var entry = t.Top ? t.Ladder.TopEntry.Vector : t.Ladder.BottomEntry.Vector;
                // Convex full support in one box ensures the capture does not cross an empty gap.
                bool support = scene.AllSolids.Any(b => Math.Abs(b.Max.Y-Position.Y)<=BodyCollisionWorld.Epsilon &&
                    new BodyCollisionWorld([b]).HasSupport(Position,Radius) && new BodyCollisionWorld([b]).HasSupport(entry,Radius));
                return support && world.IsSegmentClear(Position,entry,Radius,StandingHeight);
            }).OrderBy(t => t.Distance).ThenBy(t => t.Ladder.Id,StringComparer.Ordinal).FirstOrDefault();
    }

    private void Climb(float direction)
    {
        if (State != PlayerTraversalState.Climbing || ladder is null) throw new InvalidOperationException("Climbing requires a captured ladder.");
        float remaining = ClimbSpeed * (float)StepSeconds;
        while (approach.Count > 0)
        {
            var target = approach.Peek(); float distance = Vector3.Distance(Position,target);
            if (distance > remaining)
            { Position = Vector3.Lerp(Position,target,remaining/distance); return; }
            Position = target; remaining -= distance; approach.Dequeue();
            if (remaining <= 0) return;
        }
        if (departing)
        {
            FinishClimb(); return;
        }
        if (direction == 0) return;
        float y = Math.Clamp(Position.Y + Math.Clamp(direction,-1,1)*remaining,ladder.Bottom.Y,ladder.Top.Y);
        Position = new(Position.X,y,Position.Z);
        if (direction > 0 && y == ladder.Top.Y || direction < 0 && y == ladder.Bottom.Y)
        {
            approach.Enqueue(direction > 0 ? ladder.TopExit.Vector : ladder.BottomExit.Vector);
            departing = true;
        }
    }
}
