# Build and launch Game Vroum from a normal VS Codium PowerShell terminal.
$ErrorActionPreference = 'Stop'

$source = Split-Path -Parent $MyInvocation.MyCommand.Path
$programFilesX86 = [Environment]::GetFolderPath('ProgramFilesX86')
$vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'vswhere.exe introuvable. Verifie Visual Studio Build Tools 2026.' }
$installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installPath) { throw 'Visual Studio avec les outils C++ MSVC introuvable.' }
$vsDevCmd = Join-Path $installPath 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) { throw "VsDevCmd.bat introuvable dans $installPath." }

Write-Host 'Configuration de l’environnement MSVC...' -ForegroundColor Cyan
$tempCmd = Join-Path $env:TEMP 'game-vroum-vsenv.cmd'
$tempEnv = Join-Path $env:TEMP 'game-vroum-vsenv.txt'
$cmdLines = @('@echo off', ('call "' + $vsDevCmd + '" -no_logo -arch=x64 -host_arch=x64'), 'if errorlevel 1 exit /b 1', ('set > "' + $tempEnv + '"'))
$cmdLines | Set-Content -Path $tempCmd -Encoding ASCII
& $env:ComSpec /d /c "`"$tempCmd`""
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $tempEnv)) { throw 'Impossible d’initialiser l’environnement Visual Studio.' }

foreach ($line in Get-Content $tempEnv) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $name = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        [Environment]::SetEnvironmentVariable($name, $value, 'Process')
    }
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue) -or -not (Get-Command nmake.exe -ErrorAction SilentlyContinue)) { throw 'MSVC ou NMake inaccessible apres initialisation.' }
if (-not (Get-Command cmake.exe -ErrorAction SilentlyContinue)) { throw 'CMake est absent du PATH.' }

$build = Join-Path $source 'build-vscodium'
Write-Host 'Compilation de Game Vroum...' -ForegroundColor Cyan
cmake -S $source -B $build -G 'NMake Makefiles'
if ($LASTEXITCODE -ne 0) { throw 'Configuration CMake echouee.' }
cmake --build $build --parallel
if ($LASTEXITCODE -ne 0) { throw 'Compilation du jeu echouee.' }

$exe = Join-Path $build 'karting_cpp.exe'
if (-not (Test-Path $exe)) { throw "Impossible de trouver karting_cpp.exe dans $build." }
Write-Host 'Lancement de Game Vroum !' -ForegroundColor Green
& $exe
exit $LASTEXITCODE