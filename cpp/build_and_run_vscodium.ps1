# Build and launch Game Vroum from a normal VS Codium PowerShell terminal.
$ErrorActionPreference = 'Stop'

$source = Split-Path -Parent $MyInvocation.MyCommand.Path
$vswhere = Join-Path \${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe est introuvable. Verifie l'installation de Visual Studio Build Tools 2026."
}

$installPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $installPath) {
    throw "Impossible de trouver Visual Studio avec les outils C++ MSVC."
}

$vsDevCmd = Join-Path $installPath 'Common7\Tools\VsDevCmd.bat'
if (-not (Test-Path $vsDevCmd)) {
    throw "VsDevCmd.bat est introuvable dans $installPath."
}

Write-Host "Configuration de l'environnement MSVC..." -ForegroundColor Cyan
$command = '"' + $vsDevCmd + '" -no_logo -arch=x64 -host_arch=x64 && set'
$environmentOutput = & $env:ComSpec /d /s /c "\`"$command\`""
if ($LASTEXITCODE -ne 0) {
    throw "Impossible d'initialiser l'environnement Visual Studio."
}

foreach ($line in $environmentOutput) {
    $separator = $line.IndexOf('=')
    if ($separator -gt 0) {
        $name = $line.Substring(0, $separator)
        $value = $line.Substring($separator + 1)
        [Environment]::SetEnvironmentVariable($name, $value, 'Process')
    }
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue) -or
    -not (Get-Command nmake.exe -ErrorAction SilentlyContinue)) {
    throw "MSVC ou NMake n'est toujours pas accessible apres l'initialisation."
}
if (-not (Get-Command cmake.exe -ErrorAction SilentlyContinue)) {
    throw "CMake est absent du PATH."
}

$build = Join-Path $source 'build-vscodium'
Write-Host "Compilation de Game Vroum..." -ForegroundColor Cyan
cmake -S $source -B $build -G "NMake Makefiles"
if ($LASTEXITCODE -ne 0) { throw "Configuration CMake echouee." }

cmake --build $build --parallel
if ($LASTEXITCODE -ne 0) { throw "Compilation du jeu echouee." }

$exe = Join-Path $build 'karting_cpp.exe'
if (-not (Test-Path $exe)) {
    throw "Impossible de trouver karting_cpp.exe dans $build."
}

Write-Host "Lancement de Game Vroum !" -ForegroundColor Green
& $exe
exit $LASTEXITCODE
