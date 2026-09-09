using System.IO;
using System.Reflection;

// Explicit test-only second startup hook. Not present in normal MCP launch.
internal static class StartupHook
{
    internal static string ResultPath="";
    internal static string Mode="session";
    private static int registered;
    public static void Initialize()
    {
        ResultPath=Environment.GetEnvironmentVariable("RAT_MCP_QUALIFICATION_RESULT")??"";
        Environment.SetEnvironmentVariable("RAT_MCP_QUALIFICATION_RESULT",null);
        Mode=Environment.GetEnvironmentVariable("RAT_MCP_QUALIFICATION_MODE")??"session";
        Environment.SetEnvironmentVariable("RAT_MCP_QUALIFICATION_MODE",null);
        if(!Path.IsPathFullyQualified(ResultPath))return;
        if(Mode=="readiness-failure")
        {
            // Deliberately prevent host Main/adapter readiness in this owned test.
            // WorkingDirectory is the explicitly selected solution directory.
            var root=Path.Combine(Environment.CurrentDirectory,"Rat.Expedition.Authoring");
            foreach(var category in new[]{"Assets","Resources"})
            {
                var folder=Path.Combine(root,category,"McpResourceQualification");
                if(Directory.Exists(folder))throw new InvalidOperationException("Refusing existing readiness fixture folder.");
                Directory.CreateDirectory(folder);File.WriteAllText(Path.Combine(folder,"failed-readiness.txt"),"Owned failed-readiness evidence");
            }
            File.WriteAllText(ResultPath+".failure-started.json",System.Text.Json.JsonSerializer.Serialize(new{processId=Environment.ProcessId}));
            Thread.Sleep(15000);
            return;
        }
        AppDomain.CurrentDomain.AssemblyLoad+=(_,eventArgs)=>Register(eventArgs.LoadedAssembly);
        foreach(var assembly in AppDomain.CurrentDomain.GetAssemblies())Register(assembly);
    }
    private static void Register(Assembly assembly)
    {
        if(assembly.GetName().Name!="Stride.Core.Assets.Editor"||Interlocked.Exchange(ref registered,1)!=0)return;
        try{RegisterPlugin();}catch(Exception error){File.WriteAllText(ResultPath,error.ToString());}
    }
    private static void RegisterPlugin()=>Stride.Core.Assets.Editor.Services.AssetsPlugin.RegisterPlugin(typeof(Rat.StrideMcp.Qualification.QualificationPlugin));
}
