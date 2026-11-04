#include "WiderEngine.h"
#include <vector>
#include <iostream>
#include <type_traits>
struct WiderTestAccess { static void seed(WiderEngine& e) { e.fpdL=0x12345678u; e.fpdR=0x87654321u; } };
extern "C" void* referenceCreate(double,float,float,float);
extern "C" void referenceFloat(void*,float**,int);
extern "C" void referenceDouble(void*,double**,int);
extern "C" void referenceDelete(void*);
template<class T> bool check(double rate,float a,float b,float mix) {
    WiderEngine e; e.reset(rate); WiderTestAccess::seed(e); e.parameters(a,b,mix);
    void* ref=referenceCreate(rate,a,b,mix);
    for(int block=0;block<24;++block) {
        const int n=block%3==0?1:block%3==1?127:512;
        std::vector<T> l(n),r(n);
        for(int i=0;i<n;++i) { l[i]=block<3?T(0):T(.7*std::sin((block*512+i)*.073)); r[i]=block<3?T(0):T(.3*std::cos((block*512+i)*.037)); }
        auto rl=l,rr=r; T* x[]={l.data(),r.data()};T* y[]={rl.data(),rr.data()};
        if constexpr(std::is_same_v<T,float>) { e.processReplacing(x,x,n); referenceFloat(ref,y,n); }
        else { e.processDoubleReplacing(x,x,n);referenceDouble(ref,y,n); }
        for(int i=0;i<n;++i) if(l[i]!=rl[i] || r[i]!=rr[i]) { referenceDelete(ref); return false; }
    }
    referenceDelete(ref); return true;
}
int main() {
    int count=0;
    for(double rate:{44100.,48000.,96000.,192000.}) for(float a:{0.f,.5f,1.f}) for(float b:{0.f,.5f,1.f}) for(float c:{0.f,.5f,1.f}) {
        if(!check<float>(rate,a,b,c)||!check<double>(rate,a,b,c)) return 1;
        count+=2;
    }
    std::cout<<count<<" bit-exact upstream comparisons passed\n";
}
