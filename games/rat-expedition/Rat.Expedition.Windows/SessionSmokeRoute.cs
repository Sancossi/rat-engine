using System.Numerics;
using Rat.Expedition.Core;

namespace Rat.Expedition.Windows;

// Commands and observations only. No diagnostic position/state setter exists.
internal sealed class SessionSmokeRoute(string route)
{
    private int phase,wait,legs;
    private long revision;
    private bool mixedFallCaptured,mixedFallHolding,mixedFallDone;
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
            if(route=="mixed"&&phase==10&&!mixedFallDone)
            {
                // Stop driving horizontally after leaving the upper deck. The
                // resulting vertical trail keeps the rear companion supported
                // while the leader and first companion fall below the deck.
                if(s.Mode==TraversalMode.Falling&&s.Position.Y<3.6f){mixedFallHolding=true;return new(Vector2.Zero);}
                if(mixedFallHolding)mixedFallDone=true;
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
        if(route=="dremma")
        {
            SessionInput Point(float x,float z,bool crouch,string? capture)
            {
                if(!Near(x,z))return Towards(x,z,crouch);
                if(s.Mode==TraversalMode.Falling)return new(Vector2.Zero,crouch);
                if(++wait<8)return new(Vector2.Zero,crouch);
                Capture=capture;phase++;wait=0;return new(Vector2.Zero,crouch);
            }
            switch(phase)
            {
                case 0:return Point(0,5,false,"dremma-plaza");
                case 1:return Point(0,0,false,"dremma-lower-centre");
                case 2:return Point(0,-4,false,"dremma-lower-south");
                case 3:return Point(1.9f,-2.5f,false,null);
                case 4:
                    if(wait++<45)return Towards(1.9f,0);
                    Capture="dremma-arch-standing-blocked";phase++;wait=0;return new(Vector2.Zero);
                case 5:return Point(1.9f,2.5f,true,"dremma-arch-crouched");
                case 6:
                    if(s.Stance!=BodyStance.Standing)return new(Vector2.Zero);
                    if(++wait<8)return new(Vector2.Zero);
                    Capture="dremma-arch-standing-clear";phase++;wait=0;return new(Vector2.Zero);
                case 7:return Point(-6.2f,2.5f,false,null);
                case 8:
                    if(wait++<45)return Towards(-6.2f,0);
                    Capture="dremma-stairs-side-blocked";phase++;wait=0;return new(Vector2.Zero);
                case 9:return Point(-11.25f,2.5f,false,null);
                case 10:return Point(-11.25f,0,false,null);
                case 11:return Point(-9,0,false,"dremma-stairs-low");
                case 12:return Point(-6.2f,0,false,"dremma-stairs-mid");
                case 13:
                    if(wait++<45)return Towards(-6.2f,1.5f);
                    Capture="dremma-stairs-rail-blocked";phase++;wait=0;return new(Vector2.Zero);
                case 14:return Point(-6.2f,0,false,null);
                case 15:return Point(-3.5f,0,false,"dremma-bridge-west");
                case 16:return Point(3.5f,0,false,"dremma-bridge-east");
                case 17:return Point(5.5f,0,false,"dremma-lower-restored");
                case 18:
                    if(!Near(0,-4))return Towards(0,-4);
                    Capture="dremma-complete";Complete=true;return new(Vector2.Zero);
            }
        }
        if(route is "dremma-portals" or "dremma-resource-failure")
        {
            if(phase==0)
            {
                var portal=session.Scene.Portals.OrderBy(p=>Vector2.Distance(new(s.Position.X,s.Position.Z),new(p.Anchor.X,p.Anchor.Z))).First();
                if(!Near(portal.Anchor.X,portal.Anchor.Z))return Towards(portal.Anchor.X,portal.Anchor.Z);
                revision=session.WorldRevision;phase=1;return new(Vector2.Zero);
            }
            if(phase==1){phase=2;return new(Vector2.Zero,InteractHeld:true);}
            if(route=="dremma-resource-failure")
            {
                if(session.WorldRevision!=revision||session.LastError is null)throw new InvalidOperationException("Failed Dremma resource candidate did not preserve old scene");
                Capture="dremma-resource-rejected";Complete=true;return new(Vector2.Zero);
            }
            if(session.WorldRevision==revision)throw new InvalidOperationException("Dremma portal route did not change scene: "+session.LastError);
            if(++wait<5)return new(Vector2.Zero,InteractHeld:true);
            legs++;Capture=$"dremma-portal-{legs:D2}";wait=0;phase=0;
            if(legs==20)Complete=true;
            return new(Vector2.Zero);
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

    public void Observe(ExpeditionSession session,LocalOcclusion occlusion)
    {
        if(route!="mixed"||mixedFallCaptured)return;
        var s=session.Leader;
        var companions=session.Trail.Companions;
        if(s.Mode==TraversalMode.Falling&&companions[0].Mode==TraversalMode.Falling&&
            s.Position.Y>0&&s.Position.Y<companions[0].Position.Y&&companions[0].Position.Y<companions[1].Position.Y&&
            Math.Abs(companions[1].Position.Y-3.6f)<.001f&&
            !occlusion.HidesCompanion(companions[0])&&occlusion.HidesCompanion(companions[1]))
        {
            Capture="mixed-companions";mixedFallCaptured=true;
        }
    }
}
