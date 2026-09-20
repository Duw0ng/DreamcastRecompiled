#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <vector>

struct SoftVertex {
    float x{}, y{}, z{}, u{}, v{};
    std::uint32_t argb{}, offset_argb{};
    std::uint8_t source{};
};
struct GpuVertex {
    float x{}, y{}, z{}, u{}, v{};
    std::uint32_t argb{}, offset_argb{};
};
struct OldTri { SoftVertex a{}, b{}, c{}; std::uint32_t state{}; float sort_z{}; std::uint64_t order{}; };
struct NewTri { std::uint32_t i0{}, i1{}, i2{}, state{}; float sort_z{}; std::uint64_t order{}; };

static GpuVertex gpu(const SoftVertex& v) {
    return {v.x,v.y,v.z,v.u,v.v,v.argb,v.offset_argb};
}
static bool same(const SoftVertex& a, const SoftVertex& b) {
    return std::memcmp(&a.x,&b.x,5*sizeof(float))==0 && a.argb==b.argb &&
           a.offset_argb==b.offset_argb && a.source==b.source;
}
static SoftVertex make_v(std::uint32_t n) {
    SoftVertex v{};
    v.x=float(n%640); v.y=float((n*7)%480); v.z=float((n%997)+1)/998.0f;
    v.u=float(n%257)/256.0f; v.v=float((n*3)%257)/256.0f;
    v.argb=0xFF000000u | ((n*17u)&0x00FFFFFFu);
    v.offset_argb=(n*13u)&0x00FFFFFFu; v.source=std::uint8_t((n&1u)+1u);
    return v;
}

struct OldFrame { std::vector<OldTri> tris; };
struct NewFrame { std::vector<SoftVertex> vertices; std::vector<NewTri> tris; };

static void emit_old(OldFrame& f, std::uint32_t strips, std::uint32_t vertices_per_strip) {
    std::uint32_t n=0; std::uint64_t order=0;
    f.tris.clear();
    f.tris.reserve(std::size_t(strips)*(vertices_per_strip-2));
    for(std::uint32_t s=0;s<strips;++s){
        SoftVertex a=make_v(n++), b=make_v(n++);
        for(std::uint32_t j=2;j<vertices_per_strip;++j){
            SoftVertex c=make_v(n++);
            f.tris.push_back({a,b,c,s&31u,a.z+b.z+c.z,order++});
            a=b; b=c;
        }
    }
}
static void emit_new(NewFrame& f, std::uint32_t strips, std::uint32_t vertices_per_strip) {
    std::uint32_t n=0; std::uint64_t order=0;
    f.vertices.clear(); f.tris.clear();
    f.vertices.reserve(std::size_t(strips)*vertices_per_strip);
    f.tris.reserve(std::size_t(strips)*(vertices_per_strip-2));
    for(std::uint32_t s=0;s<strips;++s){
        const std::uint32_t base=std::uint32_t(f.vertices.size());
        f.vertices.push_back(make_v(n++)); f.vertices.push_back(make_v(n++));
        for(std::uint32_t j=2;j<vertices_per_strip;++j){
            f.vertices.push_back(make_v(n++));
            const auto i0=base+j-2, i1=base+j-1, i2=base+j;
            const auto& a=f.vertices[i0]; const auto& b=f.vertices[i1]; const auto& c=f.vertices[i2];
            f.tris.push_back({i0,i1,i2,s&31u,a.z+b.z+c.z,order++});
        }
    }
}

static std::uint64_t build_old(const OldFrame& f, std::vector<GpuVertex>& out){
    out.clear(); out.reserve(f.tris.size()*3);
    std::uint64_t sum=0;
    for(const auto& t:f.tris){ out.push_back(gpu(t.a)); out.push_back(gpu(t.b)); out.push_back(gpu(t.c)); sum+=t.a.argb+t.b.argb+t.c.argb; }
    return sum;
}
static std::uint64_t build_new(const NewFrame& f, std::vector<GpuVertex>& out, std::vector<std::uint32_t>& idx){
    out.clear(); idx.clear(); out.reserve(f.vertices.size()); idx.reserve(f.tris.size()*3);
    std::uint64_t sum=0;
    for(const auto& v:f.vertices){ out.push_back(gpu(v)); sum+=v.argb; }
    for(const auto& t:f.tris){ idx.push_back(t.i0); idx.push_back(t.i1); idx.push_back(t.i2); }
    return sum;
}

template<class F> static double bench_ms(F&& fn, int reps){
    using C=std::chrono::steady_clock; std::vector<double> samples; samples.reserve(reps);
    for(int r=0;r<reps;++r){ auto t0=C::now(); fn(); auto t1=C::now(); samples.push_back(std::chrono::duration<double,std::milli>(t1-t0).count()); }
    std::sort(samples.begin(),samples.end()); return samples[samples.size()/2];
}

int main(){
    constexpr std::uint32_t strips=1000, vps=20; // 20k TA vertices, 18k triangles/frame
    OldFrame oldf; NewFrame newf; emit_old(oldf,strips,vps); emit_new(newf,strips,vps);
    if(oldf.tris.size()!=newf.tris.size()) return 2;
    for(std::size_t i=0;i<oldf.tris.size();++i){
        const auto& o=oldf.tris[i]; const auto& n=newf.tris[i];
        if(n.i0>=newf.vertices.size()||n.i1>=newf.vertices.size()||n.i2>=newf.vertices.size()) return 3;
        if(!same(o.a,newf.vertices[n.i0])||!same(o.b,newf.vertices[n.i1])||!same(o.c,newf.vertices[n.i2])||o.state!=n.state||o.order!=n.order) return 4;
    }
    std::vector<GpuVertex> oldgpu,newgpu; std::vector<std::uint32_t> idx;
    volatile std::uint64_t sink=0;
    const double old_reg=bench_ms([&]{emit_old(oldf,strips,vps); sink^=oldf.tris.size();},101);
    const double new_reg=bench_ms([&]{emit_new(newf,strips,vps); sink^=newf.tris.size();},101);
    const double old_build=bench_ms([&]{sink^=build_old(oldf,oldgpu);},101);
    const double new_build=bench_ms([&]{sink^=build_new(newf,newgpu,idx);},101);
    const std::size_t old_deferred=oldf.tris.size()*sizeof(OldTri);
    const std::size_t new_deferred=newf.vertices.size()*sizeof(SoftVertex)+newf.tris.size()*sizeof(NewTri);
    const std::size_t old_upload=oldf.tris.size()*3*sizeof(GpuVertex);
    const std::size_t new_upload=newf.vertices.size()*sizeof(GpuVertex)+newf.tris.size()*3*sizeof(std::uint32_t);
    std::cout << "PVR indexed equivalence: PASS\n"
              << "soft-vertex-bytes=" << sizeof(SoftVertex) << " old-tri-bytes=" << sizeof(OldTri) << " indexed-tri-bytes=" << sizeof(NewTri) << "\n"
              << "frame: vertices=" << newf.vertices.size() << " triangles=" << newf.tris.size() << " refs=" << newf.tris.size()*3 << "\n"
              << "deferred-bytes old/new=" << old_deferred << "/" << new_deferred << "\n"
              << "gpu-upload-bytes old/new=" << old_upload << "/" << new_upload << "\n"
              << "median-register-ms old/new=" << old_reg << "/" << new_reg << "\n"
              << "median-build-ms old/new=" << old_build << "/" << new_build << "\n"
              << "sink=" << sink << "\n";
    return 0;
}
