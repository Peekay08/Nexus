# NEXUS PowerShell Build Script

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host " 🚀 Building NEXUS Compiler & Runtime   " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

# Ensure g++ is present
if (!(Get-Command g++ -ErrorAction SilentlyContinue)) {
    Write-Error "Error: 'g++' was not found in your PATH. Please make sure MSYS2 or MinGW GCC is installed."
    Exit 1
}

# Source files list for compiling
$sources = @(
    "compiler/lexer/lexer.cpp",
    "compiler/parser/parser.cpp",
    "compiler/ir/ir.cpp",
    "compiler/ir/ir_loader.cpp",
    "compiler/optimizer/optimizer.cpp",
    "runtime/interpreter/builtins.cpp",
    "runtime/interpreter/vm.cpp"
)

# 1. Compile Main Compiler CLI (nxs.exe)
Write-Host "`n[nxs-build] Compiling compiler CLI (nxs.exe)..." -ForegroundColor Yellow
$cliArgs = @("-std=c++17", "-O3", "-I.", "cli/main.cpp") + $sources + @("-o", "nxs.exe")
& g++ $cliArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "[nxs-build] CLI compilation failed."
    Exit 1
}
Write-Host "[nxs-build] nxs.exe compiled successfully! ✅" -ForegroundColor Green

# 2. Compile Test Suite (test_suite.exe)
Write-Host "`n[nxs-build] Compiling verification test suite..." -ForegroundColor Yellow
$testArgs = @("-std=c++17", "-O3", "-I.", "tests/test_suite.cpp") + $sources + @("-o", "test_suite.exe")
& g++ $testArgs

if ($LASTEXITCODE -ne 0) {
    Write-Error "[nxs-build] Test suite compilation failed."
    Exit 1
}
Write-Host "[nxs-build] test_suite.exe compiled successfully! ✅" -ForegroundColor Green

Write-Host "`n=========================================" -ForegroundColor Cyan
Write-Host " 🎉 NEXUS Toolchain build completed!     " -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
