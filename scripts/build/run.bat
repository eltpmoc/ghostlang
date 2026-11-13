@echo off
cd /d "%~dp0..\.."
.\build_win\ghost.exe hello2.g -o hello2.exe > logs\run_log.txt 2>&1
echo EXIT CODE: %errorlevel% >> logs\run_log.txt
