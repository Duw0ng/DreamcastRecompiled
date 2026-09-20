#include <array>
#include <bit>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <vector>

struct State { std::uint32_t base{0xD0806040u}, offs{0xA04080C0u}; std::uint8_t source{2u}; };
struct V { float x{},y{},z{},u{},v{}; std::uint32_t argb{0xFFFFFFFFu},offs{}; std::uint8_t source{}; };
static std::uint32_t u32(const std::uint8_t* p){ std::uint32_t v; std::memcpy(&v,p,4); return v; }
static float f32(const std::uint8_t* p){ return std::bit_cast<float>(u32(p)); }
static std::uint32_t iu(std::uint32_t face,float intensity){
    auto clamp=[](float x){ if(x<=0) return 0u; if(x>=1) return 255u; return std::uint32_t(x*255.0f+0.5f);};
    const auto i=clamp(intensity), a=(face>>24)&255u, r=(((face>>16)&255u)*i)/256u,
               g=(((face>>8)&255u)*i)/256u, b=((face&255u)*i)/256u;
    return (a<<24)|(r<<16)|(g<<8)|b;
}
static void uv16(const std::uint8_t* p,float& u,float& v){
    std::uint16_t vh=std::uint16_t(p[0])|(std::uint16_t(p[1])<<8), uh=std::uint16_t(p[2])|(std::uint16_t(p[3])<<8);
    u=std::bit_cast<float>(std::uint32_t(uh)<<16); v=std::bit_cast<float>(std::uint32_t(vh)<<16);
}
static V base(const State&s,const std::uint8_t*p){V v{};v.source=s.source;v.x=f32(p+4);v.y=f32(p+8);v.z=f32(p+12);return v;}
static V generic(const State&s,const std::uint8_t*p,std::uint8_t t){
    V v=base(s,p); switch(t){
    case 0:v.argb=u32(p+24);break; case 1:v.argb=u32(p+16);break; case 2:v.argb=iu(s.base,f32(p+24));break;
    case 3:v.u=f32(p+16);v.v=f32(p+20);v.argb=u32(p+24);v.offs=u32(p+28);break;
    case 4:uv16(p+16,v.u,v.v);v.argb=u32(p+24);v.offs=u32(p+28);break;
    case 5:v.u=f32(p+16);v.v=f32(p+20);break; case 6:uv16(p+16,v.u,v.v);break;
    case 7:v.u=f32(p+16);v.v=f32(p+20);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));break;
    case 8:uv16(p+16,v.u,v.v);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));break;
    case 9:v.argb=u32(p+16);break; case 10:v.argb=iu(s.base,f32(p+16));break;
    case 11:v.u=f32(p+16);v.v=f32(p+20);v.argb=u32(p+24);v.offs=u32(p+28);break;
    case 12:uv16(p+16,v.u,v.v);v.argb=u32(p+24);v.offs=u32(p+28);break;
    case 13:v.u=f32(p+16);v.v=f32(p+20);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));break;
    case 14:uv16(p+16,v.u,v.v);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));break; }
    return v;
}
template<int T> static V spec(const State&s,const std::uint8_t*p){
    V v=base(s,p);
    if constexpr(T==0)v.argb=u32(p+24); else if constexpr(T==1)v.argb=u32(p+16); else if constexpr(T==2)v.argb=iu(s.base,f32(p+24));
    else if constexpr(T==3){v.u=f32(p+16);v.v=f32(p+20);v.argb=u32(p+24);v.offs=u32(p+28);} else if constexpr(T==4){uv16(p+16,v.u,v.v);v.argb=u32(p+24);v.offs=u32(p+28);}
    else if constexpr(T==5){v.u=f32(p+16);v.v=f32(p+20);} else if constexpr(T==6)uv16(p+16,v.u,v.v);
    else if constexpr(T==7){v.u=f32(p+16);v.v=f32(p+20);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));} else if constexpr(T==8){uv16(p+16,v.u,v.v);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));}
    else if constexpr(T==9)v.argb=u32(p+16); else if constexpr(T==10)v.argb=iu(s.base,f32(p+16));
    else if constexpr(T==11){v.u=f32(p+16);v.v=f32(p+20);v.argb=u32(p+24);v.offs=u32(p+28);} else if constexpr(T==12){uv16(p+16,v.u,v.v);v.argb=u32(p+24);v.offs=u32(p+28);}
    else if constexpr(T==13){v.u=f32(p+16);v.v=f32(p+20);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));} else if constexpr(T==14){uv16(p+16,v.u,v.v);v.argb=iu(s.base,f32(p+24));v.offs=iu(s.offs,f32(p+28));}
    return v;
}
using Fn=V(*)(const State&,const std::uint8_t*);
static Fn pick(int t){ static constexpr Fn f[]={spec<0>,spec<1>,spec<2>,spec<3>,spec<4>,spec<5>,spec<6>,spec<7>,spec<8>,spec<9>,spec<10>,spec<11>,spec<12>,spec<13>,spec<14>}; return (t>=0&&t<15)?f[t]:nullptr; }
static bool same(const V&a,const V&b){return std::memcmp(&a.x,&b.x,5*sizeof(float))==0&&a.argb==b.argb&&a.offs==b.offs&&a.source==b.source;}
int main(){
    State s; std::array<std::uint8_t,64> p{}; auto put=[&](int o,float f){auto x=std::bit_cast<std::uint32_t>(f);std::memcpy(p.data()+o,&x,4);};
    put(4,123.25f);put(8,77.5f);put(12,.625f);put(16,.25f);put(20,.75f);put(24,.5f);put(28,.875f);
    for(int t=0;t<15;++t){ if(!same(generic(s,p.data(),t),pick(t)(s,p.data()))) return 2+t; }
    constexpr int N=3000000; volatile std::uint64_t sink=0; auto once=[&](bool specialized){ auto t0=std::chrono::steady_clock::now(); Fn fn=pick(3); for(int i=0;i<N;++i){V v=specialized?fn(s,p.data()):generic(s,p.data(),3);sink+=v.argb+std::uint32_t(v.x);} auto t1=std::chrono::steady_clock::now(); return std::chrono::duration<double,std::milli>(t1-t0).count();};
    auto median=[&](bool specialized){std::vector<double> x; x.reserve(21); for(int i=0;i<21;++i)x.push_back(once(specialized)); std::sort(x.begin(),x.end()); return x[x.size()/2];};
    double g=median(false), sp=median(true); std::cout<<"PVR specialized decoder equivalence: PASS\n"<<"type3-iterations="<<N<<" median-generic-ms="<<g<<" median-specialized-ms="<<sp<<" sink="<<sink<<"\n"; return 0;
}
