using System.Numerics;
using Rat.Expedition.Core;

namespace Rat.Expedition.Windows;

// Observes the real motor; never assigns player coordinates or state.
internal sealed class BodySmokeRoute
{
    private const float MapScale=2.25f;
    private int phase, idleFrames;
    private bool upCaptured, downCaptured;
    public string? Capture { get; set; }
    public bool Complete => phase == 11;
    public List<object> Milestones { get; } = [];

    public TraversalInput Next(TraversalSnapshot s)
    {
        TraversalInput Towards(float x,float z,bool crouch=false)
        {
            var delta=new Vector2(x*MapScale-s.Position.X,z*MapScale-s.Position.Z);
            if(delta.Length()<.045f) return new(Vector2.Zero,crouch);
            var direction=Vector2.Normalize(delta);
            return new(new(Vector2.Dot(direction,TraversalMotor.CameraRight),Vector2.Dot(direction,TraversalMotor.CameraForward)),crouch);
        }
        bool Near(float x,float z) => Vector2.Distance(new(s.Position.X,s.Position.Z),new(x*MapScale,z*MapScale))<.05f;
        switch(phase)
        {
            case 0:
                if(Near(-4,3)) phase=1;
                return Towards(-4,3);
            case 1:
                if(s.Position.Z<=1.901f*MapScale) { Capture="standing-blocked";phase=2;return new(Vector2.Zero); }
                return Towards(-4,1.5f);
            case 2:
                if(Near(-4,1.5f)) { Capture="crouched";phase=3;return new(Vector2.Zero,true); }
                return Towards(-4,1.5f,true);
            case 3:
                if(s.StandBlocked) { Capture="blocked-stand";phase=4; }
                return new(Vector2.Zero);
            case 4:
                if(s.Stance==BodyStance.Standing && s.Position.Z<.95f*MapScale) { Capture="clear-standing";phase=5; }
                return Towards(-4,.7f);
            case 5:
                if(Near(-4.4f,-.95f)) { phase=6;return new(Vector2.Zero,false,true); }
                return Towards(-4.4f,-.95f);
            case 6:
                if(!upCaptured && s.Mode==TraversalMode.Climbing && s.Position.Y>.75f*MapScale) {Capture="climb-up";upCaptured=true;}
                if(s.Mode==TraversalMode.Grounded && MathF.Abs(s.Position.Y-1.6f*MapScale)<.001f) {Capture="upper-exit";phase=7;return new(Vector2.Zero,false,true);}
                return new(new(0,1),true,true);
            case 7:
                if(++idleFrames>15) {Capture="held-interact-top";phase=8;return new(Vector2.Zero);}
                return new(Vector2.Zero,false,true);
            case 8:
                if(Near(-3.3f,-3)) {Capture="top-walking";phase=9;}
                return Towards(-3.3f,-3);
            case 9:
                if(Near(-4.4f,-2.6f)) {phase=10;return new(Vector2.Zero,false,true);}
                return Towards(-4.4f,-2.6f);
            case 10:
                if(!downCaptured && s.Mode==TraversalMode.Climbing && s.Position.Y<.9f*MapScale) {Capture="climb-down";downCaptured=true;}
                if(s.Mode==TraversalMode.Grounded && s.Position.Y==0) {Capture="lower-exit";phase=11;return new(Vector2.Zero,false,true);}
                return new(new(0,-1),true,true);
            default: return new(Vector2.Zero);
        }
    }
}
