using System.Numerics;
using Rat.Expedition.Core;

internal static class PresentationTests
{
    public static void Run(Action<string,Action> check,Action<bool,string> require)
    {
        Vector2 Screen(Vector2 v)=>new(Vector2.Dot(v,TraversalMotor.CameraRight),Vector2.Dot(v,TraversalMotor.CameraForward));
        var scene=new SceneDefinition(3,"trail",new("floor",new(-6.75f,-.675f,-6.75f),new(20.25f,0,6.75f)),new(-2.25f,0,0),[],[new("deck",new(9,3.15f,-2.25f),new(18,3.6f,2.25f))],[])
            {Ramps=[new("ramp",new(0,-2.25f),new(9,2.25f),RampAxis.X,0,3.6f,.45f)]};
        check("party keeps equal compact intervals after walking and stopping",()=>{
            var s=new ExpeditionSession(new("trail",[scene with {Ramps=[],Structures=[]}]));
            for(int tick=0;tick<180;tick++)s.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX)));
            var party=s.Trail.Companions.ToArray();
            require(Math.Abs(s.Leader.Position.X-party[0].Position.X-.7f)<.0001f,"First companion distance changed");
            require(Math.Abs(s.Leader.Position.X-party[1].Position.X-1.4f)<.0001f,$"Rear distance was {s.Leader.Position.X-party[1].Position.X}, expected 1.4");
            require(Math.Abs(party[0].Position.X-party[1].Position.X-.7f)<.0001f,"Companions do not have equal intervals");
            for(int tick=0;tick<60;tick++)s.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero));
            require(party.SequenceEqual(s.Trail.Companions),"Idle changed compact trail positions");
            s.Advance(0,new(Vector2.Zero,PauseHeld:true));
            s.Advance(.1,new(Screen(Vector2.UnitX)));
            require(party.SequenceEqual(s.Trail.Companions),"Pause changed compact trail positions");
        });
        check("trail follows supported ramp and crest during multi-tick catchup",()=>{
            var s=new ExpeditionSession(new("trail",[scene]));var world=scene.CreateWorld();
            for(int i=0;i<23;i++)
            {
                s.Advance(.1,new(Screen(Vector2.UnitX)));
                foreach(var pose in s.Trail.Companions)
                    require(world.HasSupport(pose.Position,TraversalMotor.Radius)&&world.HasClearance(pose.Position,TraversalMotor.Radius,TraversalMotor.StandingHeight),$"Trail cut ramp at {pose.Position}");
            }
            var before=s.Trail.Companions.ToArray();s.Advance(0,new(Vector2.Zero,PauseHeld:true));s.Advance(.1,new(Screen(Vector2.UnitX)));
            require(before.SequenceEqual(s.Trail.Companions),"Paused trail advanced");
        });
        check("trail preserves walking corners rather than a render-frame chord",()=>{
            var flat=scene with {Ramps=[],Structures=[]};var s=new ExpeditionSession(new("trail",[flat]));
            for(int i=0;i<10;i++)s.Advance(.1,new(Screen(Vector2.UnitX)));
            for(int i=0;i<3;i++)s.Advance(.1,new(Screen(Vector2.UnitY)));
            var rear=s.Trail.Companions[1].Position;
            require(Math.Abs(rear.Z)<.0001&&rear.X<2,"Rear actor cut corner diagonally");
            s.Trail.Reset(s.Leader);require(s.Trail.Companions.All(p=>p.Position==s.Leader.Position),"Reset left old scene history");
        });
        check("trail preserves crouched incoming segments when leader stands at crawl exit",()=>{
            var crawl=scene with {Spawn=new(-2.256f,0,0),Ramps=[],Structures=[new("roof",new(-1.125f,1.35f,-2.25f),new(1.125f,1.8f,2.25f))]};
            var s=new ExpeditionSession(new("trail",[crawl]));var world=crawl.CreateWorld();
            for(int i=0;i<338;i++)s.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX),CrouchHeld:true));
            var old=s.Trail.AtDistance(.005f);
            require(old.Stance==BodyStance.Crouched,"Fixture did not record crouch");
            s.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero));
            require(s.Leader.Stance==BodyStance.Standing,"Fixture did not exit roof");
            require(s.Trail.AtDistance(.005f).Stance==BodyStance.Crouched,"Stationary stand rewrote a travelled segment");
            // Exercise both sides of the exact body-clearance edge, not only the
            // fixed companion spacing, which can coincide with a sample endpoint.
            var moving=new ExpeditionSession(new("trail",[crawl]));
            for(int i=0;i<308;i++)moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX),CrouchHeld:true));
            moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX)));
            for(float distance=.026f;distance<.04f;distance+=.001f)
            {
                var pose=moving.Trail.AtDistance(distance);
                require(world.HasClearance(pose.Position,TraversalMotor.Radius,pose.Stance==BodyStance.Crouched?TraversalMotor.CrouchedHeight:TraversalMotor.StandingHeight),$"Follower stood beneath roof at {pose.Position}");
            }
            for(int tick=0;tick<65;tick++)
            {
                moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX)));
                foreach(var pose in moving.Trail.Companions)
                    require(world.HasClearance(pose.Position,TraversalMotor.Radius,pose.Stance==BodyStance.Crouched?TraversalMotor.CrouchedHeight:TraversalMotor.StandingHeight),$"Actual companion stood beneath roof at {pose.Position}");
            }
            moving.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero,CrouchHeld:true));
            require(moving.Trail.AtDistance(.005f).Stance==BodyStance.Standing,"Stationary crouch rewrote a travelled standing segment");
        });
        var occlusionScene=scene with {Decorations=[new("rail",new(9,3.6f,2.025f),new(18,4.5f,2.25f))],
            OcclusionGroups=[new("deck-group",["deck"]),new("rail-group",["rail"],"deck-group")]};
        var leader=new TraversalMotor(occlusionScene).Snapshot;
        check("local deck cut protects upper support and hides only attached upper companion",()=>{
            var occlusion=new LocalOcclusion(occlusionScene);
            var lower=leader with {Position=new(11.25f,0,0)};
            occlusion.Update(lower,[new(new(11.25f,9,0),new(11.25f,.9f,0))],0);
            require(occlusion.Hidden.SetEquals(new[]{"deck-group","rail-group"}),"Deck attachment remained floating");
            require(occlusion.HidesCompanion(new(new(11.25f,3.6f,0),BodyStance.Standing,TraversalMode.Grounded)),"Upper companion not hidden");
            require(!occlusion.HidesCompanion(new(new(11.25f,0,0),BodyStance.Standing,TraversalMode.Grounded))&&!occlusion.HidesCompanion(new(new(-2.25f,3.6f,0),BodyStance.Standing,TraversalMode.Grounded)),"Unrelated actor hidden");
            occlusion.Update(lower,[],.1);require(occlusion.Hidden.Contains("deck-group"),"Restored before hysteresis");
            occlusion.Update(lower,[],.051);require(occlusion.Hidden.Count==0,"Did not restore");
            occlusion.Update(leader with {Position=new(11.25f,3.6f,0)},[new(new(11.25f,9,0),new(11.25f,3.6f,0))],0);
            require(!occlusion.Hidden.Contains("deck-group"),"Supporting deck hidden");
        });
        check("occlusion clips actual slab surfaces and excludes geometry behind actor",()=>{
            var occlusion=new LocalOcclusion(occlusionScene);
            require(!occlusion.Intersects("ramp",new(new(7.875f,.9f,-6.75f),new(7.875f,.9f,6.75f))),"Ramp AABB hid empty underside");
            require(occlusion.Intersects("ramp",new(new(1.8f,.675f,-6.75f),new(1.8f,.675f,6.75f))),"Finite ramp surface missed");
            require(!occlusion.Intersects("deck",new(new(11.25f,9,0),new(11.25f,6.75f,0))),"Ray extended behind actor");
        });
        check("occlusion ramp support uses the accepted body radius",()=>{
            var rampScene=scene with {OcclusionGroups=[new("ramp-group",["ramp"])]};
            var occlusion=new LocalOcclusion(rampScene);var ramp=rampScene.Ramps[0].ToWorld();
            var feet=new Vector3(4.5f,(float)ramp.TopAt(4.5f+TraversalMotor.Radius,0),0);
            var ray=new SightSegment(new(1.8f,.675f,-6.75f),new(1.8f,.675f,6.75f));
            occlusion.Update(leader with {Position=new(11.25f,0,0)},[ray],0);
            require(occlusion.Hidden.Contains("ramp-group")&&occlusion.HidesCompanion(new(feet,BodyStance.Standing,TraversalMode.Grounded)),"Accepted-radius ramp companion was not linked to its hidden support");
            occlusion.Update(leader with {Position=feet},[ray],0);
            require(!occlusion.Hidden.Contains("ramp-group"),"Accepted-radius ramp support was hidden beneath leader");
        });
        check("occlusion dependency rejects dangling and cyclic attachments",()=>{
            foreach(var groups in new[]{new[]{new OccluderGroup("deck-group",["deck"],"missing")},new[]{new OccluderGroup("deck-group",["deck"],"rail-group"),new OccluderGroup("rail-group",["rail"],"deck-group")}})
            {
                try{(occlusionScene with {OcclusionGroups=groups}).Validate();}catch(InvalidDataException){continue;}
                throw new Exception("Invalid attachment accepted");
            }
        });
    }
}
