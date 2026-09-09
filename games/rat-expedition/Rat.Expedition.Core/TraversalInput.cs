using System.Numerics;

namespace Rat.Expedition.Core;

public readonly record struct TraversalInput(Vector2 Move, bool CrouchHeld = false, bool InteractHeld = false);
public enum BodyStance { Standing, Crouched }
public enum TraversalMode { Grounded, Climbing, Falling }
public enum PlayerTraversalState { Standing, Crouched, Climbing, FallingStanding, FallingCrouched }
public enum ContextKind { Ladder, Portal }
public sealed record TraversalContext(string Id,ContextKind Kind,float Distance,LadderDefinition? Ladder,bool Top,PortalDefinition? Portal);
public sealed record TraversalSnapshot(string SceneId, Vector3 Position, PlayerTraversalState State, BodyStance Stance, TraversalMode Mode,
    float BodyHeight, bool StandBlocked, string? ContextId, string Hint);
