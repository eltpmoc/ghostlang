// APIs nativas da IA no compilador e no REPL (paridade conferida em 13/11/2026):
// text_model, transformer_model, transformer_model_v5, matrix_g, knowledge_consult (sem rede),
// text_perplexity e text_int; os self_test conferem gradientes por diferencas finitas.
function main() {
    textmodel tm = text_model(4, 4, 8, 2, 64, 19);
    print("textmodel backend: ", tm.backend(), " epochs: ", tm.epochs());
    tm.self_test();
    var mg = matrix_g(2, 2);
    mg.set(0, 1, ghost(1.5, 2.0, 0.5, 0.25));
    // GhostMatrixG guarda GhostQubit (real, imag, theta): a componente p nao e guardada
    print("matrix_g: ", mg.get(0, 1), " bytes=", mg.size_bytes());
    var tr = transformer_model(32, 16, 2, 1, 2, 7);
    print("transformer: contexto=", tr.context_size(), " updates=", tr.updates());
    tr.self_test();
    var tg = transformer_model_v5(32, 24, 2, 1, 2, 7);
    print("transformer_v5: contexto=", tg.context_size(), " updates=", tg.updates());
    tg.self_test();
    var k = knowledge_consult("GhostLang", false, false);
    print("knowledge: allowed=", k.allowed, " found=", k.found);
    print("perplexidade(0)=", text_perplexity(0.0), " text_int=", text_int("42", 0, 100));
}
