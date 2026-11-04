#include "WiderEngine.h"
#include <iostream>
int main() {
    WiderEngine e; e.reset(48000);
    double l[512],r[512];double* data[]={l,r};
    for(int mode=0;mode<2;++mode) {
        e.reset(48000);e.parameters(.5f,.5f,static_cast<float>(mode));
        for(int i=0;i<512;++i) { l[i]=.2;r[i]=-.1; }
        e.processDoubleReplacing(data,data,512);
        for(int i=0;i<512;++i) if(std::abs(l[i]-(mode?.3:.2))>1.e-12 || std::abs(r[i]-(mode?-.15:-.1))>1.e-12) return 1;
    }
    std::cout<<"Dry identity and original neutral full-wet 1.5x law passed\n";
}
