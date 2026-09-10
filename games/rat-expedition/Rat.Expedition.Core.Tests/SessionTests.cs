using System.Numerics;
using System.Diagnostics;
using System.Text.Json;
using System.Text.Json.Nodes;
using Rat.Expedition.Core;

internal static class SessionTests
{
    private sealed class Prepared(Action activate,Action dispose):IPreparedSceneChange
    {public void Activate()=>activate();public void Dispose()=>dispose();}
    public static void Run(Action<string,Action> check,Action<bool,string> require)
    {
        const double dt=1.0/60;
        Vector2 Screen(Vector2 world)=>new(Vector2.Dot(world,TraversalMotor.CameraRight),Vector2.Dot(world,TraversalMotor.CameraForward));
        SceneDefinition Scene(string id,string other,float y=0)=>new(3,id,new("floor",new(-3,y-.3f,-3),new(3,y,3)),new(-1,y,0),[],[],[])
        {Portals=[new("portal",new(.6f,y-.05f,-.4f),new(1.4f,y+.05f,.4f),new(1,y,0),other,"entry")]};
        var a=Scene("a","b");var b=Scene("b","a");
        ExpeditionSession Session()=>new(new("a",[a,b]));
        void Frames(ExpeditionSession s,SessionInput input,int count){for(int i=0;i<count;i++)s.Advance(dt,input);}
        void Walk(ExpeditionSession s,Vector2 target)
        {
            for(int i=0;i<600;i++)
            {
                var p=s.Leader.Position;var delta=target-new Vector2(p.X,p.Z);
                if(delta.Length()<.002f)return;
                var world=delta/Math.Max(delta.Length(),TraversalMotor.Speed*(float)dt);
                s.Advance(dt,new(Screen(world)));
            }
            throw new Exception($"Walk could not reach {target}, at {s.Leader.Position}");
        }
        void Press(ExpeditionSession s){s.Advance(dt,new(Vector2.Zero));s.Advance(dt,new(Vector2.Zero,InteractHeld:true));}
        void Reject(Action action){try{action();}catch(InvalidDataException){return;}throw new Exception("Invalid data accepted");}
        void WriteProject(string dir,SceneDefinition first,SceneDefinition second)
        {
            Directory.CreateDirectory(dir);
            File.WriteAllText(Path.Combine(dir,"a.json"),JsonSerializer.Serialize(first));
            File.WriteAllText(Path.Combine(dir,"b.json"),JsonSerializer.Serialize(second));
            File.WriteAllText(Path.Combine(dir,"project.json"),JsonSerializer.Serialize(new ProjectDefinition(1,"a",[new("a","a.json"),new("b","b.json")])));
        }

        check("session performs ten portal round trips and held interaction cannot return",()=>{
            var s=Session();
            for(int cycle=0;cycle<10;cycle++)foreach(string target in new[]{"b","a"})
            {
                Walk(s,new(1,0));Press(s);
                require(s.Scene.Id==target&&s.Leader.State==PlayerTraversalState.Standing,"Wrong portal outcome");
                require(s.SafePoint==s.Leader.Position&&s.Leader.Position==new Vector3(-1,0,0),"Portal did not reset safe spawn");
                require(s.Trail.Companions.All(p=>p.Position==s.Leader.Position),"Portal retained old companion trail");
                Frames(s,new(Vector2.Zero,InteractHeld:true),20);
                require(s.Scene.Id==target,"Held interaction returned immediately");
            }
            require(s.WorldRevision==20,"Not exactly twenty scene changes");
        });
        check("session queues short-frame action but pause and focus clear it",()=>{
            var s=Session();Walk(s,new(1,0));
            s.Advance(TraversalMotor.StepSeconds*.4,new(Vector2.Zero,InteractHeld:true));
            require(s.Scene.Id=="a","Portal acted outside tick");
            s.Advance(TraversalMotor.StepSeconds*.7,new(Vector2.Zero));require(s.Scene.Id=="b","Partial-tick action lost");
            Walk(s,new(1,0));var p=s.Leader.Position;long ticks=s.Ticks;
            s.Advance(0,new(Vector2.Zero,PauseHeld:true));
            Frames(s,new(new(1,1),InteractHeld:true),10);
            // First new Enter resumes but is consumed by the menu, never the portal.
            require(s.Mode==SessionMode.Explore&&s.Scene.Id=="b","Resume Enter leaked into portal");
            s.Advance(0,new(Vector2.Zero),false);ticks=s.Ticks;p=s.Leader.Position;
            Frames(s,new(new(1,1),InteractHeld:true),10);
            require(s.Mode==SessionMode.Paused&&s.Ticks==ticks&&s.Leader.Position==p,"Focus pause auto-resumed or advanced ticks");
            s.Advance(dt,new(Vector2.Zero));s.Advance(dt,new(Vector2.Zero,PauseHeld:true,InteractHeld:true));
            require(s.Mode==SessionMode.Explore&&s.Scene.Id=="b","Explicit focus resume leaked interaction");
            Frames(s,new(Vector2.Zero,InteractHeld:true),10);require(s.Scene.Id=="b","Held Enter activated after resume");
        });
        check("pause flushes a queued partial-tick interaction and consumes resume Enter",()=>{
            var s=Session();Walk(s,new(1,0));long ticks=s.Ticks;var p=s.Leader.Position;
            s.Advance(TraversalMotor.StepSeconds*.4,new(Vector2.Zero,InteractHeld:true));
            s.Advance(0,new(Vector2.Zero,InteractHeld:true,PauseHeld:true));
            Frames(s,new(new(1,0),InteractHeld:true),5);
            require(s.Mode==SessionMode.Paused&&s.Ticks==ticks&&s.Leader.Position==p,"Queued action/motion advanced paused world");
            s.Advance(dt,new(Vector2.Zero));s.Advance(dt,new(Vector2.Zero,InteractHeld:true));
            Frames(s,new(Vector2.Zero,InteractHeld:true),5);
            require(s.Mode==SessionMode.Explore&&s.Scene.Id=="a"&&s.Leader.Position==p,"Queued action leaked through pause/resume");
            Press(s);require(s.Scene.Id=="b","Fresh interaction after resume rejected");
        });
        check("pause stops a captured ladder and resumed held interaction preserves that action",()=>{
            var ladder=new LadderDefinition("ladder",new(0,0,0),new(0,1.6f,0),new(0,0,.5f),new(0,1.6f,-1),new(0,0,.5f),new(0,1.6f,-1));
            var scene=new SceneDefinition(3,"ladder-scene",a.Floor,new(0,0,.5f),[],[new("platform",new(-1,1.4f,-2),new(1,1.6f,-.5f))],[ladder]);
            var s=new ExpeditionSession(new(scene.Id,[scene]));Press(s);Frames(s,new(new(0,1)),60);
            require(s.Leader.Mode==TraversalMode.Climbing,"Not on ladder");
            s.Advance(dt,new(new(0,1),PauseHeld:true));var p=s.Leader.Position;long ticks=s.Ticks;
            Frames(s,new(new(0,1)),30);require(s.Ticks==ticks&&s.Leader.Position==p,"Paused ladder advanced");
            s.Advance(dt,new(new(0,1),InteractHeld:true));
            require(s.Mode==SessionMode.Explore&&s.Leader.Position==p,"Resume itself advanced ladder");
            Frames(s,new(new(0,1),InteractHeld:true),10);
            require(s.Leader.Mode==TraversalMode.Climbing&&s.Leader.Position.Y>p.Y,"Resume recaptured or froze ladder");
        });
        check("failed disk and presentation candidates preserve current scene position and safe point",()=>{
            string dir=Path.Combine(Path.GetTempPath(),"rat-session-"+Guid.NewGuid());
            try
            {
                WriteProject(dir,a,b);var project=ExpeditionProject.Load(dir);
                bool failRender=false;int activated=0,disposed=0;
                var s=new ExpeditionSession(project,c=>{
                    if(failRender&&c.Reason==SceneChangeReason.Portal)throw new InvalidDataException("render candidate failed");
                    return new Prepared(()=>activated++,()=>disposed++);
                });
                Walk(s,new(1,0));var before=s.Snapshot;
                File.WriteAllText(Path.Combine(dir,"b.json"),"{broken");Press(s);
                require(s.Scene.Id=="a"&&s.Leader.Position==before.Leader.Position&&s.SafePoint==before.SafePoint&&s.WorldRevision==0&&s.LastError is not null,"Disk failure replaced active state");
                File.WriteAllText(Path.Combine(dir,"b.json"),JsonSerializer.Serialize(b));failRender=true;Press(s);
                require(s.Scene.Id=="a"&&s.Leader.Position==before.Leader.Position&&s.WorldRevision==0&&activated==1,"Failed renderer replaced state");
                failRender=false;Press(s);
                require(s.Scene.Id=="b"&&activated==2&&disposed==2&&s.LastError is null,"Valid prepared candidate not committed/disposed");
            }
            finally{Directory.Delete(dir,true);}
        });
        check("one shared portal ladder selection filters blocked and upper targets first",()=>{
            var ladder=new LadderDefinition("near-ladder",new(0,0,0),new(0,1.6f,0),new(0,0,.5f),new(0,1.6f,-1),new(0,0,.5f),new(0,1.6f,-1));
            var scene=a with {Spawn=new(2f,0,-1),Walls=[new("bar",new(.45f,0,.35f),new(.46f,2,.65f))],
                Structures=[new("platform",new(-1,1.4f,-2),new(1,1.6f,-.5f)),new("upper",new(.6f,1.8f,0),new(1.6f,2f,1))],Ladders=[ladder],
                Portals=[new("far-portal",new(.7f,-.05f,.3f),new(1.5f,.05f,1.3f),new(1.1f,0,1),"b","entry"),
                    new("upper-portal",new(.7f,1.95f,.3f),new(1.5f,2.05f,.7f),new(1.1f,2f,.5f),"b","entry")]};
            var s=new ExpeditionSession(new("a",[scene,b]));Walk(s,new(1.1f,.5f));
            require(s.Leader.ContextId=="far-portal","Wrong-height/blocked target displaced available portal");
            Press(s);require(s.Scene.Id=="b"&&s.Leader.Mode==TraversalMode.Grounded,"One E activated ladder as well as portal");
        });
        check("crouched step-off preserves short body beneath overhang and air steering",()=>{
            var scene=new SceneDefinition(3,"crouch-fall",a.Floor,new(-1.5f,1,0),[],[
                new("ledge",new(-2,.8f,-1),new(.5f,1,1)),new("roof",new(-.5f,1.9f,-1),new(1.3f,2.1f,1))],[]);
            var m=new TraversalMotor(scene);var move=Screen(new(1,0));
            for(int i=0;i<400&&m.Mode!=TraversalMode.Falling;i++)m.Advance(dt,new(move,true));
            require(m.Mode==TraversalMode.Falling&&m.Stance==BodyStance.Crouched,"Crouched step-off not reached");
            float x=m.Position.X;
            for(int i=0;i<5;i++)
            {
                m.Advance(dt,new(move));
                require(m.BodyHeight==TraversalMotor.CrouchedHeight&&scene.CreateWorld().HasClearance(m.Position,TraversalMotor.Radius,m.BodyHeight),"Fall grew into roof or penetrated ledge");
            }
            require(m.Position.X>x,"Partial ledge contact froze air control");
        });
        check("motor falls from upper ledge and lands on thin lower deck without tunneling",()=>{
            var scene=new SceneDefinition(3,"fall-deck",a.Floor,new(0,3.2f,0),[],[
                new("high",new(-1,3,-1),new(.5f,3.2f,1)),new("deck",new(1.5f,1.4f,-1),new(4,1.6f,1))],[]);
            var m=new TraversalMotor(scene);bool fell=false,landed=false;
            for(int i=0;i<180;i++)
            {
                m.Advance(dt,new(Screen(new(1,0))));fell|=m.Mode==TraversalMode.Falling;
                require(scene.CreateWorld().HasClearance(m.Position,TraversalMotor.Radius,m.BodyHeight),"Fall penetrated thin deck");
                if(fell&&m.Mode==TraversalMode.Grounded){landed=true;break;}
            }
            require(landed&&Math.Abs(m.Position.Y-1.6f)<.0001f,"Failed to land on intermediate deck");
        });
        check("recovery uses current scene standing-safe layer before and after portal",()=>{
            SceneDefinition Raised(string id,string target,float y)=>new(3,id,new("floor",new(-1,y-.3f,-1),new(1,y,1)),new(0,y,0),[],[],[])
            {Portals=[new("portal",new(.3f,y-.05f,-.3f),new(.7f,y+.05f,.3f),new(.5f,y,0),target,"entry")]};
            var s=new ExpeditionSession(new("a",[Raised("a","b",1.6f),Raised("b","a",.5f)]));
            void FallRecover(float expectedY)
            {
                long revision=s.WorldRevision;
                for(int i=0;i<400&&s.WorldRevision==revision;i++)s.Advance(dt,new(Screen(new(0,1))));
                require(s.WorldRevision==revision+1&&s.Leader.Mode==TraversalMode.Grounded&&Math.Abs(s.Leader.Position.Y-expectedY)<.0001f,"Recovery selected another layer/scene");
                require(s.SafePoint==s.Leader.Position,"Recovery did not reset safe position");
            }
            FallRecover(1.6f);Walk(s,new(.5f,0));Press(s);require(s.Scene.Id=="b","Portal after recovery failed");
            FallRecover(.5f);require(s.Scene.Id=="b","Recovery returned previous scene");
        });
        check("named startup and portal spawns below recovery threshold are rejected",()=>{
            var low=new WorldBox("low",new(-.5f,-3.2f,-.5f),new(.5f,-3,.5f));
            var invalidStart=a with {Spawn=new(0,-3,0),Structures=[low],Portals=[]};
            Reject(()=>new ExpeditionSession(new("a",[invalidStart])));
            var invalidTarget=b with {Structures=[low],Spawns=[new("entry",b.Spawn),new("low-entry",new(0,-3,0))]};
            Reject(()=>new ExpeditionProject("a",[a with {Portals=[a.Portals[0] with {TargetSpawn="low-entry"}]},invalidTarget]));
        });
        check("nearby ramp portal and ladder accept continuous rising approach",()=>{
            var ramp=new RampDefinition("ramp",new(0,-1),new(4,1),RampAxis.X,0,1.6f,.2f);
            var scene=a with {Spawn=new(1,.58f,0),Ramps=[ramp],Portals=[new("ramp-portal",new(1.5f,.7f,-.5f),new(2.5f,1.3f,.5f),new(2,.98f,0),"b","entry")]};
            var s=new ExpeditionSession(new("a",[scene,b]));Walk(s,new(1.9f,0));
            require(Math.Abs(s.Leader.Position.Y-.94f)<.0001f&&s.Leader.ContextId=="ramp-portal","Continuous rising portal approach rejected");
            Press(s);require(s.Scene.Id=="b","Ramp portal did not activate");
            var ladder=new LadderDefinition("ramp-ladder",new(2,.98f,.6f),new(2,2.6f,.6f),new(2,.98f,0),new(2,2.6f,1.7f),new(2,.98f,0),new(2,2.6f,1.7f));
            scene=scene with {Portals=[],Ladders=[ladder],Structures=[new("platform",new(1,2.4f,1.2f),new(3,2.6f,2.4f))]};
            s=new ExpeditionSession(new("a",[scene]));Walk(s,new(1.9f,0));
            require(s.Leader.ContextId=="ramp-ladder","Continuous rising ladder approach rejected");
            Press(s);require(s.Leader.Mode==TraversalMode.Climbing,"Ramp ladder did not capture");
        });
        check("ladder entry follows ramp crest support in both directions at each tick",()=>{
            foreach(var route in new[]{(StartX:3.6f,StartY:1.6f,EntryX:4f,EntryY:1.6f),(StartX:4f,StartY:1.6f,EntryX:3.6f,EntryY:1.6f)})
            {
                var ladder=new LadderDefinition("crest-ladder",new(route.EntryX,route.EntryY,.6f),new(route.EntryX,3.2f,.6f),
                    new(route.EntryX,route.EntryY,0),new(route.EntryX,3.2f,1.7f),new(route.EntryX,route.EntryY,0),new(route.EntryX,3.2f,1.7f));
                var scene=new SceneDefinition(3,"crest",new("floor",new(-5,-.3f,-3),new(10,0,3)),new(route.StartX,route.StartY,0),[],
                    [new("deck",new(4,1.4f,-1),new(8,1.6f,1)),new("platform",new(3,3,1.2f),new(5,3.2f,2.4f))],[ladder])
                    {Ramps=[new("ramp",new(0,-1),new(4,1),RampAxis.X,0,1.6f,.2f)]};
                var m=new TraversalMotor(scene);var world=scene.CreateWorld();
                require(m.Context?.Id=="crest-ladder","Crest entry not offered");
                m.Advance(0,new(Vector2.Zero,InteractHeld:true));
                for(int i=0;i<100&&(m.Mode!=TraversalMode.Climbing||Math.Abs(m.Position.X-route.EntryX)>.00001f);i++)
                {
                    var before=m.Position;m.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero));
                    require(world.HasClearance(m.Position,TraversalMotor.Radius,TraversalMotor.StandingHeight)&&world.HasSupport(m.Position,TraversalMotor.Radius),$"Crest approach penetrated/lost support at {m.Position}");
                    require(Vector3.Distance(before,m.Position)<=TraversalMotor.ClimbSpeed*TraversalMotor.StepSeconds+.00001,"Crest capture exceeded climb speed");
                }
                require(m.Mode==TraversalMode.Climbing&&Vector3.Distance(m.Position,ladder.BottomEntry.Vector)<.00001f,"Crest approach did not reach entry");
            }
        });
        check("candidate reload rejects parent reparse replacement and keeps active session",()=>{
            string temp=Path.GetFullPath(Path.GetTempPath());
            string root=Path.GetFullPath(Path.Combine(temp,"rat-reparse-"+Guid.NewGuid()));
            require(root.StartsWith(Path.TrimEndingDirectorySeparator(temp)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase),"Fixture escaped temporary root");
            string content=Path.Combine(root,"Content"),outside=Path.Combine(root,"outside");
            string link=Path.Combine(content,"targets"),original=Path.Combine(content,"targets-original");
            try
            {
                WriteProject(content,a,b);Directory.CreateDirectory(link);Directory.CreateDirectory(outside);
                File.WriteAllText(Path.Combine(link,"b.json"),JsonSerializer.Serialize(b));
                File.WriteAllText(Path.Combine(outside,"b.json"),JsonSerializer.Serialize(b));
                File.WriteAllText(Path.Combine(content,"project.json"),JsonSerializer.Serialize(new ProjectDefinition(1,"a",[new("a","a.json"),new("b","targets/b.json")])));
                var s=new ExpeditionSession(ExpeditionProject.Load(content));Walk(s,new(1,0));var before=s.Snapshot;
                Directory.Move(link,original);
                if(OperatingSystem.IsWindows())
                {
                    var info=new ProcessStartInfo("powershell") {UseShellExecute=false,CreateNoWindow=true,WindowStyle=ProcessWindowStyle.Hidden,RedirectStandardOutput=true,RedirectStandardError=true};
                    foreach(string arg in new[]{"-NoProfile","-NonInteractive","-Command",
                        "$ErrorActionPreference='Stop'; New-Item -ItemType Junction -Path '"+link.Replace("'","''")+"' -Target '"+outside.Replace("'","''")+"' | Out-Null"})info.ArgumentList.Add(arg);
                    using var process=Process.Start(info)!;
                    if(!process.WaitForExit(30000)){process.Kill(true);throw new Exception("Fixture junction helper exceeded 30 seconds");}
                    require(process.ExitCode==0,"Junction creation failed: "+process.StandardError.ReadToEnd());
                }
                else Directory.CreateSymbolicLink(link,outside);
                require((File.GetAttributes(link)&FileAttributes.ReparsePoint)!=0,"Fixture is not a reparse point");
                Press(s);
                require(s.Scene.Id=="a"&&s.Leader.Position==before.Leader.Position&&s.SafePoint==before.SafePoint&&s.WorldRevision==before.WorldRevision&&s.LastError is not null,
                    "Reload followed replacement outside Content or changed active session");
            }
            finally
            {
                // Never recursively remove a directory containing a link. Delete only
                // the known junction itself, then explicit fixture files/directories.
                if(Directory.Exists(link)&&(File.GetAttributes(link)&FileAttributes.ReparsePoint)!=0)Directory.Delete(link,false);
                foreach(string file in new[]{Path.Combine(content,"a.json"),Path.Combine(content,"b.json"),Path.Combine(content,"project.json"),Path.Combine(original,"b.json"),Path.Combine(outside,"b.json"),Path.Combine(link,"b.json")})
                    if(File.Exists(file))File.Delete(file);
                foreach(string dir in new[]{original,link,outside,content,root})if(Directory.Exists(dir))Directory.Delete(dir,false);
            }
        });
        check("strict project schema validates actual content and rejects missing ramp coordinates and references",()=>{
            string content=Path.GetFullPath(Path.Combine(AppContext.BaseDirectory,"../../../../Content"));
            var actual=ExpeditionProject.Load(content);require(actual.Scenes.Count==2,"Expected two content scenes");
            string dir=Path.Combine(Path.GetTempPath(),"rat-project-"+Guid.NewGuid());
            try
            {
                WriteProject(dir,a,b);
                var rampScene=a with {Ramps=[new("ramp",new(-2,-2),new(2,-1),RampAxis.X,0,1.6f,.2f)]};
                foreach(string bounds in new[]{"Min","Max"})foreach(string name in new[]{"X","Z"})
                {
                    var node=JsonSerializer.SerializeToNode(rampScene)!;node["Ramps"]![0]![bounds]!.AsObject().Remove(name);
                    File.WriteAllText(Path.Combine(dir,"a.json"),node.ToJsonString());Reject(()=>ExpeditionProject.Load(dir));
                }
                var obsolete=JsonSerializer.SerializeToNode(a)!;obsolete["spawn"]=JsonSerializer.SerializeToNode(new Point3(9,9,9));
                File.WriteAllText(Path.Combine(dir,"a.json"),obsolete.ToJsonString());Reject(()=>ExpeditionProject.Load(dir));
                WriteProject(dir,a,b);
                foreach(string path in new[]{"../a.json","/a.json","C:/a.json","sub/../a.json",".. /a.json","missing.json"})
                {
                    File.WriteAllText(Path.Combine(dir,"project.json"),JsonSerializer.Serialize(new ProjectDefinition(1,"a",[new("a",path),new("b","b.json")])));
                    Reject(()=>ExpeditionProject.Load(dir));
                }
                Reject(()=>new ExpeditionProject("a",[a with {Portals=[a.Portals[0] with {TargetScene="missing"}]},b]));
                Reject(()=>new ExpeditionProject("a",[a with {Portals=[a.Portals[0] with {TargetSpawn="missing"}]},b]));
                Reject(()=>(a with {Portals=[a.Portals[0] with {Min=new(-float.MaxValue,-.05f,-.4f),Max=new(float.MaxValue,.05f,.4f)}]}).Validate());
                Reject(()=>(a with {OcclusionGroups=[new("bad-group",["missing"])]}).Validate());
                Reject(()=>(a with {OcclusionGroups=[new("group-a",["floor"]),new("group-b",["floor"])]}).Validate());
                Reject(()=>(a with {Spawns=[new("entry",a.Spawn),new("entry",a.Spawn)]}).Validate());
                Reject(()=>(a with {SchemaVersion=2}).Validate());
            }
            finally{Directory.Delete(dir,true);}
        });
    }
}
