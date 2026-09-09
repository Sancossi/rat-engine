using System.Reflection;

// Registration route adapted from AkerMCP's Apache-2.0 startup hook; see ../NOTICE.
// .NET requires this exact type name in the global namespace.
internal static class StartupHook
{
    private static string directory="",connection="",pipe="";
    private static int registered;
    public static void Initialize()
    {
        if(!string.Equals(Path.GetFileNameWithoutExtension(Environment.ProcessPath),"Stride.GameStudio",StringComparison.OrdinalIgnoreCase))return;
        connection=Environment.GetEnvironmentVariable("RAT_MCP_CONNECTION")??"";
        pipe=Environment.GetEnvironmentVariable("RAT_MCP_PIPE")??"";
        // Clear before host Main: compiler, F5 game and restarted unrelated processes
        // cannot inherit our startup hook or connection configuration.
        Environment.SetEnvironmentVariable("DOTNET_STARTUP_HOOKS",null);
        Environment.SetEnvironmentVariable("RAT_MCP_CONNECTION",null);
        Environment.SetEnvironmentVariable("RAT_MCP_PIPE",null);
        if(!Path.IsPathFullyQualified(connection)||!pipe.StartsWith("rat-stride-mcp-",StringComparison.Ordinal))return;
        directory=Path.GetDirectoryName(typeof(StartupHook).Assembly.Location)!;
        AppDomain.CurrentDomain.AssemblyResolve+=(_,args)=>{
            var name=new AssemblyName(args.Name).Name;
            if(name is not ("Rat.StrideMcp.Adapter"))return null;
            return Assembly.LoadFrom(Path.Combine(directory,name+".dll"));
        };
        AppDomain.CurrentDomain.AssemblyLoad+=(_,args)=>Register(args.LoadedAssembly);
        foreach(var assembly in AppDomain.CurrentDomain.GetAssemblies())Register(assembly);
    }
    private static void Register(Assembly assembly)
    {
        if(assembly.GetName().Name!="Stride.Core.Assets.Editor"||Interlocked.Exchange(ref registered,1)!=0)return;
        try
        {
            var adapter=Assembly.LoadFrom(Path.Combine(directory,"Rat.StrideMcp.Adapter.dll"));
            adapter.GetType("Rat.StrideMcp.Adapter.Bootstrap",true)!.GetMethod("Register")!.Invoke(null,[connection,pipe]);
        }
        catch(Exception error){File.AppendAllText(connection+".hook.log",error+Environment.NewLine);}
    }
}
