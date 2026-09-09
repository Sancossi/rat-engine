using Rat.Expedition.Core;
using Stride.Core.Mathematics;
using Stride.Graphics;
using Stride.Rendering.ProceduralModels;

namespace Rat.Expedition.Authoring;

    public sealed class FiniteRampPrimitive(WorldRamp ramp):PrimitiveProceduralModelBase
    {
        protected override GeometricMeshData<VertexPositionNormalTexture> CreatePrimitiveMeshData()
        {
            // Eight geometric corners; split into 24 vertices for flat face normals.
            Vector3 Top(float x,float z)=>new(x,(float)ramp.TopAt(x,z),z);
            Vector3[] p=[Top(ramp.Min.X,ramp.Min.Y),Top(ramp.Max.X,ramp.Min.Y),Top(ramp.Max.X,ramp.Max.Y),Top(ramp.Min.X,ramp.Max.Y)];
            p=p.Concat(p.Select(v=>v-new Vector3(0,ramp.Thickness,0))).ToArray();
            int[][] faces=[[0,3,2,1],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]];
            var vertices=new List<VertexPositionNormalTexture>();var indices=new List<int>();
            foreach(var f in faces)
            {
                int start=vertices.Count;var normal=Vector3.Normalize(Vector3.Cross(p[f[1]]-p[f[0]],p[f[2]]-p[f[0]]));
                Vector2[] uv=[new(0,0),new(1,0),new(1,1),new(0,1)];
                for(int i=0;i<4;i++)vertices.Add(new(p[f[i]],normal,uv[i]));
                // Stride's right-handed primitive front-face winding is opposite
                // the outward cross-product normal (same convention as Cube.New).
                indices.AddRange(new[]{start,start+2,start+1,start,start+3,start+2});
            }
            return new(vertices.ToArray(),indices.ToArray(),false);
        }
    }
