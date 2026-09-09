using System.Numerics;

namespace Rat.Expedition.Core;

public readonly record struct TrailPose(Vector3 Position,BodyStance Stance,TraversalMode Mode);

// Distance along the actual motor polyline, never a chord across render frames.
public sealed class PartyTrail
{
    public const float Spacing=.7f;
    public const float RearDistance=2*Spacing;
    private readonly List<TrailPose> points=[];
    public PartyTrail(TraversalSnapshot leader)=>Reset(leader);
    public void Reset(TraversalSnapshot leader){points.Clear();points.Add(new(leader.Position,leader.Stance,leader.Mode));}
    internal void Record(IEnumerable<Vector3> path,TraversalSnapshot leader)
    {
        foreach(var p in path)
        {
            var pose=new TrailPose(p,leader.Stance,leader.Mode);
            // Each endpoint describes the segment that arrived there. A later
            // stationary stance change must not rewrite that travelled segment.
            if(points[^1].Position!=p)points.Add(pose);
        }
        double distance=0;int keep=points.Count-1;
        while(keep>0&&distance<RearDistance+.5f){distance+=Vector3.Distance(points[keep].Position,points[keep-1].Position);keep--;}
        if(keep>0)points.RemoveRange(0,keep);
    }
    public TrailPose AtDistance(float distance)
    {
        if(!float.IsFinite(distance)||distance<0)throw new ArgumentOutOfRangeException(nameof(distance));
        for(int i=points.Count-1;i>0;i--)
        {
            float length=Vector3.Distance(points[i].Position,points[i-1].Position);
            if(distance<=length)return points[i] with {Position=Vector3.Lerp(points[i].Position,points[i-1].Position,distance/length)};
            distance-=length;
        }
        return points[0];
    }
    public IReadOnlyList<TrailPose> Companions=>new[]{AtDistance(Spacing),AtDistance(RearDistance)};
}
