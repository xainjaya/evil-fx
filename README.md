# Evil FX

**Free, open-source multi-FX for your DAW — crimson Frutiger Aero gloss meets the old-school internet.**
By [Xain Jaya](https://xainjaya.com). VST3 · AU · LV2 · Standalone. Windows, macOS, Linux.

![Evil FX](docs/screenshot.png)

27 effects, one tidy chain — but you only see the ones you add. Hit **+ ADD FX**, pick an effect from the categorized menu and its card drops into your **rack** (picking one that's already there jumps to it). Remove a card with its X, or bypass it with its power button. Effects that are off cost no CPU, and a safety limiter at the end keeps anything from blowing up your speakers.

---

## The chain

Signal always flows in this order, whichever effects you add.

| # | Effect | What it does | Controls |
|---|--------|--------------|----------|
| 1 | **Crimson Filter** | Resonant filter (LP / BP / HP) with an LFO sweep | Mode, Cutoff, Reso, LFO Rate, LFO Depth |
| 2 | **Glass EQ** | 3-band EQ: low shelf, sweepable mid, high shelf | Low, Mid, Mid Freq, High |
| 3 | **Scorch Drive** | Warm saturation with tilt EQ and auto-gain | Drive, Tone, Mix |
| 4 | **NIGHTCORE.EXE** | Pitch shifter, ±12 semitones | Pitch, Grain, Mix |
| 5 | **RINGTONE.EXE** | Ring modulator; Warble makes it a polyphonic-ringtone arpeggio | Freq, Shape, Warble, Mix |
| 6 | **TTS.EXE** | Robotic text-to-speech voice (metallic comb + buzz) | Pitch, Metal, Buzz, Mix |
| 7 | **CODEC.EXE** | 90s web audio: 22–5.5 kHz through 8-bit PCM, u-Law, A-Law or IMA ADPCM | Rate, Codec, Aliasing, Mix |
| 8 | **8BIT.EXE** | Bit-depth + sample-rate reduction | Bits, Rate, Mix |
| 9 | **DIALUP.EXE** | Phone line with hiss, hum, crackle and optional 56k modem squeal | Band, Noise, Crackle, Mix, Modem |
| 10 | **VINYL.EXE** | Record crackle, dust pops, worn-out highs and 33⅓ rpm wow | Crackle, Wear, Wow, Mix |
| 11 | **LOBITRATE.EXE** | Real spectral MP3-style codec artifacts ("swirlies") | Bitrate, Swirl, Mix |
| 12 | **PACKETLOSS.EXE** | VoIP dropouts in bursts — robot buzz, silence or comfort noise | Loss, Packet, Concealment, Mix |
| 13 | **BUFFERING.EXE** | Streaming stutters and dropouts | Chance, Slice, Dropout, Mix |
| 14 | **CORRUPT.EXE** | Bit flips and reversed fragments, like a damaged download | Amount, Bits, Reverse, Mix |
| 15 | **LAG.EXE** | Lag spikes: slows down, then fast-forwards to catch up | Chance, Length, Depth, Mix |
| 16 | **JITTER.EXE** | Wow, flutter and clock drift | Wow, Flutter, Drift, Mix |
| 17 | **GLITCH.EXE** 🎵 | Beat repeat / stutter / reverse / tape stop on the grid | Division, Style, Chance, Mix |
| 18 | **GATE.EXE** 🎵 | Rhythmic gate with 8 patterns | Pattern, Division, Depth, Smooth |
| 19 | **PUMP.EXE** 🎵 | Sidechain-style pumping on 1/2 to 1/16 notes | Rate, Depth, Release |
| 20 | **Angel Chorus** | Lush stereo chorus | Rate, Depth, Feedback, Mix |
| 21 | **Cloud Phaser** | Multi-stage phaser | Rate, Depth, Feedback, Mix |
| 22 | **Trem / Pan** | Tremolo that morphs into auto-pan | Rate, Depth, Shape, Pan |
| 23 | **Stereo Spread** | Mono-safe widener with slow motion | Width, Motion, Mix |
| 24 | **Halo Delay** 🎵 | Stereo / ping-pong delay with tone and tempo sync | Sync, Time, Feedback, Tone, Mix |
| 25 | **Cathedral Verb** | Big hall reverb | Size, Damp, Width, Mix |
| 26 | **Glory Comp** | Easy compressor with auto makeup | Squash, Ratio, Speed, Mix |
| 27 | **SPEAKER.EXE** | PC Speaker, Laptop, Flip Phone, Earbuds, CRT TV, Boombox | Model, Drive, Mix |
| — | **Master** | Input / output gain, stereo width, global mix, safety limiter + meter | |

🎵 = locks to your project tempo. With the transport stopped it keeps running on its own clock at the project BPM, so you can still hear it.

## Features

- **+ ADD FX menu** — effects grouped into Tone & Dynamics, Pitch & Voice, Lo-Fi & Internet Damage, Rhythm (BPM synced), and Space & Motion. Ticks show what's already in your rack.
- **Full-width rack** — 5 cards across, scrolls when you add more, with a "more FX below" button and a glow on the card you just added or jumped to.
- **48 factory presets** in categories: Heavenly, Internet Era, Rhythm & Glitch, Lo-Fi & Speakers, Chaos.
- **Save your own presets** — hit **SAVE**. They're stored in `Documents/Evil FX/Presets` and appear under *Your Presets*. Share the `.evilfx` files with friends.
- **RANDOM** — switches on a few random effects with random settings.
- **Themes** — Frutiger Metro (default), Crimson Grid, Aero Sky and Midnight, saved with your project.

![Themes](docs/themes.png)

- **VISUALIZER.EXE** — WinAmp-style spectrum bars or scope. Off by default; zero cost while off.
- **Lightweight** — the background is drawn once per theme, the UI only redraws what changes, and the audio thread never allocates.
- **Safe** — click-free switching, latency reported to your DAW, NaN guard, and a safety limiter.
- **? button** — quick help inside the plugin.

## CPU cost

Measured with `EvilFXTests --bench` (Release, 48 kHz stereo, one desktop core). Effects that are off are skipped entirely.

| Effect | CPU | Effect | CPU | Effect | CPU |
|---|---|---|---|---|---|
| Crimson Filter | 0.06 % | DIALUP.EXE | 0.23 % | PUMP.EXE | 0.08 % |
| Glass EQ | 0.07 % | VINYL.EXE | 0.14 % | Angel Chorus | 0.12 % |
| Scorch Drive | 0.21 % | LOBITRATE.EXE | 1.73 % | Cloud Phaser | 0.40 % |
| NIGHTCORE.EXE | 0.14 % | PACKETLOSS.EXE | 0.03 % | Trem / Pan | 0.14 % |
| RINGTONE.EXE | 0.22 % | BUFFERING.EXE | 0.05 % | Stereo Spread | 0.08 % |
| TTS.EXE | 0.27 % | CORRUPT.EXE | 0.06 % | Halo Delay | 0.19 % |
| CODEC.EXE | 0.13 % | LAG.EXE | 0.03 % | Cathedral Verb | 0.25 % |
| 8BIT.EXE | 0.02 % | JITTER.EXE | 0.09 % | Glory Comp | 0.13 % |
| GLITCH.EXE | 0.03 % | GATE.EXE | 0.03 % | SPEAKER.EXE | 0.33 % |
| | | | | **All 27 on** | **~5 %** |

## Building

You need **CMake 3.22+** and a C++17 compiler (Visual Studio 2022, Xcode 14+, or GCC 11+/Clang 14+). JUCE 8 is downloaded automatically.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Plugins land in `build/EvilFX_artefacts/Release/` (`VST3/`, `AU/`, `LV2/`, `Standalone/`).

GitHub Actions (`.github/workflows/build.yml`) builds Windows, macOS and Linux on every push, runs the tests and uploads the plugins as artifacts.

**Linux packages:** `build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev` (or add `-DEVIL_MINIMAL_LINUX_DEPS=ON` in a stripped-down container).

**macOS:** add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` for a universal build. Unsigned builds may need `xattr -cr "Evil FX.vst3"`.

### Installing

| Format | Windows | macOS | Linux |
|--------|---------|-------|-------|
| VST3 | `C:\Program Files\Common Files\VST3\` (copy the whole `Evil FX.vst3` folder) | `~/Library/Audio/Plug-Ins/VST3/` | `~/.vst3/` |
| AU | — | `~/Library/Audio/Plug-Ins/Components/` | — |
| LV2 | — | — | `~/.lv2/` |

**FL Studio:** Options → Manage plugins → make sure the VST3 folder is listed → tick *Verify plugins* → *Find installed plugins* → star **Evil FX**.

---

## Project layout

```
Source/
  Params.h              every parameter (IDs, ranges, units)
  Rack.h                chain order + which effects are shown in the rack
  Presets.h             factory presets, user presets, randomize
  PluginProcessor.*     the chain, transport/BPM, master, limiter, latency
  PluginEditor.*        window, + ADD FX menu, rack, themes, presets UI
  dsp/Modules.h         core effects + Slot wrapper (power switch / fades)
  dsp/NetModules.h      internet-era pack (codec, packet loss, corrupt, lag, jitter, speaker)
  dsp/BeatModules.h     glitch, gate, nightcore, ringtone, glory comp
  dsp/ExtraModules.h    glass EQ, vinyl, TTS, pump, stereo spread
  ui/EvilLookAndFeel.h  glossy crimson orbs + Win9x bevels
  ui/ModuleCard.h       one card per effect
  ui/Visualizer.h       VISUALIZER.EXE
Tests/HeadlessTests.cpp stress test, fuzzing, presets, CPU bench, UI snapshot
```

### Adding an effect

1. Add its parameters (including `yourfx_on`) in `Params.h`.
2. Write a class deriving from `xfx::Module` (`prepare`, `reset`, `process(L, R, n)`).
3. Add it in the `EvilFXProcessor` constructor, `ModuleIndex`, and `Rack.h` (same position, bump `numModules`).
4. Add a `CardSpec` in `moduleSpecs()` in `PluginEditor.cpp` (same position) and give it a menu category in `categoryOf()`.

## License

**AGPL-3.0-or-later** — see [LICENSE](LICENSE). Built on [JUCE](https://juce.com) under its AGPLv3 option.

VST is a trademark of Steinberg Media Technologies GmbH. Audio Unit is a trademark of Apple Inc.
