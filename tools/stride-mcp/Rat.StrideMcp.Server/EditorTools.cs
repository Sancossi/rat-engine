using System.ComponentModel;
using System.Text.Json;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;

namespace Rat.StrideMcp.Server;

[McpServerToolType]
public sealed class EditorTools(BridgeClient bridge)
{
    [McpServerTool(Name="editor_status"), Description("Inspect the explicitly selected editor process, project, session revision and entire-session Undo/Redo targets.")]
    public Task<CallToolResult> Status(CancellationToken cancellationToken) => bridge.Call("status",new {},cancellationToken);
    [McpServerTool(Name="scene_list"), Description("List native scene asset IDs in the selected project.")]
    public Task<CallToolResult> Scenes(CancellationToken cancellationToken) => bridge.Call("scenes",new {},cancellationToken);
    [McpServerTool(Name="scene_inspect"), Description("Inspect exact scene native ID, entities, component IDs and editable serialized properties.")]
    public Task<CallToolResult> Inspect(string sceneId,CancellationToken cancellationToken) => bridge.Call("inspect",new {sceneId},cancellationToken);
    [McpServerTool(Name="scene_open"), Description("Open exact native scene. Returns operation ID; poll editor_operation for completion.")]
    public Task<CallToolResult> Open(string sceneId,CancellationToken cancellationToken) => bridge.Call("open_scene",new {sceneId},cancellationToken);
    [McpServerTool(Name="scene_close"), Description("Close a saved scene by exact ID; refuses dirty scenes and stale session revision.")]
    public Task<CallToolResult> Close(string sceneId,long expectedRevision,CancellationToken cancellationToken) => bridge.Call("close_scene",new {sceneId,expectedRevision},cancellationToken);
    [McpServerTool(Name="entity_set_property"), Description("Edit one serialized property through native asset Quantum/Undo. Exact scene/entity/component GUIDs and current session revision required. Vector fields use X/Y/Z; quaternion adds W.")]
    public Task<CallToolResult> Set(string sceneId,string entityId,string componentId,string property,JsonElement value,long expectedRevision,CancellationToken cancellationToken)
        => bridge.Call("set_property",new {sceneId,entityId,componentId,property,value,expectedRevision},cancellationToken);
    [McpServerTool(Name="editor_undo"), Description("Undo the next ENTIRE SESSION transaction only if its ID and revision match. May affect another scene.")]
    public Task<CallToolResult> Undo(long expectedRevision,string expectedTransactionId,CancellationToken cancellationToken) => bridge.Call("undo",new {expectedRevision,expectedTransactionId},cancellationToken);
    [McpServerTool(Name="editor_redo"), Description("Redo the next ENTIRE SESSION transaction only if its ID and revision match.")]
    public Task<CallToolResult> Redo(long expectedRevision,string expectedTransactionId,CancellationToken cancellationToken) => bridge.Call("redo",new {expectedRevision,expectedTransactionId},cancellationToken);
    [McpServerTool(Name="save_session"), Description("Save ALL dirty assets in the selected session using native save. Requires revision. Returns operation ID; started native save cannot be cancelled. Poll editor_operation.")]
    public Task<CallToolResult> Save(long expectedRevision,CancellationToken cancellationToken) => bridge.Call("save_session",new {expectedRevision},cancellationToken);
    [McpServerTool(Name="editor_operation"), Description("Poll a native open/save operation; completed success is distinct from started.")]
    public Task<CallToolResult> Operation(string operationId,CancellationToken cancellationToken) => bridge.Call("operation",new {operationId},cancellationToken);
    [McpServerTool(Name="viewport_capture"), Description("Capture the addressed open visible scene's actual GPU backbuffer. No desktop fallback; hidden or faulted viewport returns error.")]
    public Task<CallToolResult> Capture(string sceneId,CancellationToken cancellationToken) => bridge.Call("capture",new {sceneId},cancellationToken);
    [McpServerTool(Name="editor_diagnostics"), Description("Read bridge capabilities and selected editor session diagnostics.")]
    public Task<CallToolResult> Diagnostics(CancellationToken cancellationToken) => bridge.Call("diagnostics",new {},cancellationToken);

    [McpServerTool(Name="asset_list"), Description("List supported editable resource asset IDs and explicit package keys within the selected solution. No UI selection or filesystem search.")]
    public Task<CallToolResult> Assets(CancellationToken cancellationToken)=>bridge.Call("assets",new {},cancellationToken);
    [McpServerTool(Name="asset_inspect"), Description("Inspect allowlisted native resource fields, item/entity IDs and dependencies. Does not dump arbitrary CLR graphs.")]
    public Task<CallToolResult> AssetInspect(string assetId,CancellationToken cancellationToken)=>bridge.Call("asset_inspect",new {assetId},cancellationToken);
    [McpServerTool(Name="asset_set_property"), Description("Change one allowlisted native resource property through Quantum Undo. Optional itemIndex selects an inspected sprite; elementId selects a UI TextBlock. Exact expectedRevision is required.")]
    public Task<CallToolResult> AssetSet(string assetId,string property,JsonElement value,long expectedRevision,CancellationToken cancellationToken,int? itemIndex=null,string? elementId=null)
        =>bridge.Call("asset_set",new {assetId,property,value,expectedRevision,itemIndex,elementId},cancellationToken);
    [McpServerTool(Name="asset_set_reference"), Description("Assign an existing same-package asset to an allowlisted typed native Model/Material/UI Page/Font reference. No arbitrary member paths. Optional entity/component, itemIndex or elementId addresses the inspected owner.")]
    public Task<CallToolResult> AssetReference(string assetId,string property,string targetAssetId,long expectedRevision,CancellationToken cancellationToken,string? entityId=null,string? componentId=null,int? itemIndex=null,string? elementId=null)
        =>bridge.Call("asset_reference",new {assetId,property,targetAssetId,expectedRevision,entityId,componentId,itemIndex,elementId},cancellationToken);
}
