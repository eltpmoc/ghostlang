#pragma once
#include <string>
// Operacoes numericas da extensao da IA; o fluxo da aplicacao fica em main.g.
void v2_iniciar();
void v2_preparar(std::string treino, std::string validacao, std::string origem);
double v2_epoca(double taxa);
double v2_validar();
double v2_acuracia();
double v2_perplexidade(double loss);
int v2_epocas();
int v2_paciencia();
void v2_salvar(std::string caminho);
void v2_carregar(std::string caminho);
double v2_avaliar_arquivo(std::string caminho);
std::string v2_gerar(std::string texto, int quantidade);
void v2_chat();
void v2_info();
void v2_autoteste();
int v2_inteiro(std::string valor, int minimo, int maximo);
double v2_decimal(std::string valor, double minimo, double maximo);
void v2_falhar(std::string mensagem);

