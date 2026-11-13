// Gerado de fichas revisadas; a consulta nao usa pesos neurais.
function string minusculas(string texto) {
    texto = regex_replace(texto, "A", "a");
    texto = regex_replace(texto, "B", "b");
    texto = regex_replace(texto, "C", "c");
    texto = regex_replace(texto, "D", "d");
    texto = regex_replace(texto, "E", "e");
    texto = regex_replace(texto, "F", "f");
    texto = regex_replace(texto, "G", "g");
    texto = regex_replace(texto, "H", "h");
    texto = regex_replace(texto, "I", "i");
    texto = regex_replace(texto, "J", "j");
    texto = regex_replace(texto, "K", "k");
    texto = regex_replace(texto, "L", "l");
    texto = regex_replace(texto, "M", "m");
    texto = regex_replace(texto, "N", "n");
    texto = regex_replace(texto, "O", "o");
    texto = regex_replace(texto, "P", "p");
    texto = regex_replace(texto, "Q", "q");
    texto = regex_replace(texto, "R", "r");
    texto = regex_replace(texto, "S", "s");
    texto = regex_replace(texto, "T", "t");
    texto = regex_replace(texto, "U", "u");
    texto = regex_replace(texto, "V", "v");
    texto = regex_replace(texto, "W", "w");
    texto = regex_replace(texto, "X", "x");
    texto = regex_replace(texto, "Y", "y");
    texto = regex_replace(texto, "Z", "z");
    texto = regex_replace(texto, "(Á|À|Â|Ã|Ä|á|à|â|ã|ä)", "a");
    texto = regex_replace(texto, "(É|È|Ê|Ë|Ẽ|é|è|ê|ë|ẽ)", "e");
    texto = regex_replace(texto, "(Í|Ì|Î|Ï|í|ì|î|ï)", "i");
    texto = regex_replace(texto, "(Ó|Ò|Ô|Õ|Ö|ó|ò|ô|õ|ö)", "o");
    texto = regex_replace(texto, "(Ú|Ù|Û|Ü|ú|ù|û|ü)", "u");
    texto = regex_replace(texto, "(Ç|ç)", "c");
    return texto;
}
function string normalizar(string texto) {
    texto = regex_replace(minusculas(texto), "[^a-z0-9]+", " ");
    var palavras = string_split(texto, 32);
    string saida = " ";
    int n = 0;
    while (n < palavras.size()) {
        string palavra = palavras[n];
        string busca = " " + palavra + " ";
        string ignorar = " a agora apresente as breve codigo com como completo criar crie curto da das de depois descreva detalhe detalhes do dois dos duas e em eu exemplo exemplos explique favor mais me meu mim mostrar mostre na nas no nos o os para pode por pratico quais qual que resuma resumo se simples sobre um uma umas uns voce é ";
        if (palavra.size() > 0 && string_contains(ignorar, busca) == "false" && string_contains(saida, busca) == "false") { saida = saida + palavra + " "; }
        n = n + 1;
    }
    return saida;
}
function double similaridade(string pergunta, string exemplo) {
    var palavras = string_split(pergunta, 32);
    var termos = string_split(exemplo, 32);
    double comuns = 0.0;
    double total = 0.0;
    double tamanho = 0.0;
    int i = 0;
    while (i < palavras.size()) {
        if (palavras[i].size() > 0) {
            total = total + 1.0;
            if (string_contains(exemplo, " " + palavras[i] + " ") == "true") { comuns = comuns + 1.0; }
        }
        i = i + 1;
    }
    i = 0;
    while (i < termos.size()) { if (termos[i].size() > 0) { tamanho = tamanho + 1.0; } i = i + 1; }
    if (total < 1.0 || tamanho < 1.0) { return 0.0; }
    if (tamanho > total) { total = tamanho; }
    return comuns / total;
}
function int localizar(string texto) {
    if (texto.size() > 4000) { return -1; }
    string pergunta = normalizar(texto);
    double melhor = 0.0;
    double segundo = 0.0;
    double pontuacao = 0.0;
    double atual = 0.0;
    int ficha = -1;
    pontuacao = 0.0;
    atual = similaridade(pergunta, " ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " linguagem ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " defina ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero conhecer ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 0;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " compilar programa ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " comando compila arquivo g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " gerar executavel ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 1;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " foi criada ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " estamos usando ghostlang ia modelo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " v3 usa realmente ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 2;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " textmodel ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " declarar modelo texto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tipo nativo cria modelo linguagem ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 3;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " arquitetura v3 ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " v3 transformer ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " funciona rede texto v3 ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 4;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " treinar textmodel ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " metodos treinam modelo nativo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ajustar pesos ia ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 5;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " salvar carregar modelo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " guarda checkpoint v3 ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " retomar treinamento ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 6;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " selecionar cpu ou gpu automaticamente ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ativar reconhecimento automatico hardware ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ia detecta rtx automaticamente ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 7;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " forcar execucao cpu ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero rodar ia sem gpu ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " escolher somente processador ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 8;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " forcar cuda v3 ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero executar ia gpu nvidia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " usar pragma gpu ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 9;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " gpu intel uhd executa cuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " posso usar cuda placa integrada intel ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " placa deste computador roda backend cuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 10;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " tudo ghostlang roda gpu ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " operacoes usam cuda nativa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " gpu acelera nesta versao ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 11;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " state ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " representar vetor estado ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve tipo state ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 12;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " gate ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " declarar matriz pesos ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve tipo gate ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " matriz ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " matrizes ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " multiplicacao matricial ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 13;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " montar network ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " faz operador encadeamento ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " conectar camadas ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 14;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " funciona multiplicacao matricial cuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " matriz cuda preserva numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " componentes gpu multiplica matrizes ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 15;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " teoria numeros fantasmas g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " numeros fantasmas g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " teoria g definida projeto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " teoria g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " numeros fantasmas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " sao numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 16;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " sao componentes numero g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " significam x y p theta2 ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " representar numero fantasma ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 17;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " somar numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " regra adicao fantasmas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " funciona soma ghostnumberg ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " soma ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " adicao numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 18;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " multiplicar numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " formula produto fantasma ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " funciona multiplicacao ghostnumberg ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " produto ghostlang ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " multiplicacao numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 19;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " theta nilpotente ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quanto theta vezes ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quadrado fantasma puro zero ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 20;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " multiplicacao numeros g associativa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " posso mudar parenteses produto g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " existe contraexemplo associatividade ghostnumberg ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 21;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " acontece divisao zero g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quanto dividido zero ghostnumberg ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " divisao trata divisor fantasma puro ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 22;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " campo theta2 preservado operacoes ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " theta2 quarta coordenada aditiva ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " acontece theta2 soma produto ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 23;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " norma quadrada numero g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " funciona norm squared ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " formula usa x y p theta2 norma ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 24;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " calcular conjugado numero g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " faz conjugate ghostnumberg ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " componentes mudam conjugacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 25;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " funciona rotate numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " faz rotacao componentes fantasmas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ordem componentes rotate ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 26;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " manual g concordam ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " diferencas existem descricao 4d ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " teoria g ja esta consistente todo projeto ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 27;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " pesos v3 sao numeros g ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " textmodel treina componentes fantasmas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " rede texto usa algebra fantasma pesos ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 28;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " diferenca entre consulta geracao neural ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " resposta documental foi gerada pela rede ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " sei v3 consultou fonte ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 29;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " aprende automaticamente enquanto conversamos ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " conversas alteram seus pesos ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " chat treina modelo tempo real ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 30;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " quem ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " sao suas habilidades capacidades ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quando foi treinada ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 31;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " ola ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " oi tudo bem ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ola vai ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 32;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " bom dia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " oi bom dia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " bom dia esta ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 33;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " boa tarde ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " oi boa tarde ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " boa tarde tudo bem ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 34;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " boa noite ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " oi boa noite ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " boa noite vai ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 35;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " obrigado pela ajuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " muito obrigado ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " valeu pela ajuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 36;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " ate logo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " preciso sair ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " vou embora ate ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 37;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " sabe tudo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " nunca erra ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " posso confiar qualquer resposta sua ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 38;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " nao entendi sua resposta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explicar outro jeito ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " sua explicacao ficou confusa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 39;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " dar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " consegue ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 40;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " quero resposta curta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " responda forma ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " resumir sua resposta ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 41;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " quero conversar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " podemos conversar pouco ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " gostaria falar alguma coisa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 42;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " nao sei perguntar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " estou sem ideias conversar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " comeco esta conversa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 43;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " organizar dia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " preciso planejar minhas tarefas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero organizar melhor minha rotina ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 44;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " tenho muitas tarefas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " nao sei onde comecar trabalho ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " escolher fazer primeiro ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 45;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " habito ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero manter rotina nova ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " comecar habito pequeno ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 46;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " estudar melhor ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " preciso melhorar meus estudos ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " forma organizar estudo ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 47;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " lembrar estudei ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " esqueco conteudo estudar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ajuda revisar materia ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 48;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " aprender palavra nova ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero ampliar vocabulario ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " estudar palavras novas ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 49;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " comecar ler ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero habito leitura ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " incluir leitura rotina ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 50;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " escrever mensagem clara ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero melhorar minha escrita ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " deixar texto facil entender ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 51;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " fazer ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " preciso resumir texto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " identificar essencial texto ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 52;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " revisar texto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " devo verificar antes enviar mensagem ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " encontrar erros minha escrita ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 53;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " ter ideias historia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero escrever historia curta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " onde comeco historia ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 54;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " conte historia curta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero pequena historia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " contar historia ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 55;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " planejar projeto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tenho ideia quero comecar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " transformar ideia projeto ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 56;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " dividir problema grande ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tarefa parece dificil demais ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tornar trabalho complexo ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 57;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " algoritmo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explicar algoritmo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " significado algoritmo ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 58;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " programacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve programar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explica programacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 59;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " variavel ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explicar variavel programacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve variavel ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 60;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " teste software ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " testar programa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " testes ajudam programacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 61;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " programa deu erro ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " nao esta funcionando ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " comeco investigar falha programa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 62;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " inteligencia artificial ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explica ia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ideia inteligencia artificial ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 63;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " rede neural aprende ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " acontece treinamento ia ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " modelo ajusta suas respostas ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 64;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " sao dados treinamento ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " servem treinamento ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " textos ajudam ia aprender ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 65;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " separar treino validacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " conjunto validacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " verificar modelo aprendeu alem ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 66;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " memoria ram ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve memoria computador ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explica ram ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 67;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " faz gpu ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " serve placa video ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " gpu ajuda calculos ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 68;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " copia seguranca ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " fazer backup ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " proteger arquivo contra perda ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 69;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " escolher senha ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero melhorar seguranca conta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ajuda proteger minhas senhas ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 70;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " fonte confiavel ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " verificar informacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " avaliar noticia esta correta ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 71;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " diferenca entre fato opiniao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " reconhecer opiniao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " opiniao mesmo fato ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 72;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " ouvir melhor pessoa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero melhorar conversa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ajuda entender alguem diz ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 73;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " pedir ajuda ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tenho duvida nao sei explicar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " descrever problema outra pessoa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 74;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " cometi erro trabalho ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " algo tentei nao deu certo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " aprender erro ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 75;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " estou desanimado projeto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " projeto parece nao avancar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " perdi motivacao continuar tarefa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 76;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " nao tenho certeza resposta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " estou duvida informacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " fazer quando nao sei algo ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 77;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " pergunta aberta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " fazer pergunta incentive conversa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " explicar pergunta aberta ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 78;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " preparar apresentacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " preciso falar assunto outras pessoas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " organizar ideias apresentacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 79;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " significa comparar alternativas ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " analisar opcoes ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " organizar escolha entre possibilidades ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 80;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " fazer lista compras ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero organizar compras casa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " evitar esquecer itens ao comprar ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 81;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " cuidar planta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero comecar cultivar planta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " observar ao cuidar plantas ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 82;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " organizar meus arquivos ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tenho muitos documentos computador ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " encontrar meus arquivos facilidade ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 83;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " aprender ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " tenho pronto quero entender ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " estudar solucao ja feita ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 84;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " proximo passo ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " continuar comecar ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ja fiz primeira etapa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 85;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " fazer pergunta ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " quero comece conversa ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " pergunte alguma coisa ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 86;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " gostei explicacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " entendi melhor ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " sua resposta ajudou ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 87;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    pontuacao = 0.0;
    atual = similaridade(pergunta, " preciso contexto ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " ainda falta informacao ");
    if (atual > pontuacao) { pontuacao = atual; }
    atual = similaridade(pergunta, " nao tenho suficientes decidir ");
    if (atual > pontuacao) { pontuacao = atual; }
    if (pontuacao > melhor) {
        segundo = melhor;
        melhor = pontuacao; ficha = 88;
    } else { if (pontuacao > segundo) { segundo = pontuacao; } }
    if (melhor < 0.68 || melhor - segundo < 0.10) { return -1; }
    return ficha;
}
function int pedido_exemplo(string texto) {
    return regex_search(minusculas(texto), "\\b(exemplo|exemplos|codigo|demonstre)\\b");
}
function int pedido_continuacao(string texto) {
    return normalizar(texto) == " " && regex_search(minusculas(texto), "\\b(exemplo|exemplos|codigo|detalhe|detalhes|explique|resuma|resumo)\\b");
}
function string responder_ficha(int ficha, string pedido) {
    int exemplo = pedido_exemplo(pedido);
    int breve = regex_search(minusculas(pedido), "\\b(resuma|resumo|breve|curto)\\b");
    string resposta = "";
    if (ficha == 0) {
        resposta = "[Consulta documental] GhostLang é a linguagem deste projeto. Seu compilador transforma arquivos .g em C++ e gera um executável. Ela oferece tipos e operações para matrizes, redes neurais e Números G.";
        if (breve == 0) { resposta = resposta + "\n\nComo funciona: você escreve um arquivo .g, o compilador o transforma em C++ e gera um executável local. O programa pode usar variáveis, funções, condições e laços. Para cálculos, há state (vetor), gate (matriz), network (rede de camadas) e ghost (Número G); textmodel reúne o modelo de texto treinável.\nUm programa simples começa em function void main(). Cada instrução termina com ponto e vírgula."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    string nome = \"GhostLang\";\n    int dobro = 2 * 5;\n    print(\"Ola, \" + nome + \"!\");\n    print(\"O dobro de 5 e \" + dobro);\n}\n```\n\nstring nome guarda um texto; int dobro calcula 2 * 5; print mostra os dois resultados. Esse exemplo não depende de CUDA.\n\nResultado esperado:\nOla, GhostLang!\nO dobro de 5 e 10\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/ola.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: docs/SPEC.md:5; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/ola.g";
    }
    if (ficha == 1) {
        resposta = "[Consulta documental] Na raiz do projeto, use build_native/ghost programa.g -o programa. Depois execute ./programa. O compilador nativo liga automaticamente a biblioteca ghost_runtime.";
        if (breve == 0) { resposta = resposta + "\n\nSalve o código em um arquivo .g. Na raiz do projeto, compile com build_native/ghost arquivo.g -o programa e execute ./programa. Para construir o compilador, use bash ia_propria/scripts/compilar_v3.sh. Os exemplos abaixo já existem no projeto."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    string nome = \"GhostLang\";\n    int dobro = 2 * 5;\n    print(\"Ola, \" + nome + \"!\");\n    print(\"O dobro de 5 e \" + dobro);\n}\n```\n\nO compilador liga automaticamente o runtime nativo. O exemplo funciona em CPU.\n\nResultado esperado:\nOla, GhostLang!\nO dobro de 5 e 10\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/ola.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/ghost_compiler.cpp:175; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/ola.g";
    }
    if (ficha == 2) {
        resposta = "[Consulta documental] Sim. A V3 declara seu textmodel e controla treino e execução em GhostLang. O compilador traduz o programa para C++; o runtime nativo implementa os cálculos em C++ e CUDA. Isso não significa que os kernels CUDA sejam escritos em sintaxe GhostLang.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v3/src/main.g:13";
    }
    if (ficha == 3) {
        resposta = "[Consulta documental] textmodel é o tipo nativo para um modelo de linguagem treinável localmente. Exemplo: textmodel ia = text_model(64, 32, 192, 32, 4096, 42); Os argumentos são contexto, embedding, camada oculta, lote, limite de vocabulário e semente.";
        if (breve == 0) { resposta = resposta + "\n\nO modelo aprende a prever o próximo token: converte palavras em embeddings, combina uma janela de contexto na camada oculta e calcula probabilidades na saída. O treinamento compara a previsão com o alvo e ajusta os pesos reais float32 com AdamW.\nprepare carrega os dados; epoch ajusta os pesos; validate mede a perda. Uma perda menor não garante uma boa resposta completa. É necessário testar também relevância, fatos e generalização."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\n// Execute na raiz do projeto; requer os arquivos de corpus da V3.\nfunction void main() {\n    pragma auto;\n    textmodel ia = text_model(8, 8, 24, 16, 2048, 42);\n    ia.prepare(\"ia_propria/v3/dados/treino.txt\", \"ia_propria/v3/dados/validacao.txt\");\n    double perda = ia.epoch(0.001);\n    print(\"Erro medio no treino: \" + perda);\n    print(\"Erro na validacao: \" + ia.validate());\n    print(\"Uma epoca demonstra a API; ainda nao cria um bom conversador.\");\n}\n```\n\nExecute na raiz do projeto para que os caminhos dos corpus sejam encontrados. Este exemplo usa uma rede pequena e não salva checkpoint.\n\nResultado esperado:\nAs perdas numéricas variam conforme os dados e o dispositivo. Ao final: Uma epoca demonstra a API; ainda nao cria um bom conversador.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/textmodel.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: include/GhostTextModel.hpp:32; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/textmodel.g";
    }
    if (ficha == 4) {
        resposta = "[Consulta documental] A V3 usa embeddings de palavras, uma janela fixa de contexto, camada oculta tanh e saída softmax. Treina com entropia cruzada e AdamW. É uma rede pequena, sem atenção Transformer, e sua capacidade de generalização é limitada.";
        if (breve == 0) { resposta = resposta + "\n\nO modelo aprende a prever o próximo token: converte palavras em embeddings, combina uma janela de contexto na camada oculta e calcula probabilidades na saída. O treinamento compara a previsão com o alvo e ajusta os pesos reais float32 com AdamW.\nprepare carrega os dados; epoch ajusta os pesos; validate mede a perda. Uma perda menor não garante uma boa resposta completa. É necessário testar também relevância, fatos e generalização."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\n// Execute na raiz do projeto; requer os arquivos de corpus da V3.\nfunction void main() {\n    pragma auto;\n    textmodel ia = text_model(8, 8, 24, 16, 2048, 42);\n    ia.prepare(\"ia_propria/v3/dados/treino.txt\", \"ia_propria/v3/dados/validacao.txt\");\n    double perda = ia.epoch(0.001);\n    print(\"Erro medio no treino: \" + perda);\n    print(\"Erro na validacao: \" + ia.validate());\n    print(\"Uma epoca demonstra a API; ainda nao cria um bom conversador.\");\n}\n```\n\nExecute na raiz do projeto para que os caminhos dos corpus sejam encontrados. Este exemplo usa uma rede pequena e não salva checkpoint.\n\nResultado esperado:\nAs perdas numéricas variam conforme os dados e o dispositivo. Ao final: Uma epoca demonstra a API; ainda nao cria um bom conversador.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/textmodel.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostTextModel.cpp:243; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/textmodel.g";
    }
    if (ficha == 5) {
        resposta = "[Consulta documental] Declare o textmodel, chame prepare com arquivos de treino e validação, execute epoch com uma taxa de aprendizado, avalie com validate e grave com save. O script V3 oferece o comando treinar e preserva o melhor checkpoint de validação.";
        if (breve == 0) { resposta = resposta + "\n\nO modelo aprende a prever o próximo token: converte palavras em embeddings, combina uma janela de contexto na camada oculta e calcula probabilidades na saída. O treinamento compara a previsão com o alvo e ajusta os pesos reais float32 com AdamW.\nprepare carrega os dados; epoch ajusta os pesos; validate mede a perda. Uma perda menor não garante uma boa resposta completa. É necessário testar também relevância, fatos e generalização."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\n// Execute na raiz do projeto; requer os arquivos de corpus da V3.\nfunction void main() {\n    pragma auto;\n    textmodel ia = text_model(8, 8, 24, 16, 2048, 42);\n    ia.prepare(\"ia_propria/v3/dados/treino.txt\", \"ia_propria/v3/dados/validacao.txt\");\n    double perda = ia.epoch(0.001);\n    print(\"Erro medio no treino: \" + perda);\n    print(\"Erro na validacao: \" + ia.validate());\n    print(\"Uma epoca demonstra a API; ainda nao cria um bom conversador.\");\n}\n```\n\nExecute na raiz do projeto para que os caminhos dos corpus sejam encontrados. Este exemplo usa uma rede pequena e não salva checkpoint.\n\nResultado esperado:\nAs perdas numéricas variam conforme os dados e o dispositivo. Ao final: Uma epoca demonstra a API; ainda nao cria um bom conversador.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/textmodel.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: include/GhostTextModel.hpp:16; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/textmodel.g";
    }
    if (ficha == 6) {
        resposta = "[Consulta documental] save grava pesos, momentos do AdamW, vocabulário, arquitetura, épocas e estado aleatório. load restaura o modelo. Para retomar com os textos, passe o checkpoint como terceiro argumento de prepare. O comando continuar faz isso na V3.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostTextModel.cpp:446";
    }
    if (ficha == 7) {
        resposta = "[Consulta documental] O padrão é GHOST_DEVICE=auto. O runtime testa CUDA e usa a GPU disponível; sem CUDA funcional, usa CPU. Você pode chamar use_device(\"auto\") antes de preparar ou carregar o modelo. device_info informa a seleção.";
        if (breve == 0) { resposta = resposta + "\n\nA escolha do dispositivo deve acontecer antes de prepare ou load. auto tenta um kernel real na NVIDIA e usa CPU se CUDA não estiver disponível; cuda exige GPU funcional; cpu força o processador. O textmodel mantém pesos e momentos do otimizador na GPU durante o treino. Tokenização e interface permanecem na CPU."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    print(device_info());\n    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);\n    ia.self_test();\n}\n```\n\nTroque pragma auto por pragma cpu para forçar CPU ou por pragma gpu para exigir CUDA.\n\nResultado esperado:\nInformações do dispositivo e mensagem OK do autoteste. A identificação varia conforme CPU/GPU.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/cuda.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostDevice.cpp:17; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/cuda.g";
    }
    if (ficha == 8) {
        resposta = "[Consulta documental] Use bash scripts/ia_v3.sh --device cpu chat, dentro de ia_propria. Em GhostLang, use use_device(\"cpu\") antes de prepare ou load. A versão CPU também pode ser compilada com GHOST_CUDA=OFF.";
        if (breve == 0) { resposta = resposta + "\n\nA escolha do dispositivo deve acontecer antes de prepare ou load. auto tenta um kernel real na NVIDIA e usa CPU se CUDA não estiver disponível; cuda exige GPU funcional; cpu força o processador. O textmodel mantém pesos e momentos do otimizador na GPU durante o treino. Tokenização e interface permanecem na CPU."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    print(device_info());\n    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);\n    ia.self_test();\n}\n```\n\nTroque pragma auto por pragma cpu para forçar CPU ou por pragma gpu para exigir CUDA.\n\nResultado esperado:\nInformações do dispositivo e mensagem OK do autoteste. A identificação varia conforme CPU/GPU.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/cuda.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: ia_propria/scripts/ia_v3.sh:5; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/cuda.g";
    }
    if (ficha == 9) {
        resposta = "[Consulta documental] Use bash scripts/ia_v3.sh --device cuda chat. Em GhostLang, use pragma gpu ou use_device(\"cuda\") antes de preparar o modelo. Esse modo exige CUDA funcional e informa erro se ela não estiver disponível.";
        if (breve == 0) { resposta = resposta + "\n\nA escolha do dispositivo deve acontecer antes de prepare ou load. auto tenta um kernel real na NVIDIA e usa CPU se CUDA não estiver disponível; cuda exige GPU funcional; cpu força o processador. O textmodel mantém pesos e momentos do otimizador na GPU durante o treino. Tokenização e interface permanecem na CPU."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    print(device_info());\n    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);\n    ia.self_test();\n}\n```\n\nTroque pragma auto por pragma cpu para forçar CPU ou por pragma gpu para exigir CUDA.\n\nResultado esperado:\nInformações do dispositivo e mensagem OK do autoteste. A identificação varia conforme CPU/GPU.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/cuda.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/Parser.cpp:667; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/cuda.g";
    }
    if (ficha == 10) {
        resposta = "[Consulta documental] Este backend procura dispositivos pelo runtime CUDA. A RTX NVIDIA é a placa usada neste projeto. Não foi implementado um backend para a Intel UHD; a alternativa disponível aqui é CPU.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostCuda.cu:40";
    }
    if (ficha == 11) {
        resposta = "[Consulta documental] O textmodel executa passagem direta, gradientes e atualização AdamW na GPU. Produtos de GhostMatrix também têm kernel CUDA para x, y e p. Tokenização e interface ficam na CPU. Isso não torna automaticamente todos os recursos da linguagem operações de GPU.";
        if (breve == 0) { resposta = resposta + "\n\nA escolha do dispositivo deve acontecer antes de prepare ou load. auto tenta um kernel real na NVIDIA e usa CPU se CUDA não estiver disponível; cuda exige GPU funcional; cpu força o processador. O textmodel mantém pesos e momentos do otimizador na GPU durante o treino. Tokenização e interface permanecem na CPU."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    print(device_info());\n    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);\n    ia.self_test();\n}\n```\n\nTroque pragma auto por pragma cpu para forçar CPU ou por pragma gpu para exigir CUDA.\n\nResultado esperado:\nInformações do dispositivo e mensagem OK do autoteste. A identificação varia conforme CPU/GPU.\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/cuda.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostTextCuda.cu:136; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/cuda.g";
    }
    if (ficha == 12) {
        resposta = "[Consulta documental] state representa um vetor coluna de valores. Por exemplo: state entrada = [1.0, 0.0]; Ele pode ser usado como entrada de operações matriciais e redes neurais.";
        if (breve == 0) { resposta = resposta + "\n\nPara multiplicar uma matriz por um vetor, o número de colunas da matriz deve ser igual ao número de entradas do vetor. No exemplo, uma matriz 2x2 multiplica um vetor 2x1. Preenchemos todos os pesos explicitamente para obter um resultado reproduzível."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    state entrada = [1.0, 2.0];\n    gate pesos(2, 2);\n    pesos(0, 0) = GhostNumberG(1.0);\n    pesos(0, 1) = GhostNumberG(0.0);\n    pesos(1, 0) = GhostNumberG(0.0);\n    pesos(1, 1) = GhostNumberG(2.0);\n    state saida = pesos * entrada;\n    print(saida(0, 0).x);\n    print(saida(1, 0).x);\n}\n```\n\nA primeira linha calcula 1*1 + 0*2 = 1. A segunda calcula 0*1 + 2*2 = 4. O modo auto pode manter esse produto pequeno na CPU.\n\nResultado esperado:\n1\n4\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/matrizes.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/Parser.cpp:33; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/matrizes.g";
    }
    if (ficha == 13) {
        resposta = "[Consulta documental] gate representa uma matriz. Por exemplo: gate camada(3, 2); cria três linhas e duas colunas. A sintaxe também permite portas predefinidas como hadamard.";
        if (breve == 0) { resposta = resposta + "\n\nPara multiplicar uma matriz por um vetor, o número de colunas da matriz deve ser igual ao número de entradas do vetor. No exemplo, uma matriz 2x2 multiplica um vetor 2x1. Preenchemos todos os pesos explicitamente para obter um resultado reproduzível."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    pragma auto;\n    state entrada = [1.0, 2.0];\n    gate pesos(2, 2);\n    pesos(0, 0) = GhostNumberG(1.0);\n    pesos(0, 1) = GhostNumberG(0.0);\n    pesos(1, 0) = GhostNumberG(0.0);\n    pesos(1, 1) = GhostNumberG(2.0);\n    state saida = pesos * entrada;\n    print(saida(0, 0).x);\n    print(saida(1, 0).x);\n}\n```\n\nA primeira linha calcula 1*1 + 0*2 = 1. A segunda calcula 0*1 + 2*2 = 4. O modo auto pode manter esse produto pequeno na CPU.\n\nResultado esperado:\n1\n4\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/matrizes.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/Parser.cpp:87; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/matrizes.g";
    }
    if (ficha == 14) {
        resposta = "[Consulta documental] network conecta matrizes de pesos com o operador >>. Exemplo: network rede = camada1 >> camada2; É um recurso diferente de textmodel, que já reúne o modelo de linguagem e seu treinamento.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/Parser.cpp:461";
    }
    if (ficha == 15) {
        resposta = "[Consulta documental] O kernel matricial reproduz as fórmulas atuais de GhostNumberG para x, y e p e devolve theta2 igual a zero, como o caminho CPU. O modo auto evita GPU em produtos pequenos para reduzir o custo de transferências.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostCuda.cu:58";
    }
    if (ficha == 16) {
        resposta = "[Consulta documental] No código deste projeto, um Número G é representado por (x + i y) + p theta, com o parâmetro theta2. x é a parte real, y a imaginária e p a fantasma. A implementação usa regras próprias; há diferenças documentadas em relação à descrição 4D do manual.";
        if (breve == 0) { resposta = resposta + "\n\nAs componentes são x (real), y (imaginária) e p (fantasma). Na soma, cada componente é adicionada separadamente. No produto, x=x1*x2-y1*y2, y=x1*y2+y1*x2 e p=x1*p2+x2*p1.\nPara a=(1,2,3) e b=(4,5,6), a soma é (5,7,9) e o produto é (-6,13,18). A implementação zera theta2 nesses resultados. Essas regras não permitem assumir associatividade; há diferenças entre o manual e o código."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    ghost a = GhostNumberG(1.0, 2.0, 3.0);\n    ghost b = GhostNumberG(4.0, 5.0, 6.0);\n    ghost soma = a + b;\n    ghost produto = a * b;\n    print(\"Soma: \" + soma.x + \", \" + soma.y + \", \" + soma.p);\n    print(\"Produto: \" + produto.x + \", \" + produto.y + \", \" + produto.p);\n}\n```\n\nOs construtores usam literais decimais explícitos. O exemplo lê as componentes .x, .y e .p para mostrar o cálculo sem depender dos sufixos algébricos antigos.\n\nResultado esperado:\nSoma: 5, 7, 9\nProduto: -6, 13, 18\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/numeros_g.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:198; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/numeros_g.g";
    }
    if (ficha == 17) {
        resposta = "[Consulta documental] Os componentes usados nas operações principais são x, y e p. O campo theta2 é um parâmetro adicional. Para criar um exemplo explícito, use ghost z = GhostNumberG(1.0, 2.0, 3.0, 0.0);";
        if (breve == 0) { resposta = resposta + "\n\nAs componentes são x (real), y (imaginária) e p (fantasma). Na soma, cada componente é adicionada separadamente. No produto, x=x1*x2-y1*y2, y=x1*y2+y1*x2 e p=x1*p2+x2*p1.\nPara a=(1,2,3) e b=(4,5,6), a soma é (5,7,9) e o produto é (-6,13,18). A implementação zera theta2 nesses resultados. Essas regras não permitem assumir associatividade; há diferenças entre o manual e o código."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    ghost a = GhostNumberG(1.0, 2.0, 3.0);\n    ghost b = GhostNumberG(4.0, 5.0, 6.0);\n    ghost soma = a + b;\n    ghost produto = a * b;\n    print(\"Soma: \" + soma.x + \", \" + soma.y + \", \" + soma.p);\n    print(\"Produto: \" + produto.x + \", \" + produto.y + \", \" + produto.p);\n}\n```\n\nOs construtores usam literais decimais explícitos. O exemplo lê as componentes .x, .y e .p para mostrar o cálculo sem depender dos sufixos algébricos antigos.\n\nResultado esperado:\nSoma: 5, 7, 9\nProduto: -6, 13, 18\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/numeros_g.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:36; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/numeros_g.g";
    }
    if (ficha == 18) {
        resposta = "[Consulta documental] A soma adiciona x, y e p componente a componente. O resultado recebe theta2 igual a zero. Por exemplo, (1, 2, 3) + (4, 5, 6) produz (5, 7, 9) nessas três componentes.";
        if (breve == 0) { resposta = resposta + "\n\nAs componentes são x (real), y (imaginária) e p (fantasma). Na soma, cada componente é adicionada separadamente. No produto, x=x1*x2-y1*y2, y=x1*y2+y1*x2 e p=x1*p2+x2*p1.\nPara a=(1,2,3) e b=(4,5,6), a soma é (5,7,9) e o produto é (-6,13,18). A implementação zera theta2 nesses resultados. Essas regras não permitem assumir associatividade; há diferenças entre o manual e o código."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    ghost a = GhostNumberG(1.0, 2.0, 3.0);\n    ghost b = GhostNumberG(4.0, 5.0, 6.0);\n    ghost soma = a + b;\n    ghost produto = a * b;\n    print(\"Soma: \" + soma.x + \", \" + soma.y + \", \" + soma.p);\n    print(\"Produto: \" + produto.x + \", \" + produto.y + \", \" + produto.p);\n}\n```\n\nOs construtores usam literais decimais explícitos. O exemplo lê as componentes .x, .y e .p para mostrar o cálculo sem depender dos sufixos algébricos antigos.\n\nResultado esperado:\nSoma: 5, 7, 9\nProduto: -6, 13, 18\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/numeros_g.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:249; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/numeros_g.g";
    }
    if (ficha == 19) {
        resposta = "[Consulta documental] A regra implementada é x = x1*x2 - y1*y2; y = x1*y2 + y1*x2; p = x1*p2 + x2*p1. O resultado tem theta2 igual a zero. Termos envolvendo y e p não entram na parte fantasma dessa fórmula.";
        if (breve == 0) { resposta = resposta + "\n\nAs componentes são x (real), y (imaginária) e p (fantasma). Na soma, cada componente é adicionada separadamente. No produto, x=x1*x2-y1*y2, y=x1*y2+y1*x2 e p=x1*p2+x2*p1.\nPara a=(1,2,3) e b=(4,5,6), a soma é (5,7,9) e o produto é (-6,13,18). A implementação zera theta2 nesses resultados. Essas regras não permitem assumir associatividade; há diferenças entre o manual e o código."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    ghost a = GhostNumberG(1.0, 2.0, 3.0);\n    ghost b = GhostNumberG(4.0, 5.0, 6.0);\n    ghost soma = a + b;\n    ghost produto = a * b;\n    print(\"Soma: \" + soma.x + \", \" + soma.y + \", \" + soma.p);\n    print(\"Produto: \" + produto.x + \", \" + produto.y + \", \" + produto.p);\n}\n```\n\nOs construtores usam literais decimais explícitos. O exemplo lê as componentes .x, .y e .p para mostrar o cálculo sem depender dos sufixos algébricos antigos.\n\nResultado esperado:\nSoma: 5, 7, 9\nProduto: -6, 13, 18\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/numeros_g.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:197; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/numeros_g.g";
    }
    if (ficha == 20) {
        resposta = "[Consulta documental] Sim, no produto implementado, theta = (0, 0, 1) satisfaz theta*theta = (0, 0, 0). Essa afirmação descreve a regra executada no código, que retorna theta2 igual a zero.";
        if (breve == 0) { resposta = resposta + "\n\nSer nilpotente aqui significa que o produto do elemento por si mesmo chega a zero, embora o elemento inicial não seja zero. O exemplo usa theta=(0,0,1) e aplica exatamente o operador já existente. Isso não altera as demais propriedades da álgebra."; }
        if (exemplo == 1) { resposta = resposta + "\n\nExemplo completo em GhostLang:\n```ghost\nfunction void main() {\n    ghost theta = GhostNumberG(0.0, 0.0, 1.0);\n    ghost quadrado = theta * theta;\n    print(quadrado.x);\n    print(quadrado.y);\n    print(quadrado.p);\n}\n```\n\nCada linha mostra uma componente do quadrado: x, y e p.\n\nResultado esperado:\n0\n0\n0\n\nPara executar, na raiz do projeto:\nbuild_native/ghost ia_propria/v3/exemplos/nilpotente.g -o /tmp/ghost_exemplo\n/tmp/ghost_exemplo"; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:197; ia_propria/v3/GUIA_EXEMPLOS.md; ia_propria/v3/exemplos/nilpotente.g";
    }
    if (ficha == 21) {
        resposta = "[Consulta documental] A implementação atual não é associativa em geral. Com i = (0, 1, 0) e theta = (0, 0, 1), temos (i*i)*theta = -theta, mas i*(i*theta) = 0. A integração CUDA preserva esse comportamento; não altera a teoria silenciosamente.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: tests/cpp/native_runtime.cpp:22";
    }
    if (ficha == 22) {
        resposta = "[Consulta documental] Se x e y do divisor forem próximos de zero, a divisão implementada devolve x = 0, y = 0 e transfere a magnitude complexa do numerador para p, ou preserva seu p quando essa magnitude é quase zero. Assim, 1.0 dividido por zero produz (0, 0, 1). É uma convenção do projeto, não a divisão usual.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:229";
    }
    if (ficha == 23) {
        resposta = "[Consulta documental] Soma, subtração e multiplicação retornam theta2 igual a zero na implementação atual. Outros métodos, como conjugate e set_theta2, podem preservá-lo ou alterá-lo. Portanto, o código atual não trata theta2 simplesmente como uma quarta coordenada aditiva.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:249";
    }
    if (ficha == 24) {
        resposta = "[Consulta documental] norm_squared calcula x*x + y*y + p*p*theta2. Por exemplo, x = 3, y = 4, p = 2 e theta2 = 2 dão 33. real_norm_squared usa apenas x*x + y*y.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:109";
    }
    if (ficha == 25) {
        resposta = "[Consulta documental] conjugate troca o sinal de y e preserva x, p e theta2. Portanto, o conjugado de (1, 2, 3, 4) é (1, -2, 3, 4).";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:96";
    }
    if (ficha == 26) {
        resposta = "[Consulta documental] rotate modifica o próprio objeto, fazendo x receber o antigo y, y receber o antigo p e p receber o antigo x. Por exemplo, (1, 2, 3) passa a (2, 3, 1).";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostNumberG.cpp:275";
    }
    if (ficha == 27) {
        resposta = "[Consulta documental] Há diferenças. O manual descreve a + bi + cg + d theta, enquanto as operações principais usam x, y e p e frequentemente zeram theta2. A V3 distingue documentação de comportamento executável. Consulte docs/NUMEROS_G_IMPLEMENTACAO.md para os exemplos verificados.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: docs/MANUAL.md:889";
    }
    if (ficha == 28) {
        resposta = "[Consulta documental] Não. O textmodel usa pesos reais float32 em CPU e CUDA. Ele recebe conteúdo sobre Números G nos textos de treinamento e na base documental. As matrizes GhostMatrix usam componentes G; isso é diferente de treinar o modelo de texto com pesos fantasmas.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: src/GhostTextModel.cpp:113";
    }
    if (ficha == 29) {
        resposta = "[Consulta documental] O modo fontes busca respostas revisadas na base local e mostra a origem. O modo neural gera tokens pelos pesos treinados e pode errar. No chat, use /neural ou /fontes. Uma boa resposta documental não mede a capacidade da rede.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v3/src/main.g:12";
    }
    if (ficha == 30) {
        resposta = "[Consulta documental] Não. O chat não altera os pesos e não grava automaticamente suas mensagens. Para aprender novos textos, prepare um corpus e execute treinar ou continuar. A base de fontes também precisa ser revisada e recompilada para incluir novas respostas.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v3/src/main.g:12";
    }
    if (ficha == 31) {
        resposta = "[Consulta documental] Sou a IA V3 experimental deste projeto. Posso consultar a base local sobre GhostLang e Números G, com fontes, ou gerar texto com um modelo pequeno treinado localmente. Não tenho conhecimento geral amplo nem acesso automático à internet. As métricas e a data do treino ficam nos arquivos do checkpoint.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v3/README.md:1";
    }
    if (ficha == 32) {
        resposta = "[Consulta documental] Olá! Como posso ajudar você hoje?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:2";
    }
    if (ficha == 33) {
        resposta = "[Consulta documental] Bom dia! Espero que seu dia comece bem. Sobre o que você gostaria de conversar?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:3";
    }
    if (ficha == 34) {
        resposta = "[Consulta documental] Boa tarde! Estou aqui para conversar e ajudar com suas ideias.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:4";
    }
    if (ficha == 35) {
        resposta = "[Consulta documental] Boa noite! Como posso ajudar antes de você encerrar o dia?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:5";
    }
    if (ficha == 36) {
        resposta = "[Consulta documental] De nada! Fico à disposição para continuar a conversa.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:8";
    }
    if (ficha == 37) {
        resposta = "[Consulta documental] Até mais! Quando quiser continuar, estarei por aqui.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:9";
    }
    if (ficha == 38) {
        resposta = "[Consulta documental] Não. Posso errar ou não ter informação suficiente. É importante verificar as respostas, especialmente em decisões importantes.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:11";
    }
    if (ficha == 39) {
        resposta = "[Consulta documental] Claro. Diga qual parte ficou confusa e tentarei explicar com palavras mais simples e um exemplo.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:12";
    }
    if (ficha == 40) {
        resposta = "[Consulta documental] Posso tentar. Sobre qual assunto você gostaria de ver um exemplo?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:13";
    }
    if (ficha == 41) {
        resposta = "[Consulta documental] Claro. Vou procurar responder de maneira direta e destacar a ideia principal.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:14";
    }
    if (ficha == 42) {
        resposta = "[Consulta documental] Podemos conversar. Você prefere falar sobre seu dia, algum projeto ou uma dúvida?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:15";
    }
    if (ficha == 43) {
        resposta = "[Consulta documental] Podemos começar com algo simples: o que chamou sua atenção hoje?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:16";
    }
    if (ficha == 44) {
        resposta = "[Consulta documental] Liste suas tarefas, escolha as mais importantes e reserve um horário para cada uma. Deixe espaço para pausas e imprevistos.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:17";
    }
    if (ficha == 45) {
        resposta = "[Consulta documental] Separe o que é urgente do que pode esperar. Escolha uma tarefa importante e divida o trabalho em passos pequenos.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:18";
    }
    if (ficha == 46) {
        resposta = "[Consulta documental] Comece com uma ação pequena e repita em um horário definido. Acompanhe seu progresso e ajuste a rotina quando necessário.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:19";
    }
    if (ficha == 47) {
        resposta = "[Consulta documental] Divida o conteúdo em partes, estude com atenção e tente explicar o que aprendeu sem consultar o material. Faça revisões e exercícios.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:20";
    }
    if (ficha == 48) {
        resposta = "[Consulta documental] Faça perguntas sobre o conteúdo e tente responder de memória. Revise em dias diferentes e use exercícios para descobrir suas dúvidas.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:21";
    }
    if (ficha == 49) {
        resposta = "[Consulta documental] Leia a palavra em uma frase, descubra seu significado e crie um exemplo próprio. Volte a usá-la em outros momentos.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:22";
    }
    if (ficha == 50) {
        resposta = "[Consulta documental] Escolha um texto que desperte seu interesse e reserve alguns minutos por dia. Comece com uma meta pequena e mantenha a regularidade.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:23";
    }
    if (ficha == 51) {
        resposta = "[Consulta documental] Comece pela informação principal. Use frases claras, retire repetições e revise o texto pensando em quem vai ler.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:24";
    }
    if (ficha == 52) {
        resposta = "[Consulta documental] Leia o texto, identifique a ideia principal e selecione os pontos que a sustentam. Escreva com suas palavras sem mudar o sentido.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:25";
    }
    if (ficha == 53) {
        resposta = "[Consulta documental] Confira o sentido, a ordem das ideias, a pontuação e a ortografia. Uma leitura em voz alta pode ajudar a perceber frases confusas.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:26";
    }
    if (ficha == 54) {
        resposta = "[Consulta documental] Escolha um personagem, um lugar e um problema. Pense no que o personagem deseja e em como suas escolhas mudam a situação.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:27";
    }
    if (ficha == 55) {
        resposta = "[Consulta documental] Uma menina plantou uma semente e cuidou dela todos os dias. Quando surgiu a primeira folha, percebeu que a paciência também fazia parte da descoberta.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:28";
    }
    if (ficha == 56) {
        resposta = "[Consulta documental] Defina o objetivo, descreva uma primeira versão pequena e liste as etapas necessárias. Teste cedo e ajuste o plano com base no resultado.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:29";
    }
    if (ficha == 57) {
        resposta = "[Consulta documental] Descreva o resultado desejado e separe o trabalho em partes que possam ser verificadas. Resolva uma parte por vez e acompanhe as dependências.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:30";
    }
    if (ficha == 58) {
        resposta = "[Consulta documental] Um algoritmo é uma sequência de instruções para realizar uma tarefa ou resolver um problema. Uma receita simples é um exemplo de passos organizados.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:31";
    }
    if (ficha == 59) {
        resposta = "[Consulta documental] Programar é escrever instruções para que um computador execute tarefas. O trabalho envolve entender o problema, criar uma solução e testar o resultado.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:32";
    }
    if (ficha == 60) {
        resposta = "[Consulta documental] Uma variável dá um nome a um valor usado pelo programa. Esse valor pode representar, por exemplo, uma quantidade, uma palavra ou um resultado.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:33";
    }
    if (ficha == 61) {
        resposta = "[Consulta documental] Um teste verifica se o programa se comporta como esperado em determinada situação. Ele ajuda a encontrar erros e a conferir mudanças.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:34";
    }
    if (ficha == 62) {
        resposta = "[Consulta documental] Leia a mensagem de erro, identifique onde a falha ocorre e tente reproduzir o problema com um exemplo pequeno. Qual mensagem apareceu?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:35";
    }
    if (ficha == 63) {
        resposta = "[Consulta documental] Inteligência artificial reúne métodos para realizar tarefas como reconhecer padrões e produzir previsões. Muitos modelos aprendem esses padrões a partir de exemplos.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:36";
    }
    if (ficha == 64) {
        resposta = "[Consulta documental] O modelo faz uma previsão, compara o resultado com um alvo e ajusta seus parâmetros para reduzir o erro. Esse processo é repetido com muitos exemplos.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:37";
    }
    if (ficha == 65) {
        resposta = "[Consulta documental] São os exemplos usados para ajustar o modelo. A qualidade, a diversidade e a organização desses dados influenciam o que ele consegue aprender.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:38";
    }
    if (ficha == 66) {
        resposta = "[Consulta documental] A validação usa exemplos separados para avaliar o comportamento do modelo. Isso ajuda a perceber quando ele apenas memoriza os dados de treinamento.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:39";
    }
    if (ficha == 67) {
        resposta = "[Consulta documental] A memória RAM guarda temporariamente dados que os programas estão usando. Seu conteúdo normalmente não é preservado quando o computador é desligado.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:40";
    }
    if (ficha == 68) {
        resposta = "[Consulta documental] Uma GPU executa muitas operações em paralelo. Além de produzir imagens, ela pode acelerar cálculos quando o programa possui um backend compatível.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:41";
    }
    if (ficha == 69) {
        resposta = "[Consulta documental] Uma cópia de segurança permite recuperar dados quando o original é perdido. Guarde cópias em locais separados e verifique se consegue restaurá-las.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:42";
    }
    if (ficha == 70) {
        resposta = "[Consulta documental] Use uma senha longa e exclusiva para cada conta. Um gerenciador de senhas e a autenticação em dois fatores podem ajudar a proteger o acesso.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:43";
    }
    if (ficha == 71) {
        resposta = "[Consulta documental] Confira quem publicou a informação, procure a fonte original e compare com outras fontes independentes. Observe a data e diferencie evidência de opinião.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:44";
    }
    if (ficha == 72) {
        resposta = "[Consulta documental] Um fato pode ser verificado por evidências. Uma opinião expressa uma avaliação ou preferência. Um texto pode conter os dois, por isso é útil distingui-los.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:45";
    }
    if (ficha == 73) {
        resposta = "[Consulta documental] Preste atenção, evite interromper e faça perguntas para confirmar o que entendeu. Tente compreender a ideia antes de preparar sua resposta.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:46";
    }
    if (ficha == 74) {
        resposta = "[Consulta documental] Explique o que você queria fazer, o que aconteceu e o que já tentou. Um exemplo concreto ajuda a outra pessoa a entender sua dúvida.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:47";
    }
    if (ficha == 75) {
        resposta = "[Consulta documental] Procure entender o que aconteceu e quais fatores você pode mudar. Escolha um ajuste pequeno, teste novamente e registre o que aprendeu.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:48";
    }
    if (ficha == 76) {
        resposta = "[Consulta documental] Talvez ajude revisar o objetivo e escolher uma etapa pequena para hoje. Se quiser, podemos conversar sobre o que está dificultando seu progresso.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:49";
    }
    if (ficha == 77) {
        resposta = "[Consulta documental] É possível reconhecer a dúvida e buscar mais informações. Podemos começar identificando exatamente o que precisa ser verificado.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:50";
    }
    if (ficha == 78) {
        resposta = "[Consulta documental] Uma pergunta aberta permite uma resposta desenvolvida, em vez de apenas sim ou não. Perguntar como alguém chegou a uma ideia é um exemplo.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:51";
    }
    if (ficha == 79) {
        resposta = "[Consulta documental] Defina a mensagem principal, escolha poucos pontos de apoio e prepare exemplos. Ensaie a explicação e ajuste o tempo disponível.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:52";
    }
    if (ficha == 80) {
        resposta = "[Consulta documental] Escolha critérios relevantes, como objetivo, esforço e limitações. Compare cada opção pelos mesmos critérios e registre o que ainda precisa descobrir.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:53";
    }
    if (ficha == 81) {
        resposta = "[Consulta documental] Confira o que já existe em casa, anote o que está faltando e agrupe os itens por categoria. Revise a lista antes de sair.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:54";
    }
    if (ficha == 82) {
        resposta = "[Consulta documental] Identifique a espécie e procure suas necessidades de luz e água. Observe o solo e o crescimento, pois plantas diferentes precisam de cuidados diferentes.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:55";
    }
    if (ficha == 83) {
        resposta = "[Consulta documental] Crie pastas com nomes claros, use um padrão para os arquivos e separe materiais atuais dos antigos. Mantenha uma cópia de segurança dos documentos importantes.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:56";
    }
    if (ficha == 84) {
        resposta = "[Consulta documental] Observe cada etapa, explique por que ela existe e altere uma parte para testar seu entendimento. Depois tente resolver um problema parecido sem copiar.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:57";
    }
    if (ficha == 85) {
        resposta = "[Consulta documental] Depende do objetivo e do resultado da primeira etapa. Conte o que você já fez e o que deseja alcançar para definirmos o próximo passo.";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:58";
    }
    if (ficha == 86) {
        resposta = "[Consulta documental] Qual foi uma coisa interessante que você aprendeu ou percebeu hoje?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:59";
    }
    if (ficha == 87) {
        resposta = "[Consulta documental] Que bom que a explicação ajudou. Você gostaria de explorar algum detalhe ou passar para outra dúvida?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:60";
    }
    if (ficha == 88) {
        resposta = "[Consulta documental] Vamos identificar o que está faltando. Qual informação mudaria sua compreensão do problema?";
        if (exemplo == 1) { resposta = resposta + "\n\nEsta ficha ainda não possui exemplo executável validado. Posso apresentar exemplos de GhostLang, matrizes, Números G, textmodel ou CUDA."; }
        return resposta + "\nFonte: ia_propria/v2/dados/conversas.json:61";
    }
    return "[Sem fonte suficiente] Não encontrei uma resposta inequívoca para esse assunto. Posso explicar GhostLang, matrizes, textmodel, CUDA e Números G com exemplos revisados. Não consigo criar programas arbitrários a partir desta base.";
}
function string consultar(string texto) {
    if (texto.size() > 4000) { return "Mensagem longa demais (4000 bytes)."; }
    int ficha = localizar(texto);
    if (ficha >= 0) { return responder_ficha(ficha, texto); }
    string separado = regex_replace(minusculas(texto), " e ", "\n");
    var partes = string_split(separado, 10);
    if (partes.size() == 2) {
        int primeira = localizar(partes[0]); int segunda = localizar(partes[1]);
        if (primeira >= 0 && segunda >= 0 && primeira != segunda) {
            return responder_ficha(primeira, partes[0]) + "\n\n" + responder_ficha(segunda, partes[1]);
        }
    }
    return responder_ficha(-1, texto);
}
