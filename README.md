# Noir Guitar

Minimal guitar amp + effects chain. VST3 plugin and standalone app for Windows (x64) and macOS.

Signal chain: Gate → Drive (tube-screamer style) → Amp (2 triode stages, tone stack, presence) → Cab (3 voicings or your own IR) → Delay (tape ping-pong) → Reverb → Output.

Plug the guitar into input 1 of any audio interface.

## Download (latest version)

| System | Direct link |
|---|---|
| **Windows** (x64, VST3 + standalone) | [NoirGuitar-Setup.exe](https://github.com/BenjaminPT/noir-guitar/releases/latest/download/NoirGuitar-Setup.exe) |
| **macOS** (Apple Silicon + Intel, VST3 + AU + app) | [NoirGuitar-macOS.pkg](https://github.com/BenjaminPT/noir-guitar/releases/latest/download/NoirGuitar-macOS.pkg) |

All versions: [Releases](https://github.com/BenjaminPT/noir-guitar/releases).

- Windows: VST3 goes to `C:\Program Files\Common Files\VST3\`, the standalone app appears in the Start menu.
- macOS: VST3 to `/Library/Audio/Plug-Ins/VST3`, AU to `/Library/Audio/Plug-Ins/Components`, app to `/Applications`. The package is not notarized: right-click the .pkg -> Open the first time.

A new release is built automatically on every push to `main`.

## Multi-track looper
Up to 30 independent tracks, up to 20 s each, looping until stopped. Drag the handles at both ends of a waveform to trim, double-click a waveform to reset the trim, mouse wheel scrolls the track list.

## Build it yourself
Fork the repo and push: GitHub Actions builds both installers (Actions tab -> latest run -> Artifacts, or the Releases page).
