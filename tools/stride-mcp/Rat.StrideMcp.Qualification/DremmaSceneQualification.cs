using System.IO;
using System.Text.Json;
using Stride.Assets.Entities;
using Stride.Core.Assets.Editor.ViewModel;

namespace Rat.StrideMcp.Qualification;

// Test-only rendezvous for the external MCP client. The client drives the real
// editor APIs; this hook only records native identities and waits for its result.
internal static class DremmaSceneQualification
{
    private const string SceneId="d740bf16-9a5c-5844-9eb3-6a9fdbef9c7f";
    internal static async Task Run(SessionViewModel session)
    {
        var result=new Dictionary<string,object?>();
        try
        {
            await Task.Delay(1500);
            var asset=session.AllAssets.Single(a=>a.Id.ToString()==SceneId&&a.Asset is SceneAsset);
            var entityIds=((SceneAsset)asset.Asset).Hierarchy.Parts.Values.Select(p=>p.Entity.Id.ToString()).Order().ToArray();
            var ready=StartupHook.ResultPath+".ready.json";
            File.WriteAllText(ready,JsonSerializer.Serialize(new{sceneId=SceneId,entityIds,processId=Environment.ProcessId}));
            var client=StartupHook.ResultPath+".client.json";
            var deadline=DateTime.UtcNow.AddSeconds(90);
            while(!File.Exists(client)&&DateTime.UtcNow<deadline)await Task.Delay(100);
            if(!File.Exists(client))throw new TimeoutException("Dremma scene MCP client exceeded 90 seconds.");
            var clientEvidence=JsonDocument.Parse(File.ReadAllText(client)).RootElement.Clone();
            if(!clientEvidence.GetProperty("passed").GetBoolean())throw new InvalidOperationException("Dremma scene MCP client failed.");
            if(session.AllAssets.Any(a=>a.IsDirty))throw new InvalidOperationException("Dremma editor qualification left dirty assets.");
            result["passed"]=true;result["sceneId"]=SceneId;result["entityIds"]=entityIds;result["mcpClient"]=clientEvidence;
        }
        catch(Exception error){result["passed"]=false;result["error"]=error.ToString();}
        File.WriteAllText(StartupHook.ResultPath,JsonSerializer.Serialize(result,new JsonSerializerOptions{WriteIndented=true}));
    }
}
