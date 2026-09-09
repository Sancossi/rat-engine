using Stride.Core;
using Stride.Engine;

namespace Rat.Expedition.Authoring;

// Native scene/prefab metadata. Core receives validated plain values through
// the future A1.2 adapter; it never references this Stride assembly.
[DataContract("ExpeditionIdentity")]
[Display("Expedition identity")]
public sealed class ExpeditionIdentityComponent : EntityComponent
{
    [DataMember(10)] public string GameId { get; set; } = "";
    [DataMember(20)] public string DisplayLabel { get; set; } = "";
}
