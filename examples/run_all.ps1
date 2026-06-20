# PML Examples 一键渲染脚本
# 用法: powershell -ExecutionPolicy Bypass -File examples/run_all.ps1

param(
    [string]$PmlExe = "",
    [string]$OutDir = "examples/output"
)

$ErrorActionPreference = "Continue"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir

# 自动定位 pml.exe：先尝试构建产物，再尝试 PATH
if ([string]::IsNullOrWhiteSpace($PmlExe)) {
    $candidates = @(
        Join-Path $repoRoot "build/debug/src/pml/cli/Debug/pml.exe"
        Join-Path $repoRoot "build/release/src/pml/cli/Release/pml.exe"
        "pml.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c -PathType Leaf) {
            $PmlExe = $c
            break
        }
    }
}

if (-not (Test-Path $PmlExe -PathType Leaf)) {
    Write-Error "找不到 pml.exe。请通过 -PmlExe 指定路径，或先构建项目。"
    exit 1
}

$PmlExe = Resolve-Path $PmlExe
$OutDir = Join-Path $repoRoot $OutDir
if (-not (Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

Write-Host "PML executable: $PmlExe" -ForegroundColor Cyan
Write-Host "Output dir:    $OutDir" -ForegroundColor Cyan
Write-Host ""

# 生成示例素材
$genScript = Join-Path $scriptDir "assets/gen.pml"
Write-Host "[gen] $genScript" -NoNewline
$proc = Start-Process -FilePath $PmlExe -ArgumentList @($genScript, "-o", (Join-Path $scriptDir "assets")) -Wait -PassThru -NoNewWindow
if ($proc.ExitCode -ne 0) {
    Write-Host " -> FAILED" -ForegroundColor Red
    Write-Error "素材生成失败，停止后续渲染。"
    exit 1
}
Write-Host " -> OK" -ForegroundColor Green

# 收集要运行的示例入口文件
# 格式: @{File=完整路径; Category=类别名; SubDir=子目录名(可选)}
$examples = @()

# 功能示例：每个子目录下的 .pml（排除子模块）
$featureDirs = @(
    "01-core",
    "02-graphics",
    "03-canvas-render",
    "04-animation",
    "05-layer-composition-filter",
    "06-sprites-style-palette",
    "08-asset-bitmap"
)
foreach ($d in $featureDirs) {
    $dir = Join-Path $scriptDir $d
    if (Test-Path $dir) {
        Get-ChildItem -Path $dir -Filter "*.pml" -File | ForEach-Object {
            $examples += @{File=$_.FullName; Category=$d; SubDir=""}
        }
    }
}

# 复杂项目：只运行每个项目目录下的 main.pml
$projectDir = Join-Path $scriptDir "09-projects"
if (Test-Path $projectDir) {
    Get-ChildItem -Path $projectDir -Directory | ForEach-Object {
        $main = Join-Path $_.FullName "main.pml"
        if (Test-Path $main) {
            $examples += @{File=$main; Category="09-projects"; SubDir=$_.Name}
        }
    }
}

$examples = $examples | Sort-Object { $_.File }

$success = 0
$failed = 0
$failures = @()

foreach ($entry in $examples) {
    $file = $entry.File
    $rel = $file.Substring($repoRoot.Length + 1)

    # 构建输出子目录
    $targetDir = Join-Path $OutDir $entry.Category
    if ($entry.SubDir) {
        $targetDir = Join-Path $targetDir $entry.SubDir
    }
    if (-not (Test-Path $targetDir)) {
        New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
    }

    Write-Host "[run] $rel" -NoNewline
    $proc = Start-Process -FilePath $PmlExe -ArgumentList @($file, "-o", $targetDir) -Wait -PassThru -NoNewWindow
    if ($proc.ExitCode -eq 0) {
        Write-Host " -> OK" -ForegroundColor Green
        $success++
    } else {
        Write-Host " -> FAILED (exit $($proc.ExitCode))" -ForegroundColor Red
        $failed++
        $failures += $rel
    }
}

Write-Host ""
Write-Host "==============================" -ForegroundColor Cyan
Write-Host "Total:  $($success + $failed)"
Write-Host "Success: $success" -ForegroundColor Green
Write-Host "Failed:  $failed" -ForegroundColor $(if ($failed -gt 0) { "Red" } else { "Green" })

if ($failed -gt 0) {
    Write-Host ""
    Write-Host "Failed examples:" -ForegroundColor Red
    foreach ($f in $failures) { Write-Host "  - $f" -ForegroundColor Red }
    exit 1
}

exit 0
