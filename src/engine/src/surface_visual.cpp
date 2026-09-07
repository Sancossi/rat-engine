#include "rat/surface_visual.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <map>
#include <set>
#include <utility>

namespace rat {
namespace {
using Point = std::array<double, 3>;
struct Face { std::array<Point, 4> p; Point normal; std::uint32_t color; bool contour=true; };
Point subtract(Point a, Point b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
double dot(Point a, Point b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
Point normal(const std::array<Point,4>& p) {
  // Actual fill indices are 0,2,1 and 0,3,2. Cast precedes differences/products.
  auto a = subtract(p[2],p[0]), b = subtract(p[1],p[0]);
  Point n{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
  double length = std::sqrt(dot(n,n));
  if (length == 0) {
    a = subtract(p[3],p[0]); b = subtract(p[2],p[0]);
    n = {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
    length = std::sqrt(dot(n,n));
  }
  if (length > 0) for (auto& value : n) value /= length;
  return n;
}
Vec3 vec(Point p) { return {static_cast<float>(p[0]),static_cast<float>(p[1]),static_cast<float>(p[2])}; }
struct Rectangle { Face face; int axis; double lo_u,hi_u,lo_v,hi_v; };
bool rectangle(const Face& f, Rectangle& r) {
  for (int axis=0;axis<3;++axis) {
    const int u=(axis+1)%3,v=(axis+2)%3;
    if (std::abs(f.normal[axis]) < 0.999999) continue;
    double lo_u=f.p[0][u],hi_u=lo_u,lo_v=f.p[0][v],hi_v=lo_v;
    for (auto p:f.p) {
      if (p[axis] != f.p[0][axis]) return false;
      lo_u=std::min(lo_u,p[u]); hi_u=std::max(hi_u,p[u]);
      lo_v=std::min(lo_v,p[v]); hi_v=std::max(hi_v,p[v]);
    }
    std::set<std::pair<double,double>> corners;
    for (auto p:f.p) {
      if ((p[u]!=lo_u && p[u]!=hi_u) || (p[v]!=lo_v && p[v]!=hi_v)) return false;
      corners.emplace(p[u],p[v]);
    }
    if (corners.size()!=4 || lo_u==hi_u || lo_v==hi_v) return false;
    r={f,axis,lo_u,hi_u,lo_v,hi_v}; return true;
  }
  return false;
}
struct Segment { Point a,b,n; double lo,hi; };
}  // namespace

SurfaceVisualMesh build_surface_visual_mesh(const GreyboxFillMesh& fill, float tile_size) {
  SurfaceVisualMesh out;
  std::vector<Face> exposed;
  std::map<std::pair<int,double>,std::vector<Rectangle>> planes;
  std::map<std::array<Point,4>,Face> oblique;
  for (std::size_t i=0;i+3<fill.vertices.size();i+=4) {
    Face f{}; f.color=fill.vertices[i].abgr;
    for (int j=0;j<4;++j) {
      const auto& p=fill.vertices[i+static_cast<std::size_t>(j)]; f.p[j]={p.x,p.y,p.z};
    }
    f.normal=normal(f.p);
    if(i/4<fill.face_normals.size()) {
      const auto hint=fill.face_normals[i/4];
      if(hint.x!=0 || hint.y!=0 || hint.z!=0) f.normal={hint.x,hint.y,hint.z};
    }
    if (dot(f.normal,f.normal)<0.5) continue;
    Rectangle r;
    if (rectangle(f,r)) planes[{r.axis,f.p[0][r.axis]}].push_back(r);
    else {
      auto key=f.p; std::sort(key.begin(),key.end());
      auto found=oblique.find(key);
      if (found!=oblique.end() && dot(found->second.normal,f.normal)<-0.99999) oblique.erase(found);
      else oblique.emplace(key,f);
    }
  }
  for (const auto& [key,f] : oblique) { (void)key; exposed.push_back(f); }
  const std::size_t split_budget=fill.vertices.size()/4+1024;
  std::size_t added_faces=0;
  for (const auto& [plane,rects] : planes) {
    std::set<double> us,vs;
    for (const auto& r:rects) { us.insert(r.lo_u);us.insert(r.hi_u);vs.insert(r.lo_v);vs.insert(r.hi_v); }
    const std::vector<double> u(us.begin(),us.end()),v(vs.begin(),vs.end());
    struct Owners { const Rectangle* positive=nullptr; const Rectangle* negative=nullptr; };
    std::map<std::pair<std::size_t,std::size_t>,Owners> cells;
    bool bounded_fallback=false;
    std::size_t visits=0;
    for (const auto& r:rects) {
      auto begin_u=std::lower_bound(u.begin(),u.end(),r.lo_u)-u.begin();
      auto end_u=std::lower_bound(u.begin(),u.end(),r.hi_u)-u.begin();
      auto begin_v=std::lower_bound(v.begin(),v.end(),r.lo_v)-v.begin();
      auto end_v=std::lower_bound(v.begin(),v.end(),r.hi_v)-v.begin();
      for (auto a=begin_u;a<end_u;++a) for (auto b=begin_v;b<end_v;++b) {
        if(++visits>262144 || cells.size()>=rects.size()+split_budget-added_faces) {
          bounded_fallback=true;break;
        }
        auto& owners=cells[{static_cast<std::size_t>(a),static_cast<std::size_t>(b)}];
        (r.face.normal[r.axis]>0 ? owners.positive : owners.negative)=&r;
      }
      if(bounded_fallback) break;
    }
    if(bounded_fallback) {
      // Keep all original surfaces if pathological overlapping rectangles would
      // multiply the visual mesh. Omit their uncertain contours, never the fill.
      for(const auto& r:rects) { auto face=r.face;face.contour=false;exposed.push_back(face); }
      continue;
    }
    if(cells.size()>rects.size()) added_faces+=cells.size()-rects.size();
    for (const auto& [cell,owners]:cells) {
      if (owners.positive && owners.negative) continue;
      const auto& r=*(owners.positive ? owners.positive : owners.negative);
      Face f=r.face; const int a=r.axis,b=(a+1)%3,c=(a+2)%3;
      const auto [i,j]=cell;
      for (auto& p:f.p) p[a]=plane.second;
      f.p[0][b]=u[i]; f.p[0][c]=v[j];
      f.p[1][b]=u[i+1]; f.p[1][c]=v[j];
      f.p[2][b]=u[i+1]; f.p[2][c]=v[j+1];
      f.p[3][b]=u[i]; f.p[3][c]=v[j+1];
      if (f.normal[a]>0) std::swap(f.p[1],f.p[3]);
      exposed.push_back(f);
    }
  }
  std::map<std::array<double,5>,std::vector<Segment>> lines;
  const double unit=tile_size>0 ? tile_size : 1;
  for (const auto& f:exposed) {
    const auto base=static_cast<std::uint32_t>(out.vertices.size());
    for (auto p:f.p) out.vertices.push_back({static_cast<float>(p[0]),static_cast<float>(p[1]),static_cast<float>(p[2]),
      static_cast<float>(f.normal[0]),static_cast<float>(f.normal[1]),static_cast<float>(f.normal[2]),f.color});
    for (auto index:{0u,2u,1u,0u,3u,2u}) out.indices.push_back(base+index);
    if(!f.contour) continue;
    for (int i=0;i<4;++i) {
      Point a=f.p[i],b=f.p[(i+1)%4],d=subtract(b,a);
      int axis=0; for (int j=1;j<3;++j) if (std::abs(d[j])>std::abs(d[axis])) axis=j;
      if (d[axis]==0) continue;
      const int u=(axis+1)%3,v=(axis+2)%3;
      const double slope_u=d[u]/d[axis],slope_v=d[v]/d[axis];
      const double intercept_u=a[u]-slope_u*a[axis],intercept_v=a[v]-slope_v*a[axis];
      const bool aligned=d[u]==0 && d[v]==0;
      std::array<double,5> key{static_cast<double>(axis),std::round(slope_u*1e6),std::round(slope_v*1e6),
        aligned ? intercept_u : std::round(intercept_u/unit*1e6)*unit/1e6,
        aligned ? intercept_v : std::round(intercept_v/unit*1e6)*unit/1e6};
      if (a[axis]>b[axis]) std::swap(a,b);
      lines[key].push_back({a,b,f.normal,a[axis],b[axis]});
    }
  }
  for (const auto& [key,segments]:lines) {
    const auto axis=static_cast<int>(key[0]);
    struct Event { std::size_t segment; bool starts; };
    std::map<double,std::vector<Event>> events;
    for(std::size_t i=0;i<segments.size();++i) {
      events[segments[i].lo].push_back({i,true});
      events[segments[i].hi].push_back({i,false});
    }
    std::set<std::size_t> active;
    std::map<Point,std::size_t> normals;
    for(auto at=events.begin();at!=events.end();++at) {
      // Sweep endpoints once. Never rescan all line segments for every interval.
      for(const auto event:at->second) {
        ++out.contour_endpoint_events;
        const auto& n=segments[event.segment].n;
        if(event.starts) { active.insert(event.segment);++normals[n]; }
        else {
          active.erase(event.segment);
          const auto found=normals.find(n);
          if(--found->second==0) normals.erase(found);
        }
      }
      const auto next=std::next(at);
      if(next==events.end() || active.empty()) continue;
      const double lo=at->first,hi=next->first;
      const Segment* first=&segments[*active.begin()];
      bool crease=false,uncertain=false;
      std::size_t checks=0;
      for(const auto& [n,count]:normals) {
        (void)count;
        // A pathological non-manifold edge may carry arbitrarily many distinct
        // normals. Bound classification too; omit an uncertain contour, not fill.
        if(checks==32) { uncertain=true;break; }
        ++checks;++out.contour_normal_checks;
        if(std::abs(dot(first->n,n))<0.99999) { crease=true;break; }
      }
      if(uncertain || (active.size()>1 && !crease)) continue;
      auto position=[&](double t) {
        Point p{}; const double blend=(t-first->lo)/(first->hi-first->lo);
        for (int j=0;j<3;++j) p[j]=first->a[j]+(first->b[j]-first->a[j])*blend;
        p[axis]=t; return vec(p);
      };
      out.contours.push_back({position(lo),position(hi)});
    }
  }
  return out;
}
}  // namespace rat
