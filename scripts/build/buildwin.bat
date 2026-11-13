@echo off
cd /d "%~dp0..\.."
if not exist build_win mkdir build_win
echo "Compilando o compilador C++ (ghost.exe)..."
g++ -Iinclude -O3 -fopenmp src\ghost_compiler.cpp src\Lexer.cpp src\Parser.cpp src\Interpreter.cpp src\GhostNumberG.cpp -o build_win\ghost.exe -static -static-libgcc -static-libstdc++ -lws2_32 -lwininet
if %errorlevel% neq 0 (
    echo "Erro de compilação!"
    exit /b %errorlevel%
)
echo "Compilação concluída!"

if exist teste_deep_learning_novo.g (
    echo "Testando código GhostLang (teste_deep_learning_novo.g)..."
    .\build_win\ghost.exe teste_deep_learning_novo.g -o build_win\teste_dl.exe
    if %errorlevel% neq 0 (
        echo "Erro ao compilar teste_deep_learning_novo.g!"
        exit /b %errorlevel%
    )

    echo "Executando teste..."
    .\build_win\teste_dl.exe
) else (
    echo "Arquivo de teste (teste_deep_learning_novo.g) não encontrado, pulando teste."
)
