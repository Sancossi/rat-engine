using System.Numerics;

namespace Rat.Expedition.Core;

public sealed class TraversalMotor
{
    public const float Radius = 0.45f, StandingHeight = 1.8f, CrouchedHeight = .9f;
    public const float Speed = 3, CrouchSpeed = 1.5f, ClimbSpeed = 1;
    public const double StepSeconds = 1.0 / 120;
    public const int MaximumCatchUpSteps = 12;
    public static readonly Vector2 CameraRight = Vector2.Normalize(new(1, -1));
    public static readonly Vector2 CameraForward = Vector2.Normalize(new(-1, -1));
    private readonly SceneDefinition scene;
    private readonly LayeredCollisionWorld world;
    private double accumulator;
    private bool interactWasHeld, pendingInteract;
    private LadderDefinition? ladder;
    private readonly Queue<Vector3> approach = new();
    private bool capturingEntry, departing;
    private Vector3 position;
    internal List<Vector3> TickPath {get;}=[];
    public Vector3 Position { get=>position; private set {position=value;TickPath.Add(value);} }
    public long Ticks { get; private set; }
    public PlayerTraversalState State { get; private set; } = PlayerTraversalState.Standing;
    public BodyStance Stance => State is PlayerTraversalState.Crouched or PlayerTraversalState.FallingCrouched ? BodyStance.Crouched : BodyStance.Standing;
    public TraversalMode Mode => State == PlayerTraversalState.Climbing ? TraversalMode.Climbing : State is PlayerTraversalState.FallingStanding or PlayerTraversalState.FallingCrouched ? TraversalMode.Falling : TraversalMode.Grounded;
    public float BodyHeight => Stance == BodyStance.Standing ? StandingHeight : CrouchedHeight;
    public bool StandBlocked { get; private set; }
    public float VerticalVelocity {get; private set;}
    public TraversalContext? Context => Mode==TraversalMode.Grounded ? FindContext() : null;
    internal bool StandingSafe=>State==PlayerTraversalState.Standing&&CanStand(Position);

    public TraversalMotor(SceneDefinition scene)
    {
        scene.Validate(); this.scene = scene;
        world = scene.CreateWorld(); Position = scene.Spawn.Vector;
    }

    internal TraversalMotor(SceneDefinition scene,Point3 spawn):this(scene)
    {
        if(!scene.IsFree(spawn.Vector))throw new InvalidDataException("Motor entry needs standing-safe support.");
        Position=spawn.Vector;
    }

    public TraversalSnapshot Snapshot
    {
        get
        {
            var context = Context;
            string hint = ladder is not null ? "W / S — вверх / вниз" : Mode==TraversalMode.Falling ? "Падение" : StandBlocked ? "Здесь нельзя встать" :
                context is not null ? context.Kind==ContextKind.Portal ? "E / Enter — перейти" : "E / Enter — лестница" : "Ctrl — присесть";
            return new(scene.Id,Position,State,Stance,Mode,BodyHeight,StandBlocked,ladder?.Id ?? context?.Id,hint);
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
            bool interact=pendingInteract; pendingInteract=false;
            Tick(input,interact); accumulator -= StepSeconds;
            // A climb-direction input belongs to this action until the exit snapshot is emitted.
            if(wasClimbing && State != PlayerTraversalState.Climbing) {accumulator=0;break;}
        }
    }

    // Session owns the clock and action edge. Standalone Advance remains a fixture helper.
    internal PortalDefinition? Tick(TraversalInput input,bool interact)
    {
        TickPath.Clear();TickPath.Add(Position);
        Ticks++;
        if (State == PlayerTraversalState.Climbing) { Climb(input.Move.Y); return null; }
        if (Mode==TraversalMode.Falling) {MoveAir(HorizontalDelta(input)); Fall(); return null;}
        if (input.CrouchHeld) { State = PlayerTraversalState.Crouched; StandBlocked = false; }
        else TryStand();
        var target = interact ? FindContext() : null;
        if(target?.Kind==ContextKind.Portal)return target.Portal;
        if (target is not null && TryBeginClimb(target)) { Climb(0); return null; }
        var delta=HorizontalDelta(input);
        GroundAxis(new(delta.X,0)); GroundAxis(new(0,delta.Y));
        if(Mode==TraversalMode.Falling)Fall();
        return null;
    }

    private Vector2 HorizontalDelta(TraversalInput input)
    {
        var move = input.Move;
        if (move.LengthSquared() > 1) move = Vector2.Normalize(move);
        return (CameraRight * move.X + CameraForward * move.Y) * ((Stance == BodyStance.Crouched ? CrouchSpeed : Speed) * (float)StepSeconds);
    }

    private void GroundAxis(Vector2 delta)
    {
        if(Mode==TraversalMode.Falling){MoveAir(delta);return;}
        var move=world.MoveSupported(Position,delta,Radius,BodyHeight,path:TickPath); Position=move.Position;
        if(!move.LostSupport)return;
        State=Stance==BodyStance.Crouched?PlayerTraversalState.FallingCrouched:PlayerTraversalState.FallingStanding;
        VerticalVelocity=0; StandBlocked=false;
        MoveAir(delta*(1-move.Fraction));
    }
    private void MoveAir(Vector2 delta)
    {
        foreach(var shift in new[]{new Vector3(delta.X,0,0),new Vector3(0,0,delta.Y)})
        {
            var target=Position+shift;
            Position=Vector3.Lerp(Position,target,world.SweepFraction(Position,target,Radius,BodyHeight));
        }
    }
    private void Fall()
    {
        VerticalVelocity=Math.Max(-12,VerticalVelocity-9.8f*(float)StepSeconds);
        var target=Position+new Vector3(0,VerticalVelocity*(float)StepSeconds,0);
        float fraction=world.SweepFraction(Position,target,Radius,BodyHeight);
        Position=Vector3.Lerp(Position,target,fraction);
        if(fraction<1)
        {
            VerticalVelocity=0;
            if(world.HasSupport(Position,Radius))
            {
                State=Stance==BodyStance.Crouched?PlayerTraversalState.Crouched:PlayerTraversalState.Standing;
                StandBlocked=!world.HasClearance(Position,Radius,StandingHeight);
            }
            // Partial edge contact can hold vertical motion but never disables air steering.
        }
    }

    private bool TryStand()
    {
        if (State == PlayerTraversalState.Climbing) return false;
        StandBlocked = !world.HasClearance(Position,Radius,StandingHeight);
        State = StandBlocked ? PlayerTraversalState.Crouched : PlayerTraversalState.Standing;
        return !StandBlocked;
    }

    private bool TryBeginClimb(TraversalContext target)
    {
        if (Mode!=TraversalMode.Grounded || !CanStand(Position)) return false;
        ladder = target.Ladder!; State = PlayerTraversalState.Climbing; StandBlocked = false;
        approach.Enqueue(target.Top ? ladder.TopEntry.Vector : ladder.BottomEntry.Vector);
        approach.Enqueue(target.Top ? ladder.Top.Vector : ladder.Bottom.Vector);
        capturingEntry = true; departing = false; return true;
    }

    private void FinishClimb()
    {
        if (State != PlayerTraversalState.Climbing || !CanStand(Position))
            throw new InvalidOperationException("Validated ladder exit lost support or standing clearance.");
        State = PlayerTraversalState.Standing; StandBlocked = false; ladder = null; departing = false;
    }
    private bool CanStand(Vector3 p)=>world.HasSupport(p,Radius)&&world.HasClearance(p,Radius,StandingHeight);
    private bool CanReach(Vector3 entry)
    {
        var move=world.MoveSupported(Position,new(entry.X-Position.X,entry.Z-Position.Z),Radius,StandingHeight);
        return !move.Blocked&&!move.LostSupport&&Vector3.Distance(move.Position,entry)<=BodyCollisionWorld.Epsilon;
    }
    private TraversalContext? FindContext()
    {
        if(!CanStand(Position))return null;
        var targets=new List<TraversalContext>();
        foreach(var l in scene.Ladders)foreach(bool top in new[]{false,true})
        {
            var entry=top?l.TopEntry:l.BottomEntry; float distance=Vector3.Distance(Position,entry.Vector);
            if(distance<=.55f&&CanReach(entry.Vector))targets.Add(new(l.Id,ContextKind.Ladder,distance,l,top,null));
        }
        foreach(var portal in scene.Portals)
            if(portal.Contains(Position)&&CanReach(portal.Anchor.Vector))
                targets.Add(new(portal.Id,ContextKind.Portal,Vector3.Distance(Position,portal.Anchor.Vector),null,false,portal));
        return targets.OrderBy(t=>t.Distance).ThenBy(t=>t.Id,StringComparer.Ordinal).FirstOrDefault();
    }

    private void Climb(float direction)
    {
        if (State != PlayerTraversalState.Climbing || ladder is null) throw new InvalidOperationException("Climbing requires a captured ladder.");
        float remaining = ClimbSpeed * (float)StepSeconds;
        if (capturingEntry)
        {
            var entry=approach.Peek();
            bool moved=Position!=entry;
            var move=world.MoveSupported(Position,new(entry.X-Position.X,entry.Z-Position.Z),
                Radius,StandingHeight,remaining,TickPath);
            if(move.Blocked || move.LostSupport)
                throw new InvalidOperationException("Validated ladder entry approach lost its supported path.");
            Position=move.Position;
            if(move.Fraction<1) return;
            approach.Dequeue(); capturingEntry=false;
            // Expose the grounded entry boundary before entering the separately validated
            // ladder corridor. Never cut the ramp crest with a player-to-entry chord.
            if(moved) return;
        }
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
