# Evil FX

**Free, open-source multi-FX for your DAW — evil crimson Frutiger Aero gloss meets the old-school internet.**
By [Xain Jaya](https://xainjaya.com). VST3 · AU · LV2 · Standalone. Windows, macOS, Linux.

![Evil FX](docs/screenshot.png)

Seventeen effects, one tidy chain — but you only see the ones you pick. Choose effects from the **FX Library** on the left and they drop into your **rack**; remove them with the X. Each card also has a power switch to bypass it without removing it. Effects that are off cost no CPU. A safety limiter at the end keeps anything from blowing up your speakers, no matter how hard you push it.

---

## The chain

Signal always flows in this order, whichever effects you add (the rack keeps them in chain order).

| # | Module | Style | What it does | Controls |
|---|--------|-------|--------------|----------|
| 1 | **Crimson Filter** | Aero | Resonant state-variable filter with an LFO sweep | Mode (LP / BP / HP), Cutoff, Reso, LFO Rate, LFO Depth |
| 2 | **Scorch Drive** | Aero | Warm, slightly asymmetric saturation with a tilt EQ and auto-gain | Drive, Tone, Mix |
| 3 | **CODEC.EXE** | Web 1.0 | 90s web audio formats: 22 / 16 / 11 / 8 / 5.5 kHz through 8-bit PCM, u-Law, A-Law or 4-bit IMA ADPCM, with or without anti-aliasing | Rate, Codec, Aliasing, Mix |
| 4 | **8BIT.EXE** | Web 1.0 | Bit-depth + sample-rate reduction | Bits, Rate, Mix |
| 5 | **DIALUP.EXE** | Web 1.0 | Mono phone line: band-limiting, carbon-mic squash, hiss, 60 Hz hum, crackle — plus optional 56k modem chatter | Band, Noise, Crackle, Mix, Modem squeal |
| 6 | **LOBITRATE.EXE** | Web 1.0 | A real spectral "codec" (STFT): bandwidth cutoff, masking threshold and random bin holes give you genuine low-kbps swirlies | Bitrate, Swirl, Mix |
| 7 | **PACKETLOSS.EXE** | Web 1.0 | VoIP packet loss in realistic bursts, concealed the way cheap voice codecs did it: robot buzz, silence or comfort noise | Loss, Packet size, Concealment, Mix |
| 8 | **BUFFERING.EXE** | Web 1.0 | Streaming-audio stutters: random slice repeats and dropouts, click-free | Chance, Slice, Dropout, Mix |
| 9 | **CORRUPT.EXE** | Web 1.0 | A damaged download: bursts of flipped bits in 16-bit samples (low bits fizz, high bits pop) plus reversed fragments | Amount, Bits, Reverse, Mix |
| 10 | **LAG.EXE** | Web 1.0 | Lag spikes: the audio slows down or freezes, then fast-forwards to catch up — always lands back in sync, zero added latency | Chance, Length, Depth, Mix |
| 11 | **JITTER.EXE** | Web 1.0 | Wow, flutter and clock drift, like a worn cassette rip or a drifting soundcard clock | Wow, Flutter, Drift, Mix |
| 12 | **Demon Chorus** | Aero | Lush stereo chorus | Rate, Depth, Feedback, Mix |
| 13 | **Boiling Phaser** | Aero | Swirly multi-stage phaser | Rate, Depth, Feedback, Mix |
| 14 | **Trem / Pan** | Aero | Tremolo that morphs into auto-pan; shape morphs sine → square | Rate, Depth, Shape, Pan |
| 15 | **Hellfire Delay** | Aero | Tape-ish stereo / ping-pong delay with tone control and tempo sync | Sync, Time, Feedback, Tone, Mix, Ping-pong |
| 16 | **Abyss Verb** | Aero | Big dark hall reverb | Size, Damp, Width, Mix |
| 17 | **SPEAKER.EXE** | Web 1.0 | Hear it on the hardware of the day: PC Speaker, Laptop, Flip Phone, Earbuds, CRT TV, Boombox | Model, Drive, Mix |
| — | **Master** | | Input / output gain, stereo width, global dry/wet, safety limiter + meter | Input, Output, Width, Mix, Limiter |

### Designed not to break

- **No animation overhead** — the UI only redraws what changes (meters, counters, the status ticker).
- **Click-free switching** — every module fades in/out over 20 ms and resets its state when it wakes up, so turning a delay on doesn't spit out an old tail.
- **Safety limiter** — peak limiter to −0.3 dBFS with a soft knee. Transparent below −2 dBFS.
- **NaN guard** — if anything ever goes non-finite, the chain resets instead of screaming.
- **Latency is reported** — Lo-Bitrate needs 512 samples of look-ahead. The plugin only reports that latency while Lo-Bitrate is on, and the global dry signal is delayed to match, so the Mix knob never comb-filters.
- **No allocation or locks on the audio thread.** Any host block size works.
- **Stress-tested** — `Tests/HeadlessTests.cpp` runs every module at defaults and both extremes (with +24 dB input), 300 rounds of random-parameter fuzzing with random block sizes, every preset, a state save/restore round-trip, and checks the Lo-Bitrate STFT reconstructs at ~98 dB SNR.

### CPU cost

Measured with `EvilFXTests --bench` (Release build, 48 kHz stereo, one desktop core). Effects that are off are skipped entirely.

| Effect | CPU | Effect | CPU |
|---|---|---|---|
| Crimson Filter | 0.07 % | PACKETLOSS.EXE | 0.03 % |
| Scorch Drive | 0.27 % | BUFFERING.EXE | 0.03 % |
| CODEC.EXE | 0.13 % | CORRUPT.EXE | 0.06 % |
| 8BIT.EXE | 0.02 % | LAG.EXE | 0.03 % |
| DIALUP.EXE | 0.23 % | JITTER.EXE | 0.09 % |
| LOBITRATE.EXE | 1.69 % | Demon Chorus | 0.12 % |
| Boiling Phaser | 0.39 % | Trem / Pan | 0.14 % |
| Hellfire Delay | 0.19 % | Abyss Verb | 0.25 % |
| SPEAKER.EXE | 0.33 % | **All 17 on** | **~4 %** |

### VISUALIZER.EXE

A WinAmp-style spectrum analyser (segmented bars with peak caps) or oscilloscope, at the bottom right. It's **off by default** and costs nothing while off: the audio thread only copies samples into a lock-free buffer while it's on *and* the window is open, and the UI only redraws the visualizer itself.

### Factory presets

Init (Clean) · Crimson Glass Pad · Blood Bath · Screensaver Nightmare · Dial-Up 1999 · Instant Messenger Call · Buffering... · Low Bitrate Rip · Webring Radio · Y2K Mall Speaker · 8-Bit Arcade Lobby · Glossy Tape Stack · RealAudio 1998 · VoIP Nightmare · Laptop Speaker Stream · Corrupted Download · Lag Spike Lobby · PC Speaker Demon · Warped Cassette Rip · CRT TV at 3AM · Total Server Meltdown

### Tips

- Loading a preset fills the rack with exactly the effects it uses. Your rack layout is saved with your DAW project.
- Double-click any knob to reset it. Click the value under a knob to type a number.
- The window resizes (corner drag) from 60% to 140%, aspect-locked.
- While the delay is tempo-synced, the Time knob fades out because it isn't being used.

---

## Building

You need **CMake 3.22+** and a C++17 compiler (Visual Studio 2022, Xcode 14+, or GCC 11+/Clang 14+). JUCE 8 is downloaded automatically on first configure (or drop a JUCE checkout into `./JUCE` to use that instead).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

Plugins land in `build/EvilFX_artefacts/Release/` (`VST3/`, `AU/`, `LV2/`, `Standalone/`).

**Linux** packages (Debian/Ubuntu):

```bash
sudo apt install build-essential cmake ninja-build libasound2-dev libjack-jackd2-dev \
  libfreetype6-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \
  libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libglu1-mesa-dev
```

In a stripped-down container without ALSA/Xrandr headers, add `-DEVIL_MINIMAL_LINUX_DEPS=ON`.

**macOS universal binary:** add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. Unsigned builds downloaded from the internet may need `xattr -cr "Evil FX.vst3"` before your DAW will load them.

**Run the tests / render the UI:**

```bash
./build/EvilFXTests_artefacts/Release/EvilFXTests                       # DSP stress test
./build/EvilFXTests_artefacts/Release/EvilFXTests --snapshot shot.png   # renders the UI
```

GitHub Actions (`.github/workflows/build.yml`) builds Windows, macOS and Linux on every push, runs the tests, and uploads the plugins as downloadable artifacts.

### Where to install

| Format | Windows | macOS | Linux |
|--------|---------|-------|-------|
| VST3 | `C:\Program Files\Common Files\VST3\` | `~/Library/Audio/Plug-Ins/VST3/` | `~/.vst3/` |
| AU | — | `~/Library/Audio/Plug-Ins/Components/` | — |
| LV2 | — | — | `~/.lv2/` |

---

## Project layout

```
Source/
  Params.h              every parameter (IDs, ranges, units) in one place
  Presets.h             factory presets
  PluginProcessor.*     the chain, master section, limiter, latency
  PluginEditor.*        window, header, FX library, rack, status bar
  Rack.h                which effects are shown in the rack (saved with the session)
  dsp/Modules.h         core effects + the Slot wrapper (power switch / fades)
  dsp/NetModules.h      the internet-era damage pack (codec, packet loss, corrupt, lag, jitter, speaker)
  ui/Visualizer.h       VISUALIZER.EXE (spectrum bars / scope)
  ui/EvilLookAndFeel.h  glossy crimson orbs + Win9x bevels
  ui/ModuleCard.h       one card per module
Tests/HeadlessTests.cpp DSP stress test + UI snapshot
```

### Adding a module

1. Add its parameters (including a `yourfx_on` switch) in `Params.h`.
2. Write a class deriving from `xfx::Module` in `dsp/Modules.h` (`prepare`, `reset`, `process(L, R, n)`).
3. Add it to the chain in the `EvilFXProcessor` constructor and to `ModuleIndex`.
4. Add its `_on` ID to `Rack.h` (bump `numModules`) and a `CardSpec` in `moduleSpecs()` in `PluginEditor.cpp` (set `retro = true` for the Web 1.0 look). It appears in the FX Library automatically.

The test suite picks up new parameters automatically in the fuzz test.

---

## License

**AGPL-3.0-or-later** — see [LICENSE](LICENSE). Evil FX is built on [JUCE](https://juce.com), which is available to open-source projects under the AGPLv3. If you want to ship a closed-source fork, you'll need a commercial JUCE license.

VST is a trademark of Steinberg Media Technologies GmbH. Audio Unit is a trademark of Apple Inc.
