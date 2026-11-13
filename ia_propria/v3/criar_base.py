#!/usr/bin/env python3
"""Base autoral revisada contra fontes locais; nenhum download/modelo externo."""
import json
from pathlib import Path
HERE = Path(__file__).resolve().parent
cards = []
def add(id, questions, answer, source, anchor):
    cards.append(dict(id=id, perguntas=questions, resposta=answer, arquivo=source, ancora=anchor))

add('ghostlang', ['O que é GhostLang?', 'Explique a linguagem GhostLang.', 'Para que serve GhostLang?'],
    'GhostLang é a linguagem deste projeto. Seu compilador transforma arquivos .g em C++ e gera um executável. Ela oferece tipos e operações para matrizes, redes neurais e Números G.', 'docs/SPEC.md', '## 1. Introdução')
add('compilar', ['Como compilar um programa GhostLang?', 'Qual comando compila um arquivo .g?', 'Como gerar um executável GhostLang?'],
    'Na raiz do projeto, use build_native/ghost programa.g -o programa. Depois execute ./programa. O compilador nativo liga automaticamente a biblioteca ghost_runtime.', 'src/ghost_compiler.cpp', '// Compiladores antigos')
add('criada', ['Você foi criada em GhostLang?', 'Estamos usando GhostLang para criar a IA e o modelo?', 'A V3 usa realmente GhostLang?'],
    'Sim. A V3 declara seu textmodel e controla treino e execução em GhostLang. O compilador traduz o programa para C++; o runtime nativo implementa os cálculos em C++ e CUDA. Isso não significa que os kernels CUDA sejam escritos em sintaxe GhostLang.', 'ia_propria/v3/src/main.g', 'textmodel modelo')
add('textmodel', ['O que é textmodel?', 'Como declarar um modelo de texto?', 'Qual tipo nativo cria o modelo de linguagem?'],
    'textmodel é o tipo nativo para um modelo de linguagem treinável localmente. Exemplo: textmodel ia = text_model(64, 32, 192, 32, 4096, 42); Os argumentos são contexto, embedding, camada oculta, lote, limite de vocabulário e semente.', 'include/GhostTextModel.hpp', 'inline GhostTextModel text_model')
add('arquitetura', ['Qual é a arquitetura da V3?', 'A V3 é um Transformer?', 'Como funciona a rede de texto da V3?'],
    'A V3 usa embeddings de palavras, uma janela fixa de contexto, camada oculta tanh e saída softmax. Treina com entropia cruzada e AdamW. É uma rede pequena, sem atenção Transformer, e sua capacidade de generalização é limitada.', 'src/GhostTextModel.cpp', 'float cpu_batch')
add('treinar', ['Como treinar o textmodel?', 'Quais métodos treinam o modelo nativo?', 'Como ajustar os pesos da IA em GhostLang?'],
    'Declare o textmodel, chame prepare com arquivos de treino e validação, execute epoch com uma taxa de aprendizado, avalie com validate e grave com save. O script V3 oferece o comando treinar e preserva o melhor checkpoint de validação.', 'include/GhostTextModel.hpp', 'void prepare')
add('checkpoint', ['Como salvar e carregar o modelo?', 'O que guarda o checkpoint da V3?', 'Como retomar um treinamento?'],
    'save grava pesos, momentos do AdamW, vocabulário, arquitetura, épocas e estado aleatório. load restaura o modelo. Para retomar com os textos, passe o checkpoint como terceiro argumento de prepare. O comando continuar faz isso na V3.', 'src/GhostTextModel.cpp', 'void save(')
add('auto', ['Como selecionar CPU ou GPU automaticamente?', 'Como ativar o reconhecimento automático de hardware?', 'A IA detecta a RTX automaticamente?'],
    'O padrão é GHOST_DEVICE=auto. O runtime testa CUDA e usa a GPU disponível; sem CUDA funcional, usa CPU. Você pode chamar use_device("auto") antes de preparar ou carregar o modelo. device_info informa a seleção.', 'src/GhostDevice.cpp', 'void initialize()')
add('cpu', ['Como forçar a execução em CPU?', 'Quero rodar a IA sem GPU.', 'Como escolher somente o processador?'],
    'Use bash scripts/ia_v3.sh --device cpu chat, dentro de ia_propria. Em GhostLang, use use_device("cpu") antes de prepare ou load. A versão CPU também pode ser compilada com GHOST_CUDA=OFF.', 'ia_propria/scripts/ia_v3.sh', 'GHOST_DEVICE')
add('cuda', ['Como forçar CUDA na V3?', 'Quero executar a IA na GPU NVIDIA.', 'Como usar pragma gpu?'],
    'Use bash scripts/ia_v3.sh --device cuda chat. Em GhostLang, use pragma gpu ou use_device("cuda") antes de preparar o modelo. Esse modo exige CUDA funcional e informa erro se ela não estiver disponível.', 'src/Parser.cpp', 'void Parser::parsePragma()')
add('intel', ['A GPU Intel UHD executa CUDA?', 'Posso usar CUDA na placa integrada Intel?', 'Qual placa deste computador roda o backend CUDA?'],
    'Este backend procura dispositivos pelo runtime CUDA. A RTX NVIDIA é a placa usada neste projeto. Não foi implementado um backend para a Intel UHD; a alternativa disponível aqui é CPU.', 'src/GhostCuda.cu', 'ghost_cuda_probe')
add('gpu_escopo', ['Tudo no GhostLang roda na GPU?', 'Quais operações usam CUDA nativa?', 'O que a GPU acelera nesta versão?'],
    'O textmodel executa passagem direta, gradientes e atualização AdamW na GPU. Produtos de GhostMatrix também têm kernel CUDA para x, y e p. Tokenização e interface ficam na CPU. Isso não torna automaticamente todos os recursos da linguagem operações de GPU.', 'src/GhostTextCuda.cu', 'ghost_text_cuda_step')
add('state', ['O que é state no GhostLang?', 'Como representar um vetor de estado?', 'Para que serve o tipo state?'],
    'state representa um vetor coluna de valores. Por exemplo: state entrada = [1.0, 0.0]; Ele pode ser usado como entrada de operações matriciais e redes neurais.', 'src/Parser.cpp', 'void Parser::parseState()')
add('gate', ['O que é gate no GhostLang?', 'Como declarar uma matriz de pesos?', 'Para que serve o tipo gate?'],
    'gate representa uma matriz. Por exemplo: gate camada(3, 2); cria três linhas e duas colunas. A sintaxe também permite portas predefinidas como hadamard.', 'src/Parser.cpp', 'void Parser::parseGate()')
add('network', ['Como montar uma network?', 'O que faz o operador de encadeamento >>?', 'Como conectar camadas em GhostLang?'],
    'network conecta matrizes de pesos com o operador >>. Exemplo: network rede = camada1 >> camada2; É um recurso diferente de textmodel, que já reúne o modelo de linguagem e seu treinamento.', 'src/Parser.cpp', 'void Parser::parseDeepNetwork()')
add('matmul', ['Como funciona a multiplicação matricial CUDA?', 'A matriz CUDA preserva os Números G?', 'Quais componentes a GPU multiplica nas matrizes?'],
    'O kernel matricial reproduz as fórmulas atuais de GhostNumberG para x, y e p e devolve theta2 igual a zero, como o caminho CPU. O modo auto evita GPU em produtos pequenos para reduzir o custo de transferências.', 'src/GhostCuda.cu', 'ghost_cuda_matmul')
add('teoria', ['O que é a Teoria dos Números Fantasmas G?', 'Explique os Números Fantasmas G.', 'Qual é a teoria G definida no projeto?'],
    'No código deste projeto, um Número G é representado por (x + i y) + p theta, com o parâmetro theta2. x é a parte real, y a imaginária e p a fantasma. A implementação usa regras próprias; há diferenças documentadas em relação à descrição 4D do manual.', 'src/GhostNumberG.cpp', '// g = (x1 + iy1) + p1θ')
add('componentes', ['Quais são os componentes de um Número G?', 'O que significam x y p e theta2?', 'Como representar um Número Fantasma no código?'],
    'Os componentes usados nas operações principais são x, y e p. O campo theta2 é um parâmetro adicional. Para criar um exemplo explícito, use ghost z = GhostNumberG(1.0, 2.0, 3.0, 0.0);', 'src/GhostNumberG.cpp', 'GhostNumberG::GhostNumberG(double x, double y, double p, double theta2)')
add('soma_g', ['Como somar Números G?', 'Qual é a regra de adição dos fantasmas?', 'Como funciona a soma no GhostNumberG?'],
    'A soma adiciona x, y e p componente a componente. O resultado recebe theta2 igual a zero. Por exemplo, (1, 2, 3) + (4, 5, 6) produz (5, 7, 9) nessas três componentes.', 'src/GhostNumberG.cpp', 'GhostNumberG GhostNumberG::operator+(const GhostNumberG& other)')
add('produto_g', ['Como multiplicar Números G?', 'Qual é a fórmula do produto fantasma?', 'Como funciona a multiplicação em GhostNumberG?'],
    'A regra implementada é x = x1*x2 - y1*y2; y = x1*y2 + y1*x2; p = x1*p2 + x2*p1. O resultado tem theta2 igual a zero. Termos envolvendo y e p não entram na parte fantasma dessa fórmula.', 'src/GhostNumberG.cpp', 'GhostNumberG GhostNumberG::operator*(const GhostNumberG& other)')
add('nilpotente', ['Theta é nilpotente?', 'Quanto é theta vezes theta?', 'O quadrado do fantasma puro é zero?'],
    'Sim, no produto implementado, theta = (0, 0, 1) satisfaz theta*theta = (0, 0, 0). Essa afirmação descreve a regra executada no código, que retorna theta2 igual a zero.', 'src/GhostNumberG.cpp', 'GhostNumberG GhostNumberG::operator*(const GhostNumberG& other)')
add('associatividade', ['A multiplicação dos Números G é associativa?', 'Posso mudar os parênteses no produto G?', 'Existe um contraexemplo de associatividade em GhostNumberG?'],
    'A implementação atual não é associativa em geral. Com i = (0, 1, 0) e theta = (0, 0, 1), temos (i*i)*theta = -theta, mas i*(i*theta) = 0. A integração CUDA preserva esse comportamento; não altera a teoria silenciosamente.', 'tests/cpp/native_runtime.cpp', 'near(((i*i)*theta).p,-1)')
add('divisao_zero', ['O que acontece na divisão por zero em G?', 'Quanto é um dividido por zero no GhostNumberG?', 'Como a divisão trata um divisor fantasma puro?'],
    'Se x e y do divisor forem próximos de zero, a divisão implementada devolve x = 0, y = 0 e transfere a magnitude complexa do numerador para p, ou preserva seu p quando essa magnitude é quase zero. Assim, 1.0 dividido por zero produz (0, 0, 1). É uma convenção do projeto, não a divisão usual.', 'src/GhostNumberG.cpp', '// Cenário A (Singularidade / Fantasma Puro)')
add('theta2', ['O campo theta2 é preservado nas operações?', 'Theta2 é uma quarta coordenada aditiva?', 'O que acontece com theta2 na soma e no produto?'],
    'Soma, subtração e multiplicação retornam theta2 igual a zero na implementação atual. Outros métodos, como conjugate e set_theta2, podem preservá-lo ou alterá-lo. Portanto, o código atual não trata theta2 simplesmente como uma quarta coordenada aditiva.', 'src/GhostNumberG.cpp', 'GhostNumberG GhostNumberG::operator+(const GhostNumberG& other)')
add('norma', ['Qual é a norma quadrada de um Número G?', 'Como funciona norm_squared?', 'Qual fórmula usa x y p e theta2 na norma?'],
    'norm_squared calcula x*x + y*y + p*p*theta2. Por exemplo, x = 3, y = 4, p = 2 e theta2 = 2 dão 33. real_norm_squared usa apenas x*x + y*y.', 'src/GhostNumberG.cpp', 'double GhostNumberG::norm_squared() const')
add('conjugado', ['Como calcular o conjugado de um Número G?', 'O que faz conjugate no GhostNumberG?', 'Quais componentes mudam na conjugação?'],
    'conjugate troca o sinal de y e preserva x, p e theta2. Portanto, o conjugado de (1, 2, 3, 4) é (1, -2, 3, 4).', 'src/GhostNumberG.cpp', 'GhostNumberG GhostNumberG::conjugate() const')
add('rotacao', ['Como funciona rotate nos Números G?', 'O que faz a rotação das componentes fantasmas?', 'Qual é a ordem das componentes depois de rotate?'],
    'rotate modifica o próprio objeto, fazendo x receber o antigo y, y receber o antigo p e p receber o antigo x. Por exemplo, (1, 2, 3) passa a (2, 3, 1).', 'src/GhostNumberG.cpp', 'void GhostNumberG::rotate()')
add('divergencias', ['O manual e o código G concordam?', 'Quais diferenças existem na descrição 4D?', 'A teoria G já está consistente em todo o projeto?'],
    'Há diferenças. O manual descreve a + bi + cg + d theta, enquanto as operações principais usam x, y e p e frequentemente zeram theta2. A V3 distingue documentação de comportamento executável. Consulte docs/NUMEROS_G_IMPLEMENTACAO.md para os exemplos verificados.', 'docs/MANUAL.md', '## 1. Números Quânticos 4D')
add('tipo_pesos', ['Os pesos da V3 são Números G?', 'O textmodel treina componentes fantasmas?', 'A rede de texto usa a álgebra fantasma nos pesos?'],
    'Não. O textmodel usa pesos reais float32 em CPU e CUDA. Ele recebe conteúdo sobre Números G nos textos de treinamento e na base documental. As matrizes GhostMatrix usam componentes G; isso é diferente de treinar o modelo de texto com pesos fantasmas.', 'src/GhostTextModel.cpp', 'vector<float> weights, m, v;')
add('modos', ['Qual é a diferença entre consulta e geração neural?', 'A resposta documental foi gerada pela rede?', 'Como sei se a V3 consultou uma fonte?'],
    'O modo fontes busca respostas revisadas na base local e mostra a origem. O modo neural gera tokens pelos pesos treinados e pode errar. No chat, use /neural ou /fontes. Uma boa resposta documental não mede a capacidade da rede.', 'ia_propria/v3/src/main.g', 'function void conversa')
add('aprendizado_chat', ['Você aprende automaticamente enquanto conversamos?', 'As conversas alteram seus pesos?', 'O chat treina o modelo em tempo real?'],
    'Não. O chat não altera os pesos e não grava automaticamente suas mensagens. Para aprender novos textos, prepare um corpus e execute treinar ou continuar. A base de fontes também precisa ser revisada e recompilada para incluir novas respostas.', 'ia_propria/v3/src/main.g', 'function void conversa')
add('identidade', ['Quem é você?', 'Quais são suas habilidades e capacidades?', 'Quando você foi treinada?'],
    'Sou a IA V3 experimental deste projeto. Posso consultar a base local sobre GhostLang e Números G, com fontes, ou gerar texto com um modelo pequeno treinado localmente. Não tenho conhecimento geral amplo nem acesso automático à internet. As métricas e a data do treino ficam nos arquivos do checkpoint.', 'ia_propria/v3/README.md', '# IA própria V3')

(HERE/'dados').mkdir(parents=True,exist_ok=True)
if (HERE/'dados/conhecimento.json').exists():
    raise SystemExit('A base ja existe. Edite dados/conhecimento.json para preservar as melhorias locais.')
(HERE/'dados/conhecimento.json').write_text(json.dumps(cards,ensure_ascii=False,indent=2)+'\n')
print(f'{len(cards)} fichas de conhecimento criadas.')
