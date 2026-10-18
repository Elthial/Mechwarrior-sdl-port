param(
    [ValidateSet('Debug','Release')][string]$Configuration = 'Release',
    [ValidateSet('Build','Rebuild','Clean')][string]$Action = 'Build',
    [string]$Target = '',
    [string]$SDL3Directory = $env:SDL3_DIR,
    [string]$SDLSourceDirectory = $env:MECHWARRIOR_SDL_SOURCE_DIR,
    [switch]$Headless
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$build = Join-Path $projectRoot $(if ($Headless) { 'build-headless' } else { 'build-visual-studio' })
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Install Visual Studio Desktop development with C++ and CMake tools.' }
$cmake = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
if ($Action -eq 'Clean') {
    if (Test-Path (Join-Path $build 'CMakeCache.txt')) { & $cmake --build $build --config $Configuration --target clean }
    if ($LASTEXITCODE) { throw 'CMake clean failed.' }; exit 0
}
$options = @('-S',$projectRoot,'-B',$build,'-A','x64',"-DMECHWARRIOR_BUILD_SDL=$(if ($Headless) {'OFF'} else {'ON'})")
if ($SDLSourceDirectory) { $options += "-DMECHWARRIOR_SDL_SOURCE_DIR=$SDLSourceDirectory" }
elseif ($SDL3Directory) { $options += @('-DMECHWARRIOR_SDL_SOURCE_DIR=',"-DSDL3_DIR=$SDL3Directory") }
& $cmake @options
if ($LASTEXITCODE) { throw 'Configuration failed. Supply an existing SDL3 package/source tree for a live build.' }
$arguments = @('--build',$build,'--config',$Configuration,'--parallel','6')
if ($Target) { $arguments += @('--target',$Target) }
if ($Action -eq 'Rebuild') { $arguments += '--clean-first' }
& $cmake @arguments
if ($LASTEXITCODE) { throw 'Build failed.' }
# F5 launches the real game with isolated existing save/capture directories.
foreach ($directory in @('saves','captures')) {
    New-Item -ItemType Directory -Force -Path (Join-Path $projectRoot "local/visual-studio/$directory") | Out-Null
}
