using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Windows.Threading;
using Stride.Assets.Entities;
using Stride.Assets.Presentation.AssetEditors.EntityHierarchyEditor.ViewModels;
using Stride.Assets.Presentation.AssetEditors.GameEditor.Services;
using Stride.Core;
using Stride.Core.Assets.Editor.Services;
using Stride.Core.Assets.Editor.ViewModel;
using Stride.Core.Mathematics;
using Stride.Core.Presentation.Services;
using Stride.Core.Quantum;
using Stride.Editor.EditorGame.Game;
using Stride.Engine;
using Stride.Graphics;

namespace Rat.StrideMcp.Adapter;

internal sealed partial class EditorBridge
{
    private readonly SessionViewModel session;
    private readonly Dispatcher dispatcher;
    private readonly IUndoRedoService undo;
    private readonly Dictionary<string,Task<bool>> operations=[];
    private long revision;
    private Task<bool>? activeOperation;
    public string SessionId {get;}=Guid.NewGuid().ToString();
    public string ProjectId {get;}
    private static readonly JsonSerializerOptions Json=new(){IncludeFields=true};
    public EditorBridge(SessionViewModel session,Dispatcher dispatcher)
    {
        this.session=session;this.dispatcher=dispatcher;undo=session.ServiceProvider.Get<IUndoRedoService>();
        ProjectId=Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(Path.GetFullPath(session.SessionFilePath.ToString()).ToUpperInvariant())));
        undo.Done+=(_,_)=>Interlocked.Increment(ref revision);
        undo.Undone+=(_,_)=>Interlocked.Increment(ref revision);
        undo.Redone+=(_,_)=>Interlocked.Increment(ref revision);
        undo.Aborted+=(_,_)=>Interlocked.Increment(ref revision);
        undo.Cleared+=(_,_)=>Interlocked.Increment(ref revision);
        undo.TransactionDiscarded+=(_,_)=>Interlocked.Increment(ref revision);
        // Includes asset reloads/external edits, even if they didn't use this bridge.
        session.AssetPropertiesChanged+=(_,_)=>Interlocked.Increment(ref revision);
    }
    public async Task<JsonObject> Dispatch(JsonObject request,CancellationToken cancellation)
    {
        return await UiRequestQueue.Run(dispatcher,()=>{
            cancellation.ThrowIfCancellationRequested();CheckNativeSession(false);CheckAddress(request["address"]!.AsObject());
            return Execute(request,cancellation);
        },cancellation);
    }
    private void CheckAddress(JsonObject address)
    {
        if(address["processId"]?.GetValue<int>()!=Environment.ProcessId||address["sessionId"]?.GetValue<string>()!=SessionId||address["projectId"]?.GetValue<string>()!=ProjectId)
            throw new InvalidOperationException("Wrong editor process/project/session identity.");
    }
    private void Guard(JsonObject args,CancellationToken cancellation)
    {
        CheckNativeSession(true);
        if(activeOperation is {IsCompleted:false})throw new InvalidOperationException("A session operation is still running; inspect operation/status first.");
        if(session.AllAssets.Where(a=>a.Asset is SceneAsset).Any(a=>((SceneAsset)a.Asset).Hierarchy.Parts.Values.Any(p=>p.Entity.Components.Any(c=>c is Stride.Core.Yaml.IUnloadable))))
            throw new InvalidOperationException("Session contains unloadable scene components. Fix restore/build before editing or saving.");
        // Commit pending native control edits before checking the caller's revision.
        // A pending manual value therefore conflicts, instead of overwriting our change later.
        session.ServiceProvider.Get<IEditorDialogService>().ClearKeyboardFocus();
        cancellation.ThrowIfCancellationRequested();
        CheckNativeSession(true);
        if(undo.HasFailedTransaction)throw new InvalidOperationException("Native rollback failed; session state is uncertain. Reload the project before editing.");
        if(undo.TransactionInProgress||undo.UndoRedoInProgress)throw new InvalidOperationException("Native editor transaction is in progress.");
        if(args["expectedRevision"]?.GetValue<long>()!=Interlocked.Read(ref revision))throw new InvalidOperationException("Stale session revision; refresh status and inspect before retrying.");
    }
    private void CheckNativeSession(bool requireIdle)
    {
        dispatcher.VerifyAccess();
        if(session.IsSessionDisposed||session.IsClosing)
            throw new InvalidOperationException("Native editor session is closing or disposed.");
        if(requireIdle&&session.IsSaving)
            throw new InvalidOperationException("Native session save is running; wait for completion before editing.");
        if(requireIdle&&session.IsAssetOperationInProgress)
            throw new InvalidOperationException("Native asset source update is running; wait for completion before editing.");
    }
    private object Status()=>new {processId=Environment.ProcessId,projectId=ProjectId,sessionId=SessionId,projectPath=session.SessionFilePath.ToString(),revision=Interlocked.Read(ref revision),
        scope="session",stride="4.4.0-dev",busy=session.IsSaving||session.IsAssetOperationInProgress||undo.HasFailedTransaction||activeOperation is {IsCompleted:false},session.IsSaving,session.IsClosing,session.IsSessionDisposed,session.IsAssetOperationInProgress,undo.TransactionInProgress,undo.UndoRedoInProgress,undo.HasFailedTransaction,
        undoTransactionId=session.ActionHistory.Transactions.LastOrDefault(t=>t.IsDone)?.Id.ToString(),
        redoTransactionId=session.ActionHistory.Transactions.FirstOrDefault(t=>!t.IsDone)?.Id.ToString(),
        hookCleared=Environment.GetEnvironmentVariable("DOTNET_STARTUP_HOOKS") is null,
        dirtyAssets=session.AllAssets.Where(a=>a.IsDirty).Select(a=>new{id=a.Id.ToString(),url=a.Url}).ToArray()};
    private AssetViewModel Scene(JsonObject args)
    {
        string id=Guid.Parse(args["sceneId"]!.GetValue<string>()).ToString();
        return session.AllAssets.SingleOrDefault(a=>a.Id.ToString()==id&&a.Asset is SceneAsset)
            ??throw new InvalidOperationException("Scene native id is not present in this session.");
    }
    private Entity Entity(AssetViewModel asset,JsonObject args)
    {
        var id=Guid.Parse(args["entityId"]!.GetValue<string>());
        return ((SceneAsset)asset.Asset).Hierarchy.Parts.TryGetValue(id,out var part)?part.Entity:throw new InvalidOperationException("Entity native id is absent from the addressed scene.");
    }
    private static object Describe(Entity entity)=>new {id=entity.Id,name=entity.Name,parentId=entity.Transform.Parent?.Entity.Id,
        components=entity.Components.Select(c=>new {id=c.Id,type=c.GetType().FullName,
            properties=c is TransformComponent t?new Dictionary<string,object?>{{"Position",t.Position},{"Rotation",t.Rotation},{"Scale",t.Scale}}:
                c.GetType().GetProperties().Where(p=>p.GetCustomAttribute<DataMemberAttribute>() is not null&&Simple(p.PropertyType)).ToDictionary(p=>p.Name,p=>p.GetValue(c))}).ToArray()};
    private static bool Simple(Type type)=>type==typeof(string)||type==typeof(bool)||type==typeof(float)||type==typeof(double)||type==typeof(int)||type.IsEnum||type==typeof(Vector3)||type==typeof(Quaternion);
    private object Change(AssetViewModel asset,JsonObject args)
    {
        if(!asset.IsEditable)throw new InvalidOperationException("Asset is read-only.");
        var entity=Entity(asset,args);var componentId=Guid.Parse(args["componentId"]!.GetValue<string>());
        var component=entity.Components.SingleOrDefault(c=>c.Id==componentId)??throw new InvalidOperationException("Component native id not found on entity.");
        string property=args["property"]!.GetValue<string>();
        if(property.Length>100||property.Contains('.'))throw new InvalidOperationException("Use one serialized property, not an arbitrary member path.");
        if(component is TransformComponent)
        {if(property is not ("Position" or "Rotation" or "Scale"))throw new InvalidOperationException("Only Position/Rotation/Scale transforms are editable.");}
        else if(component.GetType().GetProperty(property)?.GetCustomAttribute<DataMemberAttribute>() is null)
            throw new InvalidOperationException("Custom property must explicitly declare DataMember.");
        // Adapted from AkerMCP SceneBridge: mutate the ASSET Quantum graph, never
        // the runtime preview entity. This registers dirty state and Undo operations.
        var node=session.AssetNodeContainer.GetOrCreateNode(component);
        var member=node.TryGetChild(property)??throw new InvalidOperationException("Serialized Quantum member is unavailable.");
        if(!Simple(member.Type))throw new InvalidOperationException("Unsupported property type in the first MCP slice.");
        string json=args["value"]?.ToJsonString()??"null";
        if(json.Length>4096)throw new InvalidOperationException("Property value exceeds 4096 characters.");
        if(member.Type==typeof(Vector3)||member.Type==typeof(Quaternion))
        {
            var fields=args["value"] as JsonObject??throw new InvalidOperationException("Vector/quaternion requires an object.");
            string[] required=member.Type==typeof(Vector3)?["X","Y","Z"]:["X","Y","Z","W"];
            if(fields.Count!=required.Length||required.Any(name=>!fields.ContainsKey(name)))
                throw new InvalidOperationException("Provide every exact vector/quaternion coordinate, without extra fields.");
        }
        object? value=JsonSerializer.Deserialize(json,member.Type,Json);
        if(value is null)throw new InvalidOperationException("Null property value is not supported.");
        if(member.Type.IsEnum&&!Enum.IsDefined(member.Type,value))throw new InvalidOperationException("Undefined enum value.");
        if(value is float f&&!float.IsFinite(f)||value is double d&&!double.IsFinite(d))throw new InvalidOperationException("Numeric value must be finite.");
        if(value is Vector3 v&&(!float.IsFinite(v.X)||!float.IsFinite(v.Y)||!float.IsFinite(v.Z)))throw new InvalidOperationException("Vector must be finite.");
        if(value is Quaternion q&&(!float.IsFinite(q.X)||!float.IsFinite(q.Y)||!float.IsFinite(q.Z)||!float.IsFinite(q.W)))throw new InvalidOperationException("Quaternion must be finite.");
        using(var transaction=undo.CreateTransaction())
        {member.Update(value);undo.SetName(transaction,$"MCP: {asset.Name}/{entity.Name}.{property}");}
        return new {sceneId=asset.Id.ToString(),entityId=entity.Id,componentId,property,value=member.Retrieve(),dirty=asset.IsDirty};
    }
    private JsonObject StartOperation(Task<bool> task,string kind)
    {
        if(operations.Count>=16)foreach(var key in operations.Where(p=>p.Value.IsCompleted).Select(p=>p.Key).Take(8).ToArray())operations.Remove(key);
        string id=Guid.NewGuid().ToString();operations.Add(id,task);activeOperation=task;
        return Wire.Object(new {ok=true,kind,operationId=id,state="started",scope="session",cancellableAfterStart=false});
    }
    private object Diagnostics()
    {
        var messages=session.AssetLog.Messages.ToArray();
        var problems=messages.Where(m=>m.Type>=Stride.Core.Diagnostics.LogMessageType.Warning).ToArray();
        return new {status=Status(),errorCount=messages.Count(m=>m.Type>=Stride.Core.Diagnostics.LogMessageType.Error),
            warningCount=messages.Count(m=>m.Type==Stride.Core.Diagnostics.LogMessageType.Warning),
            errorsAndWarnings=problems.TakeLast(100).Select(m=>new {level=m.Type.ToString(),text=m.ToString()}).ToArray(),
            recentAssetLog=messages.TakeLast(20).Select(m=>new {level=m.Type.ToString(),text=m.ToString()}).ToArray(),
            capabilities=new[]{"native-scene-inspect","quantum-properties","session-undo-redo-save","viewport-backbuffer","local-resource-catalog","allowlisted-resource-fields","typed-native-references"},
            unsupportedResourceOperations=new[]{"import","reimport","rename","delete","prefab-placement"},captureFallback="none",historyScope="entire session"};
    }
    private async Task<JsonObject> Execute(JsonObject request,CancellationToken cancellation)
    {
        var args=request["arguments"]?.AsObject()??new();string command=request["command"]!.GetValue<string>();object? data;
        if(command is not ("status" or "operation"))CheckNativeSession(true);
        var manager=session.ServiceProvider.Get<IAssetEditorsManager>();
        switch(command)
        {
            case "status":data=Status();break;
            case "scenes":data=session.AllAssets.Where(a=>a.Asset is SceneAsset).Select(a=>new{id=a.Id.ToString(),url=a.Url,name=a.Name,dirty=a.IsDirty,opened=manager.TryGetAssetEditor<EntityHierarchyEditorViewModel>(a,out _)}).ToArray();break;
            case "inspect":var asset=Scene(args);data=new {sceneId=asset.Id.ToString(),url=asset.Url,entities=((SceneAsset)asset.Asset).Hierarchy.Parts.Values.Select(p=>Describe(p.Entity)).ToArray()};break;
            case "set_property":Guard(args,cancellation);data=Change(Scene(args),args);break;
            case "undo":case "redo":
                Guard(args,cancellation);
                var target=command=="undo"?session.ActionHistory.Transactions.LastOrDefault(t=>t.IsDone):session.ActionHistory.Transactions.FirstOrDefault(t=>!t.IsDone);
                if(target is null||target.Id.ToString()!=args["expectedTransactionId"]?.GetValue<string>())throw new InvalidOperationException("Undo/Redo target transaction changed or is unavailable.");
                if(command=="undo")undo.Undo();else undo.Redo();session.CheckConsistency();data=new {scope="session",transactionId=target.Id};break;
            case "save_session":Guard(args,cancellation);return StartOperation(session.SaveSession(),"save_session");
            case "open_scene":
                if(activeOperation is {IsCompleted:false})throw new InvalidOperationException("A session operation is still running.");
                cancellation.ThrowIfCancellationRequested();var opening=Scene(args);
                async Task<bool> Open(){await manager.OpenAssetEditorWindow(opening);return manager.TryGetAssetEditor<EntityHierarchyEditorViewModel>(opening,out _);}
                return StartOperation(Open(),"open_scene");
            case "close_scene":Guard(args,cancellation);var closing=Scene(args);if(closing.IsDirty)throw new InvalidOperationException("Save the dirty scene before closing it.");data=new {closed=manager.CloseAssetEditorWindow(closing,false)};break;
            case "operation":
                if(!operations.TryGetValue(args["operationId"]!.GetValue<string>(),out var task))throw new InvalidOperationException("Unknown or expired operation id.");
                data=new {state=task.IsCompleted?"completed":"running",success=task.IsCompletedSuccessfully&&task.Result,error=task.Exception?.GetBaseException().Message};break;
            case "capture":data=await Capture(Scene(args),manager,cancellation);break;
            case "diagnostics":data=Diagnostics();break;
            case "assets":data=AssetCatalog();break;
            case "asset_inspect":data=InspectAsset(OwnedAsset(args));break;
            case "asset_set":Guard(args,cancellation);data=SetAssetField(OwnedAsset(args),args);break;
            case "asset_reference":Guard(args,cancellation);data=SetAssetReference(OwnedAsset(args),args);break;
            default:throw new InvalidOperationException("Unsupported editor command.");
        }
        return new JsonObject{{"ok",true},{"revision",Interlocked.Read(ref revision)},{"data",JsonSerializer.SerializeToNode(data,Json)}};
    }
    private async Task<object> Capture(AssetViewModel asset,IAssetEditorsManager manager,CancellationToken cancellation)
    {
        if(!manager.TryGetAssetEditor<EntityHierarchyEditorViewModel>(asset,out var editor))throw new InvalidOperationException("Open the addressed scene before capture.");
        object Get(object instance,string name)
        {
            for(var type=instance.GetType();type is not null;type=type.BaseType)
            {var member=type.GetProperty(name,BindingFlags.Instance|BindingFlags.Public|BindingFlags.NonPublic|BindingFlags.DeclaredOnly);if(member is not null)return member.GetValue(instance)!;}
            throw new NotSupportedException($"Pinned editor property {name} unavailable.");
        }
        var controller=(IEditorGameController)Get(editor,"Controller");var game=(EditorServiceGame)Get(controller,"Game");
        return await controller.InvokeTask(async()=>{
            cancellation.ThrowIfCancellationRequested();
            if(game.IsEditorHidden||game.Faulted)throw new InvalidOperationException("Addressed viewport is hidden/faulted; no desktop fallback.");
            await game.Script.NextFrame();cancellation.ThrowIfCancellationRequested();
            using var memory=new MemoryStream();var backbuffer=game.GraphicsDevice.Presenter.BackBuffer;
            backbuffer.Save(game.GraphicsContext.CommandList,memory,ImageFileType.Png);
            return (object)new {sceneId=asset.Id.ToString(),source="editor-game-backbuffer",width=backbuffer.Width,height=backbuffer.Height,pngBase64=Convert.ToBase64String(memory.ToArray())};
        },cancellation).WaitAsync(cancellation);
    }
}
