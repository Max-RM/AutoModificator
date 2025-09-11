@echo off
call build_llvm_x64.bat
call build_llvm_x86.bat
call build_llvm_ARM64.bat
call build_llvm_ARM32.bat
echo All builds finished.
pause