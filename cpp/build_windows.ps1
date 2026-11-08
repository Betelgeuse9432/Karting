# Compile et lance la version native C++20 / raylib de Karting V6.
# A lancer depuis n'importe quel repertoire : powershell -ExecutionPolicy Bypass -File cpp/build_windows.ps1
$ErrorActionPreference = 'Stop'
$source = Split-Path -Parent $MyInvocation.MyCommand.Path
$build = Join-Path $source 'build'
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake n'est pas installe ou absent du PATH."
}
cmake -S $source -B $build
if ($LASTEXITCODE -ne 0) { throw "Configuration CMake echouee." }
cmake --build $build --config Release --parallel
if ($LASTEXITCODE -ne 0) { throw "Compilation du jeu echouee." }
$executables = @(
    (Join-Path $build 'Release/karting_cpp.exe'),
    (Join-Path $build 'karting_cpp.exe')
)
$exe = $executables | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $exe) { throw "Impossible de trouver karting_cpp.exe dans $build." }
& $exe
exit $LASTEXITCODE
