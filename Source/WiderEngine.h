#pragma once
#include <cmath>
#include <cstdint>
#include <cstdlib>
class WiderEngine {
public:
    WiderEngine() {
        fpdL=1; while(fpdL<16386) fpdL=std::rand()*UINT32_MAX;
        fpdR=1; while(fpdR<16386) fpdR=std::rand()*UINT32_MAX;
    }
    void reset(double rate) { *this = WiderEngine{}; sampleRate = rate; }
    void parameters(float width, float center, float mix) { A=width; B=center; C=mix; }
    void processReplacing(float**, float**, int);
    void processDoubleReplacing(double**, double**, int);
private:
    friend struct WiderTestAccess;
    double getSampleRate() const { return sampleRate; }
    double sampleRate=44100;
    double p[4099]{};
    int count=0;
    uint32_t fpdL=1, fpdR=1;
    float A=.5f, B=.5f, C=1.f;
};
