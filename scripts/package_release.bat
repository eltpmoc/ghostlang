@echo off
setlocal enabledelayedexpansion

echo ========================================================
echo   Empacotador de Release Portatil - GhostLang
echo ========================================================

set SCRIPT_DIR=%~dp0
set ROOT_DIR=%SCRIPT_DIR%..
set DIST_DIR=%ROOT_DIR%\dist\GhostLang-Portable-x64

if exist "%DIST_DIR%" rmdir /s /q "%DIST_DIR%"
mkdir "%DIST_DIR%\bin"
mkdir "%DIST_DIR%\include"
mkdir "%DIST_DIR%\libs"
mkdir "%DIST_DIR%\docs"
mkdir "%DIST_DIR%\examples"

echo [1/5] Copiando binarios executaveis...
copy "%ROOT_DIR%\build\ghost.exe" "%DIST_DIR%\bin\" >nul
if exist "%ROOT_DIR%\build\ghost_node.exe" copy "%ROOT_DIR%\build\ghost_node.exe" "%DIST_DIR%\bin\" >nul
if exist "%ROOT_DIR%\build\ghost_swarm.exe" copy "%ROOT_DIR%\build\ghost_swarm.exe" "%DIST_DIR%\bin\" >nul
if exist "%ROOT_DIR%\build\libghost_runtime.a" copy "%ROOT_DIR%\build\libghost_runtime.a" "%DIST_DIR%\bin\" >nul

echo [2/5] Copiando headers da biblioteca C++...
copy "%ROOT_DIR%\include\*.hpp" "%DIST_DIR%\include\" >nul

echo [3/5] Copiando bibliotecas GhostLang (.g)...
copy "%ROOT_DIR%\libs\*.g" "%DIST_DIR%\libs\" >nul

echo [4/5] Copiando documentacao e manual...
copy "%ROOT_DIR%\docs\MANUAL.md" "%DIST_DIR%\docs\" >nul
copy "%ROOT_DIR%\docs\SPEC.md" "%DIST_DIR%\docs\" >nul

echo [5/5] Criando script de inicializacao rapida...
(
echo @echo off
echo set PATH=%%~dp0bin;%%PATH%%
echo echo GhostLang Portable pronto para uso!
echo echo Digite 'ghost' para entrar no REPL interativo ou 'ghost help' para opcoes.
echo cmd /k
) > "%DIST_DIR%\iniciar_prompt.bat"

echo.
echo Pacote criado com sucesso em: %DIST_DIR%
echo ========================================================
