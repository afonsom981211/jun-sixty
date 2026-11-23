# Sixty — Juno-60 Inspired Synthesizer

**An AI-assisted, vibe-coded synthesizer built with Claude, C++ and JUCE 8.**

Sixty is an experimental polyphonic synthesizer inspired by the legendary Roland Juno-60.

This project started as a Vibe Coding experiment using Claude. I'm not an experienced C++ programmer or DSP engineer. The initial synthesizer was developed with AI assistance, and has since evolved through experimentation, testing and research using the original Roland Juno-60 Service Notes and circuit schematics.

## 🎯 The Goal

The ambition is to bring Sixty as close as possible to Arturia's Juno-60 emulation in terms of sound, behaviour and overall experience.

I'm looking for experienced C++ developers, DSP engineers and synthesizer enthusiasts who can help take this project further.

**If you know more about synthesizer development than I do, this project needs you!**

The idea is to make Sixty a community-driven project where people can explore the code, improve the DSP, experiment with circuit modelling and contribute their expertise.

## 🎹 Features

### Main synthesis engine
- 6-voice polyphony.
- DCO architecture inspired by the original Juno-60.
- Sawtooth and pulse waveforms.
- Manual, LFO and envelope-based PWM.
- Sub-oscillator and noise generator.
- PolyBLEP anti-aliasing.
- Global oscillator drift.

### Filters and modulation
- 24 dB/octave low-pass ladder filter.
- Resonance with self-oscillation.
- Keyboard tracking.
- Bipolar filter envelope.
- Four-position high-pass filter.
- ADSR envelope with exponential response.
- Global LFO with adjustable delay.
- LFO modulation of pitch, filter and PWM.
- Velocity, modulation wheel and pitch bend.

### Additional features
- Three-voice-independent chorus processing architecture inspired by the original Juno-60 BBD design.
- Arpeggiator.
- Second LFO with multiple waveforms.
- Second ADSR envelope.
- Modulation matrix.
- Delay and reverb effects.
- Adjustable voice count.
- Resizable interface.
- On-screen keyboard and MIDI support.

## 🔬 Circuit Research

An important part of this project has been researching the original Roland Juno-60 circuitry.

The original Roland Juno-60 Service Notes and circuit schematics have been used as references to improve the synthesizer's behaviour.

Some examples of corrections made through this research:

- Corrected the global oscillator drift model to reflect the shared master clock architecture.
- Adjusted LFO frequency and delay ranges.
- Refined pitch modulation and tuning ranges.
- Added envelope as an additional PWM modulation source.
- Recalculated HPF cutoff frequencies using the original circuit values.
- Refined the chorus architecture and modulation characteristics.
- Adjusted the filter modulation ranges.

Not every circuit parameter has been fully verified, and some aspects remain approximations.

## ⚠️ Known Limitations & Development Areas

Sixty is still experimental. Some parts of the synthesizer need substantial DSP work.

In particular:

- The low-pass filter is currently a generic ladder model, not an accurate IR3109 circuit emulation.
- The filter does not yet use oversampling.
- Chorus BBD delay characteristics still need further verification.
- Some chorus modulation parameters are based on circuit interpretations and estimates.
- The original Juno-60 bender also affects the filter, which is not yet implemented.
- Chorus mode selection and behaviour need further refinement.
- Preset management (save, load, import and export) is still to be implemented.

## 🛠 Technology

- C++17
- JUCE 8
- CMake
- PolyBLEP oscillator anti-aliasing
- GitHub Actions

### Supported formats

- VST3 — Windows, macOS and Linux
- AU — macOS
- Standalone — Windows, macOS and Linux

## 🚀 Building

### Requirements

- CMake 3.22 or newer
- Git
- C++17 compatible compiler
- Internet connection for the initial JUCE download

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On Windows, use the Visual Studio generator if necessary:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

The generated binaries are located in:

```text
build/Sixty_artefacts/Release/
```

### Linux dependencies (Debian/Ubuntu)

```bash
sudo apt install build-essential cmake git \
libasound2-dev libfreetype-dev libfontconfig1-dev \
libx11-dev libxcomposite-dev libxcursor-dev \
libxext-dev libxinerama-dev libxrandr-dev \
libxrender-dev libgl1-mesa-dev mesa-common-dev
```

## 🤝 Contributions

Contributions are very welcome!

I'm particularly interested in developers who have experience with:

- Analog synthesizer circuit modelling.
- IR3109 filter emulation.
- BBD chorus modelling.
- DCO architecture and oscillator behaviour.
- DSP accuracy and optimization.
- Oversampling and nonlinear processing.
- C++ and JUCE plugin development.
- Preset management and plugin architecture.

You can contribute by opening issues, suggesting improvements, submitting pull requests or experimenting with your own modifications.

Don't worry if you only have a small improvement to suggest. Every contribution helps.

## 📜 Disclaimer

Sixty is an independent experimental project inspired by the Roland Juno-60.

It is not affiliated with, endorsed by or connected to Roland Corporation or Arturia.

All trademarks belong to their respective owners.

## Licensing

Sixty uses JUCE 8 and is subject to the applicable JUCE licensing terms.

The project's own distribution and licensing arrangements need to be established before redistribution.

---

**Built with Claude, curiosity and a lot of Vibe Coding.**

*The code is here. The ambition is clear. Now let's see what the community can make of it.*
