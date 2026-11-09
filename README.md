# Comp LA - by Loops (https://loops.co.ke)
Opto-style leveling compressor (2A / 3A style): VST3 plugin + standalone Comp LA.exe + Windows installer.

## Generate the installer .exe (every time you change the code)
1. Put this folder in a GitHub repository (free): upload/push everything, keeping the `.github` folder.
2. Open the repo > **Actions** > "Build Comp LA (Windows exe + VST3)". It runs automatically on every push,
   or press **Run workflow**. Takes ~10 minutes.
3. Open the finished run > **Artifacts** > download **CompLA-Setup-exe** > unzip > `CompLA-Setup-1.0.0.exe`.
   (`CompLA-portable-files` has the raw "Comp LA.exe" and "Comp LA.vst3" if you want them without the installer.)
4. Run the setup. The plugin goes to C:\Program Files\Common Files\VST3.
   FL Studio: Options > Manage plugins > Find installed plugins, then load **Comp LA** on a mixer insert.

To release a new version: change the version number in `CMakeLists.txt` and `installer/CompLA.iss`, push, repeat step 2-3.

## Build locally on Windows (alternative)
Install Visual Studio (Desktop development with C++), CMake and Inno Setup, then:
    cmake -S . -B build -A x64
    cmake --build build --config Release
    mkdir dist
    (copy build\LALeveler_artefacts\Release\VST3\"Comp LA.vst3" and ...\Standalone\"Comp LA.exe" into dist)
    iscc installer\CompLA.iss
The installer appears in dist-installer\.
