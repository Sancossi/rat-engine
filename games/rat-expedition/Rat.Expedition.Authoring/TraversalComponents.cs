using Rat.Expedition.Core;
using Stride.Core;
using Stride.Core.Mathematics;
using Stride.Core.Serialization;
using Stride.Engine;
using Stride.Engine.Design;

namespace Rat.Expedition.Authoring;

public enum GeometryRole { Floor, Wall, Structure, Decoration, Ramp }

[DataContract("ExpeditionProject")]
[Display("Expedition project")]
public sealed class TraversalProjectComponent : EntityComponent
{
    [DataMember(10)] public UrlReference<Scene>? StartScene {get;set;}
    [DataMember(20)] public List<UrlReference<Scene>> Scenes {get;set;}=[];
}

[DataContract("ExpeditionGeometry")]
[Display("Expedition geometry")]
[DefaultEntityComponentProcessor(typeof(GeometryPreviewProcessor), ExecutionMode = ExecutionMode.Editor)]
public sealed class GeometryComponent : EntityComponent
{
    [DataMember(10)] public GeometryRole Role {get;set;}=GeometryRole.Structure;
    // Box: centre at entity origin. Ramp: low/start corner at entity origin;
    // Size.X/Z span, Size.Y slab thickness, Rise may ascend or descend.
    [DataMember(20)] public Vector3 Size {get;set;}=Vector3.One;
    [DataMember(30)] public RampAxis Axis {get;set;}=RampAxis.X;
    [DataMember(40)] public float Rise {get;set;}=1;
}

[DataContract("ExpeditionScene")]
[Display("Expedition scene")]
public sealed class TraversalSceneComponent : EntityComponent
{
    [DataMember(10)] public Entity? DefaultSpawn {get;set;}
    [DataMember(20)] public float RecoveryThreshold {get;set;}=-2;
}

[DataContract("ExpeditionSpawn")]
[Display("Expedition spawn")]
public sealed class SpawnComponent : EntityComponent { }

[DataContract("ExpeditionLadder")]
[Display("Expedition ladder")]
public sealed class LadderComponent : EntityComponent
{
    [DataMember(10)] public Entity? Bottom {get;set;}
    [DataMember(20)] public Entity? Top {get;set;}
    [DataMember(30)] public Entity? BottomEntry {get;set;}
    [DataMember(40)] public Entity? TopEntry {get;set;}
    [DataMember(50)] public Entity? BottomExit {get;set;}
    [DataMember(60)] public Entity? TopExit {get;set;}
}

[DataContract("ExpeditionPortal")]
[Display("Expedition portal")]
public sealed class PortalComponent : EntityComponent
{
    // Entity origin is the safe interaction anchor. Bounds are centred on it.
    [DataMember(10)] public Vector3 Size {get;set;}=new(1,.1f,1);
    [DataMember(20)] public UrlReference<Scene>? TargetScene {get;set;}
    // Empty uses the target scene's explicit DefaultSpawn. Other spawns use native
    // entity GUID, which survives rename; the adapter validates it in the target.
    [DataMember(30)] public Guid TargetSpawnId {get;set;}
}

[DataContract("ExpeditionOcclusion")]
[Display("Expedition occlusion")]
public sealed class OcclusionComponent : EntityComponent
{
    [DataMember(10)] public List<Entity> Members {get;set;}=[];
    [DataMember(20)] public Entity? HideWith {get;set;}
}
