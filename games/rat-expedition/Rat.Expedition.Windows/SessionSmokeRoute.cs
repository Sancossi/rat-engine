using System.Numerics;
using Rat.Expedition.Core;

namespace Rat.Expedition.Windows;

// Commands and observations only. No diagnostic position/state setter exists.
internal sealed class SessionSmokeRoute(string route)
{
    private int phase,wait,legs;
    private long revision;
    private bool mixedFallCaptured;
    public string? Capture {get;set;}
    public bool Complete {get;private set;}
    public List<object> Milestones {get;}=[];
    public int Legs=>legs;
    private readonly (float X,float Z,float Y,bool Crouch,string? Capture)[] layered=BuildPoints(route);
    private static (float X,float Z,float Y,bool Crouch,string? Capture)[] BuildPoints(string route)
    {
        (float X,float Z,float Y,bool Crouch,string? Capture)[] points=
    [(-4,3,0,false,null),(-4,1.65f,0,true,"arch-empty"),(-4,3,0,true,null),
     (-6,3,0,false,null),(-6,-3.4f,0,false,null),(-3.4f,-3.4f,0,false,"ramp-entry"),
     (-2,-3.4f,.48f,false,"ramp-ascent"),(0,-3.4f,1.28f,false,null),(.8f,-3.4f,1.6f,false,"ramp-crest"),
     (2.5f,-3.6f,1.6f,false,"bridge-upper"),(2.5f,-3.05f,1.6f,false,"upper-rail"),
     (2.5f,-3.6f,1.6f,false,null),(.8f,-3.4f,1.6f,false,null),(-2,-3.4f,.48f,false,"ramp-descent"),
     (-3.4f,-3.4f,0,false,null),(-3.4f,-1.5f,0,false,null),(-2.4f,-1.5f,0,false,null),
     (-2.4f,-.7f,0,false,null),(.5f,-.7f,0,false,null),(.5f,-2.3f,0,false,null),
     (2.5f,-2.3f,0,false,null),(2.5f,-3.6f,0,false,"bridge-lower"),
     (4.5f,-3.6f,0,false,"lower-forward"),(5.5f,-3.6f,0,false,"cut-restored"),
     (4.5f,-3.6f,0,false,null),(2.5f,-3.6f,0,false,"lower-reverse"),
     (2.5f,-5.15f,0,false,"offcentre-behind-wall")];
        (float X,float Z,float Y,bool Crouch,string? Capture)[] selected=route=="mixed"?points.Take(10).Concat(new (float,float,float,bool,string?)[]{(2.5f,-4.6f,0,false,null),(2.5f,-3.6f,0,false,"mixed-lower"),(2.5f,-2.3f,0,false,"mixed-restored")}).ToArray():points;
        return selected.Select(p=>(p.X*2.25f,p.Z*2.25f,p.Y*2.25f,p.Crouch,p.Capture)).ToArray();
    }
    public SessionInput Next(ExpeditionSession session)
    {
        var s=session.Leader;
        SessionInput Towards(float x,float z,bool crouch=false)
        {
            var delta=new Vector2(x-s.Position.X,z-s.Position.Z);
            var direction=delta/Math.Max(delta.Length(),(crouch?TraversalMotor.CrouchSpeed:TraversalMotor.Speed)/30);
            return new(new(Vector2.Dot(direction,TraversalMotor.CameraRight),Vector2.Dot(direction,TraversalMotor.CameraForward)),crouch);
        }
        bool Near(float x,float z)=>Vector2.Distance(new(s.Position.X,s.Position.Z),new(x,z))<.035f;
        if(Complete)return new(Vector2.Zero);
        if(route is "layered" or "mixed")
        {
            // Retained compact spacing cannot span the scaled 3.6-unit drop plus the
            // grounded return. Observe the real fall before the rear leaves deck.
            if(route=="mixed"&&!mixedFallCaptured&&s.Mode==TraversalMode.Falling&&s.Position.Y<2.025f)
            {
                Capture="mixed-companions";mixedFallCaptured=true;
            }
            if(phase>=layered.Length){Complete=true;return new(Vector2.Zero);}
            var point=layered[phase];
            if(Near(point.X,point.Z))
            {
                if(s.Mode==TraversalMode.Falling)return new(Vector2.Zero,point.Crouch);
                if(Math.Abs(s.Position.Y-point.Y)>.04f)throw new InvalidOperationException($"Layered route wrong level at phase {phase}: {s.Position}");
                if(++wait<8)return new(Vector2.Zero,point.Crouch);
                Capture=point.Capture;phase++;wait=0;return new(Vector2.Zero,point.Crouch);
            }
            return Towards(point.X,point.Z,point.Crouch);
        }
        if(route is "portals" or "portal-failure")
        {
            if(phase==0)
            {
                // Route around the courtyard pillar before approaching the first portal.
                if(legs==0&&!Near(12.375f,9))return Towards(12.375f,9);
                phase=1;return new(Vector2.Zero);
            }
            if(phase==1)
            {
                var p=session.Scene.Portals[0].Anchor;
                if(!Near(p.X,p.Z))return Towards(p.X,p.Z);
                revision=session.WorldRevision;phase=2;return new(Vector2.Zero);
            }
            if(phase==2){phase=3;return new(Vector2.Zero,InteractHeld:true);}
            if(route=="portal-failure")
            {
                if(session.WorldRevision!=revision||session.LastError is null)throw new InvalidOperationException("Failed renderer candidate did not preserve old scene");
                Capture="candidate-rejected";Complete=true;return new(Vector2.Zero);
            }
            if(session.WorldRevision==revision)throw new InvalidOperationException("Portal route did not change scene: "+session.LastError);
            if(phase==3)
            {
                if(++wait<5)return new(Vector2.Zero,InteractHeld:true);
                legs++;Capture=$"portal-{legs:D2}";wait=0;phase=1;
                if(legs==20)Complete=true;
                return new(Vector2.Zero);
            }
        }
        if(route=="recovery")
        {
            if(phase==0){revision=session.WorldRevision;phase=1;}
            if(session.WorldRevision>revision){Capture="recovered";Complete=true;return new(Vector2.Zero);}
            if(s.Mode==TraversalMode.Falling&&wait++==0)Capture="falling";
            return Towards(session.Scene.Floor.Max.X+1.8f,session.Scene.Spawn.Z);
        }
        return new(Vector2.Zero);
    }
}
