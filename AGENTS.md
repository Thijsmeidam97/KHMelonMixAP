# MelonMix Agent Instructions

## Project rules

- Only add or copy Lua scripts into `AP documentations/Lua for notes` when the user explicitly asks for an actual note and the Lua script is part of that note. Do not archive standalone Lua scripts there.
- Always verify game addresses against the matching regional decomp before using or changing them. Do not infer addresses from other scripts or logs; treat those only as leads to verify in the decomp.
- `on_bus`, `on_bus_exec`, `on_memory`, and equivalent memory/CPU execution hooks are illegal for this project. Use frame-polled APIs such as `event.onframestart` instead.
- Do not change project or application version fields unless the user explicitly requests a version change.

## Windows build

The current checkout uses Qt6 APIs and must be built with the MSYS2 UCRT64 Qt6 toolchain. The existing `build.ps1` contains legacy Bash commands for static Qt5 and should not be run directly from PowerShell.

The verified local toolchain paths are:

```text
C:\mm-msys64
C:\mm-qt6       # no-space staging copy of the repository
C:\mb-qt6       # build directory
```

Because the repository path contains spaces, stage it in a no-space directory before invoking MinGW tools. Keep the staging copy separate from the Git checkout:

```powershell
$repo = 'E:\khdays ap\melonmix'
$stage = 'C:\mm-qt6'
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage | Out-Null
robocopy $repo $stage /E /XD (Join-Path $repo '.git') (Join-Path $repo 'build') /NFL /NDL /NJH /NJS /NP | Out-Null

$env:MSYSTEM = 'UCRT64'
$env:CHERE_INVOKING = '1'
& 'C:\mm-msys64\usr\bin\bash.exe' --login -lc "cd /c/mm-qt6 && cmake -S . -B /c/mb-qt6 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DUSE_QT6=ON -DBUILD_STATIC=OFF -DMELONDS_EMBED_BUILD_INFO=OFF && cmake --build /c/mb-qt6"
```

The executable is produced as `C:\mb-qt6\melonDS.exe`. Confirm that its version remains unchanged before packaging:

```powershell
[System.Diagnostics.FileVersionInfo]::GetVersionInfo('C:\mb-qt6\melonDS.exe') |
    Select-Object FileVersion, ProductVersion, ProductName
```

## Package and extract the ZIP

The Windows package is stored in the repository build directory as:

```text
E:\khdays ap\melonmix\build\MelonMix-windows-x86_64.zip
```

Extract it for testing or distribution to `E:\khdays ap\MelonMix Build`:

```powershell
$zip = 'E:\khdays ap\melonmix\build\MelonMix-windows-x86_64.zip'
$destination = 'E:\khdays ap\MelonMix Build'
if (-not (Test-Path -LiteralPath $destination)) {
    New-Item -ItemType Directory -Path $destination | Out-Null
}
Expand-Archive -LiteralPath $zip -DestinationPath $destination -Force
```

The packaged executable should then be at:

```text
E:\khdays ap\MelonMix Build\MelonMix.exe
```

## Native connector style

Use `melonmix/src/plugins/APConnector/MissionRewards.cpp` as the model for
native connector additions:

- Define verified game addresses as file-local constants near the top of the
  source file. Do not derive an address from another script or runtime log.
- Put connector work in a `Ctx` member inside `namespace Plugins::APC`.
- Keep the frame poll direct: read with `r8`, `r16`, or `r32`, make one clear
  decision, and write with `w8`, `w16`, or `w32` only when the state changes.
- For flag bytes, preserve every unrelated bit: read into a short `bflags`
  value, test the target mask, set it with `bflags | mask`, then write the
  complete byte back. Use a short boolean such as `set` for the decision.
- Keep names consistent with the existing file: uppercase address and flag
  constants, short lowercase locals, and concise `Ctx` poll member names.
- Keep the implementation small and frame-polled. Do not add CPU or memory
  execution hooks, hidden state machines, or unnecessary fallback paths.
