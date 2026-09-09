using System.Numerics;
using Rat.Expedition.Core;

internal static class LayeredCollisionTests
{
    public static void Run(Action<string,Action> check, Action<bool,string> require)
    {
        const float radius=.2f,height=.8f;
        WorldBox floor=new("floor",new(-5,-1,-3),new(8,0,3));
        WorldBox deck=new("deck",new(4,1.4f,-1),new(7,1.6f,1));
        WorldRamp ramp=new("ramp",new(0,-1),new(4,1),RampAxis.X,0,1.6f,.2f);
        LayeredCollisionWorld World(params WorldBox[] extra)=>new(new[]{floor,deck}.Concat(extra),[ramp]);
        void Near(float actual,float expected,string why)=>require(Math.Abs(actual-expected)<.0001f,$"{why}: {actual} != {expected}");
        void Reject(Action action) {try{action();}catch(InvalidDataException){return;}throw new Exception("Invalid geometry accepted");}

        check("ramp full-footprint feet convention and continuous flat seams both ways",()=>{
            var world=World(); var p=new Vector3(-1,0,0);
            for(int i=0;i<280;i++)
            {
                var step=world.MoveSupported(p,new(.025f,0),radius,height);
                require(!step.Blocked && !step.LostSupport,"Ascent blocked/lost support");
                require(Math.Abs(step.Position.Y-p.Y)<=.0101f,"Ascent snapped");
                p=step.Position;
                require(world.HasSupport(p,radius) && world.HasClearance(p,radius,height),"Ascent body invalid");
                Near(p.Y,Math.Clamp((p.X+radius)*.4f,0,1.6f),"Uphill-edge foot height");
            }
            Near(p.Y,1.6f,"Deck height");
            for(int i=0;i<280;i++)
            {
                var step=world.MoveSupported(p,new(-.025f,0),radius,height);
                require(!step.Blocked && !step.LostSupport,"Descent blocked/lost support");
                require(Math.Abs(step.Position.Y-p.Y)<=.0101f,"Descent snapped"); p=step.Position;
            }
            Near(p.Y,0,"Returned floor");
        });
        check("long ramp route matches repeated moves and preserves lower floor at identical XZ",()=>{
            var world=World(); var upper=world.MoveSupported(new(-1,0,0),new(7,0),radius,height);
            require(!upper.Blocked && !upper.LostSupport,"Long ascent failed"); Near(upper.Position.Y,1.6f,"Upper route");
            var lower=world.MoveSupported(new(6,0,2),new(0,-2),radius,height);
            require(!lower.Blocked && !lower.LostSupport,"Empty bridge underside blocked"); Near(lower.Position.Y,0,"Lower snapped to deck");
            require(world.HasSupport(upper.Position,radius) && world.HasSupport(lower.Position,radius),"Lost distinct supports");
            Near(upper.Position.X,lower.Position.X,"Same X"); Near(upper.Position.Z,lower.Position.Z,"Same Z");
            var back=world.MoveSupported(lower.Position,new(0,2),radius,height); Near(back.Position.Y,0,"Reverse underside");
        });
        check("diagonal supported ascent matches small moves without corner or seam tunneling",()=>{
            var world=World(); var start=new Vector3(-1,0,-.5f); var small=start;
            var longMove=world.MoveSupported(start,new(7,1),radius,height);
            require(!longMove.Blocked && !longMove.LostSupport,"Long diagonal route interrupted");
            for(int i=0;i<280;i++)
            {
                var step=world.MoveSupported(small,new(7f/280,1f/280),radius,height);
                require(!step.Blocked && !step.LostSupport && world.HasClearance(step.Position,radius,height),"Small diagonal route interrupted");
                small=step.Position;
            }
            require(Vector3.Distance(small,longMove.Position)<.0001f,"Diagonal result depends on move length");
        });
        check("ramp side and finite underside block body without ground column",()=>{
            var world=World();
            var lowSide=world.MoveSupported(new(1,0,2),new(0,-2),radius,height);
            require(lowSide.Blocked,"Walked sideways into low ramp solid"); Near(lowSide.Position.Y,0,"Side entry lifted player");
            var highUnder=world.MoveSupported(new(3,0,2),new(0,-2),radius,height);
            require(!highUnder.Blocked && !highUnder.LostSupport,"Free space below high ramp blocked"); Near(highUnder.Position.Y,0,"Under ramp snapped");
            require(world.SweepFraction(new(3,0,0),new(3,2,0),radius,height)<.2f,"Head passed through sloped underside");
            require(!world.HasClearance(new(2,.6f,0),radius,height),"Body inside ramp accepted");
        });
        check("ascending head sweep detects thin overhead obstacle between endpoints",()=>{
            var world=World(new WorldBox("beam",new(2.1f,1.6f,-1),new(2.11f,2.2f,1)));
            var step=world.MoveSupported(new(-1,0,0),new(7,0),radius,height);
            require(step.Blocked && step.Position.X<2.12f,"Thin overhead obstacle tunneled");
            require(world.HasClearance(step.Position,radius,height),"Head stopped penetrating beam");
        });
        check("downward and upward long body sweeps stop at finite thin deck faces",()=>{
            var world=World(); var from=new Vector3(6,5,0); var to=new Vector3(6,-3,0);
            var landed=Vector3.Lerp(from,to,world.SweepFraction(from,to,radius,height));
            Near(landed.Y,1.6f,"Fell through deck"); require(world.HasSupport(landed,radius),"Landing support missing");
            var head=Vector3.Lerp(new(6,0,0),new(6,3,0),world.SweepFraction(new(6,0,0),new(6,3,0),radius,height));
            Near(head.Y,.6f,"Head passed deck underside");
        });
        check("full footprint covers coplanar seams but rejects narrow interior hole",()=>{
            WorldBox Part(string id,float a,float b)=>new(id,new(a,-1,-1),new(b,0,1));
            var joined=new LayeredCollisionWorld([Part("left",-2,0),Part("right",0,2)],[]);
            require(joined.HasSupport(Vector3.Zero,radius),"Flat seam rejected");
            require(!joined.MoveSupported(new(-1,0,0),new(2,0),radius,height).LostSupport,"Flat seam interrupted movement");
            // A thin slot away from centre and all four corners must still invalidate support.
            var gap=new LayeredCollisionWorld([Part("left",-2,.051f),Part("right",.052f,2)],[]);
            require(!gap.HasSupport(Vector3.Zero,radius),"Interior slot missed by footprint test");
            var move=gap.MoveSupported(new(-1,0,0),new(2,0),radius,height);
            require(move.LostSupport && move.Fraction<.5f,"Long movement skipped unsupported slot");
        });
        check("upper edge reports support loss without selecting lower floor",()=>{
            var world=World(); var step=world.MoveSupported(new(6,1.6f,0),new(0,2),radius,height);
            require(step.LostSupport && !step.Blocked,"Upper edge not reported");
            Near(step.Position.Z,.8f,"Full footprint edge"); Near(step.Position.Y,1.6f,"Dropped directly to lower floor");
            require(!world.HasSupport(new(6,1.6f,1.1f),radius),"Lower floor masquerades as upper support");
        });
        check("ramp top lateral exit and descending Z-axis ramp use the same geometry",()=>{
            var world=World(); var edge=world.MoveSupported(new(2,.88f,0),new(0,2),radius,height);
            require(edge.LostSupport && !edge.Blocked,"Ramp side not unsupported"); Near(edge.Position.Y,.88f,"Side exit lifted/dropped");
            var reversed=new WorldRamp("z",new(-1,0),new(1,4),RampAxis.Z,1.6f,0,.2f);
            var zWorld=new LayeredCollisionWorld([floor with {Max=new(8,0,8)},new("z-deck",new(-1,1.4f,-2),new(1,1.6f,0))],[reversed]);
            var up=zWorld.MoveSupported(new(0,0,5),new(0,-6),radius,height);
            require(!up.Blocked && !up.LostSupport,$"Negative-slope Z ramp ascent failed: {up}"); Near(up.Position.Y,1.6f,"Z upper");
            var down=zWorld.MoveSupported(up.Position,new(0,6),radius,height);
            require(!down.Blocked && !down.LostSupport,"Negative-slope Z ramp descent failed"); Near(down.Position.Y,0,"Z lower");
        });
        check("sloped underside standing and crouched contact match finite plane",()=>{
            var world=World();
            var standing=world.MoveSupported(new(3.5f,0,0),new(-3,0),radius,height);
            require(standing.Blocked,"Standing passed lower wedge"); Near(standing.Position.X,2.7f,"Standing head contact");
            var crouched=world.MoveSupported(new(3.5f,0,0),new(-3,0),radius,.4f);
            require(crouched.Blocked,"Crouched passed lower wedge"); Near(crouched.Position.X,1.7f,"Crouched head contact");
            var thin=new LayeredCollisionWorld([floor],[ramp with {Min=new(0,-.1f),Max=new(4,.1f)}]);
            var hit=thin.SweepFraction(new(.8f,0,-1),new(.8f,0,1),radius,height);
            require(hit<.351f,"Long lateral sweep tunneled through thin wedge");
            Near(thin.SweepFraction(new(3.5f,0,-1),new(3.5f,0,1),radius,height),1,"Finite underside wrongly treated as hull AABB");
        });
        check("ramp deck seam has joint support and small seam gap is not skipped",()=>{
            var world=World();
            require(world.HasSupport(new(3.9f,1.6f,0),radius),"Joint ramp and deck support missing");
            var gap=new LayeredCollisionWorld([floor,deck with {Min=new(4.05f,1.4f,-1)}],[ramp]);
            require(!gap.HasSupport(new(3.9f,1.6f,0),radius),"Ground below hid upper seam gap");
            var move=gap.MoveSupported(new(-1,0,0),new(7,0),radius,height);
            require(move.LostSupport && move.Position.X<4,"Long ascent skipped seam gap");
            foreach(var sample in new[]{(-.2f,0f),(0,.08f),(2,.88f),(3.8f,1.6f),(4,1.6f),(4.2f,1.6f)})
                require(world.HasSupport(new(sample.Item1,sample.Item2,0),radius),$"Expected support at {sample}");
        });
        check("nominal float side contact supports inward and tangent movement on box and ramp",()=>{
            var flat=new LayeredCollisionWorld([new("narrow",new(-2,-1,-.5f),new(2,0,.5f))],[]);
            var slope=new LayeredCollisionWorld([],[ramp with {Min=new(0,-.5f),Max=new(4,.5f)}]);
            foreach(var item in new[]{(flat,new Vector3(0,0,.3f)),(slope,new Vector3(2,.88f,.3f))})
            {
                require(item.Item1.HasSupport(item.Item2,radius),"Nominal .3 + .2 edge lost support");
                foreach(var delta in new[]{new Vector2(0,-.1f),new Vector2(.25f,0)})
                {
                    var move=item.Item1.MoveSupported(item.Item2,delta,radius,height);
                    require(!move.LostSupport && !move.Blocked && move.Fraction==1,"Nominal edge rejected inward/tangent movement");
                }
            }
            var world=World(); var edge=world.MoveSupported(new(6,1.6f,0),new(0,2),radius,height);
            require(edge.LostSupport && world.HasSupport(edge.Position,radius),"Returned .8 + .2 edge is not supported");
            foreach(var delta in new[]{new Vector2(0,-.1f),new Vector2(.25f,0)})
            {
                var move=world.MoveSupported(edge.Position,delta,radius,height);
                require(!move.LostSupport && !move.Blocked && move.Fraction==1,"Returned edge cannot move inward/tangent");
            }
        });
        check("coverage tolerance does not seal a sub-epsilon interior hole",()=>{
            // Connected ring; unlike disconnected strips it reaches Covers as one component.
            var ring=new LayeredCollisionWorld([
                new("left",new(-1,-1,-1),new(.05f,0,1)),
                new("right",new(.050001f,-1,-1),new(1,0,1)),
                new("back",new(.05f,-1,-1),new(.050001f,0,-.1f)),
                new("front",new(.05f,-1,.1f),new(.050001f,0,1))],[]);
            require(!ring.HasSupport(Vector3.Zero,radius),"Interior hole smaller than epsilon was sealed");
            var move=ring.MoveSupported(new(-.5f,0,0),new(1,0),radius,height);
            require(move.LostSupport && move.Fraction<.5f,"Movement crossed sub-epsilon interior hole");
        });
        check("ramp metadata rejects nonfinite degenerate steep and duplicate geometry",()=>{
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {StartY=float.NaN}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {Max=new(0,1)}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {Thickness=0}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {EndY=8}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {EndY=0}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {Id="floor"}]));
            Reject(()=>new LayeredCollisionWorld([floor],[ramp with {Axis=(RampAxis)4}]));
        });
    }
}

