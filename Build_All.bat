@echo off
call build_llvm_x64.bat
call build_llvm_x86.bat
call build_llvm_arm64.bat
call build_llvm_arm.bat
echo All builds finished.
pause