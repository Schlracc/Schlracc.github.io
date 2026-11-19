@echo off
REM DEV MODE (Windows): build the previewer and render every scene to dev\out\
REM Needs g++ (install MSYS2 or "winget install BrechtSanders.WinLibs.POSIX.UCRT")
cd /d "%~dp0.."
g++ -O2 -std=c++17 dev\preview.cpp -o dev\preview.exe || goto :fail
dev\preview.exe %* || goto :fail
start "" dev\out\index.html
goto :eof
:fail
echo Build failed. Is g++ installed and on PATH?
pause
