#!/bin/bash
# build_portable.sh
# Script para criar a distribuicao portatil do GhostLang para Linux

PROJ_DIR="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$PROJ_DIR" || exit

echo -e "\e[36mIniciando build padrao do GhostLang...\e[0m"
./scripts/build/buildLinux.sh
if [ $? -ne 0 ]; then
    echo -e "\e[31mFalha ao compilar o GhostLang.\e[0m"
    exit 1
fi

DIST="GhostLang_Portable"
echo -e "\n\e[33mLimpando pasta $DIST...\e[0m"
rm -rf $DIST
mkdir -p $DIST

echo -e "\e[36mCopiando binários...\e[0m"
cp build_linux/ghost build_linux/ghost_api build_linux/ghost_node build_linux/ghost_swarm build_linux/ghost_ask $DIST/

echo -e "\e[36mCopiando bibliotecas padrão...\e[0m"
cp -r include $DIST/

echo -e "\n\e[33mBaixando motor de IA nativo (llama-cli) para Linux...\e[0m"
wget -q -O $DIST/llama.zip "https://github.com/ggml-org/llama.cpp/releases/download/b10582/llama-b10582-bin-ubuntu-x64.zip"
mkdir -p $DIST/compiler
unzip -q $DIST/llama.zip -d $DIST/compiler
rm $DIST/llama.zip

echo -e "\n\e[33mCriando script de instalacao de dependencias (setup.sh)...\e[0m"

cat << 'EOF' > $DIST/setup.sh
#!/bin/bash
echo "Bem-vindo ao GhostLang Portatil!"
echo "Verificando se o compilador g++ esta instalado..."

if command -v g++ &> /dev/null; then
    echo "g++ ja esta instalado! Tudo pronto para compilar."
    exit 0
fi

echo "g++ nao encontrado. Tentando instalar automaticamente..."
if command -v apt-get &> /dev/null; then
    sudo apt-get update && sudo apt-get install -y g++
elif command -v dnf &> /dev/null; then
    sudo dnf install -y gcc-c++
elif command -v pacman &> /dev/null; then
    sudo pacman -Sy --noconfirm gcc
elif command -v zypper &> /dev/null; then
    sudo zypper install -y gcc-c++
else
    echo "Gerenciador de pacotes nao suportado (apt/dnf/pacman/zypper nao encontrados)."
    echo "Por favor, instale o pacote g++ manualmente."
    exit 1
fi

echo "Pronto! O ambiente GhostLang esta configurado para compilar."
EOF

chmod +x $DIST/setup.sh

echo -e "\n\e[32m==================================================\e[0m"
echo -e "\e[32mSUCESSO! A pasta '$DIST' esta pronta!\e[0m"
echo -e "\e[32mVoce pode compactar essa pasta (.tar.gz) e enviar para qualquer pessoa.\e[0m"
echo -e "\e[32mPara garantir que tudo funcione no Linux destino, peca para rodarem ./setup.sh primeiro.\e[0m"
echo -e "\e[32m==================================================\e[0m"
