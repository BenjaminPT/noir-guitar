# Noir Guitar

Minimal guitar amp + effects chain. VST3 plugin and standalone app for Windows (x64).

Signal chain: Gate → Drive (tube-screamer style) → Amp (2 triode stages, tone stack, presence) → Cab (3 voicings or your own IR) → Delay (tape ping-pong) → Reverb → Output.

Plug the guitar into input 1 of any audio interface.

## Get the installer (no tools needed on your PC)
1. Create a free account on github.com, make a new empty repository, upload all these files (including the `.github` folder).
2. Open the **Actions** tab → **Build Windows installer** → **Run workflow**.
3. After ~10 minutes download `NoirGuitar-Setup.exe` from the run's Artifacts, run it.
   - VST3 is installed to `C:\Program Files\Common Files\VST3\`
   - Standalone app appears in the Start menu.
