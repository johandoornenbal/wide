// Compile the unmodified upstream DSP through a minimal host adapter.
#include "WiderEngine.h"
#define __Wider_H
#define Wider WiderEngine
#define VstInt32 int
#include "../third_party/airwindows/WiderProc.cpp"
#undef VstInt32
#undef Wider
#undef __Wider_H
