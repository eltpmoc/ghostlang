// Objetos nativos da IA no REPL: text_model, transformer_model, transformer_model_v5,
// matrix_g, temporal_context e knowledge_consult. No programa compilado essas funções
// devolvem objetos C++; no REPL o objeto fica num registro e o valor GhostLang guarda
// só o número dele (kind Object, str "__nativo__"). Os métodos chamam os mesmos
// objetos do runtime, então compilador e REPL dão o mesmo resultado.
#include "../include/Interpreter.hpp"
#include "../include/GhostKnowledge.hpp"
#include "../include/GhostMatrixG.hpp"
#include "../include/GhostTemporal.hpp"
#include "../include/GhostTextModel.hpp"
#include "../include/GhostTransformer.hpp"
#include "../include/GhostTransformerG.hpp"
#include "../include/GhostWeb.hpp"

#include <memory>
#include <sstream>
#include <stdexcept>

// definidas em src/InterpreterCore.cpp
std::string ghost_repl_text(const GhostValue &v);
double ghost_repl_num(const GhostValue &v, const std::string &ctx);
static std::string textOf(const GhostValue &v, bool) { return ghost_repl_text(v); }
static double numOf(const GhostValue &v, const std::string &ctx) { return ghost_repl_num(v, ctx); }

namespace {
using GV = GhostValue;

struct NativeObj {
    std::string kind;
    std::shared_ptr<GhostTextModel> tm;
    std::shared_ptr<GhostTransformer> tr;
    std::shared_ptr<GhostTransformerG> tg;
    std::shared_ptr<GhostMatrixG> mg;
    std::shared_ptr<GhostTemporalContext> tc;
};

std::map<long long, NativeObj>& registry() {
    static std::map<long long, NativeObj> r;
    return r;
}

GV handle(NativeObj o) {
    static long long next = 1;
    const long long id = next++;
    GV v;
    v.kind = GV::Object;
    v.str = "__nativo__";
    v.num = static_cast<double>(id);
    v.map["tipo"] = GV::text(o.kind);
    registry()[id] = std::move(o);
    return v;
}

std::string ghostText(const GhostNumberG& g) {
    std::ostringstream os;
    os << g;
    return os.str();
}
} // namespace

bool GhostInterpreter::isNativeObject(const GhostValue& v) {
    return v.kind == GV::Object && v.str == "__nativo__";
}

bool GhostInterpreter::callNativeFactory(const std::string& name, std::vector<GhostValue>& a, GhostValue& out) {
    auto n = [&](size_t k, double padrao) { return k < a.size() ? numOf(a[k], name) : padrao; };
    auto i = [&](size_t k, int padrao) { return static_cast<int>(n(k, padrao)); };
    if (name == "text_model") {
        NativeObj o{"textmodel"};
        o.tm = std::make_shared<GhostTextModel>(text_model(i(0, 64), i(1, 32), i(2, 128), i(3, 32), i(4, 2048), i(5, 42)));
        out = handle(std::move(o));
        return true;
    }
    if (name == "transformer_model") {
        NativeObj o{"transformer"};
        o.tr = std::make_shared<GhostTransformer>(transformer_model(i(0, 384), i(1, 96), i(2, 4), i(3, 2), i(4, 8), i(5, 42)));
        out = handle(std::move(o));
        return true;
    }
    if (name == "transformer_model_v5") {
        NativeObj o{"transformer_v5"};
        o.tg = std::make_shared<GhostTransformerG>(transformer_model_v5(i(0, 384), i(1, 192), i(2, 6), i(3, 4), i(4, 4), i(5, 42)));
        out = handle(std::move(o));
        return true;
    }
    if (name == "matrix_g") {
        if (a.size() != 2) throw std::runtime_error("matrix_g(linhas, colunas)");
        NativeObj o{"matrix_g"};
        o.mg = std::make_shared<GhostMatrixG>(matrix_g(i(0, 1), i(1, 1)));
        out = handle(std::move(o));
        return true;
    }
    if (name == "temporal_context") {
        NativeObj o{"temporal_context"};
        o.tc = std::make_shared<GhostTemporalContext>(temporal_context());
        out = handle(std::move(o));
        return true;
    }
    if (name == "knowledge_consult") {
        if (a.empty() || a.size() > 3) throw std::runtime_error("knowledge_consult(tema[, online[, atualizar]])");
        const GhostKnowledgeResult r = knowledge_consult(textOf(a[0], false), a.size() > 1 ? n(1, 1) != 0 : true,
                                                         a.size() > 2 ? n(2, 0) != 0 : false);
        GV v;
        v.kind = GV::Map;
        v.str = "__registro__"; // campos lidos como k.text, k.found
        v.map["allowed"] = GV::boolean(r.allowed);
        v.map["found"] = GV::boolean(r.found);
        v.map["cached"] = GV::boolean(r.cached);
        v.map["saved"] = GV::boolean(r.saved);
        v.map["stale"] = GV::boolean(r.stale);
        v.map["text"] = GV::text(r.text);
        out = v;
        return true;
    }
    if (name == "knowledge_info") { out = GV::text(knowledge_info()); return true; }
    if (name == "web_search_public") {
        if (a.size() != 1) throw std::runtime_error("web_search_public(palavras)");
        out = GV::text(web_search_public(textOf(a[0], false)));
        return true;
    }
    if (name == "text_perplexity") {
        if (a.size() != 1) throw std::runtime_error("text_perplexity(perda)");
        out = GV::number(text_perplexity(n(0, 0)));
        return true;
    }
    if (name == "text_int") {
        if (a.size() != 3) throw std::runtime_error("text_int(texto, minimo, maximo)");
        out = GV::number(text_int(textOf(a[0], false), i(1, 0), i(2, 0)), true);
        return true;
    }
    if (name == "text_decimal") {
        if (a.size() != 3) throw std::runtime_error("text_decimal(texto, minimo, maximo)");
        out = GV::number(text_decimal(textOf(a[0], false), n(1, 0), n(2, 0)));
        return true;
    }
    return false;
}

GhostValue GhostInterpreter::callNativeMethod(const GhostValue& obj, const std::string& m, std::vector<GhostValue>& a) {
    auto it = registry().find(static_cast<long long>(obj.num));
    if (it == registry().end()) throw std::runtime_error("objeto nativo invalido");
    NativeObj& o = it->second;
    auto need = [&](size_t lo, size_t hi) {
        if (a.size() < lo || a.size() > hi) throw std::runtime_error(o.kind + "." + m + "(): numero de argumentos invalido");
    };
    auto n = [&](size_t k, double padrao) { return k < a.size() ? numOf(a[k], m) : padrao; };
    auto s = [&](size_t k, const std::string& padrao) { return k < a.size() ? textOf(a[k], false) : padrao; };
    const GV nada = GV::number(0, true);

    if (o.tm) {
        GhostTextModel& t = *o.tm;
        if (m == "prepare") { need(2, 3); t.prepare(s(0, ""), s(1, ""), s(2, "")); return nada; }
        if (m == "epoch") { need(1, 1); return GV::number(t.epoch(n(0, 0))); }
        if (m == "validate") { need(0, 0); return GV::number(t.validate()); }
        if (m == "accuracy") { need(0, 0); return GV::number(t.accuracy()); }
        if (m == "epochs") { need(0, 0); return GV::number(t.epochs(), true); }
        if (m == "save") { need(1, 1); t.save(s(0, "")); return nada; }
        if (m == "load") { need(1, 1); t.load(s(0, "")); return nada; }
        if (m == "evaluate") { need(1, 1); return GV::number(t.evaluate(s(0, ""))); }
        if (m == "generate") { need(1, 2); return GV::text(t.generate(s(0, ""), static_cast<int>(n(1, 100)))); }
        if (m == "stream") { need(1, 4); return GV::text(t.stream(s(0, ""), static_cast<int>(n(1, 100)), static_cast<int>(n(2, -1)), s(3, ""))); }
        if (m == "info") { need(0, 0); t.info(); return nada; }
        if (m == "self_test") { need(0, 0); t.self_test(); return nada; }
        if (m == "backend") { need(0, 0); return GV::text(t.backend()); }
    }
    if (o.tr || o.tg) {
        auto run = [&](auto& t) -> GV {
            if (m == "prepare") { need(2, 2); t.prepare(s(0, ""), s(1, "")); return nada; }
            if (m == "steps") { need(2, 2); return GV::number(t.steps(static_cast<int>(n(0, 0)), n(1, 0))); }
            if (m == "validate") { need(0, 1); return GV::number(t.validate(static_cast<int>(n(0, 32)))); }
            if (m == "save") { need(1, 1); t.save(s(0, "")); return nada; }
            if (m == "load") { need(1, 1); t.load(s(0, "")); return nada; }
            if (m == "generate") { need(1, 2); return GV::text(t.generate(s(0, ""), static_cast<int>(n(1, 256)))); }
            if (m == "info") { need(0, 0); t.info(); return nada; }
            if (m == "self_test") { need(0, 0); t.self_test(); return nada; }
            if (m == "updates") { need(0, 0); return GV::number(t.updates(), true); }
            if (m == "context_size") { need(0, 0); return GV::number(t.context_size(), true); }
            throw std::runtime_error(o.kind + "." + m + " nao existe");
        };
        if (o.tg && m == "prepare_csv") { need(1, 1); o.tg->prepare_csv(s(0, "")); return nada; }
        if (o.tr && m == "stream") { need(1, 4); return GV::text(o.tr->stream(s(0, ""), static_cast<int>(n(1, 256)), static_cast<int>(n(2, -1)), s(3, ""))); }
        return o.tr ? run(*o.tr) : run(*o.tg);
    }
    if (o.mg) {
        GhostMatrixG& g = *o.mg;
        auto rc = [&](size_t k) {
            const double v = n(k, 0);
            if (v < 0) throw std::runtime_error("matrix_g: indice negativo");
            return static_cast<size_t>(v);
        };
        if (m == "set") {
            need(3, 6);
            GhostNumberG gn = a.size() == 3 && a[2].kind == GV::Str ? parse_ghost_number(a[2].str)
                            : GhostNumberG(n(2, 0), n(3, 0), n(4, 0), n(5, 0));
            g.set(rc(0), rc(1), gn);
            return nada;
        }
        if (m == "get") { need(2, 2); return GV::text(ghostText(g.get(rc(0), rc(1)))); }
        if (m == "size_bytes") { need(0, 0); return GV::number(static_cast<double>(g.size_bytes()), true); }
    }
    if (o.tc) {
        GhostTemporalContext& c = *o.tc;
        if (m == "clear") { need(0, 0); c.clear(); return nada; }
        if (m == "observe") { need(0, 0); c.observe(); return nada; }
        if (m == "clock") { need(0, 0); return GV::text(c.clock()); }
        if (m == "info") { need(0, 0); return GV::text(c.info()); }
        if (m == "answer") { need(1, 1); return GV::text(c.answer(s(0, ""))); }
    }
    throw std::runtime_error(o.kind + "." + m + " nao existe");
}
