using System.Numerics;

namespace Rat.Expedition.Core;

public readonly record struct SupportedMove(Vector3 Position, float Fraction, bool Blocked, bool LostSupport);

// Qualification adapter. The P1.2 motor deliberately continues using BodyCollisionWorld.
public sealed class LayeredCollisionWorld
{
    private const double Epsilon = BodyCollisionWorld.Epsilon;
    private readonly Surface[] surfaces;
    private readonly Plane[][] hulls;
    private readonly record struct Plane(double X, double Y, double Z, double D)
    {
        public double Distance(Vector3 p, float radius, float height) =>
            X*p.X + Y*p.Y + Z*p.Z - D - radius*(Math.Abs(X)+Math.Abs(Z)) - height*Math.Max(0,-Y);
        public double Along(Vector3 delta) => X*delta.X + Y*delta.Y + Z*delta.Z;
    }
    private sealed record Surface(string Id, double X0, double Z0, double X1, double Z1, double A, double B, double C)
    {
        public double Top(double x, double z) => A*x + B*z + C;
        public double Maximum(Vector3 p, float r) => Top(
            Math.Clamp(p.X + (A >= 0 ? r : -r), X0, X1),
            Math.Clamp(p.Z + (B >= 0 ? r : -r), Z0, Z1));
    }
    private readonly record struct Rect(double X0, double Z0, double X1, double Z1)
    {
        public bool HasArea => X1 > X0 && Z1 > Z0;
        public bool Contains(double x, double z) => x >= X0 && x <= X1 && z >= Z0 && z <= Z1;
    }

    public LayeredCollisionWorld(IEnumerable<WorldBox> boxes, IEnumerable<WorldRamp> ramps)
    {
        var faces = new List<Plane[]>(); var tops = new List<Surface>();
        var ids = new HashSet<string>(StringComparer.Ordinal);
        foreach (var b in boxes)
        {
            if (b is null || string.IsNullOrWhiteSpace(b.Id) || !ids.Add(b.Id) || b.Min is null || b.Max is null ||
                !b.Min.IsFinite || !b.Max.IsFinite || b.Min.X >= b.Max.X || b.Min.Y >= b.Max.Y || b.Min.Z >= b.Max.Z ||
                !float.IsFinite(b.Max.X-b.Min.X) || !float.IsFinite(b.Max.Y-b.Min.Y) || !float.IsFinite(b.Max.Z-b.Min.Z))
                throw new InvalidDataException("Layered boxes require unique ids and finite min < max.");
            faces.Add([new(-1,0,0,-b.Min.X),new(1,0,0,b.Max.X),new(0,-1,0,-b.Min.Y),new(0,1,0,b.Max.Y),new(0,0,-1,-b.Min.Z),new(0,0,1,b.Max.Z)]);
            tops.Add(new(b.Id,b.Min.X,b.Min.Z,b.Max.X,b.Max.Z,0,0,b.Max.Y));
        }
        foreach (var r in ramps)
        {
            if (r is null) throw new InvalidDataException("Null ramp.");
            r.Validate();
            if (!ids.Add(r.Id)) throw new InvalidDataException($"Duplicate geometry id '{r.Id}'.");
            double a = r.Axis == RampAxis.X ? r.Slope : 0, b = r.Axis == RampAxis.Z ? r.Slope : 0;
            double c = r.TopAt(0,0);
            tops.Add(new(r.Id,r.Min.X,r.Min.Y,r.Max.X,r.Max.Y,a,b,c));
            // The extra horizontal planes are needed by the Minkowski sum with an upright box.
            faces.Add([new(-1,0,0,-r.Min.X),new(1,0,0,r.Max.X),new(0,0,-1,-r.Min.Y),new(0,0,1,r.Max.Y),
                new(-a,1,-b,c),new(a,-1,b,r.Thickness-c),
                new(0,1,0,Math.Max(r.StartY,r.EndY)),new(0,-1,0,r.Thickness-Math.Min(r.StartY,r.EndY))]);
        }
        surfaces = tops.ToArray(); hulls = faces.ToArray();
    }

    public bool HasClearance(Vector3 feet, float radius, float height)
    {
        ValidateBody(feet, radius, height);
        return !hulls.Any(h => h.All(p => p.Distance(feet,radius,height) < -Epsilon));
    }

    // Feet is the highest surface under the entire flat square footprint. On a slope
    // only its uphill edge touches: the bounded downhill air gap is intentional.
    // Each covering patch must be locally joined at equal-height geometry boundaries.
    public bool HasSupport(Vector3 feet, float radius)
    {
        ValidateBody(feet,radius,1);
        return Candidates(feet,radius).Any(c => Math.Abs(Height(c,feet,radius)-feet.Y) <= Epsilon);
    }

    // Exact convex sweep for arbitrary 3D body motion, including downward landing and
    // upward head contact. A fraction of zero also reports an initially overlapping body.
    public float SweepFraction(Vector3 from, Vector3 to, float radius, float height)
    {
        ValidateBody(from,radius,height); ValidateBody(to,radius,height);
        if (!HasClearance(from,radius,height)) return 0;
        double first = 1; var delta = to-from;
        if (!float.IsFinite(delta.X) || !float.IsFinite(delta.Y) || !float.IsFinite(delta.Z))
            throw new ArgumentException("Body sweep displacement must be finite.");
        foreach (var hull in hulls)
        {
            double enter = 0, leave = 1; bool possible = true;
            foreach (var p in hull)
            {
                double d = p.Distance(from,radius,height), v = p.Along(delta);
                // A segment whose endpoints remain outside/touching this linear plane
                // cannot enter the hull. Use the same tolerance as clearance, including
                // a float-rounded ramp/flat seam endpoint approached from either sign.
                if (d >= -Epsilon && (v >= 0 || d+v >= -Epsilon)) { possible=false; break; }
                if (v < 0) enter = Math.Max(enter,-d/v);
                else if (v > 0) leave = Math.Min(leave,-d/v);
                if (enter >= leave) { possible=false; break; }
            }
            if (possible && leave > 0) first=Math.Min(first,Math.Max(0,enter));
        }
        return (float)first;
    }

    // Straight horizontal displacement with continuous supported Y. A caller may use
    // two axis moves for sliding. LostSupport stops at the first footprint edge and
    // reports the unconsumed fraction so a future falling motor need not teleport.
    public SupportedMove MoveSupported(Vector3 start, Vector2 delta, float radius, float height)
    {
        ValidateBody(start,radius,height);
        if (!float.IsFinite(delta.X) || !float.IsFinite(delta.Y) || !float.IsFinite(start.X+delta.X) || !float.IsFinite(start.Z+delta.Y))
            throw new ArgumentException("Movement must be finite.");
        if (!HasClearance(start,radius,height)) return new(start,0,true,false);
        if (!HasSupport(start,radius)) return new(start,0,false,true);
        if (delta == Vector2.Zero) return new(start,1,false,false);
        Vector3 At(double t) => new((float)(start.X+delta.X*t),0,(float)(start.Z+delta.Y*t));
        var events = new SortedSet<double> {0,1};
        void Event(double distance, double speed)
        { if(speed != 0) { double t=distance/speed; if(t>0 && t<1) events.Add(t); } }
        foreach(var s in surfaces)
        {
            foreach(double x in new[]{s.X0,s.X1}) foreach(int side in new[]{-1,1}) Event(x+side*radius-start.X,delta.X);
            foreach(double z in new[]{s.Z0,s.Z1}) foreach(int side in new[]{-1,1}) Event(z+side*radius-start.Z,delta.Y);
        }
        // Within each footprint/boundary interval, each clipped patch height is linear.
        // Add envelope crossings, avoiding endpoint-only or distance-based sampling.
        var baseEvents=events.ToArray();
        for(int i=1;i<baseEvents.Length;i++)
        {
            double a=baseEvents[i-1], b=baseEvents[i]; var middle=At((a+b)/2);
            var active=surfaces.Where(s=>Clip(s,middle,radius).HasArea).ToArray();
            for(int j=0;j<active.Length;j++) for(int k=j+1;k<active.Length;k++)
            {
                double da=active[j].Maximum(At(a),radius)-active[k].Maximum(At(a),radius);
                double db=active[j].Maximum(At(b),radius)-active[k].Maximum(At(b),radius);
                if(da*db<0) events.Add(a+(b-a)*da/(da-db));
            }
        }
        var ordered=events.ToArray(); Vector3 current=start;
        for(int i=1;i<ordered.Length;i++)
        {
            double a=ordered[i-1], b=ordered[i]; var begin=At(a); var end=At(b);
            var candidate=Candidates(At((a+b)/2),radius)
                .Where(c=>Math.Abs(Height(c,begin,radius)-current.Y)<=Epsilon)
                .OrderBy(c=>c.Min(s=>s.Id),StringComparer.Ordinal).FirstOrDefault();
            if(candidate is null) return new(current,(float)a,false,true);
            end.Y=(float)Height(candidate,end,radius);
            float hit=SweepFraction(current,end,radius,height);
            if(hit<1) return new(Vector3.Lerp(current,end,hit),(float)(a+(b-a)*hit),true,false);
            current=end;
        }
        return new(current,1,false,false);
    }

    private static double Height(Surface[] members, Vector3 p, float r) => members.Max(s=>s.Maximum(p,r));
    private static Rect Clip(Surface s, Vector3 p, float r) => new(Math.Max(s.X0,(double)p.X-r),Math.Max(s.Z0,(double)p.Z-r),Math.Min(s.X1,(double)p.X+r),Math.Min(s.Z1,(double)p.Z+r));

    private IEnumerable<Surface[]> Candidates(Vector3 feet,float radius)
    {
        var active=surfaces.Where(s=>Clip(s,feet,radius).HasArea).ToArray();
        var visited=new HashSet<Surface>();
        foreach(var seed in active)
        {
            if(!visited.Add(seed)) continue;
            var component=new List<Surface>{seed};
            for(int i=0;i<component.Count;i++) foreach(var s in active)
                if(!visited.Contains(s) && Joined(component[i],s,feet,radius)) {visited.Add(s);component.Add(s);}
            if(Covers(component,feet,radius)) yield return component.ToArray();
        }
    }

    private static bool Joined(Surface a,Surface b,Vector3 p,float r)
    {
        var x=Clip(a,p,r); var y=Clip(b,p,r);
        var overlap=new Rect(Math.Max(x.X0,y.X0),Math.Max(x.Z0,y.Z0),Math.Min(x.X1,y.X1),Math.Min(x.Z1,y.Z1));
        if(overlap.X1<overlap.X0 || overlap.Z1<overlap.Z0) return false;
        bool Equal(double px,double pz)=>Math.Abs(a.Top(px,pz)-b.Top(px,pz))<=Epsilon;
        if(Equal(overlap.X0,overlap.Z0) && Equal(overlap.X1,overlap.Z0) && Equal(overlap.X0,overlap.Z1) && Equal(overlap.X1,overlap.Z1)) return true;
        // Noncoplanar patches must meet along an actual surface edge, not an invented
        // height crossing in the middle of overlapping floors.
        foreach(double edge in new[]{a.X0,a.X1,b.X0,b.X1})
            if(edge>=overlap.X0 && edge<=overlap.X1 && Equal(edge,overlap.Z0) && Equal(edge,overlap.Z1)) return true;
        foreach(double edge in new[]{a.Z0,a.Z1,b.Z0,b.Z1})
            if(edge>=overlap.Z0 && edge<=overlap.Z1 && Equal(overlap.X0,edge) && Equal(overlap.X1,edge)) return true;
        return false;
    }

    private static bool Covers(List<Surface> members,Vector3 p,float r)
    {
        var rectangles=members.Select(s=>Clip(s,p,r)).ToArray();
        var xs=rectangles.SelectMany(q=>new[]{q.X0,q.X1}).Append((double)p.X-r).Append((double)p.X+r).Distinct().Order().ToArray();
        var zs=rectangles.SelectMany(q=>new[]{q.Z0,q.Z1}).Append((double)p.Z-r).Append((double)p.Z+r).Distinct().Order().ToArray();
        // Full rectangle arrangement coverage: narrow holes missed by all four corners
        // still leave an uncovered cell. Only the footprint's exterior boundary has
        // contact tolerance; never enlarge every patch and accidentally seal a hole.
        for(int x=1;x<xs.Length;x++) for(int z=1;z<zs.Length;z++)
        {
            if(rectangles.Any(q=>q.Contains((xs[x-1]+xs[x])/2,(zs[z-1]+zs[z])/2))) continue;
            bool outerX=(x==1 || x==xs.Length-1) && xs[x]-xs[x-1]<=Epsilon;
            bool outerZ=(z==1 || z==zs.Length-1) && zs[z]-zs[z-1]<=Epsilon;
            if(!outerX && !outerZ) return false;
        }
        return true;
    }

    private static void ValidateBody(Vector3 p,float r,float h)
    {
        if(!float.IsFinite(p.X) || !float.IsFinite(p.Y) || !float.IsFinite(p.Z) || !float.IsFinite(r) || !float.IsFinite(h) || r<=0 || h<=0)
            throw new ArgumentException("Body requires finite position, positive radius and height.");
    }
}
