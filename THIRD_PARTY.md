# Attribution and provenance

- Airwindows Wider: Chris Johnson / Airwindows, MIT. Original files under
  `third_party/airwindows`, from https://github.com/airwindows/airwindows,
  commit `7fd5f5cae262a41e6dd7d9a83d60c46e84d3092a`,
  `plugins/LinuxVST/src/Wider`. All three source files are preserved verbatim.
  `Source/WiderEngine.cpp` compiles the original processing file via a host adapter.
  Original random seed initialization, sample processing, parameter defaults
  and algorithm are retained. Tests alone fix the seeds for comparison.
- UI/look-and-feel and project structure adapted from Johan Doornenbal's
  MIT-licensed ButterComp Studio (`3d2daf0d2f350d73640bd03d3f9210cd1b09bbf6`)
  and informed by ToTape Studio (`89ab75645dba97d44f335a1e21acc2ed96e74069`).
  Their MIT copyright notice is retained in LICENSE.
- JUCE 8.0.12, commit `29396c22c93392d6738e021b83196283d6e4d850`,
  https://github.com/juce-framework/JUCE. Separately licensed under AGPLv3 or
  commercial terms. This project's MIT license does not relicense JUCE.
- `tests/shim/audioeffectx.h` is a minimal test-only interface from ToTape Studio;
  it contains no Steinberg SDK source and is not part of the shipped plugin.
