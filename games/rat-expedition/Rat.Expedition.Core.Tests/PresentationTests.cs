using System.Numerics;
using Rat.Expedition.Core;

internal static class PresentationTests
{
    public static void Run(Action<string,Action> check,Action<bool,string> require)
    {
        Vector2 Screen(Vector2 v)=>new(Vector2.Dot(v,TraversalMotor.CameraRight),Vector2.Dot(v,TraversalMotor.CameraForward));
        var scene=new SceneDefinition(3,"trail",new("floor",new(-3,-.3f,-3),new(9,0,3)),new(-1,0,0),[],[new("deck",new(4,1.4f,-1),new(8,1.6f,1))],[])
            {Ramps=[new("ramp",new(0,-1),new(4,1),RampAxis.X,0,1.6f,.2f)]};
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
                    require(world.HasSupport(pose.Position,.2f)&&world.HasClearance(pose.Position,.2f,.8f),$"Trail cut ramp at {pose.Position}");
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
            var crawl=scene with {Spawn=new(-1.006f,0,0),Ramps=[],Structures=[new("roof",new(-.5f,.6f,-1),new(.5f,.8f,1))]};
            var s=new ExpeditionSession(new("trail",[crawl]));var world=crawl.CreateWorld();
            for(int i=0;i<150;i++)s.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX),CrouchHeld:true));
            var old=s.Trail.AtDistance(.005f);
            require(old.Stance==BodyStance.Crouched,"Fixture did not record crouch");
            s.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero));
            require(s.Leader.Stance==BodyStance.Standing,"Fixture did not exit roof");
            require(s.Trail.AtDistance(.005f).Stance==BodyStance.Crouched,"Stationary stand rewrote a travelled segment");
            // Exercise both sides of the exact body-clearance edge, not only the
            // fixed companion spacing, which can coincide with a sample endpoint.
            var moving=new ExpeditionSession(new("trail",[crawl]));
            for(int i=0;i<137;i++)moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX),CrouchHeld:true));
            moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX)));
            for(float distance=.026f;distance<.04f;distance+=.001f)
            {
                var pose=moving.Trail.AtDistance(distance);
                require(world.HasClearance(pose.Position,.2f,pose.Stance==BodyStance.Crouched?.4f:.8f),$"Follower stood beneath roof at {pose.Position}");
            }
            for(int tick=0;tick<65;tick++)
            {
                moving.Advance(TraversalMotor.StepSeconds,new(Screen(Vector2.UnitX)));
                foreach(var pose in moving.Trail.Companions)
                    require(world.HasClearance(pose.Position,.2f,pose.Stance==BodyStance.Crouched?.4f:.8f),$"Actual companion stood beneath roof at {pose.Position}");
            }
            moving.Advance(TraversalMotor.StepSeconds,new(Vector2.Zero,CrouchHeld:true));
            require(moving.Trail.AtDistance(.005f).Stance==BodyStance.Standing,"Stationary crouch rewrote a travelled standing segment");
        });
        var occlusionScene=scene with {Decorations=[new("rail",new(4,1.6f,.9f),new(8,2,1))],
            OcclusionGroups=[new("deck-group",["deck"]),new("rail-group",["rail"],"deck-group")]};
        var leader=new TraversalMotor(occlusionScene).Snapshot;
        check("local deck cut protects upper support and hides only attached upper companion",()=>{
            var occlusion=new LocalOcclusion(occlusionScene);
            var lower=leader with {Position=new(5,0,0)};
            occlusion.Update(lower,[new(new(5,4,0),new(5,.4f,0))],0);
            require(occlusion.Hidden.SetEquals(new[]{"deck-group","rail-group"}),"Deck attachment remained floating");
            require(occlusion.HidesCompanion(new(new(5,1.6f,0),BodyStance.Standing,TraversalMode.Grounded)),"Upper companion not hidden");
            require(!occlusion.HidesCompanion(new(new(5,0,0),BodyStance.Standing,TraversalMode.Grounded))&&!occlusion.HidesCompanion(new(new(-1,1.6f,0),BodyStance.Standing,TraversalMode.Grounded)),"Unrelated actor hidden");
            occlusion.Update(lower,[],.1);require(occlusion.Hidden.Contains("deck-group"),"Restored before hysteresis");
            occlusion.Update(lower,[],.051);require(occlusion.Hidden.Count==0,"Did not restore");
            occlusion.Update(leader with {Position=new(5,1.6f,0)},[new(new(5,4,0),new(5,1.6f,0))],0);
            require(!occlusion.Hidden.Contains("deck-group"),"Supporting deck hidden");
        });
        check("occlusion clips actual slab surfaces and excludes geometry behind actor",()=>{
            var occlusion=new LocalOcclusion(occlusionScene);
            require(!occlusion.Intersects("ramp",new(new(3.5f,.4f,-3),new(3.5f,.4f,3))),"Ramp AABB hid empty underside");
            require(occlusion.Intersects("ramp",new(new(.8f,.3f,-3),new(.8f,.3f,3))),"Finite ramp surface missed");
            require(!occlusion.Intersects("deck",new(new(5,4,0),new(5,3,0))),"Ray extended behind actor");
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
