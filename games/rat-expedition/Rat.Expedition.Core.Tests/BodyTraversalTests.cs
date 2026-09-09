using System.Numerics;
using System.Text.Json;
using System.Text.Json.Nodes;
using Rat.Expedition.Core;

internal static class BodyTraversalTests
{
    public static void Run(Action<string,Action> check, Action<bool,string> require)
    {
        WorldBox floor = new("floor",new(-5,-1,-5),new(5,0,5));
        WorldBox platform = new("platform",new(-1,1.4f,-2),new(1,1.6f,-.5f));
        LadderDefinition ladder = new("ladder",new(0,0,0),new(0,1.6f,0),new(0,0,.5f),new(0,1.6f,-1),new(0,0,.5f),new(0,1.6f,-1));
        SceneDefinition scene = new(2,"body",floor,new(0,0,.5f),[],[platform],[ladder]);
        Vector2 Screen(Vector2 world) => new(Vector2.Dot(world,TraversalMotor.CameraRight),Vector2.Dot(world,TraversalMotor.CameraForward));
        void Frames(TraversalMotor motor, TraversalInput input, int frames)
        { for(int i=0;i<frames;i++) motor.Advance(1.0/60,input); }
        void Until(TraversalMotor motor, TraversalInput input, Func<bool> done)
        { for(int i=0;i<600 && !done();i++) {motor.Advance(1.0/60,input); require(motor.Mode!=TraversalMode.Climbing || motor.Stance==BodyStance.Standing,"FSM admitted Climbing+Crouched");} require(done(),"Sequence did not finish"); }
        void Reject(SceneDefinition invalid)
        { try { invalid.Validate(); } catch(InvalidDataException) { return; } throw new Exception("Invalid traversal fixture accepted"); }

        check("crouch changes body before sweep and blocked stand clears only after full exit",()=>{
            var low = scene with {Spawn=new(0,0,2),Structures=[new("roof",new(-2,.6f,-.5f),new(2,.8f,.5f))],Ladders=[]};
            var motor = new TraversalMotor(low);
            Frames(motor,new(Screen(new(0,-1))),120);
            require(motor.Position.Z>=.699f && motor.Stance==BodyStance.Standing,"Standing entered low passage");
            Frames(motor,new(Screen(new(0,-1)),true),28);
            require(motor.Position.Z<.1f && motor.Stance==BodyStance.Crouched,"Crouching did not permit passage");
            Frames(motor,new(Vector2.Zero),10);
            require(motor.StandBlocked && motor.Snapshot.Hint=="Здесь нельзя встать","Unsafe stand not rejected/explained");
            Frames(motor,new(Screen(new(0,-1))),60);
            require(motor.Position.Z<-.7f && motor.Stance==BodyStance.Standing && !motor.StandBlocked,"Automatic safe stand failed");
        });
        check("crouched speed is half walk and wall contact keeps sliding over repeated ticks",()=>{
            var plain=scene with {Structures=[],Ladders=[]};
            var a=new TraversalMotor(plain); var b=new TraversalMotor(plain);
            Frames(a,new(Screen(new(1,0))),60); Frames(b,new(Screen(new(1,0)),true),60);
            require(Math.Abs(a.Position.X-3)<.001 && Math.Abs(b.Position.X-1.5f)<.001,"Stance speeds wrong");
            var wall=plain with {Walls=[new("wall",new(.3f,0,-4),new(.4f,2,4))]};
            var m=new TraversalMotor(wall); Frames(m,new(Screen(Vector2.Normalize(new(1,-1)))),120);
            require(m.Position.X<.101f && m.Position.Z<-3,"Contact epsilon froze sliding");
        });
        check("ladder climbs both ways ignores Ctrl and held interact cannot recapture",()=>{
            var m=new TraversalMotor(scene);
            m.Advance(1.0/60,new(Vector2.Zero,true,true));
            require(m.Mode==TraversalMode.Climbing && m.Stance==BodyStance.Standing,"Ladder failed standing capture with Ctrl");
            Until(m,new(new(0,1),true,true),()=>m.Mode==TraversalMode.Grounded && m.Position.Y==1.6f);
            Frames(m,new(Vector2.Zero,true,true),20);
            require(m.Mode==TraversalMode.Grounded,"Held E recaptured at exit");
            Frames(m,new(Vector2.Zero),1); m.Advance(1.0/60,new(Vector2.Zero,false,true));
            Until(m,new(new(0,-1),true,true),()=>m.Mode==TraversalMode.Grounded && m.Position.Y==0);
            require(Vector3.Distance(m.Position,ladder.BottomExit.Vector)<.0001f,"Wrong lower exit");
        });
        check("top walking keeps explicit support and stops at platform footprint edge",()=>{
            var m=new TraversalMotor(scene with {Spawn=ladder.TopEntry});
            Frames(m,new(Screen(new(1,0))),180);
            require(Math.Abs(m.Position.X-.8f)<.001 && m.Position.Y==1.6f,"Top walking fell or changed floor");
        });
        check("pending interaction survives a short frame but is flushed by focus loss",()=>{
            var m=new TraversalMotor(scene);
            m.Advance(TraversalMotor.StepSeconds*.4,new(Vector2.Zero,false,true));
            require(m.Mode==TraversalMode.Grounded,"Action executed outside fixed tick");
            m.Advance(TraversalMotor.StepSeconds*.7,new(Vector2.Zero));
            require(m.Mode==TraversalMode.Climbing,"Queued new press lost before tick");
            var f=new TraversalMotor(scene); f.Advance(TraversalMotor.StepSeconds*.4,new(Vector2.Zero,false,true));
            f.Advance(10,new(Vector2.Zero,false,true),false);
            f.Advance(0,new(Vector2.Zero,false,true),false); // Windows first-resumed discard keeps the release latch.
            Frames(f,new(Vector2.Zero,false,true),5);
            require(f.Mode==TraversalMode.Grounded,"Focus leaked queued/held interaction");
            Frames(f,new(Vector2.Zero),1); Frames(f,new(Vector2.Zero,false,true),1);
            require(f.Mode==TraversalMode.Climbing,"Fresh press after focus/release ignored");
        });
        check("context orders stable ids and snapshot reading is pure",()=>{
            var m=new TraversalMotor(scene with {Ladders=[ladder with {Id="z"},ladder with {Id="a"}]});
            var before=m.Snapshot; for(int i=0;i<10;i++) require(m.Snapshot==before,"Snapshot mutated context/state");
            Frames(m,new(Vector2.Zero,false,true),1);
            require(m.Snapshot.ContextId=="a","Stable id tie-break failed");
            var upper=new TraversalMotor(scene with {Spawn=ladder.TopEntry});
            require(upper.Snapshot.ContextId=="ladder","Upper height context unavailable");
        });
        check("ladder rejects blocked corridor unsupported exit and unsafe exit path",()=>{
            Reject(scene with {Structures=[platform,new("bar",new(-.1f,1,-.1f),new(.1f,1.1f,.1f))]});
            Reject(scene with {Ladders=[ladder with {TopExit=new(2,1.6f,-1)}]});
            Reject(scene with {Structures=[platform,new("exit-bar",new(-.1f,1.6f,-.55f),new(.1f,2.4f,-.54f))]});
            Reject(scene with {Ladders=[ladder with {Top=new(float.NaN,1.6f,0)}]});
            Reject(scene with {Ladders=[ladder with {Id="platform"}]});
        });
        check("context rejects nearer blocked capture before choosing farther accessible ladder",()=>{
            var accessible=new LadderDefinition("accessible",new(.45f,0,1.5f),new(.45f,1.6f,1.5f),new(.45f,0,1),new(.45f,1.6f,2.5f),new(.45f,0,1),new(.45f,1.6f,2.5f));
            var fixture=scene with {Spawn=new(.45f,0,.5f),Walls=[new("capture-wall",new(.2f,0,.35f),new(.21f,2,.65f))],
                Structures=[platform,new("far-platform",new(-.5f,1.4f,2),new(1.5f,1.6f,3))],Ladders=[ladder,accessible]};
            var m=new TraversalMotor(fixture);
            require(m.Snapshot.ContextId=="accessible","Blocked nearer context displaced accessible one");
            Frames(m,new(Vector2.Zero,false,true),1);
            require(m.Snapshot.ContextId=="accessible" && m.State==PlayerTraversalState.Climbing,"FSM captured wrong context");
        });
        check("ladder can reverse midway and return safely to starting side",()=>{
            var m=new TraversalMotor(scene); Frames(m,new(Vector2.Zero,false,true),1);
            Until(m,new(new(0,1)),()=>m.Position.Y>.7f);
            Until(m,new(new(0,-1),true),()=>m.Mode==TraversalMode.Grounded);
            require(m.Position.Y==0 && Vector3.Distance(m.Position,ladder.BottomExit.Vector)<.0001f,"Midway reversal selected wrong floor");
        });
        check("ladder JSON requires nested coordinates and rejects old schema",()=>{
            string path=Path.Combine(Path.GetTempPath(),"rat-ladder-"+Guid.NewGuid()+".json");
            try {
                foreach(var field in new[]{"Bottom","Top","BottomEntry","TopEntry","BottomExit","TopExit"}) {
                    var json=JsonSerializer.SerializeToNode(scene)!; json["Ladders"]![0]![field]!.AsObject().Remove("Y");
                    File.WriteAllText(path,json.ToJsonString());
                    try {SceneDefinition.Load(path);throw new Exception("Missing ladder coordinate accepted");}catch(InvalidDataException){}
                }
                Reject(scene with {SchemaVersion=1});
            } finally {File.Delete(path);}
        });
    }
}
