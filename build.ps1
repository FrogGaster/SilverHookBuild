param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "Win32")]
    [string]$Platform = "x64"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$solution = Join-Path $repoRoot "SilverHookFinal.sln"
$msbuild = $null

$command = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
if ($command) {
    $msbuild = $command.Source
}

if (-not $msbuild) {
    $knownBuildTools = "C:\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
    if (Test-Path -LiteralPath $knownBuildTools) {
        $msbuild = $knownBuildTools
    }
}

if (-not $msbuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $installation = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installation) {
            $candidate = Join-Path $installation "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path -LiteralPath $candidate) {
                $msbuild = $candidate
            }
        }
    }
}

if (-not $msbuild) {
    throw "MSBuild was not found. Install Visual Studio 2022 Build Tools with the Desktop development with C++ workload."
}

& $msbuild $solution /m "/p:Configuration=$Configuration" "/p:Platform=$Platform" /v:minimal
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$output = Join-Path $repoRoot "$Platform\$Configuration\ZA_PUTINA.dll"
Write-Host "Built: $output"
$loaderOutput = Join-Path $repoRoot "$Platform\$Configuration\ZA_PUTINA_Loader.exe"
if (Test-Path -LiteralPath $loaderOutput) {
    $embedScript = Join-Path $repoRoot "tools\embed_dll_resource.ps1"
    & $embedScript -LoaderPath $loaderOutput -DllPath $output
    Write-Host "Built: $loaderOutput"
}

if ($Configuration -eq "Release") {
    $outputDirectory = Join-Path $repoRoot "$Platform\$Configuration"
    $sensitiveArtifacts = @(
        (Join-Path $outputDirectory "ZA_PUTINA.pdb"),
        (Join-Path $outputDirectory "ZA_PUTINA_Loader.pdb"),
        (Join-Path $outputDirectory "ZA_PUTINA_Loader.ilk"),
        (Join-Path $outputDirectory "SilverHookFinal.dll"),
        (Join-Path $outputDirectory "SilverLoader.exe"),
        (Join-Path $outputDirectory "SilverHookFinal.pdb"),
        (Join-Path $outputDirectory "SilverLoader.pdb"),
        (Join-Path $outputDirectory "SilverLoader.ilk")
    )
    foreach ($artifact in $sensitiveArtifacts) {
        if (Test-Path -LiteralPath $artifact) {
            try {
                Remove-Item -LiteralPath $artifact -Force -ErrorAction Stop
            }
            catch {
                Write-Warning "Could not remove legacy build artifact '$artifact'. Close any process using it and run the build again."
            }
        }
    }
}
