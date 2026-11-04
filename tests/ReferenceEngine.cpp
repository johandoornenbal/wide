#include <set>
#include <string>
#include <cmath>
#include "shim/audioeffectx.h"
#define private public
#include "../third_party/airwindows/Wider.h"
#undef private
#include "../third_party/airwindows/Wider.cpp"
#include "../third_party/airwindows/WiderProc.cpp"
extern "C" void* referenceCreate(double rate, float a, float b, float c) {
    auto* e=new Wider(nullptr); e->setSampleRate(rate);
    e->fpdL=0x12345678u; e->fpdR=0x87654321u;
    e->setParameter(0,a); e->setParameter(1,b); e->setParameter(2,c); return e;
}
extern "C" void referenceFloat(void* e,float** b,int n) { static_cast<Wider*>(e)->processReplacing(b,b,n); }
extern "C" void referenceDouble(void* e,double** b,int n) { static_cast<Wider*>(e)->processDoubleReplacing(b,b,n); }
extern "C" void referenceDelete(void* e) { delete static_cast<Wider*>(e); }
