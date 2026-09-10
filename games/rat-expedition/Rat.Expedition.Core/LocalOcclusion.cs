using System.Numerics;

namespace Rat.Expedition.Core;

public readonly record struct SightSegment(Vector3 Near,Vector3 Actor);

// Exact finite box/slab clipping. Presentation supplies parallel orthographic rays
// unprojected at the actor's screen samples, ending at that sample's depth.
public sealed class LocalOcclusion
{
    private readonly SceneDefinition scene;
    private readonly Dictionary<string,double> clearSeconds=[];
    private readonly HashSet<string> hidden=new(StringComparer.Ordinal);
    public IReadOnlySet<string> Hidden=>hidden;
    public LocalOcclusion(SceneDefinition scene){this.scene=scene;Reset();}
    public void Reset(){hidden.Clear();clearSeconds.Clear();}
    public void Update(TraversalSnapshot leader,IEnumerable<SightSegment> rays,double elapsed)
    {
        if(!double.IsFinite(elapsed)||elapsed<0)throw new ArgumentOutOfRangeException(nameof(elapsed));
        var segments=rays.ToArray();var direct=new HashSet<string>(StringComparer.Ordinal);
        foreach(var group in scene.OcclusionGroups)
        {
            bool protectedSupport=Supports(group,leader.Position);
            bool hit=!protectedSupport&&group.Members.Any(id=>segments.Any(ray=>Intersects(id,ray)));
            if(hit){clearSeconds[group.Id]=0;direct.Add(group.Id);}
            else
            {
                clearSeconds[group.Id]=clearSeconds.GetValueOrDefault(group.Id)+elapsed;
                if(!protectedSupport&&hidden.Contains(group.Id)&&clearSeconds[group.Id]<.15)direct.Add(group.Id);
            }
        }
        hidden.Clear();hidden.UnionWith(direct);
        // Metadata is acyclic; propagate only explicit dependent attachments.
        for(int i=0;i<scene.OcclusionGroups.Length;i++)foreach(var group in scene.OcclusionGroups)
            if(group.HideWith is string parent&&hidden.Contains(parent)&&!Supports(group,leader.Position))hidden.Add(group.Id);
    }
    public bool HidesCompanion(TrailPose pose)=>scene.OcclusionGroups.Any(g=>hidden.Contains(g.Id)&&Supports(g,pose.Position));
    private bool Supports(OccluderGroup group,Vector3 feet)
    {
        foreach(var id in group.Members)
        {
            var box=scene.AllSolids.FirstOrDefault(b=>b.Id==id);
            if(box is not null&&box.OverlapsFootprint(feet,TraversalMotor.Radius)&&Math.Abs(feet.Y-box.Max.Y)<=BodyCollisionWorld.Epsilon)return true;
            var ramp=scene.Ramps.FirstOrDefault(r=>r.Id==id)?.ToWorld();
            float radius=TraversalMotor.Radius;
            if(ramp is null||feet.X+radius<=ramp.Min.X||feet.X-radius>=ramp.Max.X||feet.Z+radius<=ramp.Min.Y||feet.Z-radius>=ramp.Max.Y)continue;
            double x=Math.Clamp(feet.X+(ramp.Axis==RampAxis.X&&ramp.Slope<0?-radius:radius),ramp.Min.X,ramp.Max.X);
            double z=Math.Clamp(feet.Z+(ramp.Axis==RampAxis.Z&&ramp.Slope<0?-radius:radius),ramp.Min.Y,ramp.Max.Y);
            if(Math.Abs(feet.Y-ramp.TopAt(x,z))<=BodyCollisionWorld.Epsilon)return true;
        }
        return false;
    }
    public bool Intersects(string id,SightSegment ray)
    {
        double enter=0,leave=1;var delta=ray.Actor-ray.Near;
        bool Plane(double x,double y,double z,double d)
        {
            double start=x*ray.Near.X+y*ray.Near.Y+z*ray.Near.Z-d;
            double slope=x*delta.X+y*delta.Y+z*delta.Z;
            if(Math.Abs(slope)<1e-12)return start<=0;
            if(slope<0)enter=Math.Max(enter,-start/slope);else leave=Math.Min(leave,-start/slope);
            return enter<=leave;
        }
        var box=scene.AllSolids.Concat(scene.Decorations).FirstOrDefault(b=>b.Id==id);
        if(box is not null)return Plane(-1,0,0,-box.Min.X)&&Plane(1,0,0,box.Max.X)&&Plane(0,-1,0,-box.Min.Y)&&Plane(0,1,0,box.Max.Y)&&Plane(0,0,-1,-box.Min.Z)&&Plane(0,0,1,box.Max.Z)&&enter<1-1e-5&&leave>1e-5;
        var ramp=scene.Ramps.First(r=>r.Id==id).ToWorld();
        double a=ramp.Axis==RampAxis.X?ramp.Slope:0,b=ramp.Axis==RampAxis.Z?ramp.Slope:0,c=ramp.TopAt(0,0);
        return Plane(-1,0,0,-ramp.Min.X)&&Plane(1,0,0,ramp.Max.X)&&Plane(0,0,-1,-ramp.Min.Y)&&Plane(0,0,1,ramp.Max.Y)&&Plane(-a,1,-b,c)&&Plane(a,-1,b,ramp.Thickness-c)&&enter<1-1e-5&&leave>1e-5;
    }
}
