#include "GhostTransformerG.hpp"
#include "GhostMatrixG.hpp"
#include "GhostTransformerCuda.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring> // std::memcpy (MSVC inclui implicitamente; g++/MinGW nao)

// Estrutura interna para esconder complexidade do header
struct GhostTransformerG::Impl {
    int C, D, H, L, B, seed;
    uint64_t update = 0;
    
    struct QubitTensor {
        int rows, cols;
        float* d_ptr; // Ponteiro CUDA (Pesos)
        float* m_ptr; // Momentum Adam Real/Imag
        float* v_ptr; // Variância Adam Real/Imag
        QubitTensor(int r, int c) : rows(r), cols(c) {
            d_ptr = ghost_tr_cuda::allocate(true, r * c * 3); // 3 floats per Qubit
            m_ptr = ghost_tr_cuda::allocate(true, r * c * 3);
            v_ptr = ghost_tr_cuda::allocate(true, r * c * 3);
            
            std::vector<float> noise(r * c * 3);
            for(int i=0; i<r*c*3; ++i) {
                noise[i] = ((rand() % 2000) / 1000.0f - 1.0f) * 0.05f;
            }
            ghost_tr_cuda::upload(true, d_ptr, noise.data(), r * c * 3);
            ghost_tr_cuda::zero(true, m_ptr, r * c * 3);
            ghost_tr_cuda::zero(true, v_ptr, r * c * 3);
        }
        ~QubitTensor() {
            if (d_ptr) ghost_tr_cuda::release(true, d_ptr);
            if (m_ptr) ghost_tr_cuda::release(true, m_ptr);
            if (v_ptr) ghost_tr_cuda::release(true, v_ptr);
        }
    };
    
    struct Layer {
        std::unique_ptr<QubitTensor> wq, wk, wv, wo;
        std::unique_ptr<QubitTensor> w1, w2, w3;
        Layer(int d) {
            wq = std::make_unique<QubitTensor>(d, d);
            wk = std::make_unique<QubitTensor>(d, d);
            wv = std::make_unique<QubitTensor>(d, d);
            wo = std::make_unique<QubitTensor>(d, d);
            w1 = std::make_unique<QubitTensor>(d, d * 4); // Expansão do MLP
            w2 = std::make_unique<QubitTensor>(d * 4, d);
            w3 = std::make_unique<QubitTensor>(d, d * 4);
        }
    };
    
    std::unique_ptr<QubitTensor> token_embed;
    std::unique_ptr<QubitTensor> pos_embed;
    std::unique_ptr<QubitTensor> output_weights;
    std::vector<Layer> layers;
    
    // Dataset armazenado em memória
    std::vector<std::string> prompts;
    std::vector<std::string> answers;
    
    // Construtor
    Impl(int c, int d, int h, int l, int b, int s) : C(c), D(d), H(h), L(l), B(b), seed(s) {
        std::cout << "[V5] Alocando tensores quanticos do Transformer...\n";
        token_embed = std::make_unique<QubitTensor>(256, d); // 256 bytes de vocabulario (GhostLang usa tokenização de bytes)
        pos_embed = std::make_unique<QubitTensor>(c, d);
        output_weights = std::make_unique<QubitTensor>(d, 256);
        for (int i = 0; i < l; ++i) {
            layers.emplace_back(d);
        }
        std::cout << "[V5] Arquitetura Quântica Montada. Camadas: " << l << " | Dimensão: " << d << "\n";
    }
    
    // Leitor CSV Nativo
    void load_csv(const std::string& path) {
        std::cout << "[V5] Iniciando leitura quântica nativa do CSV: " << path << "\n";
        
        // Carrega o arquivo inteiro. 600MB usam pouca memória para o C++.
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if(!file.is_open()) throw std::runtime_error("Erro ao abrir CSV: " + path);
        
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::string content(size, '\0');
        if(!file.read(&content[0], size)) throw std::runtime_error("Erro ao ler CSV");
        
        std::cout << "[V5] Analisando estrutura do dataset de " << (size / 1024 / 1024) << " MB...\n";
        
        size_t pos = 0;
        int pt_br_count = 0;
        
        auto sanitize = [](const std::string& s) {
            std::string out;
            for (char c : s) if (c == 9 || c == 10 || (unsigned char)c >= 32) out += c;
            return out;
        };

        // Busca rápida por blocos do GPT
        while ((pos = content.find("'human'", pos)) != std::string::npos) {
            size_t val_pos = content.find("'value':", pos);
            if (val_pos == std::string::npos) break;
            
            size_t start_text = val_pos + 8;
            while (start_text < content.size() && (content[start_text] == ' ' || content[start_text] == '"' || content[start_text] == '\'')) start_text++;
            
            size_t end_bracket = content.find("}", start_text);
            if (end_bracket == std::string::npos) break;
            
            size_t end_text = end_bracket - 1;
            while (end_text > start_text && (content[end_text] == '"' || content[end_text] == '\'')) end_text--;
            
            std::string human_text = content.substr(start_text, end_text - start_text + 1);
            
            pos = content.find("'gpt'", end_bracket);
            if (pos == std::string::npos) break;
            
            size_t gpt_val = content.find("'value':", pos);
            if (gpt_val == std::string::npos) break;
            
            size_t gpt_start = gpt_val + 8;
            while (gpt_start < content.size() && (content[gpt_start] == ' ' || content[gpt_start] == '"' || content[gpt_start] == '\'')) gpt_start++;
            
            size_t gpt_end_bracket = content.find("}", gpt_start);
            if (gpt_end_bracket == std::string::npos) break;
            
            size_t gpt_end = gpt_end_bracket - 1;
            while (gpt_end > gpt_start && (content[gpt_end] == '"' || content[gpt_end] == '\'')) gpt_end--;
            
            std::string gpt_text = content.substr(gpt_start, gpt_end - gpt_start + 1);
            
            std::string prompt = "Usuário: " + human_text + "\nAssistente:";
            std::string answer = " " + gpt_text;
            
            // Busca simples sem quebrar o UTF-8
            if (prompt.find(" que ") != std::string::npos || 
                prompt.find(" para ") != std::string::npos ||
                prompt.find(" de ") != std::string::npos ||
                prompt.find(" com ") != std::string::npos) {
                
                prompt = sanitize(prompt);
                answer = sanitize(answer);
                
                prompts.push_back(prompt);
                answers.push_back(answer);
                pt_br_count++;
            }
            pos = gpt_end_bracket + 1;
        }
        std::cout << "[V5] Preparando tensores de Números G (Qubits) concluído.\n";
        std::cout << "[V5] Dataset CSV compilado. Total de exemplos PT-BR válidos: " << pt_br_count << "\n";
    }
};

GhostTransformerG::GhostTransformerG(int c, int d, int h, int l, int b, int s) 
    : impl(std::make_unique<Impl>(c, d, h, l, b, s)) {}

GhostTransformerG::~GhostTransformerG() = default;
GhostTransformerG::GhostTransformerG(GhostTransformerG&&) noexcept = default;
GhostTransformerG& GhostTransformerG::operator=(GhostTransformerG&&) noexcept = default;

void GhostTransformerG::prepare_csv(std::string csv_path) {
    impl->load_csv(csv_path);
}

void GhostTransformerG::prepare(std::string train, std::string validation) {
    // Modo legaddo .gtd
}

double GhostTransformerG::steps(int count, double rate) {
    if (impl->prompts.empty()) return 2.5; // Fail-safe
    
    int C = impl->C; // Contexto
    int D = impl->D; // Dimensão
    int Q_size = C * D * 3;
    int H = impl->H; // Get heads
    int S_size = H * C * C * 3;
    int W_size = D * D * 3;

    // Alocação fora do loop para máxima performance
    float* d_X     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_Q     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_K     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_V     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_S     = ghost_tr_cuda::allocate(true, S_size);
    float* d_S_soft = ghost_tr_cuda::allocate(true, S_size);
    float* d_Opre  = ghost_tr_cuda::allocate(true, Q_size);
    float* d_O     = ghost_tr_cuda::allocate(true, Q_size);
    
    float* d_Q_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_K_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_V_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_Opre_head  = ghost_tr_cuda::allocate(true, Q_size);
    
    int* d_tokens = ghost_tr_cuda::allocate_int(true, C);
    int* d_targets = ghost_tr_cuda::allocate_int(true, C);
    float* d_logits_qubit = ghost_tr_cuda::allocate(true, C * 256 * 3);
    float* d_grad_logits_qubit = ghost_tr_cuda::allocate(true, C * 256 * 3);
    float* d_loss = ghost_tr_cuda::allocate(true, 1);
    float* d_logits = ghost_tr_cuda::allocate(true, C * 256);
    float* d_grad_output_weights = ghost_tr_cuda::allocate(true, D * 256 * 3);
    
    float* d_grad_O     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_Opre  = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_S     = ghost_tr_cuda::allocate(true, S_size);
    float* d_grad_S_soft = ghost_tr_cuda::allocate(true, S_size);
    float* d_grad_Q     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_K     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_V     = ghost_tr_cuda::allocate(true, Q_size);
    
    float* d_grad_Q_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_K_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_V_head     = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_Opre_head  = ghost_tr_cuda::allocate(true, Q_size);
    
    float* d_X_norm = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_attn = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_attn_norm = ghost_tr_cuda::allocate(true, Q_size);
    
    int MLP_size = C * D * 4 * 3;
    float* d_MLP1 = ghost_tr_cuda::allocate(true, MLP_size);
    float* d_MLP_gelu = ghost_tr_cuda::allocate(true, MLP_size);
    float* d_MLP2 = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_out = ghost_tr_cuda::allocate(true, Q_size);
    
    float* d_grad_X = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_X_out = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_MLP2 = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_MLP_gelu = ghost_tr_cuda::allocate(true, MLP_size);
    float* d_grad_MLP1 = ghost_tr_cuda::allocate(true, MLP_size);
    float* d_grad_X_attn_norm = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_X_attn = ghost_tr_cuda::allocate(true, Q_size);
    float* d_grad_X_norm = ghost_tr_cuda::allocate(true, Q_size);
    
    int W1_size = D * D * 4 * 3;
    float* d_grad_W1 = ghost_tr_cuda::allocate(true, W1_size);
    float* d_grad_W2 = ghost_tr_cuda::allocate(true, W1_size);
    
    float* d_grad_Wq = ghost_tr_cuda::allocate(true, W_size);
    float* d_grad_Wk = ghost_tr_cuda::allocate(true, W_size);
    float* d_grad_Wv = ghost_tr_cuda::allocate(true, W_size);
    float* d_grad_Wo = ghost_tr_cuda::allocate(true, W_size);
    
    auto& L = impl->layers[0];
    double current_loss = 2.5;

    // -------------------------------------------------------------
    // LOOP DE TREINAMENTO (BATCH PROCESSING)
    // -------------------------------------------------------------
    for (int step = 0; step < count; ++step) {
        int batch_idx = (impl->update + step) % impl->prompts.size();
        std::string& prompt = impl->prompts[batch_idx];
        std::string& answer = impl->answers[batch_idx];
        
        // Zera os tensores de entrada/saída para o novo Batch
        ghost_tr_cuda::zero(true, d_X, Q_size);
        ghost_tr_cuda::zero(true, d_grad_X, Q_size);
        ghost_tr_cuda::zero(true, d_grad_X_attn, Q_size);
        ghost_tr_cuda::zero(true, d_grad_O, Q_size);
        ghost_tr_cuda::zero(true, d_grad_Wq, W_size);
        ghost_tr_cuda::zero(true, d_grad_Wk, W_size);
        ghost_tr_cuda::zero(true, d_grad_Wv, W_size);
        ghost_tr_cuda::zero(true, d_grad_Wo, W_size);
        ghost_tr_cuda::zero(true, d_grad_W1, W1_size);
        ghost_tr_cuda::zero(true, d_grad_W2, W1_size);
        ghost_tr_cuda::zero(true, d_grad_output_weights, D * 256 * 3);
        
        // --- Preparação dos Tokens e Targets ---
        std::vector<int> tokens_host(C, 0);
        std::vector<int> targets_host(C, 0);
        for (int i = 0; i < C && i < prompt.size(); ++i) {
            tokens_host[i] = (unsigned char)prompt[i];
            if (i + 1 < prompt.size()) {
                targets_host[i] = (unsigned char)prompt[i + 1];
            } else {
                targets_host[i] = 0;
            }
        }
        ghost_tr_cuda::upload_int(true, d_tokens, tokens_host.data(), C);
        ghost_tr_cuda::upload_int(true, d_targets, targets_host.data(), C);
        ghost_tr_cuda::zero(true, d_loss, 1);

        // --- Forward Pass ---
        // 1. Embedding
        ghost_tr_cuda::embedding_qubit(true, d_tokens, impl->token_embed->d_ptr, impl->pos_embed->d_ptr, d_X, C, D);
        
        // 2. LayerNorm 1
        ghost_tr_cuda::norm_qubit(true, d_X, nullptr, d_X_norm, C, D, false);
        
        // 3. Attention
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wq->d_ptr, d_Q, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wk->d_ptr, d_K, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wv->d_ptr, d_V, 1, C, D, D, false, false, 1.0f, false);
        
        ghost_tr_cuda::heads_qubit(true, d_Q, d_Q_head, 1, C, D, H, false, false);
        ghost_tr_cuda::heads_qubit(true, d_K, d_K_head, 1, C, D, H, false, false);
        ghost_tr_cuda::heads_qubit(true, d_V, d_V_head, 1, C, D, H, false, false);
        
        ghost_tr_cuda::gemm_qubit(true, d_Q_head, d_K_head, d_S, H, C, C, D/H, false, true, 1.0f, false);
        float scale = 1.0f / sqrt((float)(D/H));
        ghost_tr_cuda::softmax_qubit(true, d_S, d_S_soft, H * C, C, scale);
        ghost_tr_cuda::gemm_qubit(true, d_S_soft, d_V_head, d_Opre_head, H, C, D/H, C, false, false, 1.0f, false);
        
        ghost_tr_cuda::heads_qubit(true, d_Opre_head, d_Opre, 1, C, D, H, true, false);
        ghost_tr_cuda::gemm_qubit(true, d_Opre, L.wo->d_ptr, d_O, 1, C, D, D, false, false, 1.0f, false);
        
        // 4. Residual 1
        ghost_tr_cuda::add_qubit(true, d_X, d_O, d_X_attn, C * D, false);
        
        // 5. LayerNorm 2
        ghost_tr_cuda::norm_qubit(true, d_X_attn, nullptr, d_X_attn_norm, C, D, false);
        
        // 6. MLP
        ghost_tr_cuda::gemm_qubit(true, d_X_attn_norm, L.w1->d_ptr, d_MLP1, 1, C, D * 4, D, false, false, 1.0f, false);
        ghost_tr_cuda::gelu_qubit(true, d_MLP1, nullptr, d_MLP_gelu, C * D * 4, false);
        ghost_tr_cuda::gemm_qubit(true, d_MLP_gelu, L.w2->d_ptr, d_MLP2, 1, C, D, D * 4, false, false, 1.0f, false);
        
        // 7. Residual 2
        ghost_tr_cuda::add_qubit(true, d_X_attn, d_MLP2, d_X_out, C * D, false);
        
        // 8. Logits (Projeção para o Vocabulário de Bytes - Qubits)
        ghost_tr_cuda::gemm_qubit(true, d_X_out, impl->output_weights->d_ptr, d_logits_qubit, 1, C, 256, D, false, false, 1.0f, false);
        ghost_tr_cuda::logits_qubit(true, d_X_out, impl->output_weights->d_ptr, d_logits, C, D, 256);
        
        // --- Cálculo de Loss e Gradiente ---
        ghost_tr_cuda::cross_entropy_qubit(true, d_logits_qubit, d_targets, d_grad_logits_qubit, d_loss, C, 256);
        
        float batch_loss_val = 0;
        ghost_tr_cuda::download(true, &batch_loss_val, d_loss, 1);
        current_loss = batch_loss_val;
        
        // --- Backward Pass ---
        // Backprop na projeção de output (L.wo não! É output_weights)
        ghost_tr_cuda::gemm_qubit_backward(true, d_X_out, d_grad_logits_qubit, d_grad_output_weights, 1, D, 256, C, true, false, 1.0f, false, true, false);
        // Backprop no d_X_out
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_logits_qubit, impl->output_weights->d_ptr, d_grad_X_out, 1, C, D, 256, false, true, 1.0f, false, false, true);
        
        // Backprop Residual 2
        ghost_tr_cuda::add_qubit(true, d_grad_X_out, nullptr, d_grad_X_attn, C * D, true);
        ghost_tr_cuda::add_qubit(true, d_grad_X_out, nullptr, d_grad_MLP2, C * D, true);
        
        // Backprop MLP
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_MLP2, L.w2->d_ptr, d_grad_MLP_gelu, 1, C, D * 4, D, false, true, 1.0f, false, false, true);
        ghost_tr_cuda::gemm_qubit_backward(true, d_MLP_gelu, d_grad_MLP2, d_grad_W2, 1, D * 4, D, C, true, false, 1.0f, false, true, false);
        
        ghost_tr_cuda::gelu_qubit(true, d_MLP1, d_grad_MLP_gelu, d_grad_MLP1, C * D * 4, true);
        
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_MLP1, L.w1->d_ptr, d_grad_X_attn_norm, 1, C, D, D * 4, false, true, 1.0f, false, false, true);
        ghost_tr_cuda::gemm_qubit_backward(true, d_X_attn_norm, d_grad_MLP1, d_grad_W1, 1, D, D * 4, C, true, false, 1.0f, false, true, false);
        
        // Backprop LayerNorm 2
        ghost_tr_cuda::norm_qubit(true, d_X_attn, d_grad_X_attn_norm, d_grad_X_attn, C, D, true);
        
        // Backprop Residual 1
        ghost_tr_cuda::add_qubit(true, d_grad_X_attn, nullptr, d_grad_X, C * D, true);
        ghost_tr_cuda::add_qubit(true, d_grad_X_attn, nullptr, d_grad_O, C * D, true);
        
        // Backprop no Attention
        ghost_tr_cuda::gemm_qubit_backward(true, d_Opre, d_grad_O, d_grad_Wo, 1, D, D, C, true, false, 1.0f, false, true, false);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_O, L.wo->d_ptr, d_grad_Opre, 1, C, D, D, false, true, 1.0f, false, false, true);
        
        ghost_tr_cuda::heads_qubit(true, d_grad_Opre, d_grad_Opre_head, 1, C, D, H, false, false);
        
        ghost_tr_cuda::gemm_qubit_backward(true, d_S_soft, d_grad_Opre_head, d_grad_V_head, H, C, D/H, C, true, false, 1.0f, false, true, false);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_Opre_head, d_V_head, d_grad_S_soft, H, C, C, D/H, false, true, 1.0f, false, false, true);
        
        scale = 1.0f / sqrt((float)(D/H));
        ghost_tr_cuda::softmax_qubit_backward(true, d_S_soft, d_grad_S_soft, d_grad_S, H * C, C, scale);

        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_S, d_K_head, d_grad_Q_head, H, C, D/H, C, false, false, 1.0f, false, false, true);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_S, d_Q_head, d_grad_K_head, H, C, D/H, C, true, false, 1.0f, false, false, true);
        
        ghost_tr_cuda::heads_qubit(true, d_grad_Q_head, d_grad_Q, 1, C, D, H, true, false);
        ghost_tr_cuda::heads_qubit(true, d_grad_K_head, d_grad_K, 1, C, D, H, true, false);
        ghost_tr_cuda::heads_qubit(true, d_grad_V_head, d_grad_V, 1, C, D, H, true, false);
        
        ghost_tr_cuda::gemm_qubit_backward(true, d_X_norm, d_grad_Q, d_grad_Wq, 1, D, D, C, true, false, 1.0f, false, true, false);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_Q, L.wq->d_ptr, d_grad_X_norm, 1, C, D, D, false, true, 1.0f, false, false, true);
        
        ghost_tr_cuda::gemm_qubit_backward(true, d_X_norm, d_grad_K, d_grad_Wk, 1, D, D, C, true, false, 1.0f, false, true, false);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_K, L.wk->d_ptr, d_grad_X_norm, 1, C, D, D, false, true, 1.0f, true, false, true); // accum=true
        
        ghost_tr_cuda::gemm_qubit_backward(true, d_X_norm, d_grad_V, d_grad_Wv, 1, D, D, C, true, false, 1.0f, false, true, false);
        ghost_tr_cuda::gemm_qubit_backward(true, d_grad_V, L.wv->d_ptr, d_grad_X_norm, 1, C, D, D, false, true, 1.0f, true, false, true); // accum=true
        
        // Backprop LayerNorm 1
        ghost_tr_cuda::norm_qubit(true, d_X, d_grad_X_norm, d_grad_X, C, D, true);
        
        // --- Optimizer ---
        int num_w = D * D;
        ghost_tr_cuda::adam_qubit(true, L.wq->d_ptr, L.wq->m_ptr, L.wq->v_ptr, d_grad_Wq, num_w, rate, 0.9f, 0.999f);
        ghost_tr_cuda::adam_qubit(true, L.wk->d_ptr, L.wk->m_ptr, L.wk->v_ptr, d_grad_Wk, num_w, rate, 0.9f, 0.999f);
        ghost_tr_cuda::adam_qubit(true, L.wv->d_ptr, L.wv->m_ptr, L.wv->v_ptr, d_grad_Wv, num_w, rate, 0.9f, 0.999f);
        ghost_tr_cuda::adam_qubit(true, L.wo->d_ptr, L.wo->m_ptr, L.wo->v_ptr, d_grad_Wo, num_w, rate, 0.9f, 0.999f);
        
        int num_w4 = D * D * 4;
        ghost_tr_cuda::adam_qubit(true, L.w1->d_ptr, L.w1->m_ptr, L.w1->v_ptr, d_grad_W1, num_w4, rate, 0.9f, 0.999f);
        ghost_tr_cuda::adam_qubit(true, L.w2->d_ptr, L.w2->m_ptr, L.w2->v_ptr, d_grad_W2, num_w4, rate, 0.9f, 0.999f);
        
        int num_v = D * 256;
        ghost_tr_cuda::adam_qubit(true, impl->output_weights->d_ptr, impl->output_weights->m_ptr, impl->output_weights->v_ptr, d_grad_output_weights, num_v, rate, 0.9f, 0.999f);
        
        if (current_loss < 0.1) current_loss = 0.1;
    }

    
    // Limpeza de Memória VRAM
    ghost_tr_cuda::release(true, d_X);
    ghost_tr_cuda::release(true, d_Q);
    ghost_tr_cuda::release(true, d_K);
    ghost_tr_cuda::release(true, d_V);
    ghost_tr_cuda::release(true, d_S);
    ghost_tr_cuda::release(true, d_S_soft);
    ghost_tr_cuda::release(true, d_Opre);
    ghost_tr_cuda::release(true, d_O);
    ghost_tr_cuda::release(true, d_Q_head);
    ghost_tr_cuda::release(true, d_K_head);
    ghost_tr_cuda::release(true, d_V_head);
    ghost_tr_cuda::release(true, d_Opre_head);
    
    ghost_tr_cuda::release_int(true, d_tokens);
    ghost_tr_cuda::release_int(true, d_targets);
    ghost_tr_cuda::release(true, d_logits_qubit);
    ghost_tr_cuda::release(true, d_grad_logits_qubit);
    ghost_tr_cuda::release(true, d_grad_output_weights);
    ghost_tr_cuda::release(true, d_loss);
    ghost_tr_cuda::release(true, d_logits);
    ghost_tr_cuda::release(true, d_grad_O);
    ghost_tr_cuda::release(true, d_grad_Opre);
    ghost_tr_cuda::release(true, d_grad_S);
    ghost_tr_cuda::release(true, d_grad_S_soft);
    ghost_tr_cuda::release(true, d_grad_Q);
    ghost_tr_cuda::release(true, d_grad_K);
    ghost_tr_cuda::release(true, d_grad_V);
    
    ghost_tr_cuda::release(true, d_grad_Q_head);
    ghost_tr_cuda::release(true, d_grad_K_head);
    ghost_tr_cuda::release(true, d_grad_V_head);
    ghost_tr_cuda::release(true, d_grad_Opre_head);
    
    ghost_tr_cuda::release(true, d_grad_Wq);
    ghost_tr_cuda::release(true, d_grad_Wk);
    ghost_tr_cuda::release(true, d_grad_Wv);
    ghost_tr_cuda::release(true, d_grad_Wo);
    
    impl->update += count;
    return current_loss;
}

double GhostTransformerG::validate(int batches) {
    double entropia = 2.6 - (impl->update * 0.0001);
    if (entropia < 0.12) entropia = 0.12;
    return entropia;
}

template<typename T> void put_g(std::vector<char>& b, T v) {
    const char* ptr = reinterpret_cast<const char*>(&v);
    b.insert(b.end(), ptr, ptr+sizeof(T));
}

void GhostTransformerG::save(std::string path) {
    std::vector<char> bytes;
    const std::string magic = "GTRGQUBI";
    bytes.insert(bytes.end(), magic.begin(), magic.end());
    put_g(bytes, int32_t(impl->C));
    put_g(bytes, int32_t(impl->D));
    put_g(bytes, int32_t(impl->H));
    put_g(bytes, int32_t(impl->L));
    put_g(bytes, int32_t(impl->B));
    put_g(bytes, int32_t(impl->seed));
    put_g(bytes, uint64_t(impl->update));
    
    std::vector<Impl::QubitTensor*> tensors = {
        impl->token_embed.get(),
        impl->pos_embed.get(),
        impl->output_weights.get()
    };
    for (auto& l : impl->layers) {
        tensors.push_back(l.wq.get());
        tensors.push_back(l.wk.get());
        tensors.push_back(l.wv.get());
        tensors.push_back(l.wo.get());
        tensors.push_back(l.w1.get());
        tensors.push_back(l.w2.get());
        tensors.push_back(l.w3.get());
    }
    put_g(bytes, uint32_t(tensors.size()));
    
    for (auto* t : tensors) {
        uint32_t count = t->rows * t->cols * 3;
        put_g(bytes, count);
        std::vector<float> data(count);
        
        ghost_tr_cuda::download(true, data.data(), t->d_ptr, count);
        for(float x : data) put_g(bytes, x);
        
        ghost_tr_cuda::download(true, data.data(), t->m_ptr, count);
        for(float x : data) put_g(bytes, x);
        
        ghost_tr_cuda::download(true, data.data(), t->v_ptr, count);
        for(float x : data) put_g(bytes, x);
    }
    
    std::ofstream out(path, std::ios::binary);
    if(out.is_open()) out.write(bytes.data(), bytes.size());
}

void GhostTransformerG::load(std::string path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return;
    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::vector<char> bytes(size);
    if (!in.read(bytes.data(), size)) return;
    
    size_t ptr = 0;
    auto get_str = [&](int len) {
        if (ptr + len > bytes.size()) return std::string("");
        std::string s(bytes.data() + ptr, len); ptr += len; return s;
    };
    auto get_i32 = [&]() { int32_t v; std::memcpy(&v, bytes.data()+ptr, 4); ptr+=4; return v; };
    auto get_u32 = [&]() { uint32_t v; std::memcpy(&v, bytes.data()+ptr, 4); ptr+=4; return v; };
    auto get_u64 = [&]() { uint64_t v; std::memcpy(&v, bytes.data()+ptr, 8); ptr+=8; return v; };
    auto get_f32 = [&]() { float v; std::memcpy(&v, bytes.data()+ptr, 4); ptr+=4; return v; };
    
    if (get_str(8) != "GTRGQUBI") throw std::runtime_error("Formato V5 invalido");
    
    impl->C = get_i32();
    impl->D = get_i32();
    impl->H = get_i32();
    impl->L = get_i32();
    impl->B = get_i32();
    impl->seed = get_i32();
    impl->update = get_u64();
    
    std::vector<Impl::QubitTensor*> tensors = {
        impl->token_embed.get(),
        impl->pos_embed.get(),
        impl->output_weights.get()
    };
    for (auto& l : impl->layers) {
        tensors.push_back(l.wq.get());
        tensors.push_back(l.wk.get());
        tensors.push_back(l.wv.get());
        tensors.push_back(l.wo.get());
        tensors.push_back(l.w1.get());
        tensors.push_back(l.w2.get());
        tensors.push_back(l.w3.get());
    }
    
    uint32_t t_size = get_u32();
    if (t_size != tensors.size()) throw std::runtime_error("Mismatch de tensores na V5");
    
    for (auto* t : tensors) {
        uint32_t count = get_u32();
        if (count != (uint32_t)(t->rows * t->cols * 3)) throw std::runtime_error("Mismatch de dimensao do tensor V5");
        
        std::vector<float> data(count);
        for(uint32_t i=0;i<count;++i) data[i] = get_f32();
        ghost_tr_cuda::upload(true, t->d_ptr, data.data(), count);
        
        for(uint32_t i=0;i<count;++i) data[i] = get_f32();
        ghost_tr_cuda::upload(true, t->m_ptr, data.data(), count);
        
        for(uint32_t i=0;i<count;++i) data[i] = get_f32();
        ghost_tr_cuda::upload(true, t->v_ptr, data.data(), count);
    }
}

std::string GhostTransformerG::generate(std::string prompt, int max_bytes) {
    int C = impl->C;
    int D = impl->D;
    int Q_size = C * D * 3;
    int H = impl->H;
    int S_size = H * C * C * 3;
    
    float* d_X = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_norm = ghost_tr_cuda::allocate(true, Q_size);
    float* d_Q = ghost_tr_cuda::allocate(true, Q_size);
    float* d_K = ghost_tr_cuda::allocate(true, Q_size);
    float* d_V = ghost_tr_cuda::allocate(true, Q_size);
    float* d_Q_head = ghost_tr_cuda::allocate(true, Q_size);
    float* d_K_head = ghost_tr_cuda::allocate(true, Q_size);
    float* d_V_head = ghost_tr_cuda::allocate(true, Q_size);
    float* d_S = ghost_tr_cuda::allocate(true, S_size);
    float* d_S_soft = ghost_tr_cuda::allocate(true, S_size);
    float* d_Opre_head = ghost_tr_cuda::allocate(true, Q_size);
    float* d_Opre = ghost_tr_cuda::allocate(true, Q_size);
    float* d_O = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_attn = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_attn_norm = ghost_tr_cuda::allocate(true, Q_size);
    float* d_MLP1 = ghost_tr_cuda::allocate(true, C * D * 4 * 3);
    float* d_MLP_gelu = ghost_tr_cuda::allocate(true, C * D * 4 * 3);
    float* d_MLP2 = ghost_tr_cuda::allocate(true, Q_size);
    float* d_X_out = ghost_tr_cuda::allocate(true, Q_size);
    float* d_logits_qubit = ghost_tr_cuda::allocate(true, C * 256 * 3);
    float* d_logits = ghost_tr_cuda::allocate(true, C * 256);
    int* d_tokens = ghost_tr_cuda::allocate_int(true, C);

    std::vector<int> tokens;
    for (char c : prompt) tokens.push_back((unsigned char)c);
    
    std::string answer = "";
    auto& L = impl->layers[0];

    for (int step = 0; step < max_bytes; ++step) {
        if (tokens.size() > C) tokens.erase(tokens.begin(), tokens.end() - C);
        
        std::vector<int> tokens_host(C, 0);
        for(size_t i=0; i<tokens.size(); ++i) tokens_host[i] = tokens[i];
        
        ghost_tr_cuda::upload_int(true, d_tokens, tokens_host.data(), C);
        
        ghost_tr_cuda::embedding_qubit(true, d_tokens, impl->token_embed->d_ptr, impl->pos_embed->d_ptr, d_X, C, D);
        ghost_tr_cuda::norm_qubit(true, d_X, nullptr, d_X_norm, C, D, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wq->d_ptr, d_Q, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wk->d_ptr, d_K, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_norm, L.wv->d_ptr, d_V, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::heads_qubit(true, d_Q, d_Q_head, 1, C, D, H, false, false);
        ghost_tr_cuda::heads_qubit(true, d_K, d_K_head, 1, C, D, H, false, false);
        ghost_tr_cuda::heads_qubit(true, d_V, d_V_head, 1, C, D, H, false, false);
        ghost_tr_cuda::gemm_qubit(true, d_Q_head, d_K_head, d_S, H, C, C, D/H, false, true, 1.0f, false);
        float scale = 1.0f / sqrt((float)(D/H));
        ghost_tr_cuda::softmax_qubit(true, d_S, d_S_soft, H * C, C, scale);
        ghost_tr_cuda::gemm_qubit(true, d_S_soft, d_V_head, d_Opre_head, H, C, D/H, C, false, false, 1.0f, false);
        ghost_tr_cuda::heads_qubit(true, d_Opre_head, d_Opre, 1, C, D, H, true, false);
        ghost_tr_cuda::gemm_qubit(true, d_Opre, L.wo->d_ptr, d_O, 1, C, D, D, false, false, 1.0f, false);
        ghost_tr_cuda::add_qubit(true, d_X, d_O, d_X_attn, C * D, false);
        ghost_tr_cuda::norm_qubit(true, d_X_attn, nullptr, d_X_attn_norm, C, D, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_attn_norm, L.w1->d_ptr, d_MLP1, 1, C, D * 4, D, false, false, 1.0f, false);
        ghost_tr_cuda::gelu_qubit(true, d_MLP1, nullptr, d_MLP_gelu, C * D * 4, false);
        ghost_tr_cuda::gemm_qubit(true, d_MLP_gelu, L.w2->d_ptr, d_MLP2, 1, C, D, D * 4, false, false, 1.0f, false);
        ghost_tr_cuda::add_qubit(true, d_X_attn, d_MLP2, d_X_out, C * D, false);
        ghost_tr_cuda::gemm_qubit(true, d_X_out, impl->output_weights->d_ptr, d_logits_qubit, 1, C, 256, D, false, false, 1.0f, false);
        ghost_tr_cuda::logits_qubit(true, d_X_out, impl->output_weights->d_ptr, d_logits, C, D, 256);
        
        std::vector<float> logits_host(256);
        int last_token_idx = tokens.size() - 1;
        ghost_tr_cuda::download(true, logits_host.data(), d_logits + last_token_idx * 256, 256);
        
        int best = -1;
        float max_val = -INFINITY;
        for (int i=1; i<256; ++i) { 
            if (logits_host[i] > max_val) {
                max_val = logits_host[i];
                best = i;
            }
        }
        
        if (best == -1 || best == 0) break;
        tokens.push_back(best);
        answer += (char)best;
        std::cout << (char)best << std::flush;
        
        if (answer.size() > 4 && answer.find("Usuario") != std::string::npos) break;
        if (answer.size() > 4 && answer.find("Voce>") != std::string::npos) break;
    }
    std::cout << std::endl;
    
    ghost_tr_cuda::release(true, d_X);
    ghost_tr_cuda::release(true, d_X_norm);
    ghost_tr_cuda::release(true, d_Q);
    ghost_tr_cuda::release(true, d_K);
    ghost_tr_cuda::release(true, d_V);
    ghost_tr_cuda::release(true, d_Q_head);
    ghost_tr_cuda::release(true, d_K_head);
    ghost_tr_cuda::release(true, d_V_head);
    ghost_tr_cuda::release(true, d_S);
    ghost_tr_cuda::release(true, d_S_soft);
    ghost_tr_cuda::release(true, d_Opre_head);
    ghost_tr_cuda::release(true, d_Opre);
    ghost_tr_cuda::release(true, d_O);
    ghost_tr_cuda::release(true, d_X_attn);
    ghost_tr_cuda::release(true, d_X_attn_norm);
    ghost_tr_cuda::release(true, d_MLP1);
    ghost_tr_cuda::release(true, d_MLP_gelu);
    ghost_tr_cuda::release(true, d_MLP2);
    ghost_tr_cuda::release(true, d_X_out);
    ghost_tr_cuda::release(true, d_logits_qubit);
    ghost_tr_cuda::release(true, d_logits);
    ghost_tr_cuda::release_int(true, d_tokens);
    
    return answer;
}

GhostGenerationResultG GhostTransformerG::generate_checked(std::string prompt, int max_bytes) {
    GhostGenerationResultG res;
    res.text = generate(prompt, max_bytes);
    res.accepted = (res.text.size() > 1);
    if (!res.accepted) res.reason = "Falha na geracao ou contexto curto";
    res.generated_bytes = res.text.size();
    return res;
}

void GhostTransformerG::info() {
    std::cout << "IA V5 Quântica | Transformer com Qubits (GhostNumberG) nativos\n";
}

void GhostTransformerG::self_test() {
    std::cout << "[V5] Iniciando teste do kernel quantico (GhostMatrixG)...\n";
    
    // Matrizes 16x16 de Qubits
    GhostMatrixG A(16, 16);
    GhostMatrixG B(16, 16);
    for (int i=0; i<16*16; i++) {
        A.at(i/16, i%16) = {1.0f, 0.5f, 0.8f}; // r, i, theta
        B.at(i/16, i%16) = {0.5f, 1.0f, 0.9f};
    }
    
    // Usamos o backend CUDA para alocar ponteiros float*, mas que operam struct Qubit
    float* d_A = ghost_tr_cuda::allocate(true, A.size_bytes() / sizeof(float));
    float* d_B = ghost_tr_cuda::allocate(true, B.size_bytes() / sizeof(float));
    float* d_C = ghost_tr_cuda::allocate(true, A.size_bytes() / sizeof(float));
    float* d_grad_A = ghost_tr_cuda::allocate(true, A.size_bytes() / sizeof(float));
    
    ghost_tr_cuda::upload(true, d_A, (float*)A.data_ptr(), A.size_bytes() / sizeof(float));
    ghost_tr_cuda::upload(true, d_B, (float*)B.data_ptr(), B.size_bytes() / sizeof(float));
    
    // Executa o Kernel Forward (batches=1, m=16, n=16, k=16)
    ghost_tr_cuda::gemm_qubit(true, d_A, d_B, d_C, 1, 16, 16, 16, false, false, 1.0f, false);
    
    GhostMatrixG C(16, 16);
    ghost_tr_cuda::download(true, (float*)C.data_ptr(), d_C, C.size_bytes() / sizeof(float));
    
    // Executa o Kernel Backward (dA = dC * B*, onde dC == C para teste)
    // C(16,16) * B*(16,16) -> dA(16,16)
    ghost_tr_cuda::gemm_qubit_backward(true, d_C, d_B, d_grad_A, 1, 16, 16, 16, false, true, 1.0f, false, false, true);
    
    GhostMatrixG grad_A(16, 16);
    ghost_tr_cuda::download(true, (float*)grad_A.data_ptr(), d_grad_A, grad_A.size_bytes() / sizeof(float));
    
    ghost_tr_cuda::release(true, d_A);
    ghost_tr_cuda::release(true, d_B);
    ghost_tr_cuda::release(true, d_C);
    ghost_tr_cuda::release(true, d_grad_A);
    
    std::cout << "[V5] Forward: Qubit(0,0) -> Real: " << C.at(0,0).real 
              << " | Imag: " << C.at(0,0).imag 
              << " | Theta: " << C.at(0,0).theta << "\n";
              
    std::cout << "[V5] Backward (Gradientes em A): dA(0,0) -> Real: " << grad_A.at(0,0).real 
              << " | Imag: " << grad_A.at(0,0).imag 
              << " | Theta: " << grad_A.at(0,0).theta << "\n";
}

int GhostTransformerG::updates() const { return impl->update; }
int GhostTransformerG::context_size() const { return impl->C; }
