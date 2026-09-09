using System.Numerics;
using Rat.Expedition.Core;

internal static class BodyCollisionTests
{
    public static void Run(Action<string, Action> check, Action<bool, string> require)
    {
        WorldBox floor = new("floor", new(-4,-1,-4), new(4,0,4));
        WorldBox ceiling = new("ceiling", new(-1,.6f,-1), new(1,.8f,1));
        check("body adapter uses complete volume and permits face contact", () => {
            var world = new BodyCollisionWorld([floor, ceiling]);
            require(!world.CanStand(Vector3.Zero,.2f,.8f), "Standing penetrated ceiling");
            require(world.CanStand(Vector3.Zero,.2f,.4f), "Crouched body blocked under finite ceiling");
            require(world.CanStand(Vector3.Zero,.2f,.6f), "Touching ceiling face rejected");
            require(!world.HasClearance(new(1.1f,0,1.1f),.2f,.8f), "Footprint corner missed ceiling");
        });
        check("body support keeps explicit feet Y and complete footprint", () => {
            var world = new BodyCollisionWorld([floor, new("platform",new(-1,1.4f,-1),new(1,1.6f,1))]);
            require(world.CanStand(Vector3.Zero,.2f,.8f), "Platform became solid column to ground");
            require(world.CanStand(new(0,1.6f,0),.2f,.8f), "Top support missing");
            require(!world.HasSupport(new(0,.8f,0),.2f), "Snapped unsupported feet to another floor");
            require(!world.HasSupport(new(.9f,1.6f,0),.2f), "Partial top footprint accepted");
            var moved = world.MoveGrounded(new(0,1.6f,0),new(10,0),.2f,.8f);
            require(Math.Abs(moved.X-.8f)<.0001f && moved.Y==1.6f, "Top movement fell/snapped at edge");
        });
        check("body sweep cannot tunnel through thin walls or ceiling corners", () => {
            var world = new BodyCollisionWorld([floor, new("thin",new(0,0,-3),new(.001f,.9f,3))]);
            var moved = world.MoveGrounded(new(-2,0,0),new(4,1),.2f,.8f);
            require(moved.X<=-.199f && moved.Z>.9f, "Thin-wall sweep or sliding failed");
            var low = new BodyCollisionWorld([floor,ceiling]);
            require(!low.IsSegmentClear(new(-2,0,-2),new(2,0,2),.2f,.8f), "Diagonal skipped ceiling");
            require(low.IsSegmentClear(new(-2,0,-2),new(2,0,2),.2f,.4f), "Crouched passage blocked");
        });
        check("body sweep rejects unsupported gap even when destination supported", () => {
            var world = new BodyCollisionWorld([new("left",new(-3,-1,-1),new(-.1f,0,1)),new("right",new(.1f,-1,-1),new(3,0,1))]);
            var moved = world.MoveGrounded(new(-1,0,0),new(3,0),.2f,.8f);
            require(moved.X<=-.299f, "Long step skipped support gap");
        });
        check("ladder body corridor and horizontal exit detect intermediate obstruction", () => {
            var free = new BodyCollisionWorld([floor]);
            require(free.IsSegmentClear(new(0,0,0),new(0,1.6f,0),.2f,.8f), "Free vertical corridor rejected");
            var blocked = new BodyCollisionWorld([floor,new("bar",new(-.1f,1,-.1f),new(.1f,1.1f,.1f))]);
            require(!blocked.IsSegmentClear(new(0,0,0),new(0,1.6f,0),.2f,.8f), "Corridor bar missed");
            var exit = new BodyCollisionWorld([new("bar",new(.4f,1.6f,-.1f),new(.41f,2.4f,.1f))]);
            require(!exit.IsSegmentClear(new(0,1.6f,0),new(1,1.6f,0),.2f,.8f), "Exit teleported through thin obstruction");
        });
    }
}
