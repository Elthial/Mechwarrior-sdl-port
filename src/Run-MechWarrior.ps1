param(
    [string]$GameDirectory = $(if ($env:MECHWARRIOR_ASSET_DIRECTORY) { $env:MECHWARRIOR_ASSET_DIRECTORY } else { Join-Path (Split-Path $PSScriptRoot -Parent) 'assets/original-game' }),
    [string]$SaveDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'local/user-saves'),
    [string]$TimingConfiguration = (Join-Path (Split-Path $PSScriptRoot -Parent) 'MechWarrior.ini'),
    [string]$BuildDirectory = (Join-Path (Split-Path $PSScriptRoot -Parent) 'build-visual-studio/Release'),
    [string]$SupportPilotDirectory = '',
    [switch]$DisableSupportPilot
)

# Launch the actual campaign/launcher executable, not a mission fixture.
$mechExecutable = Join-Path $BuildDirectory 'mechwarrior.exe'
$mechSdlLibrary = Join-Path $BuildDirectory 'SDL3.dll'
foreach ($mechRequiredFile in @($mechExecutable, $mechSdlLibrary, (Join-Path $GameDirectory 'BTECH.EXE')))
{
    if (!(Test-Path -LiteralPath $mechRequiredFile -PathType Leaf))
    { throw "Missing required file: $mechRequiredFile. See docs/PLAYABLE_TEST_BUILD.md." }
}
$mechGamePath = (Resolve-Path -LiteralPath $GameDirectory -ErrorAction Stop).Path
New-Item -ItemType Directory -Path $SaveDirectory -Force -ErrorAction Stop | Out-Null
$mechSavePath = (Resolve-Path -LiteralPath $SaveDirectory -ErrorAction Stop).Path
$mechCapturePath = Join-Path (Split-Path $PSScriptRoot -Parent) ('local/user-test-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $mechCapturePath -ErrorAction Stop | Out-Null
$mechPreviousSavePath = [Environment]::GetEnvironmentVariable('MECHWARRIOR_SAVE_DIRECTORY', 'Process')
$mechPreviousTimingPath = [Environment]::GetEnvironmentVariable('MECHWARRIOR_TIMING_CONFIG', 'Process')
try
{
    $env:MECHWARRIOR_SAVE_DIRECTORY = $mechSavePath
    $env:MECHWARRIOR_TIMING_CONFIG = (Resolve-Path -LiteralPath $TimingConfiguration -ErrorAction Stop).Path
    Write-Host "Timing configuration: $env:MECHWARRIOR_TIMING_CONFIG"
    Write-Host "Executable: $mechExecutable"
    Write-Host "Saves: $mechSavePath"
    Write-Host "F12 captures: $mechCapturePath"
    $mechLogPath = Join-Path $mechCapturePath 'runtime.log'
    Write-Host "Runtime/crash log: $mechLogPath"
    $mechPilotArguments = @()
    if ($DisableSupportPilot) { $mechPilotArguments = @('--no-support-pilot') }
    if ($SupportPilotDirectory -and !$DisableSupportPilot)
    {
        New-Item -ItemType Directory -Path $SupportPilotDirectory -Force -ErrorAction Stop | Out-Null
        $mechPilotArguments = @('--support-pilot', (Resolve-Path -LiteralPath $SupportPilotDirectory).Path)
        Write-Host "Optional Support pilot: $SupportPilotDirectory (F11 stops; requires explicit goals)"
    }
    & $mechExecutable $mechGamePath $mechCapturePath @mechPilotArguments 2>&1 | Tee-Object -FilePath $mechLogPath
    $mechExitCode = $LASTEXITCODE
    "Exit code: $mechExitCode" | Tee-Object -FilePath $mechLogPath -Append
}
finally
{
    [Environment]::SetEnvironmentVariable('MECHWARRIOR_SAVE_DIRECTORY', $mechPreviousSavePath, 'Process')
    [Environment]::SetEnvironmentVariable('MECHWARRIOR_TIMING_CONFIG', $mechPreviousTimingPath, 'Process')
}
exit $mechExitCode
