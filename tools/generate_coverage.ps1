# Native Coverage Generation Script (Simplified)
# Generates SonarQube generic coverage (single artifact: sonarqube.xml)
# Excludes platform-specific code via filters (#ifdef ARDUINO/ESP_PLATFORM)
# Mirrors CI logic for consistent local vs SonarCloud numbers.
# Usage: .\tools\generate_coverage.ps1

[CmdletBinding()]
param()

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Native Coverage (Platform-Independent)" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# Resolve paths relative to this script
# ---------------------------------------------------------------------------
$scriptDir = Split-Path -Path $PSCommandPath -Parent
$libRoot   = Split-Path -Path $scriptDir -Parent

# ---------------------------------------------------------------------------
# Step 1: Prepare .coverage/native workspace
# ---------------------------------------------------------------------------
Write-Host "`n[1/4] Preparing .coverage/native workspace..." -ForegroundColor Yellow
$coverageRoot = Join-Path $libRoot ".coverage\native"
if (Test-Path $coverageRoot) {
    Remove-Item -LiteralPath $coverageRoot -Recurse -Force -ErrorAction SilentlyContinue
}
New-Item -ItemType Directory -Path $coverageRoot | Out-Null
Remove-Item (Join-Path $coverageRoot "gcov") -Recurse -Force -ErrorAction SilentlyContinue

# Clean build
Remove-Item (Join-Path $libRoot ".pio\build\test_native") -Recurse -ErrorAction SilentlyContinue
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcda" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }
Get-ChildItem -Path $libRoot -Recurse -Filter "*.gcov" -ErrorAction SilentlyContinue | ForEach-Object { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------------------
# Step 2: Build and run native tests
# ---------------------------------------------------------------------------
Write-Host "`n[2/4] Building and running native tests..." -ForegroundColor Yellow

$pioCmd = (Get-Command pio -ErrorAction SilentlyContinue)
if (-not $pioCmd) {
    $fallbackPio = "C:\\Users\\Drew\\.platformio\\penv\\Scripts\\pio.exe"
    if (Test-Path $fallbackPio) {
        $pioCmd = $fallbackPio
    } else {
        Write-Host "PlatformIO (pio) not found." -ForegroundColor Red
        exit 1
    }
}

Push-Location $libRoot
& $pioCmd test -e test_native
Pop-Location
if ($LASTEXITCODE -ne 0) {
    Write-Host "`nTests failed!" -ForegroundColor Red
    exit 1
}

## (Removed manual gcov invocation; gcovr drives coverage directly now)

# ---------------------------------------------------------------------------
# Step 4: Generate coverage report (EXCLUDE platform-specific code)
# ---------------------------------------------------------------------------
Write-Host "`n[3/4] Generating SonarQube (generic) coverage report..." -ForegroundColor Yellow

$gcovrPath = Get-Command gcovr -ErrorAction SilentlyContinue
if (-not $gcovrPath) {
    Write-Host "  Installing gcovr..." -ForegroundColor Yellow
    pip install gcovr | Out-Null
}

$sonarXmlPath = Join-Path $coverageRoot "sonarqube.xml"
$summaryPath  = Join-Path $coverageRoot "summary.txt"

# EXCLUDE platform-specific code (#ifdef ARDUINO/ESP_PLATFORM)
gcovr --sonarqube $sonarXmlPath `
    --root $libRoot `
    --object-directory (Join-Path $libRoot ".pio/build/test_native") `
    --filter "^src/" --filter "^include/" `
    --exclude-throw-branches `
    --exclude-directories ".*libdeps.*" `
    --exclude ".*FakeIt.*" `
    --exclude ".*Arduino.*" `
    --exclude ".*unity.*" `
    --exclude "/usr/include/.*" `
    --exclude-lines-by-pattern ".*#ifdef (ARDUINO|ESP_PLATFORM).*" `
    --exclude-lines-by-pattern ".*#if defined\((ARDUINO|ESP_PLATFORM)\).*" `
    --exclude-unreachable-branches | Out-Null

if ($LASTEXITCODE -ne 0 -or -not (Test-Path $sonarXmlPath)) {
    Write-Host "`nFailed to generate SonarQube coverage report!" -ForegroundColor Red
    exit 1
}

# ---------------------------------------------------------------------------
# Step 5: Display summary
# ---------------------------------------------------------------------------
Write-Host "`n[4/4] SonarQube coverage report generated!" -ForegroundColor Green
Write-Host "`nCoverage artifacts:" -ForegroundColor Cyan
Write-Host "  - .coverage/native/sonarqube.xml" -ForegroundColor White
Write-Host "  - .coverage/native/summary.txt" -ForegroundColor White

# Parse SonarQube generic coverage XML for summary
$sonarXml = [xml](Get-Content $sonarXmlPath)
$totalLines = 0
$coveredLines = 0
$totalBranches = 0
$coveredBranches = 0
foreach ($fileNode in $sonarXml.coverage.file) {
    foreach ($lineNode in $fileNode.lineToCover) {
        $totalLines++
        if ($lineNode.covered -eq 'true') { $coveredLines++ }
        if ($lineNode.branchesToCover) {
            $totalBranches += [int]$lineNode.branchesToCover
            $coveredBranches += [int]$lineNode.coveredBranches
        }
    }
}
$linePercent = if ($totalLines -gt 0) { [math]::Round(($coveredLines / $totalLines) * 100, 1) } else { 0 }
$branchPercent = if ($totalBranches -gt 0) { [math]::Round(($coveredBranches / $totalBranches) * 100, 1) } else { 0 }

"LineCoverage=$linePercent%" | Out-File -FilePath $summaryPath -Encoding utf8 -Append
"BranchCoverage=$branchPercent%" | Out-File -FilePath $summaryPath -Encoding utf8 -Append
"Lines=$coveredLines/$totalLines" | Out-File -FilePath $summaryPath -Encoding utf8 -Append
"Branches=$coveredBranches/$totalBranches" | Out-File -FilePath $summaryPath -Encoding utf8 -Append

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Native Coverage Summary (SonarQube Generic):" -ForegroundColor Cyan
Write-Host "  Line Coverage:   $linePercent%" -ForegroundColor $(if ($linePercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "  Branch Coverage: $branchPercent%" -ForegroundColor $(if ($branchPercent -ge 80) { "Green" } else { "Yellow" })
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  (Excludes platform-specific #ifdef ARDUINO/ESP_PLATFORM code)" -ForegroundColor DarkGray

Write-Host "`nAll artifacts written to .coverage/native" -ForegroundColor Green
Write-Host "This shows coverage for platform-independent code testable in native environment." -ForegroundColor Cyan
Write-Host "For platform-specific code coverage, use generate_esp32_coverage.ps1" -ForegroundColor Cyan
