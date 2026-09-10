# Dremma native runtime contract

Date: 2026-09-10. This document records the scale and resource-ownership contract
implemented by slice 2 of [the Dremma integration specification](dremma-game-integration-spec.md).

## Physical scale

The playable body uses a `0.45` metre radius, `1.8` metre standing height, and
`0.9` metre crouched height. Courtyard and Sluice positions, sizes, portal and spawn
points, and recovery thresholds are baked at `2.25` times their former world
dimensions. Geometry IDs, references, and topology remain stable.

Camera offset, aim height, edge inset, wheel step, and orthographic range scale by
the same factor. The camera defaults to `11.25` with limits `10.125..15.75`, and
sprite pixels per world unit are `60 / 2.25`. Traversal remains `3` standing,
`1.5` crouched, and `1` climbing metre per second. Fixed-step timing, gravity,
input, state transitions, animation timing, and party spacing `0.7/1.4` are
unchanged, so the larger physical routes intentionally take longer.

## Authored native visuals

Native scene visuals are authored as ordinary Stride `ModelComponent` entities plus
a `NativeVisualComponent`. A binding can name a gameplay geometry GUID, mark the
model as its visible replacement, and select one or more skeleton node names. The
editor preview processor builds the same non-owning subset model shown at runtime.
It keeps the source model's shared materials and the component's local material
overrides. Replacement bindings suppress the matching procedural collision preview.

One geometry may have multiple linked model components. Occlusion aggregates all
linked geometry groups for each model, so entity order cannot change visibility.
Selectors use skeleton node names and keep the full source skeleton so ancestor
transforms remain available. Missing nodes, empty selections, invalid material
indices, and unsupported transforms are candidate-preparation errors.

## Resource ownership

Core remains free of Stride, graphics, and content URLs. Core loading converts a
native scene into traversal data and unloads that temporary content. Windows
presentation separately prepares a candidate-owned native visual lease before
activation. The lease retains its authored scenes and shared model resources while
the presentation is active.

Scene retirement removes the representation before unloading the lease's content in
reverse order. Subset meshes borrow source draw buffers, so the presentation never
disposes those buffers directly. Failed preparation disposes the candidate lease and
leaves the active scene and its lease unchanged.
