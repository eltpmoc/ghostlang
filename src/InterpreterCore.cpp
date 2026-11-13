// InterpreterCore.cpp - núcleo da linguagem no REPL da GhostLang.
//
// Avaliador de expressões com precedência e valores tipados (inteiro, real,
// texto, bool, lista, mapa), com a mesma semântica do compilador: 7 / 2 == 3
// entre inteiros, % , bitwise, &&/|| com curto-circuito,
// ?:, comparação de textos. Também: declarações tipadas, atribuições compostas,
// ++/--, listas/mapas com métodos, funções com escopo próprio e return,
// break/continue, switch com strings, try/catch/finally/throw e foreach sobre
// listas, mapas, textos e range().
#include "../include/GhostDB.hpp"
#include "../include/GhostGUI.hpp"
#include "../include/GhostAutograd.hpp"
#include "../include/GhostDataset.hpp"
#include "../include/GhostDensityMatrix.hpp"
#include "../include/GhostNDTensor.hpp"
#include "../include/GhostQHardware.hpp"
#include "../include/GhostDevice.hpp"
#include "../include/GhostLib.hpp"
#include "../include/GhostModules.hpp"
#include "../include/GhostNumText.hpp"
#include "../include/GhostTryOp.hpp"
#include "../include/Interpreter.hpp"


#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <numeric>
#include <sstream>
#include <stdexcept>

namespace {
using K = GhostValue::Kind;

std::string fmtNum(double v) {
  std::ostringstream ss;
  if (std::isfinite(v) && v == std::floor(v) && std::fabs(v) < 1e15)
    ss << static_cast<long long>(v);
  else
    ss << v;
  return ss.str();
}

std::string textOf(const GhostValue &v, bool quoted) {
  switch (v.kind) {
  case K::Num:
    return fmtNum(v.num);
  case K::Bool:
    return v.num != 0.0 ? "true" : "false";
  case K::Str:
    return quoted ? "\"" + v.str + "\"" : v.str;
  case K::List: {
    std::string s = "[";
    for (size_t k = 0; k < v.list.size(); ++k)
      s += (k ? ", " : "") + textOf(v.list[k], true);
    return s + "]";
  }
  case K::Map: {
    std::string s = "{";
    bool first = true;
    for (const auto &kv : v.map) {
      s +=
          (first ? "\"" : ", \"") + kv.first + "\": " + textOf(kv.second, true);
      first = false;
    }
    return s + "}";
  }
  case K::Object: { // Classe{campo: valor, ...}
    std::string s = v.str + "{";
    bool first = true;
    for (const auto &kv : v.map) {
      s += (first ? "" : ", ") + kv.first + ": " + textOf(kv.second, true);
      first = false;
    }
    return s + "}";
  }
  case K::Set: { // {1, 2, 3} como o operator<< de GhostSet<T>
    std::string s = "{";
    for (size_t k = 0; k < v.list.size(); ++k)
      s += (k ? ", " : "") + textOf(v.list[k], true);
    return s + "}";
  }
  case K::Quantum:
    return v.qreg ? v.qreg->ket()
                  : (v.qcirc ? v.qcirc->to_string() : std::string("qreg"));
  case K::Iface:
    return v.list.empty() ? v.str + "{}" : textOf(v.list[0], quoted);
  case K::Fn:
    return "<fn>";
  case K::Complex:
    return "(" + fmtNum(v.num) + "," + fmtNum(v.imag) + ")";
  case K::Result: // como o operator<< de GhostResult<T>
    if (v.num != 0.0)
      return "ok(" +
             (v.list.empty() ? std::string("true") : textOf(v.list[0], true)) +
             ")";
    return "err(\"" + (v.list.empty() ? std::string() : v.list[0].str) + "\")";
  }
  return "";
}

bool isIntVal(const GhostValue &v) {
  return (v.kind == K::Num && v.isInt) || v.kind == K::Bool;
}

double numOf(const GhostValue &v, const std::string &ctx) {
  if (v.kind == K::Num || v.kind == K::Bool)
    return v.num;
  throw std::runtime_error(
      ctx + ": esperado numero, recebido " +
      (v.kind == K::Str ? "texto \"" + v.str + "\"" : textOf(v, true)));
}

bool sameValue(const GhostValue &a, const GhostValue &b) {
  if (a.kind == K::Complex || b.kind == K::Complex)
    return a.kind == b.kind && a.num == b.num && a.imag == b.imag;
  if (a.kind == K::Str || b.kind == K::Str)
    return a.kind == b.kind && a.str == b.str;
  if ((a.kind == K::Num || a.kind == K::Bool) &&
      (b.kind == K::Num || b.kind == K::Bool))
    return a.num == b.num;
  return textOf(a, true) == textOf(b, true);
}

// Ordem de set<T>: números entre si, textos entre si (como std::set); tipos
// diferentes: números antes
bool setLess(const GhostValue &a, const GhostValue &b) {
  const bool an = a.kind == K::Num || a.kind == K::Bool,
             bn = b.kind == K::Num || b.kind == K::Bool;
  if (an && bn)
    return a.num < b.num;
  if (an != bn)
    return an;
  if (a.kind == K::Str && b.kind == K::Str)
    return a.str < b.str;
  return textOf(a, true) < textOf(b, true);
}

bool lessValue(const GhostValue &a, const GhostValue &b) {
  if (a.kind == K::Str && b.kind == K::Str)
    return a.str < b.str;
  if (a.kind == K::Str || b.kind == K::Str)
    return a.kind != K::Str; // números antes de textos
  return a.num < b.num;
}

int binPrec(const Token &t) {
  switch (t.type) {
  case GTokenType::LogicalOr:
    return 1;
  case GTokenType::LogicalAnd:
    return 2;
  case GTokenType::BitOr:
    return 3;
  case GTokenType::Caret:
    return 4;
  case GTokenType::BitAnd:
    return 5;
  case GTokenType::Eq:
  case GTokenType::Neq:
    return 6;
  case GTokenType::Lt:
  case GTokenType::Gt:
  case GTokenType::Leq:
  case GTokenType::Geq:
    return 7;
  case GTokenType::ShiftLeft:
  case GTokenType::Chain:
    return 8;
  case GTokenType::Plus:
  case GTokenType::Minus:
    return 9;
  case GTokenType::Star:
  case GTokenType::Slash:
  case GTokenType::Percent:
    return 10;
  case GTokenType::Power:
    return 11;
  case GTokenType::Number:
    return (!t.value.empty() && t.value[0] == '-') ? 9 : -1; // "x -1"
  default:
    return -1;
  }
}

GhostValue applyBinary(GTokenType op, const GhostValue &a,
                       const GhostValue &b) {
  using GV = GhostValue;
  switch (op) {
  case GTokenType::Plus:
    if (a.kind == K::Str || b.kind == K::Str)
      return GV::text(textOf(a, false) + textOf(b, false));
    if (a.kind == K::List && b.kind == K::List) {
      GV r = a;
      r.list.insert(r.list.end(), b.list.begin(), b.list.end());
      return r;
    }
    break;
  case GTokenType::Eq:
    return GV::boolean(sameValue(a, b));
  case GTokenType::Neq:
    return GV::boolean(!sameValue(a, b));
  case GTokenType::Lt:
    return GV::boolean(lessValue(a, b));
  case GTokenType::Gt:
    return GV::boolean(lessValue(b, a));
  case GTokenType::Leq:
    return GV::boolean(!lessValue(b, a));
  case GTokenType::Geq:
    return GV::boolean(!lessValue(a, b));
  default:
    break;
  }
  if (a.kind == K::Complex || b.kind == K::Complex) {
    std::complex<double> ca =
        a.kind == K::Complex ? std::complex<double>(a.num, a.imag)
                             : std::complex<double>(numOf(a, "operador"), 0.0);
    std::complex<double> cb =
        b.kind == K::Complex ? std::complex<double>(b.num, b.imag)
                             : std::complex<double>(numOf(b, "operador"), 0.0);
    switch (op) {
    case GTokenType::Plus: {
      auto cr = ca + cb;
      return GV::cplx(cr.real(), cr.imag());
    }
    case GTokenType::Minus: {
      auto cr = ca - cb;
      return GV::cplx(cr.real(), cr.imag());
    }
    case GTokenType::Star: {
      auto cr = ca * cb;
      return GV::cplx(cr.real(), cr.imag());
    }
    case GTokenType::Slash: {
      auto cr = ca / cb;
      return GV::cplx(cr.real(), cr.imag());
    }
    default:
      break;
    }
  }
  const double x = numOf(a, "operador"), y = numOf(b, "operador");
  const bool ints = isIntVal(a) && isIntVal(b);
  const long long lx = std::llround(x), ly = std::llround(y);
  switch (op) {
  case GTokenType::Plus:
    return GV::number(x + y, ints);
  case GTokenType::Minus:
    return GV::number(x - y, ints);
  case GTokenType::Star:
    return GV::number(x * y, ints);
  case GTokenType::Slash:
    if (ints) {
      if (ly == 0)
        throw std::runtime_error("divisao inteira por zero");
      return GV::number(static_cast<double>(lx / ly), true);
    }
    return GV::number(x / y);
  case GTokenType::Percent:
    if (ints) {
      if (ly == 0)
        throw std::runtime_error("resto de divisao por zero");
      return GV::number(static_cast<double>(lx % ly), true);
    }
    return GV::number(std::fmod(x, y));
  case GTokenType::BitAnd:
    return GV::number(static_cast<double>(lx & ly), true);
  case GTokenType::BitOr:
    return GV::number(static_cast<double>(lx | ly), true);
  case GTokenType::Caret:
    return GV::number(static_cast<double>(lx ^ ly), true);
  case GTokenType::ShiftLeft:
    return GV::number(static_cast<double>(lx << ly), true);
  case GTokenType::Chain:
    return GV::number(static_cast<double>(lx >> ly), true);
  case GTokenType::Power: { // int ** int é inteiro (ghost_ipow, o mesmo do
                            // compilado)
    if (ints)
      return GV::number(static_cast<double>(ghost_ipow(lx, ly)), true);
    return GV::number(std::pow(x, y));
  }
  default:
    break;
  }
  throw std::runtime_error("operador nao suportado");
}

// Pula um operando sem avaliar (lado não executado de &&, || e ?:).
void skipBalanced(std::vector<Token> &t, size_t &i) {
  int depth = 0;
  for (; i < t.size(); ++i) {
    const auto ty = t[i].type;
    if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
        ty == GTokenType::LBrace)
      depth++;
    else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
             ty == GTokenType::RBrace) {
      if (--depth == 0) {
        ++i;
        return;
      }
    }
  }
}
void skipLambdaExprBody(std::vector<Token> &t, size_t &i) {
  int depth = 0, innerTernary = 0;
  for (; i < t.size(); ++i) {
    const auto ty = t[i].type;
    if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
        ty == GTokenType::LBrace) {
      depth++;
      continue;
    }
    if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
        ty == GTokenType::RBrace) {
      if (depth == 0)
        return;
      depth--;
      continue;
    }
    if (depth == 0) {
      if (ty == GTokenType::Comma || ty == GTokenType::Semicolon)
        return;
      if (ty == GTokenType::Question &&
          !ghost_is_try_question(std::vector<Token>(
              t.begin() + static_cast<long>(i) + 1, t.end()))) {
        innerTernary++;
        continue;
      }
      if (ty == GTokenType::Colon) {
        if (innerTernary == 0)
          return;
        innerTernary--;
        continue;
      }
    }
  }
}
void skipOperand(std::vector<Token> &t, size_t &i) {
  if (i >= t.size())
    return;
  const auto ty = t[i].type;
  if (ty == GTokenType::LBracket) {
    size_t saveI = i;
    skipBalanced(t, i);
    if (i < t.size() && t[i].type == GTokenType::LParen) {
      size_t saveP = i;
      skipBalanced(t, i);
      if (i < t.size() && (t[i].type == GTokenType::Arrow ||
                           t[i].type == GTokenType::ThinArrow)) {
        // Cai no tratamento de lambda abaixo
      } else {
        i = saveI;
        skipBalanced(t, i);
      }
    }
  } else if (ty == GTokenType::LParen || ty == GTokenType::LBrace)
    skipBalanced(t, i);
  else
    ++i;
  // (params) [-> Ret] => expr | { ... } no ramo não avaliado de &&, || ou ?:
  if ((ty == GTokenType::LParen || ty == GTokenType::LBracket) &&
      i < t.size() &&
      (t[i].type == GTokenType::Arrow || t[i].type == GTokenType::ThinArrow)) {
    if (t[i].type == GTokenType::ThinArrow) {
      ++i;
      int angle = 0, paren = 0, bracket = 0;
      while (i < t.size()) {
        const auto rt = t[i].type;
        if (angle == 0 && paren == 0 && bracket == 0 && rt == GTokenType::Arrow)
          break;
        if (rt == GTokenType::Lt)
          angle++;
        else if (rt == GTokenType::Gt)
          angle--;
        else if (rt == GTokenType::Chain)
          angle -= 2;
        else if (rt == GTokenType::LParen)
          paren++;
        else if (rt == GTokenType::RParen)
          paren--;
        else if (rt == GTokenType::LBracket)
          bracket++;
        else if (rt == GTokenType::RBracket)
          bracket--;
        ++i;
      }
    }
    if (i < t.size() && t[i].type == GTokenType::Arrow) {
      ++i;
      if (i < t.size() && t[i].type == GTokenType::LBrace)
        skipBalanced(t, i);
      else {
        skipLambdaExprBody(t, i);
        return;
      }
    }
  }
  while (i < t.size()) {
    const auto n = t[i].type;
    if (n == GTokenType::LParen || n == GTokenType::LBracket)
      skipBalanced(t, i);
    else if (n == GTokenType::Dot)
      i += 2;
    else if (n == GTokenType::Question &&
             ghost_is_try_question(std::vector<Token>(
                 t.begin() + static_cast<long>(i) + 1, t.end())))
      ++i; // r?
    else
      break;
  }
}

GTokenType declaredOf(const std::string &typeName) {
  if (typeName == "int")
    return GTokenType::KwInt;
  if (typeName == "double" || typeName == "float")
    return GTokenType::KwDouble;
  if (typeName == "bool")
    return GTokenType::KwBool;
  if (typeName == "string")
    return GTokenType::KwString;
  if (typeName == "complex")
    return GTokenType::KwComplex;
  return GTokenType::KwVar;
}

std::string joinTokensForType(const std::vector<Token> &toks, size_t from,
                              size_t to) {
  std::string s;
  auto isW = [](char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
  };
  for (size_t k = from; k < to; ++k) {
    if (toks[k].type == GTokenType::Comma) {
      s += ",";
      continue;
    }
    const std::string &v = toks[k].value;
    if (!s.empty() && !v.empty() && isW(s.back()) && isW(v.front()))
      s += " ";
    s += v;
  }
  return s;
}
} // namespace

std::string ghost_repl_precise(double v); // InterpreterQuantum.cpp

GhostValue GhostInterpreter::applyBinary(GTokenType op, const GhostValue &a,
                                         const GhostValue &b) {
  return ::applyBinary(op, a, b);
}

// ---------------------------------------------------------------- valores

std::string GhostInterpreter::toText(const GhostValue &v, bool quoted) {
  return textOf(v, quoted);
}
bool GhostInterpreter::truthy(const GhostValue &v) {
  switch (v.kind) {
  case K::Num:
  case K::Bool:
    return v.num != 0.0;
  case K::Str:
    return !v.str.empty();
  case K::List:
    return !v.list.empty();
  case K::Map:
    return !v.map.empty();
  case K::Object:
    return true;
  case K::Result:
    return v.num != 0.0; // if (r) == r.is_ok()
  case K::Set:
    return !v.list.empty();
  case K::Quantum:
    return true;
  case K::Iface:
    return !v.list.empty();
  case K::Fn:
    return static_cast<bool>(v.fn);
  }
  return false;
}
double GhostInterpreter::toNum(const GhostValue &v, const std::string &ctx) {
  return numOf(v, ctx);
}

GhostInterpreter::SavedVar GhostInterpreter::saveVar(const std::string &n) {
  SavedVar s;
  if (numeric_memory.count(n)) {
    s.num = true;
    s.n = numeric_memory[n];
  }
  if (string_memory.count(n)) {
    s.str = true;
    s.s = string_memory[n];
  }
  if (coll_memory.count(n)) {
    s.coll = true;
    s.c = coll_memory[n];
  }
  if (type_memory.count(n)) {
    s.typ = true;
    s.t = type_memory[n];
  }
  if (var_class.count(n)) {
    s.vc = true;
    s.vcls = var_class[n];
  }
  if (qreg_memory.count(n)) {
    s.qreg = true;
    s.qr = std::make_shared<GhostQReg>(qreg_memory.at(n));
  }
  if (qcircuit_memory.count(n)) {
    s.qcirc = true;
    s.qc = std::make_shared<GhostQCircuit>(qcircuit_memory.at(n));
  }
  return s;
}

void GhostInterpreter::restoreVar(const std::string &n, const SavedVar &s) {
  numeric_memory.erase(n);
  string_memory.erase(n);
  coll_memory.erase(n);
  type_memory.erase(n);
  var_class.erase(n);
  qreg_memory.erase(n);
  qcircuit_memory.erase(n);
  if (s.num)
    numeric_memory[n] = s.n;
  if (s.str)
    string_memory[n] = s.s;
  if (s.coll)
    coll_memory[n] = s.c;
  if (s.typ)
    type_memory[n] = s.t;
  if (s.vc)
    var_class[n] = s.vcls;
  if (s.qreg && s.qr)
    qreg_memory.emplace(n, *s.qr);
  if (s.qcirc && s.qc)
    qcircuit_memory.emplace(n, *s.qc);
}

void GhostInterpreter::checkReturnedClosureRefs(
    const GhostValue &v, size_t returningFrameDepth) const {
  if (v.kind != GhostValue::Fn || !v.fn)
    return;
  const auto &c = *v.fn;
  if (c.defaultRefInFunction) {
    throw std::runtime_error(
        "nao pode retornar lambda que captura variavel local por referencia (a "
        "variavel deixa de existir ao sair da funcao)");
  }
  for (const auto &kv : c.refEnv) {
    if (kv.second.ownerDepth == returningFrameDepth &&
        returningFrameDepth < frame_ids.size() &&
        kv.second.ownerFrameId == frame_ids[returningFrameDepth]) {
      throw std::runtime_error(
          "nao pode retornar lambda que captura variavel local por referencia "
          "(a variavel deixa de existir ao sair da funcao)");
    }
  }
  if (c.refThis) {
    if (c.refThisOwnerDepth == returningFrameDepth &&
        returningFrameDepth < frame_ids.size() &&
        c.refThisOwnerFrameId == frame_ids[returningFrameDepth]) {
      throw std::runtime_error(
          "nao pode retornar lambda que captura variavel local por referencia "
          "(a variavel deixa de existir ao sair da funcao)");
    }
  }
}

void GhostInterpreter::declareLocal(const std::string &n) {
  if (frames.empty())
    return;
  auto &f = frames.back();
  if (!f.count(n))
    f[n] = saveVar(n);
}

GhostValue GhostInterpreter::lookupVar(const std::string &n, bool &found) {
  found = true;
  if (GhostValue *f = fieldRef(n))
    return *f; // dentro de método: campo (se não houver local com o nome)
  if (GhostValue *s = staticRef(n))
    return *s; // campo static da classe (ou da base)
  if (numeric_memory.count(n)) {
    const double v = numeric_memory[n];
    const GTokenType ty =
        type_memory.count(n) ? type_memory[n] : GTokenType::KwDouble;
    if (ty == GTokenType::KwBool)
      return GhostValue::boolean(v != 0.0);
    if (ty == GTokenType::KwInt)
      return GhostValue::number(v, true);
    if (ty == GTokenType::KwVar)
      return GhostValue::number(
          v, v == std::floor(v)); // declarado pelo caminho antigo
    return GhostValue::number(v);
  }
  if (string_memory.count(n))
    return GhostValue::text(string_memory[n]);
  if (coll_memory.count(n))
    return coll_memory[n];
  if (generic_functions.count(n)) {
    GhostValue fv;
    fv.kind = GhostValue::Fn;
    auto c = std::make_shared<GhostClosure>();
    c->isGenericFnVal = true;
    c->genericFnName = n;
    fv.fn = std::move(c);
    return fv;
  }
  if (functions_memory.count(n))
    return makeNamedFnValue(n, functions_memory.at(n));
  found = false;
  return {};
}

// Guarda um valor respeitando o tipo declarado (int trunca, bool vira
// true/false, string vira texto).
void GhostInterpreter::storeVar(const std::string &n, const GhostValue &vIn,
                                GTokenType declared) {
  GhostValue v = vIn;
  if (declared ==
      GTokenType::Identifier) { // atribuição: mantém o tipo atual da variável
    const GhostFieldDef *fi = nullptr;
    if (GhostValue *f =
            fieldRef(n, &fi)) { // campo do objeto (dentro de método)
      GhostValue c = coerce(v, fi->type, fi->cls, n);
      if (GhostValue *again = fieldRef(n))
        *again = c;
      return;
    }
    const GhostFieldDef *si = nullptr;
    if (GhostValue *s =
            staticRef(n, &si)) { // campo static (dentro de métodos da classe)
      GhostValue c = coerce(v, si->type, si->cls, n);
      if (GhostValue *again = staticRef(n))
        *again = c;
      (void)s;
      return;
    }
    if (var_class.count(n))
      v = coerce(v, GTokenType::KwVar, var_class[n],
                 n); // Animal a = ...; a = new Gato(..)
    if (numeric_memory.count(n) && type_memory.count(n))
      declared = type_memory[n];
    else if (string_memory.count(n))
      declared = GTokenType::KwString;
    else if (memory.count(n) && type_memory.count(n) && type_memory[n] == GTokenType::KwGhost)
      declared = GTokenType::KwGhost;
    else
      declared = GTokenType::KwVar;
  } else {
    var_class.erase(n);
  }
  numeric_memory.erase(n);
  string_memory.erase(n);
  coll_memory.erase(n);
  switch (declared) {
  case GTokenType::KwGhost: {
    GhostNumberG gn;
    if (v.kind == K::Str) {
      gn = parse_ghost_number(v.str);
    } else if (v.kind == K::Num) {
      gn = GhostNumberG(v.num, 0.0, 0.0, 0.0);
    } else if (v.kind == K::Complex) {
      gn = GhostNumberG(v.num, v.imag, 0.0, 0.0);
    }
    memory[n] = GhostMatrix(1, 1);
    memory[n](0, 0) = gn;
    type_memory[n] = GTokenType::KwGhost;
    return;
  }
  case GTokenType::KwInt: {
    double val = 0.0;
    if (v.kind == K::Str) {
      try { val = std::stod(v.str); } catch (...) {}
    } else {
      val = numOf(v, "int " + n);
    }
    numeric_memory[n] = std::trunc(val);
    type_memory[n] = GTokenType::KwInt;
    return;
  }
  case GTokenType::KwDouble:
  case GTokenType::KwFloat: {
    double val = 0.0;
    if (v.kind == K::Str) {
      try { val = std::stod(v.str); } catch (...) {}
    } else {
      val = numOf(v, (declared == GTokenType::KwFloat ? "float " : "double ") + n);
    }
    numeric_memory[n] = val;
    type_memory[n] = GTokenType::KwDouble;
    return;
  }
  case GTokenType::KwBool:
    numeric_memory[n] = truthy(v) ? 1.0 : 0.0;
    type_memory[n] = GTokenType::KwBool;
    return;
  case GTokenType::KwString:
    string_memory[n] = textOf(v, false);
    type_memory[n] = GTokenType::KwString;
    return;
  case GTokenType::KwComplex:
    if (v.kind != K::Complex && v.kind != K::Num)
      throw std::runtime_error("complex " + n + ": valor nao e complex");
    coll_memory[n] = v.kind == K::Complex ? v : GhostValue::cplx(v.num, 0.0);
    type_memory[n] = GTokenType::KwComplex;
    return;
  case GTokenType::KwList:
    if (v.kind != K::List)
      throw std::runtime_error("list " + n + ": valor nao e lista");
    coll_memory[n] = v;
    type_memory[n] = GTokenType::KwList;
    return;
  case GTokenType::KwMap:
    if (v.kind != K::Map)
      throw std::runtime_error("map " + n + ": valor nao e mapa");
    coll_memory[n] = v;
    type_memory[n] = GTokenType::KwMap;
    return;
  default:
    switch (v.kind) {
    case K::Num:
      numeric_memory[n] = v.num;
      type_memory[n] = v.isInt ? GTokenType::KwInt : GTokenType::KwDouble;
      return;
    case K::Bool:
      numeric_memory[n] = v.num;
      type_memory[n] = GTokenType::KwBool;
      return;
    case K::Str:
      string_memory[n] = v.str;
      type_memory[n] = GTokenType::KwString;
      return;
    case K::List:
      coll_memory[n] = v;
      type_memory[n] = GTokenType::KwList;
      return;
    case K::Map:
      coll_memory[n] = v;
      type_memory[n] = GTokenType::KwMap;
      return;
    case K::Object:
    case K::Result:
    case K::Set:
    case K::Quantum:
    case K::Iface:
    case K::Fn:
    case K::Complex:
      coll_memory[n] = v;
      type_memory[n] = GTokenType::KwClass;
      return;
    }
  }
}

// ---------------------------------------------------------------- expressões

GhostValue GhostInterpreter::evalValue(std::vector<Token> &tokensIn) {
  std::vector<Token> t = tokensIn;
  while (!t.empty() && t.back().type == GTokenType::Semicolon)
    t.pop_back();
  if (t.empty())
    return GhostValue::number(0, true);
  size_t i = 0;
  GhostValue v = exprTernary(t, i, true);
  if (i < t.size())
    throw std::runtime_error("expressao invalida perto de '" + t[i].value +
                             "'");
  return v;
}

GhostValue GhostInterpreter::exprTernary(std::vector<Token> &t, size_t &i,
                                         bool eval) {
  GhostValue c = exprBinary(t, i, 1, eval);
  if (i < t.size() && t[i].type == GTokenType::Question) {
    ++i;
    const bool cond = eval && truthy(c);
    GhostValue a = exprTernary(t, i, eval && cond);
    if (i >= t.size() || t[i].type != GTokenType::Colon)
      throw std::runtime_error("operador ?: sem ':'");
    ++i;
    GhostValue b = exprTernary(t, i, eval && !cond);
    return cond ? a : b;
  }
  return c;
}

GhostValue GhostInterpreter::exprBinary(std::vector<Token> &t, size_t &i,
                                        int minPrec, bool eval) {
  GhostValue lhs = exprUnary(t, i, eval);
  while (i < t.size()) {
    const int prec = binPrec(t[i]);
    if (prec < 0 || prec < minPrec)
      break;
    GTokenType op = t[i].type;
    if (op ==
        GTokenType::Number) { // "x -1": o '-' colado no número é a subtração
      op = GTokenType::Minus;
      t[i].value = t[i].value.substr(1);
    } else {
      ++i;
    }
    if (op == GTokenType::LogicalAnd || op == GTokenType::LogicalOr) {
      const bool l = eval && truthy(lhs);
      const bool evalR = eval && (op == GTokenType::LogicalAnd ? l : !l);
      GhostValue r = exprBinary(t, i, prec + 1, evalR);
      if (eval)
        lhs = GhostValue::boolean(
            op == GTokenType::LogicalAnd ? (l && truthy(r)) : (l || truthy(r)));
      continue;
    }
    const int nextMinPrec = (op == GTokenType::Power) ? prec : (prec + 1);
    GhostValue r = exprBinary(t, i, nextMinPrec, eval);
    if (eval)
      lhs = applyBinary(op, lhs, r);
  }
  return lhs;
}

// Operadores unários pegam o operando já com a potência: -x ** 2 == -(x ** 2)
// == -9 (como no Python e no compilado); 2 ** -1 continua valendo porque o
// expoente começa de novo por aqui.
GhostValue GhostInterpreter::exprUnary(std::vector<Token> &t, size_t &i,
                                       bool eval) {
  static const int powerPrec = binPrec(Token{GTokenType::Power, "**", 0, 0});
  if (i < t.size()) {
    switch (t[i].type) {
    case GTokenType::Minus: {
      ++i;
      GhostValue v = exprBinary(t, i, powerPrec, eval);
      if (!eval)
        return v;
      if (v.kind == K::Complex)
        return GhostValue::cplx(-v.num, -v.imag);
      return GhostValue::number(-numOf(v, "-"), isIntVal(v));
    }
    case GTokenType::Plus:
      ++i;
      return exprBinary(t, i, powerPrec, eval);
    case GTokenType::Not: {
      ++i;
      GhostValue v = exprBinary(t, i, powerPrec, eval);
      return GhostValue::boolean(eval ? !truthy(v) : false);
    }
    case GTokenType::Tilde: {
      ++i;
      GhostValue v = exprBinary(t, i, powerPrec, eval);
      return eval ? GhostValue::number(
                        static_cast<double>(~std::llround(numOf(v, "~"))), true)
                  : v;
    }
    case GTokenType::Number: // -2 ** 2: o lexer junta o '-' ao número; vale -(2
                             // ** 2) == -4
      if (!t[i].value.empty() && t[i].value[0] == '-' && i + 1 < t.size() &&
          t[i + 1].type == GTokenType::Power) {
        const std::string digits = t[i].value.substr(1);
        i += 2;
        GhostValue r = exprBinary(t, i, powerPrec, eval);
        if (!eval)
          return r;
        const bool real = digits.find_first_of(".eE") != std::string::npos;
        GhostValue p = applyBinary(
            GTokenType::Power, GhostValue::number(std::stod(digits), !real), r);
        return GhostValue::number(-numOf(p, "-"), isIntVal(p));
      }
      break;
    default:
      break;
    }
  }
  return exprPrimary(t, i, eval);
}

GhostValue GhostInterpreter::exprPrimary(std::vector<Token> &t, size_t &i,
                                         bool eval) {
  if (i >= t.size())
    throw std::runtime_error("expressao incompleta");
  if (!eval) {
    skipOperand(t, i);
    return GhostValue::number(0, true);
  }
  const Token tok = t[i];
  auto nextIs = [&](GTokenType ty) {
    return i + 1 < t.size() && t[i + 1].type == ty;
  };

  switch (tok.type) {
  case GTokenType::Number: {
    ++i;
    if (!tok.value.empty() && tok.value.back() == 'i') {
      double im = tok.value.size() > 1
                      ? std::stod(tok.value.substr(0, tok.value.size() - 1))
                      : 1.0;
      return exprPostfix(GhostValue::cplx(0.0, im), "", t, i, eval);
    }
    const bool real = tok.value.find_first_of(".eE") != std::string::npos;
    return exprPostfix(GhostValue::number(std::stod(tok.value), !real), "", t,
                       i, eval);
  }
  case GTokenType::String:
    ++i;
    return exprPostfix(GhostValue::text(tok.value), "", t, i, eval);
  case GTokenType::FString:
    ++i;
    return exprPostfix(GhostValue::text(evaluateFString(tok.value)), "", t, i,
                       eval);
  case GTokenType::BitOr:
  case GTokenType::LogicalOr: {
    if (isLambdaAt(t, i)) {
      GhostValue lam = evalLambdaAt(t, i, eval);
      return exprPostfix(lam, "", t, i, eval);
    }
    break;
  }
  case GTokenType::LParen: {
    if (isLambdaAt(t, i)) {
      GhostValue lam = evalLambdaAt(t, i, eval);
      return exprPostfix(lam, "", t, i, eval);
    }
    // Suporte a C-style cast: (int)expr, (double)expr, (float)expr, (bool)expr
    if (i + 2 < t.size() && t[i + 2].type == GTokenType::RParen &&
        (t[i + 1].type == GTokenType::KwInt ||
         t[i + 1].type == GTokenType::KwDouble ||
         t[i + 1].type == GTokenType::KwFloat ||
         t[i + 1].type == GTokenType::KwBool)) {
      const GTokenType castTy = t[i + 1].type;
      i += 3;
      GhostValue operand = exprUnary(t, i, eval);
      if (!eval)
        return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
      double val = numOf(operand, "cast");
      if (castTy == GTokenType::KwInt)
        return exprPostfix(GhostValue::number(std::trunc(val), true), "", t, i,
                           eval);
      if (castTy == GTokenType::KwBool)
        return exprPostfix(GhostValue::boolean(truthy(operand)), "", t, i,
                           eval);
      return exprPostfix(GhostValue::number(val, false), "", t, i, eval);
    }
    ++i;
    GhostValue v = exprTernary(t, i, eval);
    if (i >= t.size() || t[i].type != GTokenType::RParen)
      throw std::runtime_error("faltou ')'");
    ++i;
    return exprPostfix(v, "", t, i, eval);
  }
  case GTokenType::LBracket: { // [a, b, c]   e   [expr for x in colecao if
                               // cond]   e   [&x](...) => ...
    if (isCaptureLambdaAt(t, i)) {
      GhostValue lam = evalLambdaAt(t, i, eval);
      return exprPostfix(lam, "", t, i, eval);
    }
    ++i;
    {
      int depth = 0;
      size_t forAt = 0, close = 0;
      for (size_t k = i; k < t.size(); ++k) {
        const auto ty = t[k].type;
        if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
            ty == GTokenType::LBrace)
          depth++;
        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
                 ty == GTokenType::RBrace) {
          if (--depth < 0) {
            close = k;
            break;
          }
        } else if (depth == 0 && ty == GTokenType::KwFor && !forAt)
          forAt = k;
      }
      if (forAt) {
        if (!close)
          throw std::runtime_error("compreensao de lista: faltou ']'");
        GhostValue r = comprehension(t, i, close, false, eval);
        i = close + 1;
        return exprPostfix(r, "", t, i, eval);
      }
    }
    GhostValue l;
    l.kind = K::List;
    while (i < t.size() && t[i].type != GTokenType::RBracket) {
      l.list.push_back(exprTernary(t, i, eval));
      if (i < t.size() && t[i].type == GTokenType::Comma)
        ++i;
      else
        break;
    }
    if (i >= t.size() || t[i].type != GTokenType::RBracket)
      throw std::runtime_error("faltou ']' na lista");
    ++i;
    return exprPostfix(l, "", t, i, eval);
  }
  case GTokenType::LBrace: { // {"chave": valor, ...} ou compreensao {k: v for
                             // ...} / {expr for ...}
    ++i;
    {
      int depth = 0;
      size_t forAt = 0, close = 0;
      for (size_t k = i; k < t.size(); ++k) {
        const auto ty = t[k].type;
        if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
            ty == GTokenType::LBrace)
          depth++;
        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
                 ty == GTokenType::RBrace) {
          if (--depth < 0) {
            close = k;
            break;
          }
        } else if (depth == 0 && ty == GTokenType::KwFor && !forAt)
          forAt = k;
      }
      if (forAt) {
        if (!close)
          throw std::runtime_error("compreensao: faltou '}'");
        GhostValue r = comprehension(t, i, close, true, eval);
        i = close + 1;
        return exprPostfix(r, "", t, i, eval);
      }
    }
    GhostValue m;
    m.kind = K::Map;
    while (i < t.size() && t[i].type != GTokenType::RBrace) {
      GhostValue k = exprTernary(t, i, eval);
      if (i >= t.size() || t[i].type != GTokenType::Colon)
        throw std::runtime_error("mapa: use {chave: valor}");
      ++i;
      m.map[textOf(k, false)] = exprTernary(t, i, eval);
      if (i < t.size() && t[i].type == GTokenType::Comma)
        ++i;
      else
        break;
    }
    if (i >= t.size() || t[i].type != GTokenType::RBrace)
      throw std::runtime_error("faltou '}' no mapa");
    ++i;
    return exprPostfix(m, "", t, i, eval);
  }
  case GTokenType::KwInt:
  case GTokenType::KwDouble:
  case GTokenType::KwFloat:
  case GTokenType::KwBool:
  case GTokenType::KwString:
  case GTokenType::KwComplex:
  case GTokenType::KwSin:
  case GTokenType::KwCos:
  case GTokenType::KwTan:
  case GTokenType::KwRelu:
  case GTokenType::KwSigmoid:
    if (nextIs(GTokenType::LParen)) {
      ++i;
      auto args = splitArgs(t, i);
      ++i;
      bool found = false;
      GhostValue r = callBuiltin(tok.value, args, found);
      return exprPostfix(r, "", t, i, eval);
    }
    throw std::runtime_error("'" + tok.value + "' sem '(' na expressao");
  case GTokenType::KwList:
  case GTokenType::KwMap: { // list<int>()  map<string, int>()
    const bool isList = tok.type == GTokenType::KwList;
    const size_t startTy = i;
    ++i;
    int depth = 0;
    while (i < t.size()) {
      if (t[i].type == GTokenType::Lt)
        depth++;
      else if (t[i].type == GTokenType::Gt)
        depth--;
      else if (t[i].type == GTokenType::Chain)
        depth -= 2;
      ++i;
      if (depth <= 0)
        break;
    }
    std::string rawSpec = joinTokensForType(t, startTy, i);
    std::string normSpec;
    try {
      normSpec = validateAndNormalizeTypeSpec(rawSpec, false);
      ensureTypeReady(normSpec);
    } catch (...) {
    }
    if (i + 1 < t.size() && t[i].type == GTokenType::LParen &&
        t[i + 1].type == GTokenType::RParen)
      i += 2;
    GhostValue v;
    v.kind = isList ? K::List : K::Map;
    if (isList && isListSpec(normSpec))
      v.str = normSpec.substr(5, normSpec.size() - 6);
    return exprPostfix(v, "", t, i, eval);
  }
  case GTokenType::KwQreg:
  case GTokenType::KwQcircuit: { // qreg(n) / qcircuit(n): registrador novo
    const GTokenType qt = tok.type;
    ++i;
    if (i >= t.size() || t[i].type != GTokenType::LParen)
      throw std::runtime_error("use " + tok.value + "(numero_de_qubits)");
    auto raw = splitArgs(t, i);
    ++i;
    if (raw.size() != 1)
      throw std::runtime_error(tok.value +
                               "(n): informe so o numero de qubits");
    GhostValue n = evalValue(raw[0]);
    return exprPostfix(eval ? makeQuantum(qt, numOf(n, tok.value + "(n)"))
                            : GhostValue::number(0, true),
                       "", t, i, eval);
  }
  case GTokenType::KwThis: {
    if (!inMethod())
      throw std::runtime_error("this so existe dentro de metodos de classe");
    ++i;
    return exprPostfix(this_stack.back().obj, "this", t, i, eval);
  }
  case GTokenType::KwNew: { // new Classe(args)  ==  Classe(args);  new
                            // Pilha<int>()
    ++i;
    std::string cls;
    if (i + 1 < t.size() && t[i].type == GTokenType::Identifier &&
        generic_memory.count(t[i].value) && t[i + 1].type == GTokenType::Lt) {
      const std::string head = t[i].value;
      ++i;
      cls = specFromTokens(t, i, head);
      ensureInstance(cls);
    } else {
      if (i >= t.size() || t[i].type != GTokenType::Identifier ||
          !class_memory.count(t[i].value)) {
        if (i < t.size() && generic_memory.count(t[i].value))
          throw std::runtime_error(t[i].value +
                                   " e uma classe generica: use new " +
                                   t[i].value + "<tipo>(...)");
        if (i < t.size() && generic_iface_memory.count(t[i].value))
          throw std::runtime_error(
              t[i].value +
              " e interface: crie um objeto de uma classe que implementa " +
              t[i].value + "<...>");
        throw std::runtime_error("new: classe '" +
                                 (i < t.size() ? t[i].value : std::string()) +
                                 "' nao foi declarada");
      }
      cls = t[i].value;
      ++i;
    }
    std::vector<std::vector<Token>> raw;
    if (i < t.size() && t[i].type == GTokenType::LParen) {
      raw = splitArgs(t, i);
      ++i;
    }
    std::vector<GhostValue> vals;
    for (auto &r : raw)
      vals.push_back(evalValue(r));
    return exprPostfix(construct(cls, vals), "", t, i, eval);
  }
  case GTokenType::KwSuper: { // super.metodo(args): versão da classe base, sem
                              // despacho virtual
    if (!inMethod())
      throw std::runtime_error("super so existe dentro de metodos de classe");
    const std::string base = class_memory.at(this_stack.back().cls).base;
    if (base.empty())
      throw std::runtime_error("super: a classe " + this_stack.back().cls +
                               " nao tem classe base (use extends)");
    if (!nextIs(GTokenType::Dot))
      throw std::runtime_error("super(...) so vale na primeira linha do "
                               "construtor; use super.metodo(...)");
    i += 3; // super . nome (
    if (i - 1 >= t.size() || t[i - 1].type != GTokenType::Identifier ||
        i >= t.size() || t[i].type != GTokenType::LParen)
      throw std::runtime_error("use super.metodo(...)");
    const std::string m = t[i - 1].value;
    auto raw = splitArgs(t, i);
    ++i;
    LPath self{"this", {}};
    GhostValue r = callOnPath(self, this_stack.back().obj, base, m, raw, false);
    return exprPostfix(r, "", t, i, eval);
  }
  case GTokenType::KwLoadGguf: {
    ++i;
    if (i >= t.size() || t[i].type != GTokenType::LParen)
      throw std::runtime_error("load_gguf espera '('");
    auto args = splitArgs(t, i);
    ++i;
    std::string path = args.empty() ? "" : textOf(evalValue(args[0]), false);
    load_gguf(path);
    GhostValue gv = GhostValue::text(path);
    return exprPostfix(gv, "", t, i, eval);
  }
  case GTokenType::Identifier:
  case GTokenType::KwJson:
  case GTokenType::KwInput:
    break;
  default:
    throw std::runtime_error("'" + tok.value + "' inesperado na expressao");
  }

  const std::string name = tok.value;
  if (name == "true" || name == "false") {
    ++i;
    return GhostValue::boolean(name == "true");
  }
  // LLM GGUF no REPL (cerebro.prompt(...))
  if (llm_memory.count(name) && nextIs(GTokenType::Dot)) {
    i += 2; // nome .
    std::string m = t[i].value;
    ++i;
    if (i < t.size() && t[i].type == GTokenType::LParen) {
      auto args = splitArgs(t, i);
      ++i;
      if (m == "prompt") {
        std::string p = args.empty() ? "" : textOf(evalValue(args[0]), false);
        return exprPostfix(GhostValue::text(llm_memory[name].prompt(p)), "", t,
                           i, eval);
      }
      throw std::runtime_error("metodo desconhecido de llm: " + m);
    }
  }
  bool hasVar = numeric_memory.count(name) || string_memory.count(name) ||
                coll_memory.count(name) || fieldRef(name) || staticRef(name);
  if (!hasVar && name == "i") {
    ++i;
    return exprPostfix(GhostValue::cplx(0.0, 1.0), "", t, i, eval);
  }
  if (!hasVar && name == "theta") {
    ++i;
    return exprPostfix(GhostValue::number(1.0, false), "", t, i, eval);
  }

  // geo.naoExiste: o lexer só junta geo.nome quando o módulo exporta o nome
  if (!hasVar && ghost_module_aliases().count(name) &&
      nextIs(GTokenType::Dot) && i + 2 < t.size())
    throw std::runtime_error("o modulo " + name + " nao tem '" +
                             t[i + 2].value +
                             "' (nomes do modulo: funcoes, classes, interfaces "
                             "e variaveis globais do arquivo importado)");
  // json.parse(...)  /  json.stringify(...)
  if (!hasVar && name == "json" && nextIs(GTokenType::Dot) &&
      i + 2 < t.size() && t[i + 2].type == GTokenType::Identifier) {
    const std::string member = t[i + 2].value;
    if (i + 3 < t.size() && t[i + 3].type == GTokenType::LParen) {
      i += 3;
      auto raw = splitArgs(t, i);
      ++i;
      if (member == "parse") {
        if (raw.size() != 1)
          throw std::runtime_error("json.parse espera 1 argumento (string)");
        GhostValue arg = evalValue(raw[0]);
        GhostJson gj(textOf(arg, false));
        GhostValue m;
        m.kind = K::Map;
        for (const auto &kv : gj.data) {
          m.map[kv.first] = GhostValue::text(kv.second);
        }
        return exprPostfix(m, "", t, i, eval);
      } else if (member == "stringify" || member == "toString") {
        if (raw.size() != 1)
          throw std::runtime_error("json.stringify espera 1 argumento");
        GhostValue arg = evalValue(raw[0]);
        if (arg.kind == K::Map) {
          GhostJson gj;
          for (const auto &kv : arg.map) {
            gj.data[kv.first] = textOf(kv.second, false);
          }
          return exprPostfix(GhostValue::text(gj.toString()), "", t, i, eval);
        }
        return exprPostfix(GhostValue::text(textOf(arg, false)), "", t, i, eval);
      }
    }
  }
  // Classe.campo_static  /  Classe.metodo_static(args)
  if (!hasVar && class_memory.count(name) && nextIs(GTokenType::Dot) &&
      i + 2 < t.size() && t[i + 2].type == GTokenType::Identifier) {
    const std::string member = t[i + 2].value;
    std::string gowner;
    const auto *gsm = findGenericMethod(name, member, &gowner);
    if (gsm && i + 3 < t.size() &&
        (t[i + 3].type == GTokenType::LParen || genericCallAt(t, i + 3))) {
      if (!gsm->isStatic)
        throw std::runtime_error(name + "." + member +
                                 "() nao e static: chame em um objeto");
      i += 3;
      std::vector<std::string> targs;
      if (t[i].type == GTokenType::Lt)
        targs = typeArgsAt(t, i, member);
      auto raw = splitArgs(t, i);
      ++i;
      if (!eval)
        return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
      GhostValue r = callGenericMember(nullptr, gowner, member, targs, raw);
      return exprPostfix(r, "", t, i, eval);
    }
    if (i + 3 < t.size() && t[i + 3].type == GTokenType::LParen) {
      std::string owner;
      bool priv = false;
      const GhostFunction *sf = findStaticMethod(name, member, &owner, &priv);
      if (sf) {
        checkAccess(owner, priv, "metodo static " + member + "()");
        const GhostFunction f = *sf;
        i += 3;
        auto raw = splitArgs(t, i);
        ++i;
        if (!eval)
          return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
        GhostValue r = callStatic(owner, f, member, raw);
        return exprPostfix(r, "", t, i, eval);
      }
      if (!findStatic(name, member, &owner)) {
        if (findMethod(name, member, nullptr))
          throw std::runtime_error(name + "." + member +
                                   "() nao e static: chame em um objeto");
        throw std::runtime_error("a classe " + name +
                                 " nao tem o metodo static " + member + "()");
      }
    }
    std::string owner;
    const GhostFieldDef *fd = findStatic(name, member, &owner);
    if (!fd) {
      if (findField(name, member))
        throw std::runtime_error(name + "." + member +
                                 " e campo de objeto, nao static");
      throw std::runtime_error("a classe " + name + " nao tem o campo static " +
                               member);
    }
    checkAccess(owner, fd->priv, "campo static " + member);
    i += 3;
    const std::string key = owner + "::" + member;
    return exprPostfix(static_memory[key], key, t, i, eval);
  }
  // Pilha<int>(args): cria a classe concreta (na primeira vez) e o objeto
  // Caixa<int>.total / Caixa<int>.metodo_static(...)
  if (!hasVar && generic_memory.count(name)) {
    if (nextIs(GTokenType::Dot))
      throw std::runtime_error(name + " e uma classe generica: use " + name +
                               "<tipo>.membro");
    if (!nextIs(GTokenType::Lt))
      throw std::runtime_error(name + " e uma classe generica: use " + name +
                               "<tipo>(...)");
    ++i;
    const std::string spec = specFromTokens(t, i, name);
    ensureInstance(spec);
    if (i < t.size() && t[i].type == GTokenType::Dot && i + 1 < t.size() &&
        t[i + 1].type == GTokenType::Identifier) {
      const std::string member = t[i + 1].value;
      if (i + 2 < t.size() && t[i + 2].type == GTokenType::LParen) {
        std::string owner;
        bool priv = false, prot = false;
        auto sovs =
            findStaticMethodOverloads(spec, member, &owner, &priv, &prot);
        if (sovs.empty()) {
          if (!findStatic(spec, member, &owner)) {
            if (findMethod(spec, member, nullptr))
              throw std::runtime_error(spec + "." + member +
                                       "() nao e static: chame em um objeto");
            throw std::runtime_error("a classe " + spec +
                                     " nao tem o metodo static " + member +
                                     "()");
          }
        } else {
          checkAccess(owner, priv, prot, "metodo static " + member + "()");
          i += 2;
          auto raw = splitArgs(t, i);
          ++i;
          if (!eval)
            return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
          std::vector<GhostValue> preVals;
          for (auto &r : raw)
            preVals.push_back(evalValue(r));
          const GhostFunction *chosen =
              selectOverload(sovs, preVals, owner + "." + member);
          GhostValue r = callStatic(owner, *chosen, member, raw);
          return exprPostfix(r, "", t, i, eval);
        }
      }
      std::string owner;
      const GhostFieldDef *fd = findStatic(spec, member, &owner);
      if (!fd) {
        if (findField(spec, member))
          throw std::runtime_error(spec + "." + member +
                                   " e campo de objeto, nao static");
        throw std::runtime_error("a classe " + spec +
                                 " nao tem o campo static " + member);
      }
      checkAccess(owner, fd->priv, fd->prot, "campo static " + member);
      i += 2;
      const std::string key = owner + "::" + member;
      return exprPostfix(static_memory[key], key, t, i, eval);
    }
    if (i >= t.size() || t[i].type != GTokenType::LParen)
      throw std::runtime_error(spec + ": use " + spec +
                               "(...) para criar o objeto");
    auto raw = splitArgs(t, i);
    ++i;
    std::vector<GhostValue> vals;
    for (auto &r : raw)
      vals.push_back(evalValue(r));
    return exprPostfix(eval ? construct(spec, vals)
                            : GhostValue::number(0, true),
                       "", t, i, eval);
  }
  if (!hasVar && interface_memory.count(name) && nextIs(GTokenType::LParen))
    throw std::runtime_error(
        name + " e interface: crie um objeto de uma classe que implementa " +
        name);
  if (!hasVar && generic_iface_memory.count(name)) {
    if (nextIs(GTokenType::Dot))
      throw std::runtime_error(
          name + " e interface: interfaces nao tem membros static");
    if (nextIs(GTokenType::LParen) || nextIs(GTokenType::Lt))
      throw std::runtime_error(
          name + " e interface: crie um objeto de uma classe que implementa " +
          name + "<...>");
  }

  // Classe(args): cria objeto (igual a new Classe(args))
  if (nextIs(GTokenType::LParen) && !hasVar && class_memory.count(name)) {
    ++i;
    auto raw = splitArgs(t, i);
    ++i;
    std::vector<GhostValue> vals;
    for (auto &r : raw)
      vals.push_back(evalValue(r));
    return exprPostfix(construct(name, vals), "", t, i, eval);
  }
  // Dentro de método: metodo(args) chama o método do próprio objeto (virtual),
  // antes de funções globais
  if (nextIs(GTokenType::LParen) && !hasVar && inMethod() &&
      findMethod(this_stack.back().obj.str, name, nullptr)) {
    ++i;
    auto raw = splitArgs(t, i);
    ++i;
    LPath self{"this", {}};
    GhostValue r = callOnPath(self, this_stack.back().obj,
                              this_stack.back().obj.str, name, raw, true);
    return exprPostfix(r, "", t, i, eval);
  }
  // Dentro de métodos: metodo_generico(args) / metodo_generico<int>(args) da
  // própria classe (ou da base)
  if (!hasVar && (nextIs(GTokenType::LParen) || genericCallAt(t, i + 1))) {
    const std::string ctx =
        inMethod() ? this_stack.back().obj.str : currentClassCtx();
    std::string gowner;
    const auto *gm =
        ctx.empty() ? nullptr : findGenericMethod(ctx, name, &gowner);
    if (gm) {
      ++i;
      std::vector<std::string> targs;
      if (t[i].type == GTokenType::Lt)
        targs = typeArgsAt(t, i, name);
      auto raw = splitArgs(t, i);
      ++i;
      if (!eval)
        return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
      if (gm->isStatic)
        return exprPostfix(callGenericMember(nullptr, gowner, name, targs, raw),
                           "", t, i, eval);
      if (!inMethod())
        throw std::runtime_error(gowner + "." + name +
                                 "() nao e static: chame em um objeto");
      GhostValue self = this_stack.back().obj;
      GhostValue r = callGenericMember(&self, gowner, name, targs, raw);
      if (GhostValue *p = resolvePath(LPath{"this", {}}, false, true))
        *p = self;
      return exprPostfix(r, "", t, i, eval);
    }
  }
  // Dentro de métodos: metodo_static(args) da própria classe (ou da base)
  if (nextIs(GTokenType::LParen) && !hasVar) {
    const std::string ctx = currentClassCtx();
    std::string owner;
    bool priv = false;
    const GhostFunction *sf =
        ctx.empty() ? nullptr : findStaticMethod(ctx, name, &owner, &priv);
    if (sf) {
      checkAccess(owner, priv, "metodo static " + name + "()");
      const GhostFunction f = *sf;
      ++i;
      auto raw = splitArgs(t, i);
      ++i;
      if (!eval)
        return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
      GhostValue r = callStatic(owner, f, name, raw);
      return exprPostfix(r, "", t, i, eval);
    }
  }
  if (name == "PI" && !hasVar) {
    ++i;
    return GhostValue::number(3.14159265358979323846);
  }

  // Objetos quânticos: q, q.prob(0), c.draw(), quantum_backend()
  // (um campo de mesmo nome esconde o registrador global, como no C++)
  const bool qobj = (qreg_memory.count(name) || qcircuit_memory.count(name) ||
                     qtape_memory.count(name)) &&
                    !fieldRef(name);
  if (qobj && !hasVar && !nextIs(GTokenType::Dot)) {
    GhostValue gv;
    gv.kind = GhostValue::Quantum;
    if (qreg_memory.count(name))
      gv.qreg = std::make_shared<GhostQReg>(qreg_memory.at(name));
    else if (qcircuit_memory.count(name))
      gv.qcirc = std::make_shared<GhostQCircuit>(qcircuit_memory.at(name));
    ++i;
    return exprPostfix(gv, name, t, i, eval);
  }
  if ((qobj && (nextIs(GTokenType::Dot) || !hasVar)) ||
      ((name == "quantum_backend" || name == "quantum_device") &&
       nextIs(GTokenType::LParen))) {
    QValue q = evalQuantumAt(t, i);
    ++i;
    if (q.kind == QValue::Text)
      return exprPostfix(GhostValue::text(q.str), "", t, i, eval);
    if (q.kind == QValue::Number)
      return exprPostfix(GhostValue::number(q.num), "", t, i, eval);
    if (q.kind == QValue::Map) {
      GhostValue gv;
      gv.kind = GhostValue::Kind::Map;
      gv.map = std::move(q.map);
      return exprPostfix(gv, "", t, i, eval);
    }
    return GhostValue::number(0, true);
  }

  // Funcoes genericas: maior(3, 5)  /  maior<double>(3, 5.5)  /  maior<double>
  // como valor
  if (!hasVar && generic_functions.count(name) &&
      (nextIs(GTokenType::LParen) || nextIs(GTokenType::Lt))) {
    std::vector<std::string> targs;
    ++i;
    if (t[i].type == GTokenType::Lt) {
      const std::string spec = specFromTokens(t, i, name);
      int depth = 0;
      std::string cur;
      for (size_t k = name.size() + 1; k + 1 < spec.size(); ++k) {
        const char c = spec[k];
        if (c == '<')
          depth++;
        if (c == '>')
          depth--;
        if (c == ',' && depth == 0) {
          targs.push_back(cur);
          cur.clear();
          continue;
        }
        cur += c;
      }
      targs.push_back(cur);
      if (i >= t.size() || t[i].type != GTokenType::LParen) {
        const size_t expectedCount = generic_functions.at(name).params.size();
        if (targs.size() != expectedCount)
          throw std::runtime_error(
              "para usar " + name + "<...> como valor informe todos os " +
              std::to_string(expectedCount) + " tipo(s) entre < >");
        const std::string instSpec = instantiateFunction(name, targs);
        return exprPostfix(
            makeNamedFnValue(instSpec, functions_memory.at(instSpec)), "", t, i,
            eval);
      }
    }
    auto args = splitArgs(t, i);
    ++i;
    if (!eval)
      return exprPostfix(GhostValue::number(0, true), "", t, i, eval);
    return exprPostfix(callGenericFunction(name, targs, args), "", t, i, eval);
  }

  // Chamadas: funções do usuário e funções embutidas
  if (nextIs(GTokenType::LParen) && !hasVar) {
    ++i;
    auto args = splitArgs(t, i);
    ++i;
    if (functions_memory.count(name))
      return exprPostfix(callUserFunction(name, args), "", t, i, eval);
    bool found = false;
    GhostValue r = callBuiltin(name, args, found);
    if (!found)
      throw std::runtime_error("funcao nao encontrada: " + name);
    return exprPostfix(r, "", t, i, eval);
  }

  // qlayer na expressao (Q.draw(), Q.to_string())
  if (qlayer_memory.count(name) && !hasVar) {
    ++i;
    if (i < t.size() && t[i].type == GTokenType::Dot && i + 1 < t.size() &&
        t[i + 1].type == GTokenType::Identifier) {
      std::string m = t[i + 1].value;
      i += 2;
      if (i < t.size() && t[i].type == GTokenType::LParen) {
        auto args = splitArgs(t, i);
        ++i;
        auto &ql = *qlayer_memory[name];
        if (m == "draw")
          return exprPostfix(GhostValue::text(ql.draw()), "", t, i, eval);
        if (m == "to_string")
          return exprPostfix(GhostValue::text(ql.to_string()), "", t, i, eval);
        throw std::runtime_error("metodo desconhecido de qlayer: " + m);
      }
    }
    return exprPostfix(GhostValue::text(qlayer_memory[name]->to_string()), "",
                       t, i, eval);
  }

  // rede neural hibrida na expressao (rede.loss, rede.accuracy, rede.classify,
  // rede.predict, rede.summary)
  if (hybrid_memory.count(name) && !hasVar) {
    ++i;
    if (i < t.size() && t[i].type == GTokenType::Dot && i + 1 < t.size() &&
        t[i + 1].type == GTokenType::Identifier) {
      std::string m = t[i + 1].value;
      i += 2;
      if (i < t.size() && t[i].type == GTokenType::LParen) {
        auto args = splitArgs(t, i);
        ++i;
        auto &net = *hybrid_memory[name];
        if (m == "loss" || m == "accuracy") {
          if (args.size() != 2)
            throw std::runtime_error("rede." + m + "(X, Y) espera 2 matrizes");
          std::string xName = args[0][0].value;
          std::string yName = args[1][0].value;
          if (!memory.count(xName) || !memory.count(yName)) {
            throw std::runtime_error("matriz nao encontrada em rede." + m);
          }
          double res = (m == "loss")
                           ? net.loss(memory[xName], memory[yName])
                           : net.accuracy(memory[xName], memory[yName]);
          return exprPostfix(GhostValue::number(res), "", t, i, eval);
        }
        if (m == "classify") {
          std::vector<double> xs;
          for (auto &a : args)
            xs.push_back(evaluateMath(a));
          int c = net.classify_vec(xs);
          return exprPostfix(GhostValue::number((double)c, true), "", t, i,
                             eval);
        }
        if (m == "predict") {
          std::vector<double> xs;
          for (auto &a : args)
            xs.push_back(evaluateMath(a));
          double p = net.forward_vec(xs).at(0);
          return exprPostfix(GhostValue::number(p), "", t, i, eval);
        }
        if (m == "summary")
          return exprPostfix(GhostValue::text(net.summary()), "", t, i, eval);
        throw std::runtime_error("metodo desconhecido de rede hibrida: " + m);
      }
    }
    return exprPostfix(GhostValue::text(hybrid_memory[name]->summary()), "", t,
                       i, eval);
  }

  // LLM GGUF no REPL (cerebro.prompt(...))
  if (llm_memory.count(name) && !hasVar) {
    ++i;
    if (i < t.size() && t[i].type == GTokenType::Dot && i + 1 < t.size() &&
        t[i + 1].type == GTokenType::Identifier) {
      std::string m = t[i + 1].value;
      i += 2;
      if (i < t.size() && t[i].type == GTokenType::LParen) {
        auto args = splitArgs(t, i);
        ++i;
        if (m == "prompt") {
          std::string p = args.empty() ? "" : textOf(evalValue(args[0]), false);
          return exprPostfix(GhostValue::text(llm_memory[name].prompt(p)), "",
                             t, i, eval);
        }
        throw std::runtime_error("metodo desconhecido de llm: " + m);
      }
    }
    return exprPostfix(GhostValue::text("[GhostLLM]"), "", t, i, eval);
  }

  // Matriz/array antigo (state, int a[3] = {..}): a[i] é número; a sozinho é
  // texto
  if (memory.count(name) && !hasVar) {
    GhostMatrix &m = memory[name];
    ++i;
    if (i < t.size() && t[i].type == GTokenType::LParen) {
      ++i;
      GhostValue rVal = exprTernary(t, i, eval);
      if (i >= t.size())
        throw std::runtime_error("expressao incompleta na indexacao de " +
                                 name);
      if (t[i].type == GTokenType::Comma) {
        ++i;
        GhostValue cVal = exprTernary(t, i, eval);
        if (i >= t.size() || t[i].type != GTokenType::RParen)
          throw std::runtime_error("faltou ')' na indexacao de " + name);
        ++i;
        const long long r = std::llround(numOf(rVal, "linha"));
        const long long c = std::llround(numOf(cVal, "coluna"));
        if (r < 0 || r >= static_cast<long long>(m.getRows()) || c < 0 ||
            c >= static_cast<long long>(m.getCols())) {
          throw std::runtime_error("Indice (" + std::to_string(r) + ", " +
                                   std::to_string(c) +
                                   ") fora dos limites de " + name);
        }
        return exprPostfix(
            GhostValue::number(
                m(static_cast<size_t>(r), static_cast<size_t>(c)).x),
            "", t, i, eval);
      } else if (t[i].type == GTokenType::RParen) {
        ++i;
        const long long k = std::llround(numOf(rVal, "indice"));
        if (k < 0 || k >= static_cast<long long>(m.getCols()))
          throw std::runtime_error("Indice fora dos limites: " + name);
        return exprPostfix(GhostValue::number(m(0, static_cast<size_t>(k)).x),
                           "", t, i, eval);
      } else {
        throw std::runtime_error("esperado ',' ou ')' na indexacao de " + name);
      }
    }
    if (i < t.size() && t[i].type == GTokenType::LBracket) {
      ++i;
      GhostValue idx = exprTernary(t, i, eval);
      if (i >= t.size() || t[i].type != GTokenType::RBracket)
        throw std::runtime_error("faltou ']'");
      ++i;
      const long long k = std::llround(numOf(idx, "indice"));
      if (k < 0 || k >= static_cast<long long>(m.getCols()))
        throw std::runtime_error("Indice fora dos limites: " + name);
      return GhostValue::number(m(0, static_cast<size_t>(k)).x);
    }
    std::ostringstream ss;
    ss << "[ ";
    for (size_t r = 0; r < m.getRows(); ++r) {
      for (size_t c = 0; c < m.getCols(); ++c) {
        ss << m(r, c).x;
        if (m(r, c).y != 0.0)
          ss << (m(r, c).y >= 0 ? "+" : "") << m(r, c).y << "i";
        if (m(r, c).p != 0.0)
          ss << (m(r, c).p >= 0 ? "+" : "") << m(r, c).p << "g";
        ss << " ";
      }
    }
    ss << "]";
    return GhostValue::text(ss.str());
  }

  bool found = false;
  GhostValue v = lookupVar(name, found);
  if (!found)
    throw std::runtime_error("Variavel nao encontrada: " + name);
  ++i;
  return exprPostfix(v, name, t, i, eval);
}

// x[i], m["k"], s[0], xs.metodo(...), "texto".upper()
GhostValue GhostInterpreter::exprPostfix(GhostValue base,
                                         const std::string &varName,
                                         std::vector<Token> &t, size_t &i,
                                         bool eval) {
  // path acompanha onde o valor mora (variável, campo, this, elemento) enquanto
  // isso for possível: métodos que alteram (xs.push, obj.metodo) gravam o
  // resultado de volta nesse lugar.
  LPath path{varName, {}};
  bool lv = !varName.empty();
  while (i < t.size()) {
    if (t[i].type == GTokenType::LBracket) {
      ++i;
      size_t probe = i;
      int depth = 0;
      int ternary = 0;
      bool isSlice = false;
      while (probe < t.size()) {
        const auto ty = t[probe].type;
        if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
            ty == GTokenType::LBrace)
          depth++;
        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
                 ty == GTokenType::RBrace) {
          if (depth == 0)
            break;
          depth--;
        } else if (depth == 0) {
          if (ty == GTokenType::Question)
            ternary++;
          else if (ty == GTokenType::Colon) {
            if (ternary > 0)
              ternary--;
            else {
              isSlice = true;
              break;
            }
          }
        }
        probe++;
      }
      if (isSlice) {
        if (base.kind != K::List && base.kind != K::Str)
          throw std::runtime_error("'[ : ]' so vale em lista ou texto");
        bool hasStart = false, hasEnd = false, hasStep = false;
        long long sVal = 0, eVal = 0, stepVal = 1;
        if (i < t.size() && t[i].type != GTokenType::Colon) {
          hasStart = true;
          sVal = std::llround(numOf(exprTernary(t, i, eval), "start do slice"));
        }
        if (i >= t.size() || t[i].type != GTokenType::Colon)
          throw std::runtime_error("faltou ':' no fatiamento");
        ++i; // ':'
        if (i < t.size() && t[i].type != GTokenType::Colon &&
            t[i].type != GTokenType::RBracket) {
          hasEnd = true;
          eVal = std::llround(numOf(exprTernary(t, i, eval), "end do slice"));
        }
        if (i < t.size() && t[i].type == GTokenType::Colon) {
          ++i; // ':'
          if (i < t.size() && t[i].type != GTokenType::RBracket) {
            hasStep = true;
            stepVal =
                std::llround(numOf(exprTernary(t, i, eval), "step do slice"));
          }
        }
        if (i >= t.size() || t[i].type != GTokenType::RBracket)
          throw std::runtime_error("faltou ']' no fatiamento");
        ++i;
        if (stepVal == 0)
          throw std::runtime_error("o passo do fatiamento nao pode ser zero");
        const long long n = static_cast<long long>(
            base.kind == K::List ? base.list.size() : base.str.size());
        if (base.kind == K::List) {
          GhostValue r;
          r.kind = K::List;
          r.str = base.str;
          if (stepVal > 0) {
            long long s = hasStart ? sVal : 0;
            long long e = hasEnd ? eVal : n;
            if (s < 0)
              s += n;
            if (e < 0)
              e += n;
            s = std::max(0LL, std::min(s, n));
            e = std::max(0LL, std::min(e, n));
            for (long long idx = s; idx < e; idx += stepVal)
              r.list.push_back(base.list[static_cast<size_t>(idx)]);
          } else {
            long long s = hasStart ? sVal : (n - 1);
            long long e = hasEnd ? eVal : -1; // fim omitido: até o índice 0 (o
                                              // -1 aqui não é "último")
            if (s < 0)
              s += n;
            if (hasEnd && e < 0)
              e += n;
            s = std::max(-1LL, std::min(s, n - 1));
            e = std::max(-1LL, std::min(e, n - 1));
            for (long long idx = s; idx > e; idx += stepVal)
              r.list.push_back(base.list[static_cast<size_t>(idx)]);
          }
          base = r;
        } else {
          std::string res;
          if (stepVal > 0) {
            long long s = hasStart ? sVal : 0;
            long long e = hasEnd ? eVal : n;
            if (s < 0)
              s += n;
            if (e < 0)
              e += n;
            s = std::max(0LL, std::min(s, n));
            e = std::max(0LL, std::min(e, n));
            for (long long idx = s; idx < e; idx += stepVal)
              res += base.str[static_cast<size_t>(idx)];
          } else {
            long long s = hasStart ? sVal : (n - 1);
            long long e = hasEnd ? eVal : -1; // fim omitido: até o índice 0 (o
                                              // -1 aqui não é "último")
            if (s < 0)
              s += n;
            if (hasEnd && e < 0)
              e += n;
            s = std::max(-1LL, std::min(s, n - 1));
            e = std::max(-1LL, std::min(e, n - 1));
            for (long long idx = s; idx > e; idx += stepVal)
              res += base.str[static_cast<size_t>(idx)];
          }
          base = GhostValue::text(res);
        }
        lv = false;
        continue;
      }
      GhostValue k = exprTernary(t, i, eval);
      if (i >= t.size() || t[i].type != GTokenType::RBracket)
        throw std::runtime_error("faltou ']' no indice");
      ++i;
      if (lv)
        path.steps.push_back({false, k});
      if (base.kind == K::Str)
        lv = false; // letra de texto: só leitura
      if (base.kind == K::List || base.kind == K::Str) {
        const long long n = static_cast<long long>(
            base.kind == K::List ? base.list.size() : base.str.size());
        long long idx = std::llround(numOf(k, "indice"));
        const long long orig = idx;
        if (idx < 0)
          idx += n;
        if (idx < 0 || idx >= n)
          throw std::runtime_error("indice " + std::to_string(orig) +
                                   " fora da lista de " + std::to_string(n) +
                                   " elemento(s)");
        GhostValue e = base.kind == K::List
                           ? base.list[static_cast<size_t>(idx)]
                           : GhostValue::text(std::string(
                                 1, base.str[static_cast<size_t>(idx)]));
        base = e;
      } else if (base.kind == K::Map) {
        const std::string key = textOf(k, false);
        auto it = base.map.find(key);
        if (it == base.map.end())
          throw std::runtime_error("chave \"" + key + "\" nao existe no mapa");
        GhostValue e = it->second;
        base = e;
      } else {
        throw std::runtime_error("'[ ]' so vale em lista, mapa ou texto");
      }
      continue;
    }
    if (t[i].type == GTokenType::Dot && i + 1 < t.size()) {
      const std::string m = t[i + 1].value;
      i += 2;
      const bool call = i < t.size() && t[i].type == GTokenType::LParen;
      if (base.kind == K::Iface) { // f.area(): só métodos da interface,
                                   // despachados para a classe real
        if (!call)
          throw std::runtime_error("a interface " + base.str +
                                   " nao tem campos ('." + m +
                                   "'); use os metodos de " + base.str);
        bool has = false;
        for (const auto &p : ifaceMethodList(base.str))
          if (p.first == m)
            has = true;
        if (!has)
          throw std::runtime_error("a interface " + base.str +
                                   " nao tem o metodo " + m + "()");
        if (base.list.empty())
          throw std::runtime_error(
              base.str +
              " sem objeto: guarde um objeto antes de chamar um metodo");
        auto raw = splitArgs(t, i);
        ++i;
        GhostValue inner = base.list[0];
        GhostValue r = callMethod(inner, inner.str, m, raw, true);
        if (lv)
          if (GhostValue *p = resolvePath(path, false, true))
            if (p->kind == K::Iface && !p->list.empty())
              p->list[0] = inner;
        base = r;
        lv = false;
        continue;
      }
      if (base.kind == K::Quantum) { // campo.q.h(0), q.prob(1) dentro de
                                     // métodos, r.draw()...
        if (!call)
          throw std::runtime_error("'." + m + "': use " + m +
                                   "(...) (metodo de qreg/qcircuit)");
        base = quantumCall(lv ? &path : nullptr, base, m, t, i, eval);
        lv = false;
        continue;
      }
      if (base.kind == K::Object) {
        std::string gowner;
        const auto *gm = findGenericMethod(base.str, m, &gowner);
        if (gm &&
            (call || genericCallAt(
                         t, i))) { // método genérico: obj.m(x) / obj.m<int>(x)
          std::vector<std::string> targs;
          if (!call)
            targs = typeArgsAt(t, i, m);
          auto raw = splitArgs(t, i);
          ++i;
          if (!eval) {
            base = GhostValue::number(0, true);
            lv = false;
            continue;
          }
          GhostValue self = base;
          GhostValue r = callGenericMember(&self, gowner, m, targs, raw);
          if (lv && !gm->isStatic)
            if (GhostValue *p = resolvePath(path, false, true))
              *p = self;
          base = r;
          lv = false;
          continue;
        }
        if (!call) { // obj.campo
          base = readField(base, m);
          if (lv)
            path.steps.push_back({true, GhostValue::text(m)});
          continue;
        }
        // Se m não é método mas é um campo fn<...> (ou var com função), lê o
        // campo e deixa '(' chamar a função
        if (!findMethod(base.str, m, nullptr) &&
            !findStaticMethod(base.str, m, nullptr, nullptr)) {
          const GhostFieldDef *fd = findField(base.str, m);
          std::string sowner;
          const GhostFieldDef *sfd =
              fd ? nullptr : findStatic(base.str, m, &sowner);
          if (fd || sfd) {
            GhostValue fv = readField(base, m);
            if (fv.kind == K::Fn || (fd && isFnSpec(fd->cls)) ||
                (sfd && isFnSpec(sfd->cls))) {
              base = fv;
              if (lv && fd)
                path.steps.push_back({true, GhostValue::text(m)});
              else if (sfd) {
                path = LPath{sowner + "::" + m, {}};
                lv = true;
              }
              continue;
            }
          }
        }
        auto raw = splitArgs(t, i);
        ++i;
        if (lv)
          base = callOnPath(path, base, base.str, m, raw, true);
        else {
          GhostValue tmp = base;
          base = callMethod(tmp, tmp.str, m, raw, true);
        }
        lv = false;
        continue;
      }
      std::vector<std::vector<Token>> args;
      if (call) {
        args = splitArgs(t, i);
        ++i;
      }
      const bool hasTarget =
          lv && rootRef(path.root) &&
          (base.kind == K::List || base.kind == K::Map || base.kind == K::Set);
      base = collMethod(hasTarget ? &path : nullptr, base, m, args);
      lv = false;
      continue;
    }
    if (t[i].type == GTokenType::LParen) { // f(args), (criarContador())(),
                                           // ops[0](5), obj.cb(5)
      if (base.kind != K::Fn)
        throw std::runtime_error("'" + textOf(base, true) +
                                 "' nao e uma funcao (use fn<...> ou lambda)");
      auto raw = splitArgs(t, i);
      ++i;
      if (!eval) {
        base = GhostValue::number(0, true);
        lv = false;
        continue;
      }
      const bool hasTarget = lv && rootRef(path.root) != nullptr;
      base = invokeFnValue(hasTarget ? &path : nullptr, base, raw);
      lv = false;
      continue;
    }
    // r? (result<T>): o '?' seguido de ; ) , . : ou operador binário
    // (include/GhostTryOp.hpp)
    if (t[i].type == GTokenType::Question &&
        ghost_is_try_question(std::vector<Token>(
            t.begin() + static_cast<long>(i) + 1, t.end()))) {
      ++i;
      if (eval)
        base = tryOperator(base);
      lv = false;
      continue;
    }
    break;
  }
  return base;
}

// Compreensoes de lista, conjunto e mapa com suporte a multiplos 'for', 'if'
// encadeados e 'for k, v in mapa'
GhostValue GhostInterpreter::comprehension(std::vector<Token> &t, size_t start,
                                           size_t close, bool isBrace,
                                           bool eval) {
  size_t firstFor = std::string::npos;
  int depth = 0;
  for (size_t k = start; k < close; ++k) {
    const auto ty = t[k].type;
    if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
        ty == GTokenType::LBrace)
      depth++;
    else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
             ty == GTokenType::RBrace)
      depth--;
    else if (depth == 0 && ty == GTokenType::KwFor) {
      firstFor = k;
      break;
    }
  }
  const std::string compKindDesc =
      !isBrace ? "compreensao de lista: " : "compreensao: ";
  if (firstFor == std::string::npos || firstFor == start)
    throw std::runtime_error(compKindDesc + "faltou a expressao antes de for");
  bool isMap = false;
  std::vector<Token> keyExprT, valExprT;
  if (isBrace) {
    size_t colonAt = std::string::npos;
    depth = 0;
    int ternDepth = 0;
    for (size_t k = start; k < firstFor; ++k) {
      const auto ty = t[k].type;
      if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
          ty == GTokenType::LBrace)
        depth++;
      else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
               ty == GTokenType::RBrace)
        depth--;
      else if (depth == 0) {
        if (ty == GTokenType::Question &&
            !ghost_is_try_question(
                std::vector<Token>(t.begin() + static_cast<long>(k + 1),
                                   t.begin() + static_cast<long>(firstFor)))) {
          ternDepth++;
        } else if (ty == GTokenType::Colon) {
          if (ternDepth > 0)
            ternDepth--;
          else if (colonAt == std::string::npos)
            colonAt = k;
        }
      }
    }
    if (colonAt != std::string::npos) {
      isMap = true;
      keyExprT.assign(t.begin() + static_cast<long>(start),
                      t.begin() + static_cast<long>(colonAt));
      valExprT.assign(t.begin() + static_cast<long>(colonAt + 1),
                      t.begin() + static_cast<long>(firstFor));
      if (keyExprT.empty() || valExprT.empty())
        throw std::runtime_error(
            "compreensao de mapa: use {chave: valor for ...}");
    }
  }
  if (!isMap)
    keyExprT.assign(t.begin() + static_cast<long>(start),
                    t.begin() + static_cast<long>(firstFor));
  struct Clause {
    std::vector<std::string> vars;
    std::vector<Token> collT;
    std::vector<std::vector<Token>> ifs;
  };
  std::vector<Clause> clauses;
  size_t cur = firstFor;
  while (cur < close) {
    if (t[cur].type != GTokenType::KwFor)
      throw std::runtime_error(compKindDesc + "esperado 'for'");
    ++cur;
    std::vector<std::string> cvars;
    if (cur >= close || t[cur].type != GTokenType::Identifier)
      throw std::runtime_error(compKindDesc +
                               "esperado o nome da variavel depois de for");
    cvars.push_back(t[cur++].value);
    if (cur + 1 < close && t[cur].type == GTokenType::Comma &&
        t[cur + 1].type == GTokenType::Identifier) {
      cur++;
      cvars.push_back(t[cur++].value);
    }
    if (cur >= close || t[cur].type != GTokenType::KwIn)
      throw std::runtime_error(compKindDesc + "faltou 'in' (use [expr for " +
                               cvars[0] + " in lista])");
    ++cur;
    const size_t collStart = cur;
    depth = 0;
    while (cur < close) {
      const auto ty = t[cur].type;
      if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
          ty == GTokenType::LBrace)
        depth++;
      else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
               ty == GTokenType::RBrace)
        depth--;
      else if (depth == 0 &&
               (ty == GTokenType::KwFor || ty == GTokenType::KwIf))
        break;
      cur++;
    }
    Clause cl;
    cl.vars = cvars;
    cl.collT.assign(t.begin() + static_cast<long>(collStart),
                    t.begin() + static_cast<long>(cur));
    if (cl.collT.empty())
      throw std::runtime_error(compKindDesc + "faltou a colecao depois de in");
    while (cur < close && t[cur].type == GTokenType::KwIf) {
      ++cur;
      const size_t ifStart = cur;
      depth = 0;
      while (cur < close) {
        const auto ty = t[cur].type;
        if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
            ty == GTokenType::LBrace)
          depth++;
        else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
                 ty == GTokenType::RBrace)
          depth--;
        else if (depth == 0 &&
                 (ty == GTokenType::KwFor || ty == GTokenType::KwIf))
          break;
        cur++;
      }
      std::vector<Token> cond(t.begin() + static_cast<long>(ifStart),
                              t.begin() + static_cast<long>(cur));
      if (cond.empty())
        throw std::runtime_error(compKindDesc +
                                 "faltou a condicao depois de if");
      cl.ifs.push_back(std::move(cond));
    }
    clauses.push_back(std::move(cl));
  }
  GhostValue out;
  if (isMap)
    out.kind = K::Map;
  else if (isBrace)
    out.kind = K::Set;
  else
    out.kind = K::List;
  if (!eval)
    return out;
  std::function<void(size_t)> runClause = [&](size_t idx) {
    if (idx >= clauses.size()) {
      if (isMap) {
        std::vector<Token> kt = keyExprT, vt = valExprT;
        GhostValue kv = evalValue(kt);
        GhostValue vv = evalValue(vt);
        out.map[textOf(kv, false)] = vv;
      } else {
        std::vector<Token> xt = keyExprT;
        out.list.push_back(evalValue(xt));
      }
      return;
    }
    const auto &cl = clauses[idx];
    std::vector<Token> collT = cl.collT;
    const GhostValue coll = evalValue(collT);
    std::vector<std::pair<GhostValue, GhostValue>> items;
    if (coll.kind == K::List || coll.kind == K::Set) {
      for (const auto &e : coll.list)
        items.push_back({e, GhostValue{}});
    } else if (coll.kind == K::Map) {
      for (const auto &kv : coll.map)
        items.push_back({GhostValue::text(kv.first), kv.second});
    } else if (coll.kind == K::Str) {
      for (char ch : coll.str)
        items.push_back({GhostValue::text(std::string(1, ch)), GhostValue{}});
    } else {
      std::string src = tokensToSource(cl.collT);
      while (!src.empty() && src.back() == ' ')
        src.pop_back();
      throw std::runtime_error(compKindDesc + "'" + src +
                               "' nao e lista, conjunto, mapa ou texto");
    }
    std::vector<SavedVar> saved;
    for (const auto &v : cl.vars)
      saved.push_back(saveVar(v));
    try {
      for (const auto &it : items) {
        storeVar(cl.vars[0], it.first, GTokenType::KwVar);
        if (cl.vars.size() > 1)
          storeVar(cl.vars[1], it.second, GTokenType::KwVar);
        bool pass = true;
        for (auto condT : cl.ifs) {
          if (!truthy(evalValue(condT))) {
            pass = false;
            break;
          }
        }
        if (pass)
          runClause(idx + 1);
      }
    } catch (...) {
      for (size_t vi = 0; vi < cl.vars.size(); ++vi)
        restoreVar(cl.vars[vi], saved[vi]);
      throw;
    }
    for (size_t vi = 0; vi < cl.vars.size(); ++vi)
      restoreVar(cl.vars[vi], saved[vi]);
  };
  runClause(0);
  if (out.kind == K::Set)
    normalizeSet(out);
  return out;
}

void GhostInterpreter::normalizeSet(GhostValue &s) {
  std::stable_sort(s.list.begin(), s.list.end(), setLess);
  std::vector<GhostValue> u;
  for (auto &e : s.list)
    if (u.empty() || setLess(u.back(), e))
      u.push_back(e);
  s.list.swap(u);
}

// Métodos de set<T> (os mesmos do GhostSet<T> do compilado). add/remove/clear
// alteram o conjunto guardado.
GhostValue GhostInterpreter::setMethod(const LPath *path,
                                       const GhostValue &self,
                                       const std::string &m,
                                       std::vector<GhostValue> &a) {
  using GV = GhostValue;
  auto need = [&](size_t lo, size_t hi) {
    if (a.size() < lo || a.size() > hi)
      throw std::runtime_error("set." + m + ": numero de argumentos invalido");
  };
  auto mut = [&]() -> GhostValue & {
    GhostValue *target = path ? resolvePath(*path, false, false) : nullptr;
    if (!target)
      throw std::runtime_error(
          "set." + m + " altera o conjunto: use em uma variavel (ex.: s." + m +
          "(...))");
    return *target;
  };
  auto elem = [&](const GV &v) {
    return self.str.empty() ? v : coerceSpec(v, self.str, "set." + m);
  };
  auto find = [&](const GV &s, const GV &v) {
    for (const auto &e : s.list)
      if (!setLess(e, v) && !setLess(v, e))
        return true;
    return false;
  };
  auto other = [&]() { // argumento de union/intersection/...: conjunto (ou
                       // lista) do mesmo tipo
    if (a[0].kind != K::Set && a[0].kind != K::List)
      throw std::runtime_error("set." + m + ": esperado conjunto, recebido " +
                               textOf(a[0], true));
    GV o;
    o.kind = K::Set;
    for (const auto &e : a[0].list)
      o.list.push_back(elem(e));
    normalizeSet(o);
    return o;
  };
  auto made = [&](std::vector<GV> xs) {
    GV r;
    r.kind = K::Set;
    r.str = self.str;
    r.list = std::move(xs);
    normalizeSet(r);
    return r;
  };
  const auto &L = self.list;
  if (m == "length" || m == "size") {
    need(0, 0);
    return GV::number(static_cast<double>(L.size()), true);
  }
  if (m == "empty") {
    need(0, 0);
    return GV::boolean(L.empty());
  }
  if (m == "contains" || m == "has") {
    need(1, 1);
    return GV::boolean(find(self, elem(a[0])));
  }
  if (m == "add") {
    need(1, 1);
    const GV v = elem(a[0]);
    GV &t = mut();
    if (find(t, v))
      return GV::boolean(false);
    t.list.push_back(v);
    normalizeSet(t);
    return GV::boolean(true);
  }
  if (m == "remove") {
    need(1, 1);
    const GV v = elem(a[0]);
    GV &t = mut();
    for (size_t k = 0; k < t.list.size(); ++k)
      if (!setLess(t.list[k], v) && !setLess(v, t.list[k])) {
        t.list.erase(t.list.begin() + static_cast<long>(k));
        return GV::boolean(true);
      }
    return GV::boolean(false);
  }
  if (m == "clear") {
    need(0, 0);
    mut().list.clear();
    return GV::number(0, true);
  }
  if (m == "union") {
    need(1, 1);
    std::vector<GV> r = L;
    const GV o = other();
    r.insert(r.end(), o.list.begin(), o.list.end());
    return made(r);
  }
  if (m == "intersection" || m == "difference") {
    need(1, 1);
    const GV o = other();
    std::vector<GV> r;
    for (const auto &e : L)
      if (find(o, e) == (m == "intersection"))
        r.push_back(e);
    return made(r);
  }
  if (m == "is_subset" || m == "is_superset") {
    need(1, 1);
    const GV o = other();
    const GV &small = m == "is_subset" ? self : o;
    const GV &big = m == "is_subset" ? o : self;
    for (const auto &e : small.list)
      if (!find(big, e))
        return GV::boolean(false);
    return GV::boolean(true);
  }
  if (m == "to_list") {
    need(0, 0);
    GV r;
    r.kind = K::List;
    r.list = L;
    return r;
  }
  if (m == "min" || m == "max") {
    need(0, 0);
    if (L.empty())
      throw std::runtime_error("set." + m + ": conjunto vazio");
    return m == "min" ? L.front() : L.back();
  }
  throw std::runtime_error("set nao tem o metodo ." + m +
                           " (use add, remove, contains, union, intersection, "
                           "difference, is_subset, to_list...)");
}

// Métodos de lista, mapa e texto. Os que alteram exigem um lugar (variável,
// campo, elemento).
GhostValue
GhostInterpreter::collMethod(const LPath *path, const GhostValue &self,
                             const std::string &m,
                             std::vector<std::vector<Token>> &rawArgs) {
  std::vector<GhostValue> a;
  for (auto &r : rawArgs)
    a.push_back(evalValue(r));
  if (self.kind == K::Result)
    return resultMethod(self, m, a); // r.is_ok(), r.value_or(0)...
  if (self.kind == K::Set)
    return setMethod(path, self, m, a); // s.add(3), s.union(o)...
  auto need = [&](size_t lo, size_t hi) {
    if (a.size() < lo || a.size() > hi)
      throw std::runtime_error("." + m + ": numero de argumentos invalido");
  };
  // O alvo é localizado depois de avaliar os argumentos (eles podem chamar
  // funções que mexem nas variáveis).
  auto mut = [&]() -> GhostValue & {
    GhostValue *target = path ? resolvePath(*path, false, false) : nullptr;
    if (!target)
      throw std::runtime_error(
          "." + m + " altera a colecao: use em uma variavel (ex.: xs." + m +
          "(...))");
    return *target;
  };
  auto index = [&](long long i, size_t n) {
    const long long orig = i;
    if (i < 0)
      i += static_cast<long long>(n);
    if (i < 0 || i >= static_cast<long long>(n))
      throw std::runtime_error("indice " + std::to_string(orig) +
                               " fora da lista de " + std::to_string(n) +
                               " elemento(s)");
    return static_cast<size_t>(i);
  };
  using GV = GhostValue;
  if (self.kind == K::List) {
    const auto &L = self.list;
    if (m == "length" || m == "size") {
      need(0, 0);
      return GV::number(static_cast<double>(L.size()), true);
    }
    if (m == "empty") {
      need(0, 0);
      return GV::boolean(L.empty());
    }
    if (m == "push" || m == "append") {
      need(1, 1);
      auto &t = mut();
      GV val = a[0];
      if (!t.str.empty())
        val = coerceSpec(val, t.str,
                         "list.push (elemento de list<" + t.str + ">)");
      t.list.push_back(val);
      return GV::number(0, true);
    }
    if (m == "pop") {
      need(0, 0);
      auto &t = mut();
      if (t.list.empty())
        throw std::runtime_error("list.pop: lista vazia");
      GV v = t.list.back();
      t.list.pop_back();
      return v;
    }
    if (m == "insert_at") {
      need(2, 2);
      auto &t = mut();
      long long i = std::llround(numOf(a[0], "insert_at"));
      if (i < 0)
        i += static_cast<long long>(t.list.size());
      if (i < 0 || i > static_cast<long long>(t.list.size()))
        throw std::runtime_error("list.insert_at: indice fora da lista");
      GV val = a[1];
      if (!t.str.empty())
        val = coerceSpec(val, t.str,
                         "list.insert_at (elemento de list<" + t.str + ">)");
      t.list.insert(t.list.begin() + i, val);
      return GV::number(0, true);
    }
    if (m == "remove_at") {
      need(1, 1);
      auto &t = mut();
      const size_t k =
          index(std::llround(numOf(a[0], "remove_at")), t.list.size());
      GV v = t.list[k];
      t.list.erase(t.list.begin() + static_cast<long long>(k));
      return v;
    }
    if (m == "clear") {
      need(0, 0);
      mut().list.clear();
      return GV::number(0, true);
    }
    if (m == "sort") {
      need(0, 0);
      auto &t = mut();
      std::stable_sort(t.list.begin(), t.list.end(), lessValue);
      return t;
    }
    if (m == "reverse") {
      need(0, 0);
      auto &t = mut();
      std::reverse(t.list.begin(), t.list.end());
      return t;
    }
    if (m == "sorted") {
      need(0, 0);
      GV c = self;
      std::stable_sort(c.list.begin(), c.list.end(), lessValue);
      return c;
    }
    if (m == "contains") {
      need(1, 1);
      GV needle = a[0];
      if (!self.str.empty())
        try {
          needle = coerceSpec(needle, self.str, "contains");
        } catch (...) {
        }
      for (const auto &e : L)
        if (sameValue(e, needle))
          return GV::boolean(true);
      return GV::boolean(false);
    }
    if (m == "index_of") {
      need(1, 1);
      GV needle = a[0];
      if (!self.str.empty())
        try {
          needle = coerceSpec(needle, self.str, "index_of");
        } catch (...) {
        }
      for (size_t k = 0; k < L.size(); ++k)
        if (sameValue(L[k], needle))
          return GV::number(static_cast<double>(k), true);
      return GV::number(-1, true);
    }
    if (m == "first") {
      need(0, 0);
      return L.at(index(0, L.size()));
    }
    if (m == "last") {
      need(0, 0);
      return L.at(index(-1, L.size()));
    }
    if (m == "sum") {
      need(0, 0);
      double s = 0;
      bool ints = true;
      for (const auto &e : L) {
        s += numOf(e, "list.sum");
        ints = ints && isIntVal(e);
      }
      return GV::number(s, ints);
    }
    if (m == "min" || m == "max") {
      need(0, 0);
      if (L.empty())
        throw std::runtime_error("list." + m + ": lista vazia");
      GV best = L[0];
      for (const auto &e : L)
        if (m == "min" ? lessValue(e, best) : lessValue(best, e))
          best = e;
      return best;
    }
    if (m == "join") {
      need(0, 1);
      const std::string sep = a.empty() ? "" : textOf(a[0], false);
      std::string s;
      for (size_t k = 0; k < L.size(); ++k)
        s += (k ? sep : "") + textOf(L[k], false);
      return GV::text(s);
    }
    if (m == "slice") {
      need(2, 3);
      const long long n = static_cast<long long>(L.size());
      long long from = std::llround(numOf(a[0], "slice")),
                to = std::llround(numOf(a[1], "slice"));
      long long step = a.size() == 3 ? std::llround(numOf(a[2], "slice")) : 1;
      if (step == 0)
        throw std::runtime_error("o passo do fatiamento nao pode ser zero");
      GV r;
      r.kind = K::List;
      r.str = self.str;
      if (step > 0) {
        if (from < 0)
          from += n;
        if (to < 0)
          to += n;
        from = std::max(0LL, std::min(from, n));
        to = std::max(0LL, std::min(to, n));
        for (long long idx = from; idx < to; idx += step)
          r.list.push_back(L[static_cast<size_t>(idx)]);
      } else {
        if (from < 0)
          from += n;
        if (to < 0)
          to += n;
        from = std::max(-1LL, std::min(from, n - 1));
        to = std::max(-1LL, std::min(to, n - 1));
        for (long long idx = from; idx > to; idx += step)
          r.list.push_back(L[static_cast<size_t>(idx)]);
      }
      return r;
    }
    if (m == "vec_add" || m == "vec_sub" || m == "vec_mul" || m == "vec_div") {
      need(1, 1);
      if (a[0].kind != K::List)
        throw std::runtime_error("list." + m + ": esperado outra lista");
      const auto &o = a[0].list;
      if (L.size() != o.size())
        throw std::runtime_error(m + ": vetores com tamanhos diferentes (" +
                                 std::to_string(L.size()) +
                                 " != " + std::to_string(o.size()) + ")");
      GV r;
      r.kind = K::List;
      r.str = m == "vec_div" ? "double" : self.str;
      for (size_t k = 0; k < L.size(); ++k) {
        double v1 = numOf(L[k], m);
        double v2 = numOf(o[k], m);
        if (m == "vec_add")
          r.list.push_back(
              GV::number(v1 + v2, isIntVal(L[k]) && isIntVal(o[k])));
        else if (m == "vec_sub")
          r.list.push_back(
              GV::number(v1 - v2, isIntVal(L[k]) && isIntVal(o[k])));
        else if (m == "vec_mul")
          r.list.push_back(
              GV::number(v1 * v2, isIntVal(L[k]) && isIntVal(o[k])));
        else if (m == "vec_div") {
          if (v2 == 0)
            throw std::runtime_error("divisao por zero");
          r.list.push_back(GV::number(v1 / v2, false));
        }
      }
      return r;
    }
    if (m == "vec_scale") {
      need(1, 1);
      double k = numOf(a[0], "vec_scale");
      GV r;
      r.kind = K::List;
      r.str = "double";
      for (const auto &e : L)
        r.list.push_back(GV::number(numOf(e, "vec_scale") * k, false));
      return r;
    }
    if (m == "dot") {
      need(1, 1);
      if (a[0].kind != K::List)
        throw std::runtime_error("list.dot: esperado outra lista");
      const auto &o = a[0].list;
      if (L.size() != o.size())
        throw std::runtime_error("dot: vetores com tamanhos diferentes (" +
                                 std::to_string(L.size()) +
                                 " != " + std::to_string(o.size()) + ")");
      double s = 0.0;
      for (size_t k = 0; k < L.size(); ++k)
        s += numOf(L[k], "dot") * numOf(o[k], "dot");
      return GV::number(s, false);
    }
    if (m == "norm") {
      need(0, 0);
      double s = 0.0;
      for (const auto &e : L) {
        double v = numOf(e, "norm");
        s += v * v;
      }
      return GV::number(std::sqrt(s), false);
    }
    if (m == "map") {
      need(1, 1);
      if (a[0].kind != K::Fn || !a[0].fn)
        throw std::runtime_error("list.map: esperado uma funcao ou lambda");
      GV r;
      r.kind = K::List;
      for (const auto &item : L) {
        r.list.push_back(invokeFnValueDirect(a[0], {item}));
      }
      return r;
    }
    if (m == "filter") {
      need(1, 1);
      if (a[0].kind != K::Fn || !a[0].fn)
        throw std::runtime_error("list.filter: esperado uma funcao ou lambda");
      GV r;
      r.kind = K::List;
      r.str = self.str;
      for (const auto &item : L) {
        GV keep = invokeFnValueDirect(a[0], {item});
        if (truthy(keep))
          r.list.push_back(item);
      }
      return r;
    }
    if (m == "reduce") {
      need(2, 2);
      if (a[0].kind != K::Fn || !a[0].fn)
        throw std::runtime_error("list.reduce: esperado uma funcao ou lambda");
      GV acc = a[1];
      for (const auto &item : L) {
        acc = invokeFnValueDirect(a[0], {acc, item});
      }
      return acc;
    }
    if (m == "for_each" || m == "forEach") {
      need(1, 1);
      if (a[0].kind != K::Fn || !a[0].fn)
        throw std::runtime_error("list.for_each: esperado uma funcao ou lambda");
      for (const auto &item : L) {
        invokeFnValueDirect(a[0], {item});
      }
      return GV::number(0, true);
    }
    throw std::runtime_error("list." + m + " nao existe");
  }
  if (self.kind == K::Map) {
    const auto &M = self.map;
    if (m == "toString" || m == "to_string" || m == "stringify") {
      need(0, 0);
      GhostJson gj;
      for (const auto &kv : M) {
        gj.data[kv.first] = textOf(kv.second, false);
      }
      return GV::text(gj.toString());
    }
    if (m == "length" || m == "size") {
      need(0, 0);
      return GV::number(static_cast<double>(M.size()), true);
    }
    if (m == "empty") {
      need(0, 0);
      return GV::boolean(M.empty());
    }
    if (m == "has" || m == "contains") {
      need(1, 1);
      return GV::boolean(M.count(textOf(a[0], false)) > 0);
    }
    if (m == "get") {
      need(1, 2);
      auto it = M.find(textOf(a[0], false));
      if (it != M.end())
        return it->second;
      if (a.size() == 2)
        return a[1];
      throw std::runtime_error("chave \"" + textOf(a[0], false) +
                               "\" nao existe no mapa");
    }
    if (m == "set") {
      need(2, 2);
      mut().map[textOf(a[0], false)] = a[1];
      return GV::number(0, true);
    }
    if (m == "remove") {
      need(1, 1);
      return GV::boolean(mut().map.erase(textOf(a[0], false)) > 0);
    }
    if (m == "clear") {
      need(0, 0);
      mut().map.clear();
      return GV::number(0, true);
    }
    if (m == "keys" || m == "values") {
      need(0, 0);
      GV r;
      r.kind = K::List;
      for (const auto &kv : M)
        r.list.push_back(m == "keys" ? GV::text(kv.first) : kv.second);
      return r;
    }
    throw std::runtime_error("map." + m + " nao existe");
  }
  if (self.kind == K::Str) {
    const std::string &s = self.str;
    if (m == "length" || m == "size") {
      need(0, 0);
      return GV::number(static_cast<double>(s.size()), true);
    }
    if (m == "upper" || m == "lower") {
      need(0, 0);
      std::string r = s;
      for (auto &ch : r)
        ch = static_cast<char>(
            m == "upper" ? std::toupper(static_cast<unsigned char>(ch))
                         : std::tolower(static_cast<unsigned char>(ch)));
      return GV::text(r);
    }
    if (m == "contains") {
      need(1, 1);
      return GV::boolean(s.find(textOf(a[0], false)) != std::string::npos);
    }
    if (m == "starts_with") {
      need(1, 1);
      const auto p = textOf(a[0], false);
      return GV::boolean(s.rfind(p, 0) == 0);
    }
    if (m == "ends_with") {
      need(1, 1);
      const auto p = textOf(a[0], false);
      return GV::boolean(s.size() >= p.size() &&
                         s.compare(s.size() - p.size(), p.size(), p) == 0);
    }
    if (m == "index_of") {
      need(1, 1);
      const auto p = s.find(textOf(a[0], false));
      return GV::number(p == std::string::npos ? -1.0 : static_cast<double>(p),
                        true);
    }
    if (m == "trim") {
      need(0, 0);
      const auto b = s.find_first_not_of(" \t\r\n");
      if (b == std::string::npos)
        return GV::text("");
      return GV::text(s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1));
    }
    if (m == "split") {
      need(1, 1);
      const std::string sep = textOf(a[0], false);
      GV r;
      r.kind = K::List;
      if (sep.empty()) {
        for (char ch : s)
          r.list.push_back(GV::text(std::string(1, ch)));
        return r;
      }
      size_t from = 0, at;
      while ((at = s.find(sep, from)) != std::string::npos) {
        r.list.push_back(GV::text(s.substr(from, at - from)));
        from = at + sep.size();
      }
      r.list.push_back(GV::text(s.substr(from)));
      return r;
    }
    if (m == "replace") {
      need(2, 2);
      const std::string from = textOf(a[0], false), to = textOf(a[1], false);
      if (from.empty())
        return self;
      std::string r = s;
      for (size_t p = 0; (p = r.find(from, p)) != std::string::npos;
           p += to.size())
        r.replace(p, from.size(), to);
      return GV::text(r);
    }
    if (m == "substr") {
      need(1, 2);
      const long long st = std::llround(numOf(a[0], "substr"));
      if (st < 0 || st > static_cast<long long>(s.size()))
        throw std::runtime_error("substr: inicio fora do texto");
      return GV::text(a.size() > 1 ? s.substr(static_cast<size_t>(st),
                                              static_cast<size_t>(std::llround(
                                                  numOf(a[1], "substr"))))
                                   : s.substr(static_cast<size_t>(st)));
    }
    throw std::runtime_error("string." + m + " nao existe");
  }
  throw std::runtime_error("." + m +
                           ": metodo em valor que nao e lista, mapa ou texto");
}

GhostValue GhostInterpreter::callBuiltin(const std::string &name,
                                         std::vector<std::vector<Token>> &raw,
                                         bool &found) {
  found = true;
  using GV = GhostValue;
  std::vector<GV> a;
  for (auto &r : raw)
    a.push_back(evalValue(r));
  auto need = [&](size_t lo, size_t hi) {
    if (a.size() < lo || a.size() > hi)
      throw std::runtime_error(name + "(): numero de argumentos invalido");
  };
  auto x = [&](size_t k) { return numOf(a.at(k), name); };
  if (name == "input" || name == "inputln") {
    need(0, 2);
    std::string prompt = "";
    if (!a.empty()) {
      prompt = a[0].str.empty() && a[0].kind == K::Num ? std::to_string(a[0].num) : a[0].str;
    }
    // Suporte ao formato: inputln(f"Digite seu nome: {nome}");
    if (raw.size() == 1 && !raw[0].empty() && raw[0][0].type == GTokenType::FString) {
      const std::string& rawText = raw[0][0].value;
      size_t braceOpen = rawText.rfind('{');
      size_t braceClose = rawText.rfind('}');
      if (braceOpen != std::string::npos && braceClose != std::string::npos && braceClose > braceOpen) {
        size_t varStart = braceOpen + 1;
        std::string varName = rawText.substr(varStart, braceClose - varStart);
        while (!varName.empty() && std::isspace((unsigned char)varName.front())) varName.erase(varName.begin());
        while (!varName.empty() && std::isspace((unsigned char)varName.back())) varName.pop_back();

        size_t promptEnd = braceOpen;
        if (braceOpen > 0 && rawText[braceOpen - 1] == '$') promptEnd = braceOpen - 1;
        std::string p = rawText.substr(0, promptEnd);
        if (!p.empty()) std::cout << p << std::flush;

        std::string line;
        if (std::cin.peek() == '\n') std::cin.get();
        else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
        std::getline(std::cin, line);
        if (!line.empty() && line.back() == '\r') line.pop_back();

        storeVar(varName, GV::text(line), GTokenType::Identifier);
        return GV::text(line);
      }
    } else if (raw.size() == 2 && !raw[1].empty() && raw[1][0].type == GTokenType::Identifier) {
      // inputln("prompt", var);
      if (!prompt.empty()) std::cout << prompt << std::flush;
      std::string line;
      if (std::cin.peek() == '\n') std::cin.get();
      else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
      std::getline(std::cin, line);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      storeVar(raw[1][0].value, GV::text(line), GTokenType::Identifier);
      return GV::text(line);
    } else if (raw.size() == 1 && !raw[0].empty() && raw[0][0].type == GTokenType::Identifier &&
               (numeric_memory.count(raw[0][0].value) || string_memory.count(raw[0][0].value) || coll_memory.count(raw[0][0].value))) {
      // inputln(var);
      std::string line;
      if (std::cin.peek() == '\n') std::cin.get();
      else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
      std::getline(std::cin, line);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      storeVar(raw[0][0].value, GV::text(line), GTokenType::Identifier);
      return GV::text(line);
    }

    if (!prompt.empty()) std::cout << prompt << std::flush;
    std::string line;
    if (std::cin.peek() == '\n') std::cin.get();
    else if (std::cin.peek() == '\r') { std::cin.get(); if (std::cin.peek() == '\n') std::cin.get(); }
    std::getline(std::cin, line);
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return GV::text(line);
  }
  if (name == "to_4d") {
    need(1, 2);
    bool with_pref = a.size() > 1 ? truthy(a[1]) : false;
    if (a[0].kind == K::Str) {
      GhostNumberG gn = parse_ghost_number(a[0].str);
      return GV::text(gn.to_4d(with_pref));
    }
    if (a[0].kind == K::Num) {
      GhostNumberG gn(a[0].num, 0.0, 0.0, 0.0);
      return GV::text(gn.to_4d(with_pref));
    }
    if (a[0].kind == K::Complex) {
      GhostNumberG gn(a[0].num, a[0].imag, 0.0, 0.0);
      return GV::text(gn.to_4d(with_pref));
    }
    return GV::text(a[0].str);
  }
  if (name == "to_math" || name == "to_algebraic") {
    need(1, 2);
    bool with_pref = a.size() > 1 ? truthy(a[1]) : false;
    if (a[0].kind == K::Str) {
      GhostNumberG gn = parse_ghost_number(a[0].str);
      return GV::text(gn.to_math(with_pref));
    }
    if (a[0].kind == K::Num) {
      GhostNumberG gn(a[0].num, 0.0, 0.0, 0.0);
      return GV::text(gn.to_math(with_pref));
    }
    if (a[0].kind == K::Complex) {
      GhostNumberG gn(a[0].num, a[0].imag, 0.0, 0.0);
      return GV::text(gn.to_math(with_pref));
    }
    return GV::text(a[0].str);
  }
  if (name == "sqrt" || name == "raiz_quadrada" || name == "csqrt" || name == "raiz_quadrada_complexa" || name == "complex_sqrt" || name == "ghost_csqrt") {
    need(1, 1);
    if (a[0].kind == K::Complex) {
      std::complex<double> z(a[0].num, a[0].imag);
      auto res = std::sqrt(z);
      return GV::cplx(res.real(), res.imag());
    }
    double val = numOf(a[0], name);
    if (val < 0.0 || name == "csqrt" || name == "raiz_quadrada_complexa" || name == "complex_sqrt" || name == "ghost_csqrt") {
      auto res = std::sqrt(std::complex<double>(val, 0.0));
      return GV::cplx(res.real(), res.imag());
    }
    return GV::number(std::sqrt(val));
  }
  if (name == "cbrt" || name == "raiz_cubica" || name == "ccbrt" || name == "raiz_cubica_complexa") {
    need(1, 1);
    if (a[0].kind == K::Complex || name == "ccbrt" || name == "raiz_cubica_complexa") {
      std::complex<double> z = a[0].kind == K::Complex ? std::complex<double>(a[0].num, a[0].imag)
                                                       : std::complex<double>(numOf(a[0], name), 0.0);
      double r = std::abs(z);
      double theta = std::arg(z);
      double r_root = std::cbrt(r);
      double theta_root = theta / 3.0;
      auto polar_res = std::polar(r_root, theta_root);
      return GV::cplx(polar_res.real(), polar_res.imag());
    }
    return GV::number(std::cbrt(numOf(a[0], name)));
  }
  if (name == "nth_root" || name == "raiz_n" || name == "root" ||
      name == "nth_root_c" || name == "raiz_n_complexa" || name == "croot" ||
      name == "complex_nth_root" || name == "ghost_nth_root" || name == "ghost_nth_root_complex") {
    need(2, 2);
    int n = static_cast<int>(numOf(a[1], name));
    if (n <= 0)
      throw std::runtime_error("indice da raiz deve ser um numero natural maior que zero (n >= 1)");
    if (a[0].kind == K::Complex || name == "nth_root_c" || name == "raiz_n_complexa" ||
        name == "croot" || name == "complex_nth_root" || name == "ghost_nth_root_complex") {
      std::complex<double> z = a[0].kind == K::Complex ? std::complex<double>(a[0].num, a[0].imag)
                                                       : std::complex<double>(numOf(a[0], name), 0.0);
      if (n == 1) return GV::cplx(z.real(), z.imag());
      if (n == 2) {
        auto sq = std::sqrt(z);
        return GV::cplx(sq.real(), sq.imag());
      }
      double r = std::abs(z);
      double theta = std::arg(z);
      double r_root = std::pow(r, 1.0 / static_cast<double>(n));
      double theta_root = theta / static_cast<double>(n);
      auto polar_res = std::polar(r_root, theta_root);
      return GV::cplx(polar_res.real(), polar_res.imag());
    }
    double val = numOf(a[0], name);
    if (n == 1) return GV::number(val);
    if (val == 0.0) return GV::number(0.0);
    if (val < 0.0) {
      if (n % 2 != 0) return GV::number(-std::pow(-val, 1.0 / n));
      double r_root = std::pow(-val, 1.0 / static_cast<double>(n));
      double theta_root = 3.14159265358979323846 / static_cast<double>(n);
      auto polar_res = std::polar(r_root, theta_root);
      return GV::cplx(polar_res.real(), polar_res.imag());
    }
    if (n == 2) return GV::number(std::sqrt(val));
    if (n == 3) return GV::number(std::cbrt(val));
    return GV::number(std::pow(val, 1.0 / n));
  }
  if (name == "pow" || name == "potencia") {
    need(2, 2);
    return GV::number(std::pow(x(0), x(1)));
  }
  if (name == "hypot" || name == "hipotenusa") {
    need(2, 2);
    return GV::number(std::hypot(x(0), x(1)));
  }
  if (name == "abs") {
    need(1, 1);
    return GV::number(std::fabs(x(0)), isIntVal(a[0]));
  }
  if (name == "floor") {
    need(1, 1);
    return GV::number(std::floor(x(0)));
  }
  if (name == "ceil") {
    need(1, 1);
    return GV::number(std::ceil(x(0)));
  }
  if (name == "round") {
    need(1, 1);
    return GV::number(std::round(x(0)));
  }
  if (name == "trunc") {
    need(1, 1);
    return GV::number(std::trunc(x(0)));
  }
  if (name == "fmod") {
    need(2, 2);
    return GV::number(std::fmod(x(0), x(1)));
  }
  if (name == "exp") {
    need(1, 1);
    return GV::number(std::exp(x(0)));
  }
  if (name == "exp2") {
    need(1, 1);
    return GV::number(std::exp2(x(0)));
  }
  if (name == "expm1") {
    need(1, 1);
    return GV::number(std::expm1(x(0)));
  }
  if (name == "log") {
    need(1, 1);
    return GV::number(std::log(x(0)));
  }
  if (name == "log10") {
    need(1, 1);
    return GV::number(std::log10(x(0)));
  }
  if (name == "log2") {
    need(1, 1);
    return GV::number(std::log2(x(0)));
  }
  if (name == "log1p") {
    need(1, 1);
    return GV::number(std::log1p(x(0)));
  }
  if (name == "sin") {
    need(1, 1);
    return GV::number(std::sin(x(0)));
  }
  if (name == "cos") {
    need(1, 1);
    return GV::number(std::cos(x(0)));
  }
  if (name == "tan") {
    need(1, 1);
    return GV::number(std::tan(x(0)));
  }
  if (name == "asin") {
    need(1, 1);
    return GV::number(std::asin(x(0)));
  }
  if (name == "acos") {
    need(1, 1);
    return GV::number(std::acos(x(0)));
  }
  if (name == "atan") {
    need(1, 1);
    return GV::number(std::atan(x(0)));
  }
  if (name == "atan2") {
    need(2, 2);
    return GV::number(std::atan2(x(0), x(1)));
  }
  if (name == "sinh") {
    need(1, 1);
    return GV::number(std::sinh(x(0)));
  }
  if (name == "cosh") {
    need(1, 1);
    return GV::number(std::cosh(x(0)));
  }
  if (name == "tanh") {
    need(1, 1);
    return GV::number(std::tanh(x(0)));
  }
  if (name == "asinh") {
    need(1, 1);
    return GV::number(std::asinh(x(0)));
  }
  if (name == "acosh") {
    need(1, 1);
    return GV::number(std::acosh(x(0)));
  }
  if (name == "atanh") {
    need(1, 1);
    return GV::number(std::atanh(x(0)));
  }
  if (name == "erf") {
    need(1, 1);
    return GV::number(std::erf(x(0)));
  }
  if (name == "erfc") {
    need(1, 1);
    return GV::number(std::erfc(x(0)));
  }
  if (name == "gamma" || name == "tgamma") {
    need(1, 1);
    return GV::number(std::tgamma(x(0)));
  }
  if (name == "lgamma") {
    need(1, 1);
    return GV::number(std::lgamma(x(0)));
  }
  if (name == "deg2rad" || name == "graus_para_radianos") {
    need(1, 1);
    return GV::number(x(0) * (3.14159265358979323846 / 180.0));
  }
  if (name == "rad2deg" || name == "radianos_para_graus") {
    need(1, 1);
    return GV::number(x(0) * (180.0 / 3.14159265358979323846));
  }
  if (name == "gcd" || name == "mdc") {
    need(2, 2);
    return GV::number(std::gcd(static_cast<long long>(x(0)), static_cast<long long>(x(1))), true);
  }
  if (name == "lcm" || name == "mmc") {
    need(2, 2);
    return GV::number(std::lcm(static_cast<long long>(x(0)), static_cast<long long>(x(1))), true);
  }
  if (name == "factorial" || name == "fatorial") {
    need(1, 1);
    long long n = static_cast<long long>(x(0));
    if (n < 0) throw std::runtime_error("fatorial indefinido para numeros negativos");
    double res = 1.0;
    for (long long i = 2; i <= n; ++i) res *= i;
    return GV::number(res, n <= 20);
  }
  if (name == "sigmoid") {
    need(1, 1);
    return GV::number(1.0 / (1.0 + std::exp(-x(0))));
  }
  if (name == "relu") {
    need(1, 1);
    return GV::number(std::max(0.0, x(0)), isIntVal(a[0]));
  }
  if (name == "min" || name == "max") {
    need(2, 2);
    const bool ints = isIntVal(a[0]) && isIntVal(a[1]);
    return GV::number(
        name == "min" ? std::min(x(0), x(1)) : std::max(x(0), x(1)), ints);
  }
  if (name == "len") {
    need(1, 1);
    if (a[0].kind == K::Str)
      return GV::number(static_cast<double>(a[0].str.size()), true);
    if (a[0].kind == K::List)
      return GV::number(static_cast<double>(a[0].list.size()), true);
    if (a[0].kind == K::Map)
      return GV::number(static_cast<double>(a[0].map.size()), true);
    if (a[0].kind == K::Set)
      return GV::number(static_cast<double>(a[0].list.size()), true);
    throw std::runtime_error("len(): use em texto, lista, mapa ou conjunto");
  }
  if (name == "complex_real") {
    need(1, 1);
    if (a[0].kind == K::Complex)
      return GV::number(a[0].num, false);
    if (a[0].kind == K::Num)
      return a[0];
    throw std::runtime_error("complex_real espera complex");
  }
  if (name == "complex_imag") {
    need(1, 1);
    if (a[0].kind == K::Complex)
      return GV::number(a[0].imag, false);
    if (a[0].kind == K::Num)
      return GV::number(0.0, false);
    throw std::runtime_error("complex_imag espera complex");
  }
  if (name == "complex_abs") {
    need(1, 1);
    if (a[0].kind == K::Complex)
      return GV::number(std::hypot(a[0].num, a[0].imag), false);
    if (a[0].kind == K::Num)
      return GV::number(std::fabs(a[0].num), isIntVal(a[0]));
    throw std::runtime_error("complex_abs espera complex");
  }
  if (name == "complex_arg") {
    need(1, 1);
    if (a[0].kind == K::Complex)
      return GV::number(std::atan2(a[0].imag, a[0].num), false);
    if (a[0].kind == K::Num)
      return GV::number(a[0].num >= 0 ? 0.0 : 3.14159265358979323846, false);
    throw std::runtime_error("complex_arg espera complex");
  }
  if (name == "complex_conj") {
    need(1, 1);
    if (a[0].kind == K::Complex)
      return GV::cplx(a[0].num, -a[0].imag);
    if (a[0].kind == K::Num)
      return a[0];
    throw std::runtime_error("complex_conj espera complex");
  }
  if (name == "complex") {
    need(1, 2);
    double r = a[0].kind == K::Complex ? a[0].num : numOf(a[0], "complex");
    double im = a.size() == 2 ? numOf(a[1], "complex")
                              : (a[0].kind == K::Complex ? a[0].imag : 0.0);
    return GV::cplx(r, im);
  }
  if (name == "ghost_base64_encode" || name == "crypto_base64_encode") {
    need(1, 1);
    return GV::text(ghost_base64_encode(textOf(a[0], false)));
  }
  if (name == "ghost_base64_decode" || name == "crypto_base64_decode") {
    need(1, 1);
    return GV::text(ghost_base64_decode(textOf(a[0], false)));
  }
  if (name == "json_parse") {
    need(1, 1);
    std::string rawStr = textOf(a[0], false);
    GhostJson gj(rawStr);
    GV m;
    m.kind = K::Map;
    for (const auto &kv : gj.data) {
      m.map[kv.first] = GV::text(kv.second);
    }
    return m;
  }
  if (name == "json_get") {
    need(2, 2);
    const std::string key = textOf(a[1], false);
    if (a[0].kind == K::Map) {
      auto it = a[0].map.find(key);
      if (it != a[0].map.end())
        return it->second;
      return GV::text("null");
    }
    throw std::runtime_error("json_get: esperado mapa JSON");
  }
  if (name == "json_stringify") {
    need(1, 1);
    if (a[0].kind == K::Map) {
      GhostJson gj;
      for (const auto &kv : a[0].map) {
        gj.data[kv.first] = textOf(kv.second, false);
      }
      return GV::text(gj.toString());
    }
    return GV::text(textOf(a[0], false));
  }
  if (name == "db_open" || name == "ghost_db_open") {
    need(1, 1);
    return GV::number(ghost_db_open(textOf(a[0], false)), true);
  }
  if (name == "db_exec" || name == "ghost_db_exec") {
    need(2, 2);
    return GV::number(
        ghost_db_exec(static_cast<int>(numOf(a[0], name)), textOf(a[1], false)),
        true);
  }
  if (name == "db_query" || name == "ghost_db_query") {
    need(2, 2);
    return GV::text(ghost_db_query(static_cast<int>(numOf(a[0], name)),
                                   textOf(a[1], false)));
  }
  if (name == "db_close" || name == "ghost_db_close") {
    need(1, 1);
    ghost_db_close(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }
  if (name == "db_last_id" || name == "ghost_db_last_id") {
    need(1, 1);
    return GV::number(ghost_db_last_id(static_cast<int>(numOf(a[0], name))),
                      true);
  }
  if (name == "db_changes" || name == "ghost_db_changes") {
    need(1, 1);
    return GV::number(ghost_db_changes(static_cast<int>(numOf(a[0], name))),
                      true);
  }
  if (name == "gui_alert" || name == "ghost_gui_message_box") {
    need(2, 3);
    int type = a.size() >= 3 ? static_cast<int>(numOf(a[2], name)) : 0;
    return GV::number(ghost_gui_message_box(textOf(a[0], false), textOf(a[1], false), type), true);
  }
  if (name == "gui_open_browser" || name == "ghost_gui_open_url") {
    need(1, 1);
    return GV::number(ghost_gui_open_url(textOf(a[0], false)), true);
  }
  if (name == "gui_create_window" || name == "ghost_gui_window_create") {
    need(3, 3);
    return GV::number(ghost_gui_window_create(textOf(a[0], false), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name))), true);
  }
  if (name == "gui_show_window" || name == "ghost_gui_window_show") {
    need(1, 1);
    ghost_gui_window_show(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }
  if (name == "gui_close_window" || name == "ghost_gui_window_close") {
    need(1, 1);
    ghost_gui_window_close(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }

  // Autograd
  if (name == "autograd_create" || name == "ghost_autograd_create") {
    need(2, 4);
    int r = static_cast<int>(numOf(a[0], name));
    int c = static_cast<int>(numOf(a[1], name));
    bool req = a.size() >= 3 ? (numOf(a[2], name) != 0.0) : false;
    double init_v = a.size() >= 4 ? numOf(a[3], name) : 0.0;
    return GV::number(GhostAI::ghost_autograd_create(r, c, req, init_v), true);
  }
  if (name == "autograd_set" || name == "ghost_autograd_set") {
    need(4, 4);
    GhostAI::ghost_autograd_set(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name)), numOf(a[3], name));
    return GV::number(0, true);
  }
  if (name == "autograd_get" || name == "ghost_autograd_get") {
    need(3, 3);
    return GV::number(GhostAI::ghost_autograd_get(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name))));
  }
  if (name == "autograd_grad" || name == "ghost_autograd_grad") {
    need(3, 3);
    return GV::number(GhostAI::ghost_autograd_grad(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name))));
  }
  if (name == "autograd_matmul" || name == "ghost_autograd_matmul") {
    need(2, 2);
    return GV::number(GhostAI::ghost_autograd_matmul(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "autograd_add" || name == "ghost_autograd_add") {
    need(2, 2);
    return GV::number(GhostAI::ghost_autograd_add(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "autograd_relu" || name == "ghost_autograd_relu") {
    need(1, 1);
    return GV::number(GhostAI::ghost_autograd_relu(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "autograd_mse" || name == "ghost_autograd_mse") {
    need(2, 2);
    return GV::number(GhostAI::ghost_autograd_mse(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "autograd_backward" || name == "ghost_autograd_backward") {
    need(1, 1);
    GhostAI::ghost_autograd_backward(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }
  if (name == "autograd_step" || name == "ghost_autograd_step") {
    need(2, 2);
    GhostAI::ghost_autograd_step(static_cast<int>(numOf(a[0], name)), numOf(a[1], name));
    return GV::number(0, true);
  }
  if (name == "autograd_zero_grad" || name == "ghost_autograd_zero_grad") {
    need(1, 1);
    GhostAI::ghost_autograd_zero_grad(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }

  // Dataset
  if (name == "dataset_create_csv" || name == "ghost_dataset_create_csv") {
    need(1, 2);
    bool header = a.size() >= 2 ? (numOf(a[1], name) != 0.0) : true;
    return GV::number(GhostData::ghost_dataset_create_csv(textOf(a[0], false), header), true);
  }
  if (name == "dataset_create_matrix" || name == "ghost_dataset_create_matrix") {
    need(3, 3);
    int r = static_cast<int>(numOf(a[0], name));
    int c = static_cast<int>(numOf(a[1], name));
    std::vector<double> v;
    if (a[2].kind == GV::Kind::List) {
      for (const auto& item : a[2].list) v.push_back(numOf(item, name));
    }
    return GV::number(GhostData::ghost_dataset_create_matrix(r, c, v), true);
  }
  if (name == "dataset_size" || name == "ghost_dataset_size") {
    need(1, 1);
    return GV::number(GhostData::ghost_dataset_size(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "dataset_features" || name == "ghost_dataset_features") {
    need(1, 1);
    return GV::number(GhostData::ghost_dataset_features(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "dataset_split" || name == "ghost_dataset_split") {
    need(2, 2);
    int tr = 0, te = 0;
    GhostData::ghost_dataset_split(static_cast<int>(numOf(a[0], name)), numOf(a[1], name), tr, te);
    GV g;
    g.kind = GV::Kind::List;
    g.list.push_back(GV::number(tr, true));
    g.list.push_back(GV::number(te, true));
    return g;
  }
  if (name == "dataset_create_batches" || name == "ghost_dataset_create_batches") {
    need(2, 2);
    GhostData::ghost_dataset_create_batches(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)));
    return GV::number(0, true);
  }
  if (name == "dataset_num_batches" || name == "ghost_dataset_num_batches") {
    need(1, 1);
    return GV::number(GhostData::ghost_dataset_num_batches(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "dataset_get_item" || name == "ghost_dataset_get_item") {
    need(3, 3);
    return GV::number(GhostData::ghost_dataset_get_item(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name))));
  }

  // Matriz Densidade e Ruido Quantico (Kraus)
  if (name == "density_create" || name == "ghost_density_create") {
    need(1, 1);
    return GV::number(GhostQuantum::ghost_density_create(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "density_maximally_mixed" || name == "ghost_density_maximally_mixed") {
    need(1, 1);
    return GV::number(GhostQuantum::ghost_density_maximally_mixed(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "density_purity" || name == "ghost_density_purity") {
    need(1, 1);
    return GV::number(GhostQuantum::ghost_density_purity(static_cast<int>(numOf(a[0], name))));
  }
  if (name == "density_trace" || name == "ghost_density_trace") {
    need(1, 1);
    return GV::number(GhostQuantum::ghost_density_trace(static_cast<int>(numOf(a[0], name))));
  }
  if (name == "density_prob" || name == "ghost_density_prob") {
    need(2, 2);
    return GV::number(GhostQuantum::ghost_density_prob(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))));
  }
  if (name == "density_h" || name == "ghost_density_h") {
    need(2, 2);
    GhostQuantum::ghost_density_h(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)));
    return GV::number(0, true);
  }
  if (name == "density_x" || name == "ghost_density_x") {
    need(2, 2);
    GhostQuantum::ghost_density_x(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)));
    return GV::number(0, true);
  }
  if (name == "density_cnot" || name == "ghost_density_cnot") {
    need(3, 3);
    GhostQuantum::ghost_density_cnot(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name)));
    return GV::number(0, true);
  }
  if (name == "density_depolarize" || name == "ghost_density_depolarize") {
    need(3, 3);
    GhostQuantum::ghost_density_depolarize(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }
  if (name == "density_bit_flip" || name == "ghost_density_bit_flip") {
    need(3, 3);
    GhostQuantum::ghost_density_bit_flip(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }
  if (name == "density_phase_flip" || name == "ghost_density_phase_flip") {
    need(3, 3);
    GhostQuantum::ghost_density_phase_flip(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }
  if (name == "density_amplitude_damping" || name == "ghost_density_amplitude_damping") {
    need(3, 3);
    GhostQuantum::ghost_density_amplitude_damping(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }
  if (name == "density_phase_damping" || name == "ghost_density_phase_damping") {
    need(3, 3);
    GhostQuantum::ghost_density_phase_damping(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }

  // Tensores N-Dimensionais e Broadcasting (alg-tensor-nd)
  if (name == "tensor_create" || name == "ghost_tensor_create") {
    need(3, 3);
    std::vector<int> shp;
    if (a[0].kind == GhostValue::Kind::List) {
      for (const auto& item : a[0].list) {
        shp.push_back(static_cast<int>(item.kind == GhostValue::Kind::Num ? item.num : 0));
      }
    } else {
      shp.push_back(static_cast<int>(numOf(a[0], name)));
    }
    int ndim = static_cast<int>(numOf(a[1], name));
    double init_val = numOf(a[2], name);
    return GV::number(ghost_tensor_create(shp.data(), ndim, init_val), true);
  }
  if (name == "tensor_free" || name == "ghost_tensor_free") {
    need(1, 1);
    ghost_tensor_free(static_cast<int>(numOf(a[0], name)));
    return GV::number(0, true);
  }
  if (name == "tensor_ndim" || name == "ghost_tensor_ndim") {
    need(1, 1);
    return GV::number(ghost_tensor_ndim(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "tensor_dim" || name == "ghost_tensor_dim") {
    need(2, 2);
    return GV::number(ghost_tensor_dim(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_size" || name == "ghost_tensor_size") {
    need(1, 1);
    return GV::number(ghost_tensor_size(static_cast<int>(numOf(a[0], name))), true);
  }
  if (name == "tensor_get" || name == "ghost_tensor_get") {
    need(3, 3);
    int handle = static_cast<int>(numOf(a[0], name));
    std::vector<int> idx;
    if (a[1].kind == GhostValue::Kind::List) {
      for (const auto& item : a[1].list) {
        idx.push_back(static_cast<int>(item.kind == GhostValue::Kind::Num ? item.num : 0));
      }
    } else {
      idx.push_back(static_cast<int>(numOf(a[1], name)));
    }
    int idx_len = static_cast<int>(numOf(a[2], name));
    return GV::number(ghost_tensor_get(handle, idx.data(), idx_len));
  }
  if (name == "tensor_set" || name == "ghost_tensor_set") {
    need(4, 4);
    int handle = static_cast<int>(numOf(a[0], name));
    std::vector<int> idx;
    if (a[1].kind == GhostValue::Kind::List) {
      for (const auto& item : a[1].list) {
        idx.push_back(static_cast<int>(item.kind == GhostValue::Kind::Num ? item.num : 0));
      }
    } else {
      idx.push_back(static_cast<int>(numOf(a[1], name)));
    }
    int idx_len = static_cast<int>(numOf(a[2], name));
    double val = numOf(a[3], name);
    ghost_tensor_set(handle, idx.data(), idx_len, val);
    return GV::number(0, true);
  }
  if (name == "tensor_get_flat" || name == "ghost_tensor_get_flat") {
    need(2, 2);
    return GV::number(ghost_tensor_get_flat(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))));
  }
  if (name == "tensor_set_flat" || name == "ghost_tensor_set_flat") {
    need(3, 3);
    ghost_tensor_set_flat(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), numOf(a[2], name));
    return GV::number(0, true);
  }
  if (name == "tensor_reshape" || name == "ghost_tensor_reshape") {
    need(3, 3);
    int handle = static_cast<int>(numOf(a[0], name));
    std::vector<int> shp;
    if (a[1].kind == GhostValue::Kind::List) {
      for (const auto& item : a[1].list) {
        shp.push_back(static_cast<int>(item.kind == GhostValue::Kind::Num ? item.num : 0));
      }
    } else {
      shp.push_back(static_cast<int>(numOf(a[1], name)));
    }
    int new_ndim = static_cast<int>(numOf(a[2], name));
    return GV::number(ghost_tensor_reshape(handle, shp.data(), new_ndim), true);
  }
  if (name == "tensor_transpose" || name == "ghost_tensor_transpose") {
    need(3, 3);
    int handle = static_cast<int>(numOf(a[0], name));
    std::vector<int> perm;
    if (a[1].kind == GhostValue::Kind::List) {
      for (const auto& item : a[1].list) {
        perm.push_back(static_cast<int>(item.kind == GhostValue::Kind::Num ? item.num : 0));
      }
    } else {
      perm.push_back(static_cast<int>(numOf(a[1], name)));
    }
    int perm_len = static_cast<int>(numOf(a[2], name));
    return GV::number(ghost_tensor_transpose(handle, perm.data(), perm_len), true);
  }
  if (name == "tensor_slice" || name == "ghost_tensor_slice") {
    need(3, 3);
    return GV::number(ghost_tensor_slice(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name)), static_cast<int>(numOf(a[2], name))), true);
  }
  if (name == "tensor_add" || name == "ghost_tensor_add") {
    need(2, 2);
    return GV::number(ghost_tensor_add(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_sub" || name == "ghost_tensor_sub") {
    need(2, 2);
    return GV::number(ghost_tensor_sub(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_mul" || name == "ghost_tensor_mul") {
    need(2, 2);
    return GV::number(ghost_tensor_mul(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_div" || name == "ghost_tensor_div") {
    need(2, 2);
    return GV::number(ghost_tensor_div(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_matmul" || name == "ghost_tensor_matmul") {
    need(2, 2);
    return GV::number(ghost_tensor_matmul(static_cast<int>(numOf(a[0], name)), static_cast<int>(numOf(a[1], name))), true);
  }
  if (name == "tensor_sum" || name == "ghost_tensor_sum") {
    need(1, 1);
    return GV::number(ghost_tensor_sum(static_cast<int>(numOf(a[0], name))));
  }
  if (name == "tensor_mean" || name == "ghost_tensor_mean") {
    need(1, 1);
    return GV::number(ghost_tensor_mean(static_cast<int>(numOf(a[0], name))));
  }
  if (name == "tensor_to_string" || name == "ghost_tensor_to_string") {
    need(1, 1);
    return GV::text(ghost_tensor_to_string(static_cast<int>(numOf(a[0], name))));
  }
  if (name == "qhw_configure" || name == "ghost_qhw_configure") {
    need(3, 3);
    ghost_qhw_configure(textOf(a[0], false), textOf(a[1], false), textOf(a[2], false));
    return GV::number(0, true);
  }
  if (name == "qhw_submit" || name == "ghost_qhw_submit") {
    need(3, 3);
    return GV::text(ghost_qhw_submit(textOf(a[0], false), textOf(a[1], false), static_cast<int>(numOf(a[2], name))));
  }
  if (name == "qhw_status" || name == "ghost_qhw_status") {
    need(1, 1);
    return GV::text(ghost_qhw_status(textOf(a[0], false)));
  }
  if (name == "qhw_get_count" || name == "ghost_qhw_get_count") {
    need(2, 2);
    return GV::number(ghost_qhw_get_count(textOf(a[0], false), textOf(a[1], false)), true);
  }
  if (name == "qhw_summary" || name == "ghost_qhw_summary") {
    need(1, 1);
    return GV::text(ghost_qhw_summary(textOf(a[0], false)));
  }
  if (name == "str" || name == "string") {
    need(1, 1);
    return GV::text(textOf(a[0], false));
  }
  if (name == "set") { // set(lista): conjunto sem tipo declarado
    need(1, 1);
    if (a[0].kind != K::List && a[0].kind != K::Set)
      throw std::runtime_error("set(): use set(lista), recebido " +
                               textOf(a[0], true));
    GV s = a[0];
    s.kind = K::Set;
    normalizeSet(s);
    return s;
  }
  if (name == "int" || name == "to_int") {
    need(1, 1);
    if (a[0].kind == K::Str) {
      try {
        return GV::number(static_cast<double>(std::stoll(a[0].str)), true);
      } catch (...) {
        throw std::runtime_error("int(): \"" + a[0].str + "\" nao e numero");
      }
    }
    return GV::number(std::trunc(x(0)), true);
  }
  if (name == "double" || name == "float" || name == "parse_number") {
    need(1, 1);
    if (a[0].kind == K::Str) {
      try {
        return GV::number(std::stod(a[0].str));
      } catch (...) {
        throw std::runtime_error(name + "(): \"" + a[0].str +
                                 "\" nao e numero");
      }
    }
    return GV::number(x(0));
  }
  if (name == "bool") {
    need(1, 1);
    return GV::boolean(truthy(a[0]));
  }
  if (name == "typeof") {
    need(1, 1);
    switch (a[0].kind) {
    case K::Num:
      return GV::text(a[0].isInt ? "int" : "double");
    case K::Str:
      return GV::text("string");
    case K::Bool:
      return GV::text("bool");
    case K::List:
      return GV::text("list");
    case K::Map:
      return GV::text("map");
    case K::Complex:
      return GV::text("complex");
    case K::Object:
      return GV::text(a[0].str);
    case K::Result:
      return GV::text(a[0].str.empty() ? "result" : a[0].str);
    case K::Set:
      return GV::text(a[0].str.empty() ? "set" : "set<" + a[0].str + ">");
    case K::Quantum:
      return GV::text(a[0].qreg ? "qreg" : "qcircuit");
    case K::Iface:
      return GV::text(a[0].list.empty() ? a[0].str : a[0].list[0].str);
    case K::Fn:
      return GV::text(a[0].str.empty() ? "fn" : a[0].str);
    }
  }
  if (name == "range") {
    need(1, 3);
    long long start = 0, end, step = 1;
    if (a.size() == 1)
      end = std::llround(x(0));
    else {
      start = std::llround(x(0));
      end = std::llround(x(1));
      if (a.size() == 3)
        step = std::llround(x(2));
    }
    if (step == 0)
      throw std::runtime_error("range: passo zero");
    GV r;
    r.kind = K::List;
    for (long long v = start; step > 0 ? v < end : v > end; v += step)
      r.list.push_back(GV::number(static_cast<double>(v), true));
    return r;
  }
  if (name == "assert") {
    need(1, 2);
    if (!truthy(a[0]))
      throw std::runtime_error(a.size() > 1 ? textOf(a[1], false)
                                            : std::string("assert falhou"));
    return GV::boolean(true);
  }
  if (name == "now") {
    need(0, 0);
    std::time_t raw;
    std::time(&raw);
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&raw));
    return GV::text(buf);
  }
  if (name == "format_number") {
    need(2, 2);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(static_cast<int>(std::llround(x(1))))
       << x(0);
    return GV::text(ss.str());
  }
  if (name == "format_date") {
    need(2, 2);
    std::tm tm = {};
    std::istringstream ss(textOf(a[0], false));
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    char buf[80];
    std::strftime(buf, sizeof(buf), textOf(a[1], false).c_str(), &tm);
    return GV::text(buf);
  }
  if (name == "parse_date") {
    need(2, 2);
    std::tm tm = {};
    std::istringstream ss(textOf(a[0], false));
    ss >> std::get_time(&tm, textOf(a[1], false).c_str());
    char buf[80];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return GV::text(buf);
  }
  if (name == "regex_match") {
    need(2, 2);
    std::string pat = textOf(a[1], false);
    std::string normPat;
    for (size_t k = 0; k < pat.size(); ++k) {
      if (pat[k] == '\\' && k + 1 < pat.size() && pat[k + 1] == '\\') {
        normPat += '\\';
        ++k;
      } else
        normPat += pat[k];
    }
    return GV::boolean(regex_match(textOf(a[0], false), normPat));
  }
  if (name == "regex_search") {
    need(2, 2);
    std::string pat = textOf(a[1], false);
    std::string normPat;
    for (size_t k = 0; k < pat.size(); ++k) {
      if (pat[k] == '\\' && k + 1 < pat.size() && pat[k + 1] == '\\') {
        normPat += '\\';
        ++k;
      } else
        normPat += pat[k];
    }
    return GV::boolean(regex_search(textOf(a[0], false), normPat));
  }
  if (name == "regex_replace") {
    need(3, 3);
    std::string pat = textOf(a[1], false);
    std::string normPat;
    for (size_t k = 0; k < pat.size(); ++k) {
      if (pat[k] == '\\' && k + 1 < pat.size() && pat[k + 1] == '\\') {
        normPat += '\\';
        ++k;
      } else
        normPat += pat[k];
    }
    return GV::text(
        regex_replace(textOf(a[0], false), normPat, textOf(a[2], false)));
  }
  if (name == "readfile" || name == "fs_read") {
    need(1, 1);
    return GV::text(readfile(textOf(a[0], false)));
  }
  if (name == "writefile" || name == "fs_write") {
    need(2, 2);
    writefile(textOf(a[0], false), textOf(a[1], false));
    return GV::boolean(true);
  }
  if (name == "deletefile" || name == "fs_delete") {
    need(1, 1);
    deletefile(textOf(a[0], false));
    return GV::boolean(true);
  }
  if (name == "copyfile" || name == "fs_copy") {
    need(2, 2);
    copyfile(textOf(a[0], false), textOf(a[1], false));
    return GV::boolean(true);
  }
  if (name == "file_exists") {
    need(1, 1);
    return GV::boolean(file_exists(textOf(a[0], false)));
  }
  if (name == "dir_create" || name == "fs_mkdir") {
    need(1, 1);
    dir_create(textOf(a[0], false));
    return GV::boolean(true);
  }
  if (name == "ghost_env_get" || name == "os_getenv") {
    need(1, 1);
    return GV::text(ghost_env_get(textOf(a[0], false)));
  }
  if (name == "ghost_system_exec" || name == "os_exec") {
    need(1, 1);
    return GV::text(ghost_system_exec(textOf(a[0], false)));
  }
  if (name == "http_get" || name == "http_client_get") {
    need(1, 1);
    return GV::text(http_get(textOf(a[0], false)));
  }
  if (name == "http_post" || name == "http_client_post") {
    need(2, 2);
    return GV::text(http_post(textOf(a[0], false), textOf(a[1], false)));
  }
  if (name == "vec_add" || name == "vec_sub" || name == "vec_mul" ||
      name == "vec_div") {
    need(2, 2);
    if (a[0].kind != K::List || a[1].kind != K::List)
      throw std::runtime_error(name + "(): esperado duas listas");
    const auto &L = a[0].list;
    const auto &o = a[1].list;
    if (L.size() != o.size())
      throw std::runtime_error(name + ": vetores com tamanhos diferentes (" +
                               std::to_string(L.size()) +
                               " != " + std::to_string(o.size()) + ")");
    GV r;
    r.kind = K::List;
    r.str = name == "vec_div" ? "double" : a[0].str;
    for (size_t k = 0; k < L.size(); ++k) {
      double v1 = numOf(L[k], name);
      double v2 = numOf(o[k], name);
      if (name == "vec_add")
        r.list.push_back(GV::number(v1 + v2, isIntVal(L[k]) && isIntVal(o[k])));
      else if (name == "vec_sub")
        r.list.push_back(GV::number(v1 - v2, isIntVal(L[k]) && isIntVal(o[k])));
      else if (name == "vec_mul")
        r.list.push_back(GV::number(v1 * v2, isIntVal(L[k]) && isIntVal(o[k])));
      else if (name == "vec_div") {
        if (v2 == 0)
          throw std::runtime_error("divisao por zero");
        r.list.push_back(GV::number(v1 / v2, false));
      }
    }
    return r;
  }
  if (name == "vec_scale") {
    need(2, 2);
    if (a[0].kind != K::List)
      throw std::runtime_error(
          "vec_scale(): primeiro argumento precisa ser lista");
    double k = numOf(a[1], "vec_scale");
    GV r;
    r.kind = K::List;
    r.str = "double";
    for (const auto &e : a[0].list)
      r.list.push_back(GV::number(numOf(e, "vec_scale") * k, false));
    return r;
  }
  if (name == "dot") {
    need(2, 2);
    if (a[0].kind != K::List || a[1].kind != K::List)
      throw std::runtime_error("dot(): esperado duas listas");
    const auto &L = a[0].list;
    const auto &o = a[1].list;
    if (L.size() != o.size())
      throw std::runtime_error("dot: vetores com tamanhos diferentes (" +
                               std::to_string(L.size()) +
                               " != " + std::to_string(o.size()) + ")");
    double s = 0.0;
    for (size_t k = 0; k < L.size(); ++k)
      s += numOf(L[k], "dot") * numOf(o[k], "dot");
    return GV::number(s, false);
  }
  if (name == "norm") {
    need(1, 1);
    if (a[0].kind != K::List)
      throw std::runtime_error("norm(): esperado lista");
    double s = 0.0;
    for (const auto &e : a[0].list) {
      double v = numOf(e, "norm");
      s += v * v;
    }
    return GV::number(std::sqrt(s), false);
  }
  if (name == "mat_mul") {
    need(5, 5);
    if (a[0].kind != K::List || a[1].kind != K::List)
      throw std::runtime_error("mat_mul(): esperado matrizes como listas");
    const auto &A = a[0].list;
    const auto &B = a[1].list;
    int rA = static_cast<int>(std::llround(x(2))),
        cA = static_cast<int>(std::llround(x(3))),
        cB = static_cast<int>(std::llround(x(4)));
    if (rA <= 0 || cA <= 0 || cB <= 0)
      throw std::runtime_error("mat_mul: dimensoes precisam ser positivas");
    if (static_cast<size_t>(rA * cA) != A.size())
      throw std::runtime_error("mat_mul: tamanho de A nao bate com rA * cA");
    if (static_cast<size_t>(cA * cB) != B.size())
      throw std::runtime_error("mat_mul: tamanho de B nao bate com cA * cB");
    GV r;
    r.kind = K::List;
    r.str = "double";
    r.list.resize(static_cast<size_t>(rA * cB), GV::number(0.0, false));
    for (int i = 0; i < rA; ++i) {
      for (int k = 0; k < cA; ++k) {
        double aik = numOf(A[static_cast<size_t>(i * cA + k)], "mat_mul");
        for (int j = 0; j < cB; ++j) {
          double bkj = numOf(B[static_cast<size_t>(k * cB + j)], "mat_mul");
          size_t pos = static_cast<size_t>(i * cB + j);
          r.list[pos].num += aik * bkj;
        }
      }
    }
    return r;
  }
  if (name == "mat_transpose") {
    need(3, 3);
    if (a[0].kind != K::List)
      throw std::runtime_error("mat_transpose(): esperado lista");
    const auto &A = a[0].list;
    int rows = static_cast<int>(std::llround(x(1))),
        cols = static_cast<int>(std::llround(x(2)));
    if (rows <= 0 || cols <= 0)
      throw std::runtime_error(
          "mat_transpose: dimensoes precisam ser positivas");
    if (static_cast<size_t>(rows * cols) != A.size())
      throw std::runtime_error(
          "mat_transpose: tamanho de A nao bate com r * c");
    GV r;
    r.kind = K::List;
    r.str = a[0].str;
    r.list.resize(static_cast<size_t>(rows * cols));
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        r.list[static_cast<size_t>(j * rows + i)] =
            A[static_cast<size_t>(i * cols + j)];
      }
    }
    return r;
  }
  if (name == "mat2d_mul") {
    need(2, 2);
    if (a[0].kind != K::List || a[1].kind != K::List)
      throw std::runtime_error("mat2d_mul(): esperado matrizes 2D");
    const auto &A = a[0].list;
    const auto &B = a[1].list;
    if (A.empty() || B.empty() || A[0].kind != K::List || B[0].kind != K::List)
      throw std::runtime_error(
          "mat2d_mul(): matrizes nao podem ser vazias e precisam ser 2D");
    size_t rA = A.size(), cA = A[0].list.size();
    size_t rB = B.size(), cB = B[0].list.size();
    if (cA != rB)
      throw std::runtime_error("mat2d_mul: colunas de A (" +
                               std::to_string(cA) + ") != linhas de B (" +
                               std::to_string(rB) + ")");
    GV r;
    r.kind = K::List;
    r.list.resize(rA);
    for (size_t i = 0; i < rA; ++i) {
      r.list[i].kind = K::List;
      r.list[i].list.resize(cB, GV::number(0.0, false));
      for (size_t k = 0; k < cA; ++k) {
        double aik = numOf(A[i].list[k], "mat2d_mul");
        for (size_t j = 0; j < cB; ++j) {
          double bkj = numOf(B[k].list[j], "mat2d_mul");
          r.list[i].list[j].num += aik * bkj;
        }
      }
    }
    return r;
  }
  if (name == "ok") { // ok(valor); ok() == ok(true)
    need(0, 1);
    GV r;
    r.kind = K::Result;
    r.num = 1.0;
    r.list.push_back(a.empty() ? GV::boolean(true) : a[0]);
    return r;
  }
  if (name == "err") { // err(mensagem): qualquer valor vira o texto do erro
    need(1, 1);
    GV r;
    r.kind = K::Result;
    r.num = 0.0;
    r.list.push_back(GV::text(textOf(a[0], false)));
    return r;
  }
  if (name == "use_device") {
    need(1, 1);
    ghost::set_device(textOf(a[0], false));
    return GV::text(ghost::backend());
  }
  if (name == "device_backend") {
    need(0, 0);
    return GV::text(ghost::backend());
  }
  if (name == "device_info") {
    need(0, 0);
    return GV::text(ghost::device_info());
  }
  if (name == "gpu_available") {
    need(0, 0);
    return GV::number(ghost::cuda_available() ? 1.0 : 0.0, true);
  }
  if (name == "gpu_matrix_calls") {
    need(0, 0);
    return GV::number(static_cast<double>(ghost::cuda_matrix_calls()), true);
  }
  if (name == "device_supports") {
    need(1, 1);
    return GV::number(ghost::device_supports(textOf(a[0], false)) ? 1.0 : 0.0, true);
  }
  if (name == "heterogeneous_devices") {
    need(0, 0);
    return GV::text(ghost::heterogeneous_devices());
  }
  if (name == "has_blas_hardware") {
    need(0, 0);
    return GV::number(::has_blas_hardware() ? 1.0 : 0.0, true);
  }
  if (name == "blas_backend") {
    need(0, 0);
    return GV::text(::blas_backend());
  }
  found = false;
  return {};
}

// Chamada de função do usuário: parâmetros e variáveis declaradas no corpo são
// locais (quadro próprio); atribuições a variáveis de fora continuam valendo
// depois da chamada. return encerra a função.
GhostValue
GhostInterpreter::callUserFunction(const std::string &name,
                                   std::vector<std::vector<Token>> &raw) {
  auto it = function_overloads.find(name);
  if (it != function_overloads.end() && it->second.size() > 1) {
    std::vector<GhostValue> preVals;
    for (auto &r : raw)
      preVals.push_back(evalValue(r));
    const GhostFunction *chosen = selectOverload(it->second, preVals, name);
    const GhostFunction f = *chosen;
    std::vector<GhostValue> vals = preVals;
    for (size_t k = vals.size(); k < f.parameters.size(); ++k) {
      if (k < f.defaults.size() && !f.defaults[k].empty()) {
        auto d = f.defaults[k];
        vals.push_back(evalValue(d));
      } else
        throw std::runtime_error(name + "(): faltou o argumento '" +
                                 f.parameters[k].second + "'");
    }
    GhostValue res = runFunction(f, name, vals, nullptr, "");
    syncQuantumArgs(f, raw, vals);
    return res;
  }
  const GhostFunction f = functions_memory.at(name);
  std::vector<GhostValue> vals = evalArgs(f, name, raw);
  GhostValue res = runFunction(f, name, vals, nullptr, "");
  syncQuantumArgs(f, raw, vals);
  return res;
}

void GhostInterpreter::syncQuantumArgs(
    const GhostFunction &f, const std::vector<std::vector<Token>> &raw,
    const std::vector<GhostValue> &vals) {
  for (size_t k = 0;
       k < f.parameters.size() && k < raw.size() && k < vals.size(); ++k) {
    if (f.parameters[k].first == GTokenType::KwQreg ||
        f.parameters[k].first == GTokenType::KwQcircuit) {
      if (raw[k].size() == 1 && raw[k][0].type == GTokenType::Identifier) {
        const std::string &target = raw[k][0].value;
        if (qreg_memory.count(target) && vals[k].kind == GhostValue::Quantum &&
            vals[k].qreg) {
          qreg_memory[target] = *vals[k].qreg;
        } else if (qcircuit_memory.count(target) &&
                   vals[k].kind == GhostValue::Quantum && vals[k].qcirc) {
          qcircuit_memory[target] = *vals[k].qcirc;
        } else if (GhostValue *root = rootRef(target)) {
          if (root->kind == GhostValue::Quantum) {
            *root = vals[k];
          }
        }
      }
    }
  }
}

// Avalia os argumentos (no contexto de quem chama) e completa com os valores
// padrão.
std::vector<GhostValue>
GhostInterpreter::evalArgs(const GhostFunction &f, const std::string &name,
                           std::vector<std::vector<Token>> &raw) {
  if (raw.size() > f.parameters.size())
    throw std::runtime_error(
        name + "(): recebeu " + std::to_string(raw.size()) +
        " argumento(s), espera ate " + std::to_string(f.parameters.size()));
  std::vector<GhostValue> vals;
  for (size_t k = 0; k < f.parameters.size(); ++k) {
    if (k < raw.size())
      vals.push_back(evalValue(raw[k]));
    else if (k < f.defaults.size() && !f.defaults[k].empty()) {
      auto d = f.defaults[k];
      vals.push_back(evalValue(d));
    } else
      throw std::runtime_error(name + "(): faltou o argumento '" +
                               f.parameters[k].second + "'");
  }
  return vals;
}

// Executa o corpo num quadro próprio. self != nullptr: método (this = *self,
// atualizado na saída).
GhostValue GhostInterpreter::runFunction(const GhostFunction &f,
                                         const std::string &name,
                                         std::vector<GhostValue> &vals,
                                         GhostValue *self,
                                         const std::string &implClass) {
  if (vals.size() != f.parameters.size())
    throw std::runtime_error(name + "(): numero de argumentos invalido");
  if (call_depth >= 400)
    throw std::runtime_error(name +
                             "(): recursao profunda demais (limite 400)");
  RetGuard retGuard(ret_stack, f.returnClass);

  pushFrame();
  const size_t fnFrameDepth = frames.size() - 1;
  ++call_depth;
  if (self) {
    LPath effMPath = std::move(pending_method_path);
    pending_method_path = LPath{};
    this_stack.push_back(
        {*self, implClass, frames.size(), std::move(effMPath)});
  }
  const bool isStaticCall =
      !self && !implClass.empty(); // método static: campos static sem objeto
  if (isStaticCall)
    static_stack.push_back({implClass, frames.size()});
  auto leave = [&]() {
    if (self) {
      *self = std::move(this_stack.back().obj);
      this_stack.pop_back();
    }
    if (isStaticCall)
      static_stack.pop_back();
    popFrameAndRestore();
    --call_depth;
  };
  GhostValue result = GhostValue::number(0, true);
  try {
    for (size_t k = 0; k < f.parameters.size(); ++k) {
      const std::string &pn = f.parameters[k].second;
      const std::string pc =
          k < f.paramClasses.size() && knownType(f.paramClasses[k])
              ? f.paramClasses[k]
              : "";
      GhostValue pv = coerce(vals[k], f.parameters[k].first, pc, pn);
      declareLocal(pn);
      storeVar(pn, pv, pc.empty() ? f.parameters[k].first : GTokenType::KwVar);
      if (!pc.empty())
        var_class[pn] = pc;
    }
    executeLine(f.body);
  } catch (GhostReturnSignal &r) {
    if (r.hasValue) {
      checkReturnedClosureRefs(r.value, fnFrameDepth);
      result = r.value;
    }
  } catch (...) {
    leave();
    throw;
  }
  for (size_t k = 0; k < f.parameters.size() && k < vals.size(); ++k) {
    bool found = false;
    GhostValue v = lookupVar(f.parameters[k].second, found);
    if (found)
      vals[k] = v;
  }
  leave();
  const std::string rc = knownType(f.returnClass) ? f.returnClass : "";
  return coerce(result, f.returnType, rc, name + "()");
}

// ---------------------------------------------------------------- comandos

std::string GhostInterpreter::tokensToSource(const std::vector<Token> &tokens) {
  std::string s;
  for (const auto &t : tokens) {
    if (t.type == GTokenType::String)
      s += "\"" + t.value + "\" ";
    else if (t.type == GTokenType::FString)
      s += (t.value.find('"') == std::string::npos ? "f\"" + t.value + "\" "
                                                   : "f`" + t.value + "` ");
    else if (t.type == GTokenType::KwThis)
      s += "this ";
    else if (t.type == GTokenType::KwNew)
      s += "new ";
    else
      s += t.value + " ";
  }
  return s;
}

// tipo nome [= expr];  (int double float bool string var list<T> map<K,V>)
// Volta false (sem consumir) para os casos do caminho antigo: arrays C (int
// a[3] = {..}) e input().
bool GhostInterpreter::tryDeclaration(GTokenType type, Lexer &lex) {
  const size_t start = lex.getPosition();
  std::string declSpec;
  if (type == GTokenType::KwInt)
    declSpec = "int";
  else if (type == GTokenType::KwDouble)
    declSpec = "double";
  else if (type == GTokenType::KwFloat)
    declSpec = "float";
  else if (type == GTokenType::KwBool)
    declSpec = "bool";
  else if (type == GTokenType::KwString)
    declSpec = "string";
  else if (type == GTokenType::KwComplex)
    declSpec = "complex";
  else if (type == GTokenType::KwJson)
    declSpec = "json";
  bool hadArraySuffix = false;
  while (!declSpec.empty() &&
         lex.peekNextToken().type == GTokenType::LBracket) {
    const size_t sv = lex.getPosition();
    lex.getNextToken();
    if (lex.getNextToken().type == GTokenType::RBracket &&
        lex.peekNextToken().type == GTokenType::Identifier) {
      hadArraySuffix = true;
      declSpec = "list<" + declSpec + ">";
      type = GTokenType::KwList;
    } else {
      lex.setPosition(sv);
      break;
    }
  }
  if ((type == GTokenType::KwList || type == GTokenType::KwMap) &&
      !hadArraySuffix) {
    int depth = 0;
    Token g = lex.getNextToken();
    if (g.type != GTokenType::Lt)
      throw std::runtime_error(
          std::string(type == GTokenType::KwList ? "list" : "map") +
          " precisa do tipo: list<int> / map<string, int>");
    declSpec = std::string(type == GTokenType::KwList ? "list<" : "map<");
    depth = 1;
    while (depth > 0) {
      g = lex.getNextToken();
      if (g.type == GTokenType::EndOfFile)
        throw std::runtime_error("tipo generico sem '>'");
      if (g.type == GTokenType::Lt)
        depth++;
      else if (g.type == GTokenType::Gt)
        depth--;
      else if (g.type == GTokenType::Chain)
        depth -= 2;
      if (g.type == GTokenType::Comma)
        declSpec += ",";
      else
        declSpec += g.value;
    }
    try {
      declSpec = validateAndNormalizeTypeSpec(declSpec, false);
      ensureTypeReady(declSpec);
    } catch (...) {
    }
  }
  Token name = lex.getNextToken();
  if (name.type != GTokenType::Identifier) {
    lex.setPosition(start);
    return false;
  }
  Token next = lex.getNextToken();
  if (next.type == GTokenType::LBracket && type != GTokenType::KwList) {
    lex.setPosition(start);
    return false;
  }

  GhostValue v;
  if (next.type == GTokenType::Semicolon ||
      next.type == GTokenType::EndOfFile) {
    switch (type) {
    case GTokenType::KwString:
      v = GhostValue::text("");
      break;
    case GTokenType::KwBool:
      v = GhostValue::boolean(false);
      break;
    case GTokenType::KwList:
      v.kind = K::List;
      break;
    case GTokenType::KwMap:
    case GTokenType::KwJson:
      v.kind = K::Map;
      break;
    case GTokenType::KwVar:
      throw std::runtime_error("var " + name.value +
                               " precisa de valor inicial");
    default:
      v = GhostValue::number(0, type == GTokenType::KwInt);
      break;
    }
  } else if (next.type == GTokenType::Assign) {
    std::vector<Token> rhs;
    int depth = 0;
    while (true) {
      Token t = lex.peekNextToken();
      if (t.type == GTokenType::EndOfFile)
        break;
      if (depth == 0 && t.type == GTokenType::Semicolon) {
        lex.getNextToken();
        break;
      }
      if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket ||
          t.type == GTokenType::LBrace)
        depth++;
      if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket ||
          t.type == GTokenType::RBrace)
        depth--;
      rhs.push_back(lex.getNextToken());
    }
    v = evalValue(rhs);
  } else {
    lex.setPosition(start);
    return false;
  }
  declareLocal(name.value);
  if (type == GTokenType::KwList && isListSpec(declSpec)) {
    v = coerceList(v, declSpec, name.value);
    storeVar(name.value, v, type);
    var_class[name.value] = declSpec;
  } else {
    if (type == GTokenType::KwLlm) {
      llm_memory[name.value] = GhostLLM(v.str);
    }
    storeVar(name.value, v, type);
  }
  return true;
}

// Comandos que começam com um nome: x = e;  x += e;  x++;  xs[i] = e;
// xs.push(1);  f(1);
bool GhostInterpreter::tryCoreStatement(const Token &name, const Token &next,
                                        Lexer &lex) {
  const std::string &n = name.value;
  if (memory.count(n) || llm_memory.count(n))
    return false; // matrizes/estados e LLM: caminho antigo
  const size_t afterNext = lex.getPosition();

  auto readUntilSemicolon =
      [&](std::vector<Token> &out,
          int depth = 0) -> bool { // depth: parenteses ja abertos em out
    while (true) {
      Token t = lex.peekNextToken();
      if (t.type == GTokenType::EndOfFile)
        return true;
      if (depth == 0 && t.type == GTokenType::Semicolon) {
        lex.getNextToken();
        return true;
      }
      if (t.type == GTokenType::KwTranspose ||
          t.type == GTokenType::KwConv)
        return false;
      if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket ||
          t.type == GTokenType::LBrace)
        depth++;
      if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket ||
          t.type == GTokenType::RBrace)
        depth--;
      out.push_back(lex.getNextToken());
    }
  };
  auto compoundOp = [](GTokenType t) {
    switch (t) {
    case GTokenType::PlusAssign:
      return GTokenType::Plus;
    case GTokenType::MinusAssign:
      return GTokenType::Minus;
    case GTokenType::StarAssign:
      return GTokenType::Star;
    case GTokenType::SlashAssign:
      return GTokenType::Slash;
    case GTokenType::PercentAssign:
      return GTokenType::Percent;
    default:
      return GTokenType::Unknown;
    }
  };
  bool found = false;

  if (next.type == GTokenType::Assign) {
    std::vector<Token> rhs;
    if (!readUntilSemicolon(rhs)) {
      lex.setPosition(afterNext);
      return false;
    }
    // c = a + b com estados/matrizes (sem declaração): caminho antigo
    const bool known = numeric_memory.count(n) || string_memory.count(n) ||
                       coll_memory.count(n);
    if (!known) {
      for (const auto &t : rhs)
        if (t.type == GTokenType::Identifier && memory.count(t.value)) {
          lex.setPosition(afterNext);
          return false;
        }
    }
    storeVar(n, evalValue(rhs), GTokenType::Identifier);
    return true;
  }
  if (compoundOp(next.type) != GTokenType::Unknown ||
      next.type == GTokenType::Increment ||
      next.type == GTokenType::Decrement) {
    GhostValue cur = lookupVar(n, found);
    if (!found)
      throw std::runtime_error("Variavel nao encontrada: " + n);
    GhostValue r;
    if (next.type == GTokenType::Increment ||
        next.type == GTokenType::Decrement) {
      Token semi = lex.getNextToken();
      if (semi.type != GTokenType::Semicolon &&
          semi.type != GTokenType::EndOfFile && semi.type != GTokenType::RParen)
        throw std::runtime_error("esperado ';' depois de " + n + next.value);
      r = applyBinary(next.type == GTokenType::Increment ? GTokenType::Plus
                                                         : GTokenType::Minus,
                      cur, GhostValue::number(1, true));
    } else {
      std::vector<Token> rhs;
      readUntilSemicolon(rhs);
      r = applyBinary(compoundOp(next.type), cur, evalValue(rhs));
    }
    storeVar(n, r, GTokenType::Identifier);
    return true;
  }
  if (next.type == GTokenType::LBracket && coll_memory.count(n)) {
    // xs[i] = e;  m["k"] += e;
    std::vector<Token> idx;
    int depth = 1;
    while (true) {
      Token t = lex.getNextToken();
      if (t.type == GTokenType::EndOfFile)
        throw std::runtime_error("faltou ']'");
      if (t.type == GTokenType::LBracket)
        depth++;
      if (t.type == GTokenType::RBracket && --depth == 0)
        break;
      idx.push_back(t);
    }
    Token op = lex.getNextToken();
    GhostValue &target = coll_memory[n];
    const GhostValue key = evalValue(idx);
    auto slot = [&]() -> GhostValue & {
      if (target.kind == K::List) {
        long long i = std::llround(numOf(key, "indice"));
        const long long orig = i,
                        size = static_cast<long long>(target.list.size());
        if (i < 0)
          i += size;
        if (i < 0 || i >= size)
          throw std::runtime_error("indice " + std::to_string(orig) +
                                   " fora da lista de " + std::to_string(size) +
                                   " elemento(s)");
        return target.list[static_cast<size_t>(i)];
      }
      if (op.type != GTokenType::Assign &&
          !target.map.count(textOf(key, false)))
        throw std::runtime_error("chave \"" + textOf(key, false) +
                                 "\" nao existe no mapa");
      return target.map[textOf(key, false)];
    };
    if (op.type == GTokenType::Assign) {
      std::vector<Token> rhs;
      readUntilSemicolon(rhs);
      GhostValue v = evalValue(rhs);
      if (target.kind == K::List && !target.str.empty()) {
        v = coerceSpec(v, target.str, n + "[...]");
      }
      slot() = v;
      return true;
    }
    if (compoundOp(op.type) != GTokenType::Unknown) {
      std::vector<Token> rhs;
      readUntilSemicolon(rhs);
      GhostValue v = evalValue(rhs);
      GhostValue &s = slot();
      GhostValue res = applyBinary(compoundOp(op.type), s, v);
      if (target.kind == K::List && !target.str.empty())
        res = coerceSpec(res, target.str, n + "[...]");
      s = res;
      return true;
    }
    if (op.type == GTokenType::Increment || op.type == GTokenType::Decrement) {
      lex.getNextToken();
      GhostValue &s = slot();
      GhostValue res =
          applyBinary(op.type == GTokenType::Increment ? GTokenType::Plus
                                                       : GTokenType::Minus,
                      s, GhostValue::number(1, true));
      if (target.kind == K::List && !target.str.empty())
        res = coerceSpec(res, target.str, n + "[...]");
      s = res;
      return true;
    }
    throw std::runtime_error("depois de " + n + "[...] use =, +=, -=, ++ ...");
  }
  // Expressão como comando: xs.push(1);  f(2);  assert(x > 0, "msg");
  const bool isCall =
      (next.type == GTokenType::LParen &&
       (functions_memory.count(n) || n == "assert" || n == "print")) ||
      ((next.type == GTokenType::LParen || next.type == GTokenType::Lt) &&
       generic_functions.count(n)) || // troca<int>(a, b);
      (next.type == GTokenType::Lt && generic_method_names.count(n) &&
       !currentClassCtx().empty()); // anotar<int>(...);
  const bool isMethod = next.type == GTokenType::Dot &&
                        (coll_memory.count(n) || string_memory.count(n));
  if (isCall || isMethod) {
    std::vector<Token> expr{name, next};
    readUntilSemicolon(expr, next.type == GTokenType::LParen ? 1 : 0);
    evalValue(expr);
    return true;
  }
  if (next.type == GTokenType::LParen && !qreg_memory.count(n) &&
      !qcircuit_memory.count(n)) {
    std::vector<Token> expr{name, next};
    readUntilSemicolon(expr, next.type == GTokenType::LParen ? 1 : 0);
    evalValue(
        expr); // função embutida (assert, ...) ou erro "funcao nao encontrada"
    return true;
  }
  return false;
}

// Comandos sobre um caminho: obj.campo = e;  this.x += 1;  grade[1][0] = 5;
// obj.lista.push(1);  obj.metodo(); first é a raiz (nome ou this); next já foi
// lido e precisa ser '.' ou '['.
bool GhostInterpreter::tryLvalueStatement(const Token &first, const Token &next,
                                          Lexer &lex) {
  if (next.type != GTokenType::Dot && next.type != GTokenType::LBracket)
    return false;
  std::string root = first.type == GTokenType::KwThis ? "this" : first.value;
  if (first.type == GTokenType::Identifier && memory.count(root) &&
      !coll_memory.count(root))
    return false;
  // Classe.campo_static = e;  Classe.campo_static += 1;
  // Classe.lista_static.push(x);  Classe.metodo_static();
  bool staticRoot = false;
  const GhostFieldDef *staticInfo = nullptr;
  const bool classRoot = first.type == GTokenType::Identifier &&
                         next.type == GTokenType::Dot &&
                         class_memory.count(root) && !rootRef(root);
  if (classRoot) {
    const Token f = lex.peekNextToken();
    std::string owner;
    if (f.type == GTokenType::Identifier) {
      if (const GhostFieldDef *sfd = findStatic(root, f.value, &owner)) {
        checkAccess(owner, sfd->priv, "campo static " + f.value);
        staticRoot = true;
        staticInfo = sfd;
        root = owner + "::" + f.value;
      }
    }
  }
  if (!classRoot && !rootRef(root))
    return false;

  std::vector<Token> toks{first, next};
  int depth = next.type == GTokenType::LBracket ? 1 : 0;
  while (true) {
    Token t = lex.peekNextToken();
    if (t.type == GTokenType::EndOfFile)
      break;
    if (depth == 0 && t.type == GTokenType::Semicolon) {
      lex.getNextToken();
      break;
    }
    if (t.type == GTokenType::LParen || t.type == GTokenType::LBracket ||
        t.type == GTokenType::LBrace)
      depth++;
    if (t.type == GTokenType::RParen || t.type == GTokenType::RBracket ||
        t.type == GTokenType::RBrace)
      depth--;
    toks.push_back(lex.getNextToken());
  }
  auto compoundOp = [](GTokenType t) {
    switch (t) {
    case GTokenType::PlusAssign:
      return GTokenType::Plus;
    case GTokenType::MinusAssign:
      return GTokenType::Minus;
    case GTokenType::StarAssign:
      return GTokenType::Star;
    case GTokenType::SlashAssign:
      return GTokenType::Slash;
    case GTokenType::PercentAssign:
      return GTokenType::Percent;
    default:
      return GTokenType::Unknown;
    }
  };
  // operador de atribuição no nível de fora
  size_t op = std::string::npos;
  depth = 0;
  for (size_t k = 1; k < toks.size(); ++k) {
    const auto ty = toks[k].type;
    if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
        ty == GTokenType::LBrace)
      depth++;
    else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
             ty == GTokenType::RBrace)
      depth--;
    else if (depth == 0 && (ty == GTokenType::Assign ||
                            compoundOp(ty) != GTokenType::Unknown)) {
      op = k;
      break;
    } else if (depth == 0 &&
               (ty == GTokenType::Increment || ty == GTokenType::Decrement) &&
               k + 1 == toks.size()) {
      op = k;
      break;
    }
  }
  if (op ==
      std::string::npos) { // expressão como comando: obj.metodo(); xs.push(1);
    evalValue(toks);
    return true;
  }
  if (classRoot && !staticRoot)
    throw std::runtime_error("a classe " + first.value +
                             " nao tem o campo static " +
                             (toks.size() > 2 ? toks[2].value : std::string()));
  // lado esquerdo: raiz seguida de .campo e [indice]
  LPath path{root, {}};
  for (size_t k = staticRoot ? 3 : 1; k < op;) {
    if (toks[k].type == GTokenType::Dot) {
      if (k + 1 >= op || toks[k + 1].type != GTokenType::Identifier)
        throw std::runtime_error("depois de '.' vem o nome do campo");
      if (k + 2 < op && toks[k + 2].type == GTokenType::LParen)
        throw std::runtime_error("nao da para atribuir ao resultado de " +
                                 toks[k + 1].value + "(...)");
      path.steps.push_back({true, GhostValue::text(toks[k + 1].value)});
      k += 2;
    } else if (toks[k].type == GTokenType::LBracket) {
      std::vector<Token> idx;
      int d = 1;
      ++k;
      for (; k < op; ++k) {
        if (toks[k].type == GTokenType::LBracket)
          d++;
        if (toks[k].type == GTokenType::RBracket && --d == 0)
          break;
        idx.push_back(toks[k]);
      }
      if (k >= op)
        throw std::runtime_error("faltou ']'");
      ++k;
      path.steps.push_back({false, evalValue(idx)});
    } else {
      throw std::runtime_error("lado esquerdo invalido perto de '" +
                               toks[k].value + "'");
    }
  }
  const GTokenType opTy = toks[op].type;
  GhostValue rhs = GhostValue::number(1, true);
  if (opTy != GTokenType::Increment && opTy != GTokenType::Decrement) {
    std::vector<Token> r(toks.begin() + static_cast<long long>(op) + 1,
                         toks.end());
    if (r.empty())
      throw std::runtime_error("faltou o valor depois de '" + toks[op].value +
                               "'");
    rhs = evalValue(r);
  } else if (path.steps.empty() && !staticRoot) {
    throw std::runtime_error("use nome++;");
  }
  const GhostFieldDef *fi = nullptr;
  std::string listElemSpec;
  GhostValue *slot =
      resolvePath(path, opTy == GTokenType::Assign, false, &fi, &listElemSpec);
  GhostValue nv;
  if (opTy == GTokenType::Assign)
    nv = rhs;
  else if (opTy == GTokenType::Increment)
    nv = applyBinary(GTokenType::Plus, *slot, rhs);
  else if (opTy == GTokenType::Decrement)
    nv = applyBinary(GTokenType::Minus, *slot, rhs);
  else
    nv = applyBinary(compoundOp(opTy), *slot, rhs);
  if (!fi && staticRoot && path.steps.empty())
    fi = staticInfo; // Classe.total += 1 continua int
  if (fi)
    nv = coerce(nv, fi->type, fi->cls,
                fi->name); // campo int continua int, objeto é fatiado
  else if (!listElemSpec.empty())
    nv = coerceSpec(nv, listElemSpec, "elemento de list<" + listElemSpec + ">");
  *slot = nv;
  return true;
}

// try { ... } catch (e) { ... } finally { ... }
void GhostInterpreter::parseTryRepl(Lexer &lex) {
  if (lex.getNextToken().type != GTokenType::LBrace)
    throw std::runtime_error("Esperado '{' apos try.");
  const std::string tryBlock = lex.extractBlock();
  std::string catchBlock, finBlock, var;
  bool hasCatch = false, hasFinally = false;
  if (lex.peekNextToken().type == GTokenType::KwCatch) {
    lex.getNextToken();
    hasCatch = true;
    Token t = lex.getNextToken();
    if (t.type == GTokenType::LParen) {
      t = lex.getNextToken();
      if (t.type == GTokenType::KwString || t.type == GTokenType::KwVar)
        t = lex.getNextToken();
      if (t.type == GTokenType::Identifier) {
        var = t.value;
        t = lex.getNextToken();
      }
      if (t.type != GTokenType::RParen)
        throw std::runtime_error("catch: use catch (e) { ... }");
      t = lex.getNextToken();
    }
    if (t.type != GTokenType::LBrace)
      throw std::runtime_error("Esperado '{' apos catch.");
    catchBlock = lex.extractBlock();
  }
  if (lex.peekNextToken().type == GTokenType::KwFinally) {
    lex.getNextToken();
    if (lex.getNextToken().type != GTokenType::LBrace)
      throw std::runtime_error("Esperado '{' apos finally.");
    finBlock = lex.extractBlock();
    hasFinally = true;
  }
  if (!hasCatch && !hasFinally)
    throw std::runtime_error("try precisa de catch ou finally");
  auto runFinally = [&]() {
    if (hasFinally)
      executeLine(finBlock);
  };

  try {
    executeLine(tryBlock);
  } catch (const std::exception &ex) {
    if (!hasCatch) {
      runFinally();
      throw;
    }
    const std::string msg = ex.what();
    SavedVar saved;
    if (!var.empty()) {
      saved = saveVar(var);
      storeVar(var, GhostValue::text(msg), GTokenType::KwString);
    }
    error_stack.push_back(msg);
    try {
      executeLine(catchBlock);
    } catch (...) {
      error_stack.pop_back();
      if (!var.empty())
        restoreVar(var, saved);
      runFinally();
      throw;
    }
    error_stack.pop_back();
    if (!var.empty())
      restoreVar(var, saved);
    runFinally();
    return;
  } catch (...) { // break/continue/return atravessam o try, mas o finally roda
    runFinally();
    throw;
  }
  runFinally();
}

// throw expr;  throw;  (relança dentro de catch)
void GhostInterpreter::parseThrowRepl(Lexer &lex) {
  std::vector<Token> expr;
  while (true) {
    Token t = lex.getNextToken();
    if (t.type == GTokenType::Semicolon || t.type == GTokenType::EndOfFile)
      break;
    expr.push_back(t);
  }
  if (expr.empty()) {
    if (error_stack.empty())
      throw std::runtime_error("throw; fora de catch");
    throw std::runtime_error(error_stack.back());
  }
  throw std::runtime_error(textOf(evalValue(expr), false));
}

// foreach (x in xs) / (k, v in mapa) / (c in "texto") / (i in range(5)) / (x in
// matriz)
void GhostInterpreter::runForeach(const std::vector<std::string> &vars,
                                  std::vector<Token> &coll,
                                  const std::string &body,
                                  const std::string &elemSpec) {
  std::vector<std::pair<GhostValue, GhostValue>>
      items; // (chave/elemento, valor)
  if (coll.size() == 1 && coll[0].type == GTokenType::Identifier &&
      memory.count(coll[0].value) && !coll_memory.count(coll[0].value)) {
    GhostMatrix &m = memory[coll[0].value];
    for (size_t r = 0; r < m.getRows(); ++r)
      for (size_t c = 0; c < m.getCols(); ++c)
        items.push_back({GhostValue::number(m(r, c).x), {}});
  } else if (coll.size() == 1 && coll[0].type == GTokenType::Identifier &&
             qreg_memory.count(coll[0].value)) {
    int nq = qreg_memory[coll[0].value].size();
    for (int q = 0; q < nq; ++q)
      items.push_back({GhostValue::number(q, true), {}});
  } else {
    GhostValue v = evalValue(coll);
    if (v.kind == K::List || v.kind == K::Set)
      for (auto &e : v.list)
        items.push_back({e, {}});
    else if (v.kind == K::Map)
      for (auto &kv : v.map)
        items.push_back({GhostValue::text(kv.first), kv.second});
    else if (v.kind == K::Str)
      for (char ch : v.str)
        items.push_back({GhostValue::text(std::string(1, ch)), {}});
    else if (v.kind == K::Quantum) {
      int nq = v.qreg ? v.qreg->size() : 0;
      for (int q = 0; q < nq; ++q)
        items.push_back({GhostValue::number(q, true), {}});
    } else
      throw std::runtime_error(
          "foreach: '" + tokensToSource(coll) +
          "' nao e lista, conjunto, mapa, texto, qreg ou range()");
  }
  std::vector<SavedVar> saved;
  for (const auto &v : vars)
    saved.push_back(saveVar(v));
  auto restore = [&]() {
    for (size_t k = 0; k < vars.size(); ++k)
      restoreVar(vars[k], saved[k]);
  };
  try {
    for (auto &it : items) {
      GhostValue ev = it.first;
      if (!elemSpec.empty() && vars.size() == 1) {
        ev = coerceSpec(ev, elemSpec, "foreach");
      }
      storeVar(vars[0], ev, GTokenType::KwVar);
      if (!elemSpec.empty() && vars.size() == 1 && knownType(elemSpec))
        var_class[vars[0]] = elemSpec;
      if (vars.size() > 1)
        storeVar(vars[1], it.second, GTokenType::KwVar);
      try {
        executeLine(body);
      } catch (GhostBreakSignal &) {
        break;
      } catch (GhostContinueSignal &) {
        continue;
      }
    }
  } catch (...) {
    restore();
    throw;
  }
  restore();
}

// ---------------------------------------------------------------- fn<...> e
// lambdas/closures

namespace {
void specToTypePair(const std::string &spec, GTokenType &outType,
                    std::string &outCls) {
  outCls.clear();
  if (spec == "int") {
    outType = GTokenType::KwInt;
    return;
  }
  if (spec == "double") {
    outType = GTokenType::KwDouble;
    return;
  }
  if (spec == "float") {
    outType = GTokenType::KwFloat;
    return;
  }
  if (spec == "bool") {
    outType = GTokenType::KwBool;
    return;
  }
  if (spec == "string") {
    outType = GTokenType::KwString;
    return;
  }
  if (spec == "void") {
    outType = GTokenType::KwVoid;
    return;
  }
  if (spec == "ghost") {
    outType = GTokenType::KwGhost;
    return;
  }
  if (spec == "complex") {
    outType = GTokenType::KwComplex;
    return;
  }
  if (spec == "qreg") {
    outType = GTokenType::KwQreg;
    return;
  }
  if (spec == "qcircuit") {
    outType = GTokenType::KwQcircuit;
    return;
  }
  if (GhostInterpreter::isListSpec(spec)) {
    outType = GTokenType::KwList;
    outCls = spec;
    return;
  }
  if (spec.rfind("list<", 0) == 0) {
    outType = GTokenType::KwList;
    return;
  }
  if (spec.rfind("map<", 0) == 0) {
    outType = GTokenType::KwMap;
    return;
  }
  outType = GTokenType::KwVar;
  outCls = spec;
}

bool isPrimitiveTypeName(const std::string &s) {
  return s == "int" || s == "double" || s == "float" || s == "bool" ||
         s == "string" || s == "void" || s == "ghost" || s == "complex" ||
         s == "qreg" || s == "qcircuit";
}
} // namespace

GhostValue GhostInterpreter::makeNamedFnValue(const std::string &name,
                                              const GhostFunction &f) const {
  auto c = std::make_shared<GhostClosure>();
  c->isNamedFn = true;
  c->targetFuncName = name;
  c->targetFunc = f;
  c->returnType = f.returnType;
  c->returnClass = f.returnClass;
  c->returnSpec = f.returnSpec.empty() ? std::string("void") : f.returnSpec;
  c->hasExplicitRet = true;
  c->params = f.parameters;
  c->paramClasses = f.paramClasses;
  c->paramSpecs = f.paramSpecs;
  std::string ps;
  for (size_t k = 0; k < c->paramSpecs.size(); ++k)
    ps += (k ? "," : "") + c->paramSpecs[k];
  GhostValue v;
  v.kind = K::Fn;
  v.str = "fn<" + c->returnSpec + "(" + ps + ")>";
  v.fn = std::move(c);
  return v;
}

bool GhostInterpreter::isCaptureLambdaAt(const std::vector<Token> &t,
                                         size_t i) const {
  if (i >= t.size() || t[i].type != GTokenType::LBracket)
    return false;
  size_t k = i + 1;
  for (; k < t.size(); ++k) {
    const auto ty = t[k].type;
    if (ty == GTokenType::RBracket)
      break;
    if (ty != GTokenType::Comma && ty != GTokenType::Assign &&
        ty != GTokenType::BitAnd && ty != GTokenType::KwThis &&
        ty != GTokenType::Identifier)
      return false;
  }
  if (k >= t.size() || t[k].type != GTokenType::RBracket)
    return false;
  return isLambdaAt(t, k + 1);
}

bool GhostInterpreter::isLambdaAt(const std::vector<Token> &t, size_t i) const {
  if (i >= t.size())
    return false;
  auto arrowFollows = [&](size_t close) -> bool {
    if (close + 1 >= t.size())
      return false;
    if (t[close + 1].type == GTokenType::Arrow)
      return true;
    if (t[close + 1].type != GTokenType::ThinArrow)
      return false;
    int angle = 0, paren = 0, bracket = 0;
    for (size_t k = close + 2; k < t.size(); ++k) {
      const auto ty = t[k].type;
      if (angle == 0 && paren == 0 && bracket == 0 && ty == GTokenType::Arrow)
        return k > close + 2;
      if (ty == GTokenType::Lt)
        angle++;
      else if (ty == GTokenType::Gt)
        angle--;
      else if (ty == GTokenType::Chain)
        angle -= 2;
      else if (ty == GTokenType::LParen)
        paren++;
      else if (ty == GTokenType::RParen) {
        if (--paren < 0)
          return false;
      } else if (ty == GTokenType::LBracket)
        bracket++;
      else if (ty == GTokenType::RBracket) {
        if (--bracket < 0)
          return false;
      } else if (angle == 0 && paren == 0 && bracket == 0 &&
                 (ty == GTokenType::Semicolon || ty == GTokenType::LBrace ||
                  ty == GTokenType::Assign))
        return false;
    }
    return false;
  };

  if (t[i].type == GTokenType::LogicalOr) {
    return arrowFollows(i);
  }
  if (t[i].type == GTokenType::BitOr) {
    int angle = 0, paren = 0, bracket = 0, brace = 0;
    for (size_t k = i + 1; k < t.size(); ++k) {
      const auto ty = t[k].type;
      if (ty == GTokenType::Semicolon)
        return false;
      if (angle == 0 && paren == 0 && bracket == 0 && brace == 0 && ty == GTokenType::BitOr) {
        return arrowFollows(k);
      }
      if (ty == GTokenType::Lt)
        angle++;
      else if (ty == GTokenType::Gt) { if (angle > 0) angle--; }
      else if (ty == GTokenType::Chain) { if (angle >= 2) angle -= 2; }
      else if (ty == GTokenType::LParen)
        paren++;
      else if (ty == GTokenType::RParen) { if (paren > 0) paren--; }
      else if (ty == GTokenType::LBracket)
        bracket++;
      else if (ty == GTokenType::RBracket) { if (bracket > 0) bracket--; }
      else if (ty == GTokenType::LBrace)
        brace++;
      else if (ty == GTokenType::RBrace) { if (brace > 0) brace--; }
    }
    return false;
  }
  if (t[i].type != GTokenType::LParen)
    return false;
  int depth = 1;
  size_t close = t.size();
  for (size_t k = i + 1; k < t.size(); ++k) {
    const auto ty = t[k].type;
    if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
        ty == GTokenType::LBrace)
      depth++;
    else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
             ty == GTokenType::RBrace) {
      if (--depth == 0) {
        if (ty != GTokenType::RParen)
          return false;
        close = k;
        break;
      }
    }
  }
  if (close >= t.size())
    return false;
  return arrowFollows(close);
}

GhostValue GhostInterpreter::evalLambdaAt(std::vector<Token> &t, size_t &i,
                                          bool eval) {
  auto closure = std::make_shared<GhostClosure>();
  bool explicitCapture = false;
  char defMode = '='; // default [=]
  std::set<std::string> explicitCapVars;
  std::set<std::string> refCapVars;
  bool capThisByRef = false;
  bool capThisByVal = false;
  if (t[i].type == GTokenType::LBracket) {
    explicitCapture = true;
    ++i; // '['
    defMode = 0;
    std::set<std::string> seenCap;
    bool firstItem = true;
    while (i < t.size() && t[i].type != GTokenType::RBracket) {
      if (!firstItem) {
        if (t[i].type != GTokenType::Comma)
          throw std::runtime_error(
              "esperado ',' na lista de captura da lambda");
        ++i;
      }
      firstItem = false;
      if (i >= t.size())
        break;
      if (t[i].type == GTokenType::Assign && seenCap.empty() && defMode == 0 &&
          (i + 1 < t.size() && (t[i + 1].type == GTokenType::RBracket ||
                                t[i + 1].type == GTokenType::Comma))) {
        defMode = '=';
        ++i;
        continue;
      }
      if (t[i].type == GTokenType::BitAnd && seenCap.empty() && defMode == 0 &&
          (i + 1 < t.size() && (t[i + 1].type == GTokenType::RBracket ||
                                t[i + 1].type == GTokenType::Comma))) {
        defMode = '&';
        if (!frames.empty())
          closure->defaultRefInFunction = true;
        ++i;
        continue;
      }
      bool byRef = false;
      if (t[i].type == GTokenType::BitAnd) {
        byRef = true;
        ++i;
      }
      if (i >= t.size())
        throw std::runtime_error("captura incompleta na lambda");
      if (t[i].type == GTokenType::KwThis) {
        if (!inMethod())
          throw std::runtime_error(
              "this na captura da lambda so vale em metodo de instancia");
        if (!seenCap.insert("this").second)
          throw std::runtime_error("'this' repetido na captura da lambda");
        if (defMode == '=' && !byRef)
          throw std::runtime_error("com '=' o objeto this ja e copiado; use "
                                   "[&this] para capturar por referencia");
        if (defMode == '&' && byRef)
          throw std::runtime_error("com '&' o objeto this ja e capturado por "
                                   "referencia; use [this] para copiar");
        if (byRef)
          capThisByRef = true;
        else
          capThisByVal = true;
        ++i;
        continue;
      }
      if (t[i].type != GTokenType::Identifier)
        throw std::runtime_error("captura invalida na lambda: use [&x], [x], "
                                 "[=], [&], [] ou [&this]");
      const std::string vName = t[i].value;
      ++i;
      if (!seenCap.insert(vName).second)
        throw std::runtime_error("variavel '" + vName +
                                 "' repetida na captura da lambda");
      if (defMode == '=' && !byRef)
        throw std::runtime_error("com '=' as variaveis ja sao capturadas por "
                                 "valor; liste apenas &variavel (ex.: [=, &" +
                                 vName + "])");
      if (defMode == '&' && byRef)
        throw std::runtime_error(
            "com '&' as variaveis ja sao capturadas por referencia; liste "
            "apenas variaveis por valor (ex.: [&, " +
            vName + "])");
      bool foundV = numeric_memory.count(vName) || string_memory.count(vName) ||
                    coll_memory.count(vName);
      if (!foundV && inMethod())
        foundV = (fieldRef(vName) != nullptr || staticRef(vName) != nullptr);
      if (!foundV)
        throw std::runtime_error("variavel '" + vName +
                                 "' nao declarada para capturar na lambda");
      explicitCapVars.insert(vName);
      if (byRef)
        refCapVars.insert(vName);
    }
    if (i >= t.size() || t[i].type != GTokenType::RBracket)
      throw std::runtime_error("faltou ']' na captura da lambda");
    ++i; // ']'
  }
  if (defMode == 0)
    closure->disallowUncapturedLocals = true;
  const bool isLogicalOrZero = (i < t.size() && t[i].type == GTokenType::LogicalOr);
  const bool isPipe = (i < t.size() && t[i].type == GTokenType::BitOr);
  if (i >= t.size() || (!isLogicalOrZero && !isPipe && t[i].type != GTokenType::LParen))
    throw std::runtime_error("esperado '(' ou '|' na lambda");
  auto ensureConcreteTypesInSpec = [&](const std::string &spec,
                                       const std::string &ctx) {
    std::function<void(const std::string &, bool)> chk =
        [&](const std::string &s, bool allowVoid) {
          if (isPrimitiveTypeName(s)) {
            if (s == "void" && !allowVoid)
              throw std::runtime_error(ctx + " nao pode ser void");
            return;
          }
          if (isFnSpec(s)) {
            std::string r;
            std::vector<std::string> ps;
            if (!parseFnSignature(s, r, ps))
              throw std::runtime_error("tipo invalido: " + s);
            chk(r, true);
            for (const auto &p : ps)
              chk(p, false);
            return;
          }
          const size_t lt = s.find('<');
          if (lt != std::string::npos && s.back() == '>') {
            const std::string h = s.substr(0, lt);
            if (generic_memory.count(h))
              ensureInstance(s);
            std::string cur;
            int d = 0;
            for (size_t k = lt + 1; k + 1 < s.size(); ++k) {
              const char ch = s[k];
              if (ch == '<')
                d++;
              else if (ch == '>')
                d--;
              else if (ch == ',' && d == 0) {
                chk(cur, false);
                cur.clear();
                continue;
              }
              cur += ch;
            }
            if (!cur.empty())
              chk(cur, false);
            return;
          }
          if (!class_memory.count(s) && !interface_memory.count(s))
            throw std::runtime_error("tipo desconhecido '" + s + "' em " + ctx);
        };
    chk(spec, true);
  };

  // Encontra o delimitador que fecha os parametros
  size_t close = i;
  if (isLogicalOrZero) {
    // zero parametros: || => ...
    close = i;
  } else if (isPipe) {
    ++i; // consome o '|' inicial
    close = i;
    int angle = 0, paren = 0, bracket = 0, brace = 0;
    for (; close < t.size(); ++close) {
      const auto ty = t[close].type;
      if (ty == GTokenType::Semicolon)
        break;
      if (angle == 0 && paren == 0 && bracket == 0 && brace == 0 && ty == GTokenType::BitOr)
        break;
      if (ty == GTokenType::Lt)
        angle++;
      else if (ty == GTokenType::Gt || ty == GTokenType::Chain)
        angle = std::max(0, angle - (ty == GTokenType::Chain ? 2 : 1));
      else if (ty == GTokenType::LParen)
        paren++;
      else if (ty == GTokenType::RParen)
        paren--;
      else if (ty == GTokenType::LBracket)
        bracket++;
      else if (ty == GTokenType::RBracket)
        bracket--;
      else if (ty == GTokenType::LBrace)
        brace++;
      else if (ty == GTokenType::RBrace)
        brace--;
    }
    if (close >= t.size() || t[close].type != GTokenType::BitOr)
      throw std::runtime_error("lambda: faltou '|'");
  } else {
    ++i; // consome '('
    close = i;
    int d = 1;
    for (; close < t.size(); ++close) {
      if (t[close].type == GTokenType::LParen ||
          t[close].type == GTokenType::LBracket ||
          t[close].type == GTokenType::LBrace)
        d++;
      else if (t[close].type == GTokenType::RParen ||
               t[close].type == GTokenType::RBracket ||
               t[close].type == GTokenType::RBrace) {
        if (--d == 0)
          break;
      }
    }
    if (close >= t.size() || t[close].type != GTokenType::RParen)
      throw std::runtime_error("lambda: faltou ')'");
  }

  if (i < close) {
    std::vector<std::vector<Token>> rawParams(1);
    int angle = 0, paren = 0, bracket = 0;
    for (size_t k = i; k < close; ++k) {
      const Token &tk = t[k];
      if (tk.type == GTokenType::Lt &&
          (angle > 0 || (!rawParams.back().empty() &&
                         opensTypeAngle(rawParams.back().back())))) {
        angle++;
        rawParams.back().push_back(tk);
        continue;
      }
      if (angle > 0 &&
          (tk.type == GTokenType::Gt || tk.type == GTokenType::Chain)) {
        angle = std::max(0, angle - (tk.type == GTokenType::Chain ? 2 : 1));
        rawParams.back().push_back(tk);
        continue;
      }
      if (tk.type == GTokenType::LParen)
        paren++;
      else if (tk.type == GTokenType::RParen)
        paren--;
      else if (tk.type == GTokenType::LBracket)
        bracket++;
      else if (tk.type == GTokenType::RBracket)
        bracket--;
      if (angle == 0 && paren == 0 && bracket == 0 &&
          tk.type == GTokenType::Comma) {
        rawParams.emplace_back();
        continue;
      }
      rawParams.back().push_back(tk);
    }
    for (const auto &pt : rawParams) {
      if (pt.empty())
        throw std::runtime_error("lambda: faltou parametro antes de ','");
      for (const auto &x : pt)
        if (x.type == GTokenType::Assign)
          throw std::runtime_error("parametros de lambda nao tem valor padrao");
      if (pt[0].type == GTokenType::KwVar) {
        const std::string hint =
            pt.size() > 1 && pt[1].type == GTokenType::Identifier ? pt[1].value
                                                                  : "x";
        throw std::runtime_error("para inferir o tipo do parametro na lambda "
                                 "escreva apenas o nome: (" +
                                 hint + ") => ...");
      }
      if (pt.size() == 1 && pt[0].type == GTokenType::Identifier) {
        const std::string pName = pt[0].value;
        if (class_memory.count(pName) || interface_memory.count(pName) ||
            generic_memory.count(pName) || generic_iface_memory.count(pName))
          throw std::runtime_error(
              "'" + pName + "' e um tipo; informe o nome do parametro (ex.: (" +
              pName + " x) => ...)");
        for (const auto &prev : closure->params)
          if (prev.second == pName)
            throw std::runtime_error("parametro '" + pName +
                                     "' repetido na lambda");
        closure->isPolyLambda = true;
        closure->params.push_back({GTokenType::KwVar, pName});
        closure->paramClasses.push_back("");
        closure->paramSpecs.push_back("");
        continue;
      }
      bool postArr = pt.size() >= 4 && pt.back().type == GTokenType::RBracket &&
                     pt[pt.size() - 2].type == GTokenType::LBracket &&
                     pt[pt.size() - 3].type == GTokenType::Identifier;
      const size_t nameIdx = postArr ? pt.size() - 3 : pt.size() - 1;
      if (pt[nameIdx].type != GTokenType::Identifier)
        throw std::runtime_error(
            "esperado o nome do parametro da lambda depois do tipo");
      const std::string pName = pt[nameIdx].value;
      for (const auto &prev : closure->params)
        if (prev.second == pName)
          throw std::runtime_error("parametro '" + pName +
                                   "' repetido na lambda");
      std::string typeStr = joinTokensForType(pt, 0, nameIdx);
      if (postArr)
        typeStr += "[]";
      std::string pSpec = validateAndNormalizeTypeSpec(typeStr, true);
      if (pSpec == "void")
        throw std::runtime_error("parametro da lambda nao pode ser void");
      ensureConcreteTypesInSpec(pSpec, "parametro '" + pName + "' da lambda");
      GTokenType pTy = GTokenType::KwVar;
      std::string pCls;
      specToTypePair(pSpec, pTy, pCls);
      closure->params.push_back({pTy, pName});
      closure->paramClasses.push_back(pCls);
      closure->paramSpecs.push_back(pSpec);
    }
  }
  i = close + 1;

  if (i < t.size() && t[i].type == GTokenType::ThinArrow) {
    ++i;
    size_t arrowAt = i;
    int angle = 0, paren = 0, bracket = 0;
    for (; arrowAt < t.size(); ++arrowAt) {
      const auto ty = t[arrowAt].type;
      if (angle == 0 && paren == 0 && bracket == 0 && ty == GTokenType::Arrow)
        break;
      if (ty == GTokenType::Lt)
        angle++;
      else if (ty == GTokenType::Gt)
        angle--;
      else if (ty == GTokenType::Chain)
        angle -= 2;
      else if (ty == GTokenType::LParen)
        paren++;
      else if (ty == GTokenType::RParen)
        paren--;
      else if (ty == GTokenType::LBracket)
        bracket++;
      else if (ty == GTokenType::RBracket)
        bracket--;
    }
    if (arrowAt >= t.size() || arrowAt == i || t[i].type == GTokenType::KwVar)
      throw std::runtime_error("esperado tipo de retorno depois de '->' na "
                               "lambda (ex.: (int x) -> double => ...)");
    const std::string retRaw = joinTokensForType(t, i, arrowAt);
    const std::string rSpec = validateAndNormalizeTypeSpec(retRaw, true);
    ensureConcreteTypesInSpec(rSpec, "retorno da lambda");
    closure->hasExplicitRet = true;
    closure->returnSpec = rSpec;
    specToTypePair(rSpec, closure->returnType, closure->returnClass);
    i = arrowAt;
  }
  if (i >= t.size() || t[i].type != GTokenType::Arrow)
    throw std::runtime_error("esperado '=>' na lambda");
  ++i; // '=>'
  if (i >= t.size())
    throw std::runtime_error("faltou o corpo da lambda depois de '=>'");

  if (t[i].type == GTokenType::LBrace) {
    closure->isExprBody = false;
    size_t openBrace = i;
    skipBalanced(t, i);
    if (i <= openBrace + 1 || t[i - 1].type != GTokenType::RBrace)
      throw std::runtime_error("faltou '}' no bloco da lambda");
    std::vector<Token> bodyToks(t.begin() + static_cast<long>(openBrace + 1),
                                t.begin() + static_cast<long>(i - 1));
    closure->body = tokensToSource(bodyToks);
  } else {
    if (t[i].type == GTokenType::KwPrint)
      throw std::runtime_error("print(...) e um comando: use chaves na lambda, "
                               "ex.: (string s) => { print(s); }");
    closure->isExprBody = true;
    const size_t startExpr = i;
    skipLambdaExprBody(t, i);
    if (i == startExpr)
      throw std::runtime_error("faltou o corpo da lambda depois de '=>'");
    closure->exprTokens.assign(t.begin() + static_cast<long>(startExpr),
                               t.begin() + static_cast<long>(i));
  }

  GhostValue out;
  out.kind = K::Fn;
  if (!eval)
    return out;

  // Coleta todas as variaveis vivas no escopo atual
  std::set<std::string> allVars;
  for (const auto &kv : numeric_memory)
    if (kv.first.rfind("__collapse_", 0) != 0)
      allVars.insert(kv.first);
  for (const auto &kv : string_memory)
    allVars.insert(kv.first);
  for (const auto &kv : coll_memory)
    allVars.insert(kv.first);
  auto isLocalVar = [&](const std::string &vn, size_t &ownerDepth,
                        uint64_t &ownerFId) -> bool {
    for (size_t d = frames.size(); d-- > 0;) {
      if (frames[d].count(vn)) {
        ownerDepth = d;
        ownerFId = (d < frame_ids.size()) ? frame_ids[d] : 0;
        return true;
      }
    }
    ownerDepth = static_cast<size_t>(-1);
    ownerFId = 0;
    return false;
  };
  for (const auto &vn : allVars) {
    size_t oDepth = static_cast<size_t>(-1);
    uint64_t oFId = 0;
    const bool isLoc = isLocalVar(vn, oDepth, oFId);
    bool capByRef = false;
    bool capByVal = false;
    if (defMode == '&')
      capByRef = (refCapVars.count(vn) > 0 || explicitCapVars.count(vn) == 0);
    else if (defMode == '=')
      capByRef = (refCapVars.count(vn) > 0);
    else
      capByRef = (refCapVars.count(vn) > 0);
    if (defMode == '=')
      capByVal = (refCapVars.count(vn) == 0);
    else if (defMode == '&')
      capByVal = (explicitCapVars.count(vn) > 0 && refCapVars.count(vn) == 0);
    else
      capByVal = (explicitCapVars.count(vn) > 0 && refCapVars.count(vn) == 0);
    if (capByRef) {
      closure->refEnv[vn] = {oDepth, oFId};
    } else if (capByVal) {
      GhostClosure::CapturedVar cap;
      cap.declType =
          type_memory.count(vn) ? type_memory.at(vn) : GTokenType::KwVar;
      cap.varCls = var_class.count(vn) ? var_class.at(vn) : "";
      if (numeric_memory.count(vn)) {
        const double d = numeric_memory.at(vn);
        const GTokenType ty =
            type_memory.count(vn) ? type_memory.at(vn) : GTokenType::KwDouble;
        if (ty == GTokenType::KwBool)
          cap.val = GhostValue::boolean(d != 0.0);
        else if (ty == GTokenType::KwInt)
          cap.val = GhostValue::number(d, true);
        else if (ty == GTokenType::KwVar)
          cap.val = GhostValue::number(d, d == std::floor(d));
        else
          cap.val = GhostValue::number(d);
      } else if (string_memory.count(vn)) {
        cap.val = GhostValue::text(string_memory.at(vn));
      } else if (coll_memory.count(vn)) {
        cap.val = coll_memory.at(vn);
      }
      closure->env[vn] = std::move(cap);
    }
  }
  if (inMethod()) {
    const bool refThisEffective =
        capThisByRef || (defMode == '&' && !capThisByVal);
    if (refThisEffective) {
      closure->refThis = true;
      closure->refThisPath = this_stack.back().path;
      closure->refThisOwnerDepth = this_stack.back().frameDepth;
      closure->refThisOwnerFrameId =
          (this_stack.back().frameDepth < frame_ids.size())
              ? frame_ids[this_stack.back().frameDepth]
              : 0;
      closure->capturedClass = this_stack.back().cls;
    } else {
      closure->hasThis = true;
      closure->capturedThis = this_stack.back().obj;
      closure->capturedClass = this_stack.back().cls;
    }
  } else {
    closure->capturedStaticClass = currentClassCtx();
  }

  // Inferência do retorno (se nao for polimorfica)
  if (!closure->hasExplicitRet) {
    if (!closure->isPolyLambda)
      closure->inferredRetSpec = inferLambdaRetSpec(*closure);
    else {
      // Valida ? bare mesmo em polimorfica
      if (closure->isExprBody) {
        for (size_t k = 0; k < closure->exprTokens.size(); ++k) {
          if (closure->exprTokens[k].type == GTokenType::Question &&
              ghost_is_try_question(std::vector<Token>(
                  closure->exprTokens.begin() + static_cast<long>(k + 1),
                  closure->exprTokens.end()))) {
            throw std::runtime_error("o operador ? dentro de lambda exige "
                                     "retorno explicito '-> result<T>'");
          }
        }
      }
    }
  } else if (!isResultSpec(closure->returnSpec)) {
    // Confere '?' solto em corpo de expressão mesmo antes da chamada
    if (closure->isExprBody) {
      for (size_t k = 0; k < closure->exprTokens.size(); ++k) {
        if (closure->exprTokens[k].type == GTokenType::Question &&
            ghost_is_try_question(std::vector<Token>(
                closure->exprTokens.begin() + static_cast<long>(k + 1),
                closure->exprTokens.end()))) {
          throw std::runtime_error(
              "o operador ? dentro de lambda exige retorno '-> result<T>'");
        }
      }
    }
  }

  std::string dispRet =
      closure->hasExplicitRet ? closure->returnSpec : closure->inferredRetSpec;
  if (dispRet.rfind("ok<", 0) == 0 && dispRet.back() == '>')
    dispRet = "result<" + dispRet.substr(3, dispRet.size() - 4) + ">";
  std::string ps;
  for (size_t k = 0; k < closure->paramSpecs.size(); ++k)
    ps += (k ? "," : "") + closure->paramSpecs[k];
  if (!dispRet.empty() && dispRet != "result")
    out.str = "fn<" + dispRet + "(" + ps + ")>";
  out.fn = std::move(closure);
  return out;
}

GhostValue
GhostInterpreter::invokeFnValue(const LPath *path, const GhostValue &fnVal,
                                std::vector<std::vector<Token>> &rawArgs) {
  if (!fnVal.fn)
    throw std::runtime_error(
        "fn sem funcao: guarde uma funcao ou lambda antes de chamar");
  std::vector<GhostValue> argVals;
  for (auto &r : rawArgs)
    argVals.push_back(evalValue(r));
  return invokeFnValueDirect(fnVal, argVals, path);
}

GhostValue
GhostInterpreter::invokeFnValueDirect(const GhostValue &fnVal,
                                      const std::vector<GhostValue> &inArgs,
                                      const LPath *path) {
  if (!fnVal.fn)
    throw std::runtime_error(
        "fn sem funcao: guarde uma funcao ou lambda antes de chamar");
  std::vector<GhostValue> argVals = inArgs;
  const GhostClosure *origPtr = fnVal.fn.get();
  GhostClosure working = *origPtr;
  if (working.isGenericFnVal) {
    auto git = generic_functions.find(working.genericFnName);
    if (git == generic_functions.end())
      throw std::runtime_error("funcao generica nao encontrada: " +
                               working.genericFnName);
    const std::vector<std::string> args =
        deduceGenericTypes(git->second, {}, argVals, working.genericFnName);
    const std::string spec = instantiateFunction(working.genericFnName, args);
    const GhostFunction f = functions_memory.at(spec);
    if (argVals.size() > f.parameters.size())
      throw std::runtime_error(
          spec + "(): recebeu " + std::to_string(argVals.size()) +
          " argumento(s), espera ate " + std::to_string(f.parameters.size()));
    for (size_t k = argVals.size(); k < f.parameters.size(); ++k) {
      if (k < f.defaults.size() && !f.defaults[k].empty()) {
        auto d = f.defaults[k];
        argVals.push_back(evalValue(d));
      } else
        throw std::runtime_error(spec + "(): faltou o argumento '" +
                                 f.parameters[k].second + "'");
    }
    GhostValue res = runFunction(f, spec, argVals, nullptr, "");
    if (working.wrapperRetSpec == "void")
      return GhostValue::number(0, true);
    if (!working.wrapperRetSpec.empty())
      return coerceSpec(res, working.wrapperRetSpec, "retorno de " + spec);
    return res;
  }
  if (argVals.size() != working.params.size()) {
    throw std::runtime_error(
        "chamada de funcao: recebeu " + std::to_string(argVals.size()) +
        " argumento(s), espera " + std::to_string(working.params.size()));
  }

  if (working.isNamedFn) {
    GhostValue res = runFunction(working.targetFunc, working.targetFuncName,
                                 argVals, nullptr, "");
    if (working.wrapperRetSpec == "void")
      return GhostValue::number(0, true);
    if (!working.wrapperRetSpec.empty())
      return coerceSpec(res, working.wrapperRetSpec,
                        "retorno de " + working.targetFuncName);
    return res;
  }

  if (call_depth >= 400)
    throw std::runtime_error("lambda: recursao profunda demais (limite 400)");
  RetGuard retGuard(ret_stack,
                    working.hasExplicitRet ? working.returnClass : "");

  // Valida referencias vivas antes de entrar
  for (const auto &kv : working.refEnv) {
    if (kv.second.ownerDepth != static_cast<size_t>(-1)) {
      if (kv.second.ownerDepth >= frame_ids.size() ||
          frame_ids[kv.second.ownerDepth] != kv.second.ownerFrameId)
        throw std::runtime_error("referencia capturada para variavel local '" +
                                 kv.first + "' expirou");
    }
  }
  GhostValue *realThisObj = nullptr;
  if (working.refThis) {
    if (!working.refThisPath.root.empty())
      realThisObj = resolvePath(working.refThisPath, false, true);
    else if (inMethod())
      realThisObj = &this_stack.back().obj;
  }
  pushFrame();
  const size_t envFrameDepth = frames.size() - 1;
  if (working.disallowUncapturedLocals) {
    for (size_t d = 0; d < envFrameDepth; ++d) {
      for (const auto &kv : frames[d]) {
        if (!working.env.count(kv.first) && !working.refEnv.count(kv.first) &&
            !frames.back().count(kv.first)) {
          declareLocal(kv.first);
          restoreVar(kv.first, frames[0].at(kv.first));
        }
      }
    }
  }
  for (const auto &kv : working.env) {
    declareLocal(kv.first);
    storeVar(kv.first, kv.second.val,
             kv.second.varCls.empty() ? kv.second.declType : GTokenType::KwVar);
    if (!kv.second.varCls.empty())
      var_class[kv.first] = kv.second.varCls;
  }
  std::map<std::string, size_t> shadowedRefFrames;
  for (const auto &kv : working.refEnv) {
    if (kv.second.ownerDepth != static_cast<size_t>(-1)) {
      for (size_t d = frames.size() - 1; d > kv.second.ownerDepth; --d) {
        if (frames[d].count(kv.first)) {
          declareLocal(kv.first);
          restoreVar(kv.first, frames[d].at(kv.first));
          shadowedRefFrames[kv.first] = d;
          break;
        }
      }
    }
  }
  pushFrame();
  const size_t bodyFrameDepth = frames.size() - 1;
  ++call_depth;
  if (working.refThis && realThisObj) {
    this_stack.push_back({*realThisObj, working.capturedClass, frames.size(),
                          working.refThisPath});
  } else if (working.hasThis) {
    this_stack.push_back(
        {working.capturedThis, working.capturedClass, frames.size()});
  } else if (!working.capturedStaticClass.empty()) {
    static_stack.push_back({working.capturedStaticClass, frames.size()});
  }
  bool cleanExit = false;
  auto leave = [&]() {
    if (working.refThis && realThisObj) {
      if (cleanExit)
        *realThisObj = std::move(this_stack.back().obj);
      this_stack.pop_back();
    } else if (working.hasThis) {
      if (cleanExit)
        working.capturedThis = std::move(this_stack.back().obj);
      this_stack.pop_back();
    } else if (!working.capturedStaticClass.empty()) {
      static_stack.pop_back();
    }
    popFrameAndRestore();
    if (cleanExit) {
      for (auto &kv : working.env) {
        bool fnd = false;
        GhostValue upd = lookupVar(kv.first, fnd);
        if (fnd)
          kv.second.val = std::move(upd);
      }
      for (const auto &sh : shadowedRefFrames) {
        frames[sh.second][sh.first] = saveVar(sh.first);
      }
    }
    popFrameAndRestore();
    --call_depth;
  };

  GhostValue result = GhostValue::number(0, true);
  try {
    for (size_t k = 0; k < working.params.size(); ++k) {
      const std::string &pn = working.params[k].second;
      const std::string pc =
          k < working.paramClasses.size() && knownType(working.paramClasses[k])
              ? working.paramClasses[k]
              : "";
      GhostValue pv = coerce(argVals[k], working.params[k].first, pc, pn);
      declareLocal(pn);
      storeVar(pn, pv,
               pc.empty() ? working.params[k].first : GTokenType::KwVar);
      if (!pc.empty())
        var_class[pn] = pc;
    }
    if (working.isExprBody) {
      std::vector<Token> expr = working.exprTokens;
      result = evalValue(expr);
    } else {
      try {
        executeLine(working.body);
      } catch (GhostReturnSignal &r) {
        if (working.hasExplicitRet &&
            working.returnType == GTokenType::KwVoid && r.hasValue)
          throw std::runtime_error("lambda -> void nao pode retornar valor");
        if (working.hasExplicitRet &&
            working.returnType != GTokenType::KwVoid && !r.hasValue)
          throw std::runtime_error("return sem valor em lambda -> " +
                                   working.returnSpec);
        if (r.hasValue) {
          checkReturnedClosureRefs(r.value, bodyFrameDepth);
          checkReturnedClosureRefs(r.value, envFrameDepth);
          result = r.value;
        }
      } catch (GhostBreakSignal &) {
        throw std::runtime_error("break fora de laco ou switch");
      } catch (GhostContinueSignal &) {
        throw std::runtime_error("continue fora de laco");
      }
    }
  } catch (...) {
    leave();
    throw;
  }
  cleanExit = true;
  leave();

  // Se chamada a partir de um lugar atribuível (variável, elemento, campo),
  // grava a closure atualizada
  if (path) {
    if (GhostValue *slot = resolvePath(*path, false, true)) {
      if (slot->kind == K::Fn && slot->fn.get() == origPtr) {
        slot->fn = std::make_shared<GhostClosure>(std::move(working));
      }
    }
  }

  if (working.hasExplicitRet) {
    if (working.returnType == GTokenType::KwVoid) {
      result = GhostValue::number(0, true);
    } else {
      const std::string rc =
          knownType(working.returnClass) ? working.returnClass : "";
      result = coerce(result, working.returnType, rc, "retorno da lambda");
    }
  }
  if (working.wrapperRetSpec == "void")
    return GhostValue::number(0, true);
  if (!working.wrapperRetSpec.empty())
    return coerceSpec(result, working.wrapperRetSpec, "retorno de funcao");
  return result;
}

std::string GhostInterpreter::inferExprType(
    const std::vector<Token> &toks,
    const std::map<std::string, std::string> &localTypes) {
  if (toks.empty())
    return "void";
  // Verifica operador ? de result<T> sem -> result<T> será checado por quem
  // chamou se preciso.
  // 1. Ternário no nível 0: cond ? a : b
  {
    int d = 0;
    for (size_t k = 0; k < toks.size(); ++k) {
      const auto ty = toks[k].type;
      if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
          ty == GTokenType::LBrace)
        d++;
      else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
               ty == GTokenType::RBrace)
        d--;
      else if (d == 0 && ty == GTokenType::Question &&
               !ghost_is_try_question(std::vector<Token>(
                   toks.begin() + static_cast<long>(k + 1), toks.end()))) {
        int qd = 0, innerQ = 0;
        for (size_t m = k + 1; m < toks.size(); ++m) {
          const auto mt = toks[m].type;
          if (mt == GTokenType::LParen || mt == GTokenType::LBracket ||
              mt == GTokenType::LBrace)
            qd++;
          else if (mt == GTokenType::RParen || mt == GTokenType::RBracket ||
                   mt == GTokenType::RBrace)
            qd--;
          else if (qd == 0 && mt == GTokenType::Question &&
                   !ghost_is_try_question(std::vector<Token>(
                       toks.begin() + static_cast<long>(m + 1), toks.end())))
            innerQ++;
          else if (qd == 0 && mt == GTokenType::Colon) {
            if (innerQ == 0) {
              const std::string ta = inferExprType(
                  std::vector<Token>(toks.begin() + static_cast<long>(k + 1),
                                     toks.begin() + static_cast<long>(m)),
                  localTypes);
              const std::string tb = inferExprType(
                  std::vector<Token>(toks.begin() + static_cast<long>(m + 1),
                                     toks.end()),
                  localTypes);
              if (ta == tb)
                return ta;
              if ((ta == "int" && tb == "double") ||
                  (ta == "double" && tb == "int"))
                return "double";
              return "";
            }
            innerQ--;
          }
        }
      }
    }
  }
  // 2. Operadores binários de menor precedência para maior no nível 0 (fora de
  // grupos e fora de lambdas)
  {
    int bestPrec = 100;
    size_t bestAt = std::string::npos;
    GTokenType bestOp = GTokenType::Unknown;
    int d = 0;
    for (size_t k = 0; k < toks.size(); ++k) {
      if (d == 0 &&
          (((toks[k].type == GTokenType::LParen || toks[k].type == GTokenType::BitOr || toks[k].type == GTokenType::LogicalOr) && isLambdaAt(toks, k)) ||
           (toks[k].type == GTokenType::LBracket &&
            isCaptureLambdaAt(toks, k))))
        break;
      const auto ty = toks[k].type;
      if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
          ty == GTokenType::LBrace) {
        d++;
        continue;
      }
      if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
          ty == GTokenType::RBrace) {
        d--;
        continue;
      }
      if (d != 0 || k == 0)
        continue;
      // Evita confundir '<' / '>' de tipos genéricos com comparação
      if ((ty == GTokenType::Lt || ty == GTokenType::Gt ||
           ty == GTokenType::Chain) &&
          k > 0 && opensTypeAngle(toks[k - 1])) {
        int ad = 0;
        for (; k < toks.size(); ++k) {
          if (toks[k].type == GTokenType::Lt)
            ad++;
          else if (toks[k].type == GTokenType::Gt)
            ad--;
          else if (toks[k].type == GTokenType::Chain)
            ad -= 2;
          if (ad <= 0)
            break;
        }
        continue;
      }
      const int p = binPrec(toks[k]);
      if (p > 0 && p <= bestPrec) {
        // Ignora unário após outro operador
        const auto prev = toks[k - 1].type;
        if ((ty == GTokenType::Minus || ty == GTokenType::Plus ||
             ty == GTokenType::Number) &&
            (prev == GTokenType::Plus || prev == GTokenType::Minus ||
             prev == GTokenType::Star || prev == GTokenType::Slash ||
             prev == GTokenType::Percent || prev == GTokenType::Eq ||
             prev == GTokenType::Neq || prev == GTokenType::Lt ||
             prev == GTokenType::Gt || prev == GTokenType::Leq ||
             prev == GTokenType::Geq || prev == GTokenType::LogicalAnd ||
             prev == GTokenType::LogicalOr || prev == GTokenType::Not))
          continue;
        bestPrec = p;
        bestAt = k;
        bestOp = ty == GTokenType::Number ? GTokenType::Minus : ty;
      }
    }
    if (bestAt != std::string::npos) {
      std::vector<Token> lhs(toks.begin(),
                             toks.begin() + static_cast<long>(bestAt));
      std::vector<Token> rhs;
      if (toks[bestAt].type == GTokenType::Number) {
        Token posNum = toks[bestAt];
        posNum.value = posNum.value.substr(1);
        rhs.push_back(posNum);
        rhs.insert(rhs.end(), toks.begin() + static_cast<long>(bestAt + 1),
                   toks.end());
      } else {
        rhs.assign(toks.begin() + static_cast<long>(bestAt + 1), toks.end());
      }
      switch (bestOp) {
      case GTokenType::LogicalOr:
      case GTokenType::LogicalAnd:
      case GTokenType::Eq:
      case GTokenType::Neq:
      case GTokenType::Lt:
      case GTokenType::Gt:
      case GTokenType::Leq:
      case GTokenType::Geq:
        return "bool";
      case GTokenType::BitAnd:
      case GTokenType::BitOr:
      case GTokenType::Caret:
      case GTokenType::ShiftLeft:
      case GTokenType::Chain:
        return "int";
      case GTokenType::Plus: {
        const std::string tl = inferExprType(lhs, localTypes);
        const std::string tr = inferExprType(rhs, localTypes);
        if (tl == "string" || tr == "string")
          return "string";
        if (tl.rfind("list<", 0) == 0 && tl == tr)
          return tl;
        if (tl == "double" || tr == "double")
          return "double";
        if (tl == "float" || tr == "float")
          return "float";
        if (tl == "int" && tr == "int")
          return "int";
        return "";
      }
      case GTokenType::Minus:
      case GTokenType::Star:
      case GTokenType::Slash:
      case GTokenType::Percent: {
        const std::string tl = inferExprType(lhs, localTypes);
        const std::string tr = inferExprType(rhs, localTypes);
        if (tl == "string" || tr == "string")
          throw std::runtime_error("operador nao suportado para string");
        if (tl == "double" || tr == "double")
          return "double";
        if (tl == "float" || tr == "float")
          return "float";
        if ((tl == "int" || tl == "bool") && (tr == "int" || tr == "bool"))
          return "int";
        return "";
      }
      default:
        return "";
      }
    }
  }
  // 3. Unários
  if (toks[0].type == GTokenType::Not)
    return "bool";
  if (toks[0].type == GTokenType::Tilde)
    return "int";
  if (toks[0].type == GTokenType::Minus || toks[0].type == GTokenType::Plus)
    return inferExprType(std::vector<Token>(toks.begin() + 1, toks.end()),
                         localTypes);

  // 4. Primário + cadeia de sufixos
  size_t i = 0;
  std::string base;
  const Token &first = toks[0];
  if (first.type == GTokenType::String || first.type == GTokenType::FString) {
    base = "string";
    i = 1;
  } else if (first.type == GTokenType::Number) {
    base = first.value.find_first_of(".eE") != std::string::npos ? "double"
                                                                 : "int";
    i = 1;
  } else if (first.type == GTokenType::LBracket && isCaptureLambdaAt(toks, 0)) {
    std::vector<Token> copy = toks;
    GhostValue lv = evalLambdaAt(copy, i, true);
    base = lv.str;
  } else if ((first.type == GTokenType::LParen || first.type == GTokenType::BitOr || first.type == GTokenType::LogicalOr) && isLambdaAt(toks, 0)) {
    std::vector<Token> copy = toks;
    GhostValue lv = evalLambdaAt(copy, i, true);
    base = lv.str;
    if (base.empty() && lv.fn) {
      std::string r =
          lv.fn->hasExplicitRet ? lv.fn->returnSpec : lv.fn->inferredRetSpec;
      if (r.rfind("ok<", 0) == 0 && r.back() == '>')
        r = "result<" + r.substr(3, r.size() - 4) + ">";
      std::string ps;
      for (size_t k = 0; k < lv.fn->paramSpecs.size(); ++k)
        ps += (k ? "," : "") + lv.fn->paramSpecs[k];
      if (!r.empty())
        base = "fn<" + r + "(" + ps + ")>";
    }
  } else if (first.type == GTokenType::LParen) {
      size_t close = 0;
      int d = 0;
      for (size_t k = 0; k < toks.size(); ++k) {
        if (toks[k].type == GTokenType::LParen)
          d++;
        else if (toks[k].type == GTokenType::RParen && --d == 0) {
          close = k;
          break;
        }
      }
      if (close > 1)
        base = inferExprType(
            std::vector<Token>(toks.begin() + 1,
                               toks.begin() + static_cast<long>(close)),
            localTypes);
      i = close + 1;
  } else if (first.type == GTokenType::LBracket) {
    size_t close = 0;
    int d = 0;
    size_t firstComma = 0, forAt = 0;
    for (size_t k = 0; k < toks.size(); ++k) {
      const auto ty = toks[k].type;
      if (ty == GTokenType::LParen || ty == GTokenType::LBracket ||
          ty == GTokenType::LBrace)
        d++;
      else if (ty == GTokenType::RParen || ty == GTokenType::RBracket ||
               ty == GTokenType::RBrace) {
        if (--d == 0) {
          close = k;
          break;
        }
      } else if (d == 1 && ty == GTokenType::KwFor && !forAt)
        forAt = k;
      else if (d == 1 && ty == GTokenType::Comma && !firstComma)
        firstComma = k;
    }
    if (close > 1 && !forAt) {
      const size_t endFirst = firstComma ? firstComma : close;
      const std::string et = inferExprType(
          std::vector<Token>(toks.begin() + 1,
                             toks.begin() + static_cast<long>(endFirst)),
          localTypes);
      if (!et.empty())
        base = "list<" + et + ">";
    }
    i = close ? close + 1 : toks.size();
  } else if (first.type == GTokenType::KwList ||
             first.type == GTokenType::KwMap) {
    size_t k = 1;
    if (k < toks.size() && toks[k].type == GTokenType::Lt) {
      int ad = 0;
      for (; k < toks.size(); ++k) {
        if (toks[k].type == GTokenType::Lt)
          ad++;
        else if (toks[k].type == GTokenType::Gt)
          ad--;
        else if (toks[k].type == GTokenType::Chain)
          ad -= 2;
        if (ad <= 0) {
          ++k;
          break;
        }
      }
      base = validateAndNormalizeTypeSpec(joinTokensForType(toks, 0, k), false);
    }
    if (k + 1 < toks.size() && toks[k].type == GTokenType::LParen &&
        toks[k + 1].type == GTokenType::RParen)
      k += 2;
    i = k;
  } else if (first.type == GTokenType::KwInt ||
             first.type == GTokenType::KwDouble ||
             first.type == GTokenType::KwFloat ||
             first.type == GTokenType::KwBool ||
             first.type == GTokenType::KwString ||
             first.type == GTokenType::KwSin ||
             first.type == GTokenType::KwCos ||
             first.type == GTokenType::KwTan ||
             first.type == GTokenType::KwRelu ||
             first.type == GTokenType::KwSigmoid) {
    i = 1;
    if (i < toks.size() && toks[i].type == GTokenType::LParen) {
      std::vector<Token> tmp = toks;
      splitArgs(tmp, i);
      ++i;
      if (first.type == GTokenType::KwInt)
        base = "int";
      else if (first.type == GTokenType::KwBool)
        base = "bool";
      else if (first.type == GTokenType::KwString)
        base = "string";
      else if (first.type == GTokenType::KwFloat)
        base = "double";
      else
        base = "double";
    }
  } else if (first.type == GTokenType::KwNew && toks.size() > 1 &&
             toks[1].type == GTokenType::Identifier) {
    i = 2;
    std::string cls = toks[1].value;
    if (i < toks.size() && toks[i].type == GTokenType::Lt &&
        generic_memory.count(cls)) {
      std::vector<Token> tmp = toks;
      cls = specFromTokens(tmp, i, cls);
    }
    if (i < toks.size() && toks[i].type == GTokenType::LParen) {
      std::vector<Token> tmp = toks;
      splitArgs(tmp, i);
      ++i;
    }
    base = cls;
  } else if (first.type == GTokenType::KwThis) {
    auto it = localTypes.find("this");
    base = it != localTypes.end() ? it->second : "";
    i = 1;
  } else if (first.type == GTokenType::Identifier) {
    const std::string &nm = first.value;
    i = 1;
    if (nm == "true" || nm == "false") {
      base = "bool";
    } else if (nm == "PI") {
      base = "double";
    } else if (localTypes.count(nm)) {
      base = localTypes.at(nm);
    } else if (i < toks.size() && toks[i].type == GTokenType::LParen) {
      std::vector<Token> tmp = toks;
      auto args = splitArgs(tmp, i);
      ++i;
      if (class_memory.count(nm))
        base = nm;
      else if (functions_memory.count(nm))
        base = functions_memory.at(nm).returnSpec;
      else if (nm == "len" || nm == "to_int")
        base = "int";
      else if (nm == "str" || nm == "typeof" || nm == "now" ||
               nm == "format_number" || nm == "format_date" ||
               nm == "regex_replace" || nm == "readfile" || nm == "fs_read" ||
               nm == "ghost_env_get" || nm == "os_getenv" ||
               nm == "ghost_system_exec" || nm == "os_exec" ||
               nm == "http_get" || nm == "http_client_get" ||
               nm == "http_post" || nm == "http_client_post")
        base = "string";
      else if (nm == "sqrt" || nm == "cbrt" || nm == "nth_root" || nm == "raiz_n" ||
               nm == "root" || nm == "raiz_quadrada" || nm == "raiz_cubica" ||
               nm == "pow" || nm == "potencia" || nm == "hypot" || nm == "hipotenusa" ||
               nm == "floor" || nm == "ceil" || nm == "round" || nm == "trunc" ||
               nm == "exp" || nm == "exp2" || nm == "expm1" || nm == "log" ||
               nm == "log10" || nm == "log2" || nm == "log1p" || nm == "fmod" ||
               nm == "sin" || nm == "cos" || nm == "tan" || nm == "asin" ||
               nm == "acos" || nm == "atan" || nm == "atan2" || nm == "sinh" ||
               nm == "cosh" || nm == "tanh" || nm == "asinh" || nm == "acosh" ||
               nm == "atanh" || nm == "erf" || nm == "erfc" || nm == "gamma" ||
               nm == "tgamma" || nm == "lgamma" || nm == "deg2rad" || nm == "rad2deg" ||
               nm == "parse_number")
        base = "double";
      else if (nm == "gcd" || nm == "mdc" || nm == "lcm" || nm == "mmc" ||
               nm == "factorial" || nm == "fatorial")
        base = "int";
      else if (nm == "abs" || nm == "min" || nm == "max") {
        base = !args.empty() ? inferExprType(args[0], localTypes) : "double";
        if (args.size() > 1 && inferExprType(args[1], localTypes) == "double")
          base = "double";
      } else if (nm == "range")
        base = "list<int>";
      else if (nm == "assert" || nm == "regex_match" || nm == "regex_search" ||
               nm == "file_exists")
        base = "bool";
      else if (nm == "ok")
        base = "ok<" +
               (args.empty() ? std::string("bool")
                             : inferExprType(args[0], localTypes)) +
               ">";
      else if (nm == "err")
        base = "result";
    } else if (i < toks.size() && toks[i].type == GTokenType::Lt &&
               generic_memory.count(nm)) {
      std::vector<Token> tmp = toks;
      base = specFromTokens(tmp, i, nm);
      if (i < toks.size() && toks[i].type == GTokenType::LParen) {
        splitArgs(tmp, i);
        ++i;
      }
    } else if (functions_memory.count(nm)) {
      base = makeNamedFnValue(nm, functions_memory.at(nm)).str;
    }
  } else {
    return "";
  }

  while (i < toks.size() && !base.empty()) {
    if (toks[i].type == GTokenType::LParen) {
      std::vector<Token> tmp = toks;
      splitArgs(tmp, i);
      ++i;
      std::string r;
      std::vector<std::string> ps;
      if (isFnSpec(base) && parseFnSignature(base, r, ps))
        base = r;
      else
        return "";
    } else if (toks[i].type == GTokenType::LBracket) {
      std::vector<Token> tmp = toks;
      skipBalanced(tmp, i);
      if (base.rfind("list<", 0) == 0 && base.back() == '>')
        base = base.substr(5, base.size() - 6);
      else if (base.rfind("map<", 0) == 0 && base.back() == '>') {
        int d = 0;
        std::string valTy;
        for (size_t k = 4; k + 1 < base.size(); ++k) {
          if (base[k] == '<')
            d++;
          else if (base[k] == '>')
            d--;
          else if (base[k] == ',' && d == 0) {
            valTy = base.substr(k + 1, base.size() - k - 2);
            break;
          }
        }
        base = valTy;
      } else if (base == "string")
        base = "string";
      else
        return "";
    } else if (toks[i].type == GTokenType::Dot && i + 1 < toks.size()) {
      const std::string m = toks[i + 1].value;
      i += 2;
      const bool call = i < toks.size() && toks[i].type == GTokenType::LParen;
      if (call) {
        std::vector<Token> tmp = toks;
        splitArgs(tmp, i);
        ++i;
      }
      if (base == "string") {
        if (m == "length" || m == "size" || m == "index_of")
          base = "int";
        else if (m == "contains" || m == "starts_with" || m == "ends_with")
          base = "bool";
        else if (m == "split")
          base = "list<string>";
        else
          base = "string";
      } else if (base.rfind("list<", 0) == 0 && base.back() == '>') {
        const std::string el = base.substr(5, base.size() - 6);
        if (m == "length" || m == "size" || m == "index_of")
          base = "int";
        else if (m == "empty" || m == "contains")
          base = "bool";
        else if (m == "join")
          base = "string";
        else if (m == "first" || m == "last" || m == "pop" ||
                 m == "remove_at" || m == "min" || m == "max" || m == "sum")
          base = el;
        else if (m == "sorted" || m == "slice" || m == "sort" || m == "reverse")
          base = base;
        else
          base = "void";
      } else if (base.rfind("set<", 0) == 0 && base.back() == '>') {
        const std::string el = base.substr(4, base.size() - 5);
        if (m == "length" || m == "size")
          base = "int";
        else if (m == "min" || m == "max")
          base = el;
        else if (m == "to_list")
          base = "list<" + el + ">";
        else if (m == "union" || m == "intersection" || m == "difference")
          base = base;
        else
          base = "bool";
      } else if (isResultSpec(base)) {
        const std::string el = base.substr(7, base.size() - 8);
        if (m == "is_ok" || m == "is_err")
          base = "bool";
        else if (m == "value" || m == "value_or")
          base = el;
        else if (m == "error")
          base = "string";
        else
          return "";
      } else if (class_memory.count(base)) {
        const GhostFunction *mf = call ? findMethod(base, m, nullptr) : nullptr;
        if (mf)
          base = mf->returnSpec;
        else if (const GhostFieldDef *fd = findField(base, m)) {
          std::string fty;
          if (!fd->cls.empty())
            fty = fd->cls;
          else if (fd->type == GTokenType::KwInt)
            fty = "int";
          else if (fd->type == GTokenType::KwDouble ||
                   fd->type == GTokenType::KwFloat)
            fty = "double";
          else if (fd->type == GTokenType::KwBool)
            fty = "bool";
          else if (fd->type == GTokenType::KwString)
            fty = "string";
          if (call && isFnSpec(fty)) {
            std::string r;
            std::vector<std::string> ps;
            base = parseFnSignature(fty, r, ps) ? r : "";
          } else
            base = fty;
        } else
          return "";
      } else if (interface_memory.count(base)) {
        std::string foundRet;
        for (const auto &p : ifaceMethodList(base))
          if (p.first == m)
            foundRet = p.second->returnSpec;
        base = foundRet;
      } else {
        return "";
      }
    } else if (toks[i].type == GTokenType::Question &&
               ghost_is_try_question(std::vector<Token>(
                   toks.begin() + static_cast<long>(i + 1), toks.end()))) {
      ++i;
      if (isResultSpec(base))
        base = base.substr(7, base.size() - 8);
      else
        return "";
    } else {
      return "";
    }
  }
  return base;
}

std::string GhostInterpreter::inferLambdaRetSpec(const GhostClosure &c) {
  std::map<std::string, std::string> localTypes;
  auto valSpec = [&](const GhostValue &v,
                     const std::string &cls) -> std::string {
    if (!cls.empty())
      return cls;
    switch (v.kind) {
    case K::Num:
      return v.isInt ? "int" : "double";
    case K::Bool:
      return "bool";
    case K::Str:
      return "string";
    case K::Object:
    case K::Iface:
    case K::Result:
    case K::Fn:
      return v.str;
    case K::Set:
      return v.str.empty() ? "" : "set<" + v.str + ">";
    default:
      return "";
    }
  };
  if (c.hasThis && class_memory.count(c.capturedThis.str)) {
    localTypes["this"] = c.capturedThis.str;
    std::string cur = c.capturedThis.str;
    for (int g = 0; !cur.empty() && g < 256 && class_memory.count(cur); ++g) {
      for (const auto &f : class_memory.at(cur).fields) {
        if (localTypes.count(f.name))
          continue;
        if (!f.cls.empty())
          localTypes[f.name] = f.cls;
        else if (f.type == GTokenType::KwInt)
          localTypes[f.name] = "int";
        else if (f.type == GTokenType::KwDouble ||
                 f.type == GTokenType::KwFloat)
          localTypes[f.name] = "double";
        else if (f.type == GTokenType::KwBool)
          localTypes[f.name] = "bool";
        else if (f.type == GTokenType::KwString)
          localTypes[f.name] = "string";
      }
      cur = class_memory.at(cur).base;
    }
  }
  for (const auto &kv : c.env) {
    const std::string ts = valSpec(kv.second.val, kv.second.varCls);
    if (!ts.empty())
      localTypes[kv.first] = ts;
  }
  for (size_t k = 0; k < c.params.size(); ++k) {
    localTypes[c.params[k].second] = c.paramSpecs[k];
  }

  auto checkNoBareTry = [&](const std::vector<Token> &ts) {
    int d = 0;
    for (size_t k = 0; k < ts.size(); ++k) {
      if (d == 0 && (ts[k].type == GTokenType::LParen || ts[k].type == GTokenType::BitOr || ts[k].type == GTokenType::LogicalOr) && isLambdaAt(ts, k))
        break;
      if (ts[k].type == GTokenType::LParen ||
          ts[k].type == GTokenType::LBracket ||
          ts[k].type == GTokenType::LBrace)
        d++;
      else if (ts[k].type == GTokenType::RParen ||
               ts[k].type == GTokenType::RBracket ||
               ts[k].type == GTokenType::RBrace)
        d--;
      if (ts[k].type == GTokenType::Question &&
          ghost_is_try_question(std::vector<Token>(
              ts.begin() + static_cast<long>(k + 1), ts.end()))) {
        throw std::runtime_error("o operador ? dentro de lambda exige retorno "
                                 "explicito '-> result<T>'");
      }
    }
  };

  if (c.isExprBody) {
    checkNoBareTry(c.exprTokens);
    return inferExprType(c.exprTokens, localTypes);
  }

  // Corpo em bloco { ... }: inspeciona comandos fora de lambdas internas
  Lexer bl(c.body);
  std::vector<Token> all;
  for (Token tk = bl.getNextToken(); tk.type != GTokenType::EndOfFile;
       tk = bl.getNextToken())
    all.push_back(tk);

  std::vector<std::string> rets;
  bool hasVoidReturn = false;
  for (size_t k = 0; k < all.size(); ++k) {
    // Pula lambdas internas inteiras (os returns delas pertencem à lambda
    // interna!)
    if ((all[k].type == GTokenType::LParen || all[k].type == GTokenType::BitOr || all[k].type == GTokenType::LogicalOr) && isLambdaAt(all, k)) {
      while (k < all.size() && all[k].type != GTokenType::Arrow)
        ++k;
      if (k < all.size() && all[k].type == GTokenType::Arrow) {
        ++k;
        if (k < all.size() && all[k].type == GTokenType::LBrace)
          skipBalanced(all, k);
        else
          skipLambdaExprBody(all, k);
      }
      if (k > 0)
        --k;
      continue;
    }
    // Registra declarações simples: int a = ...; double b = ...; var v = expr;
    if ((all[k].type == GTokenType::KwInt ||
         all[k].type == GTokenType::KwDouble ||
         all[k].type == GTokenType::KwFloat ||
         all[k].type == GTokenType::KwBool ||
         all[k].type == GTokenType::KwString) &&
        k + 1 < all.size() && all[k + 1].type == GTokenType::Identifier) {
      localTypes[all[k + 1].value] =
          all[k].value == "float" ? "double" : all[k].value;
    } else if (all[k].type == GTokenType::KwVar && k + 2 < all.size() &&
               all[k + 1].type == GTokenType::Identifier &&
               all[k + 2].type == GTokenType::Assign) {
      const std::string vn = all[k + 1].value;
      size_t e = k + 3;
      int d = 0;
      for (; e < all.size(); ++e) {
        if (all[e].type == GTokenType::LParen ||
            all[e].type == GTokenType::LBracket ||
            all[e].type == GTokenType::LBrace)
          d++;
        else if (all[e].type == GTokenType::RParen ||
                 all[e].type == GTokenType::RBracket ||
                 all[e].type == GTokenType::RBrace)
          d--;
        else if (d == 0 && all[e].type == GTokenType::Semicolon)
          break;
      }
      const std::string vt = inferExprType(
          std::vector<Token>(all.begin() + static_cast<long>(k + 3),
                             all.begin() + static_cast<long>(e)),
          localTypes);
      if (!vt.empty())
        localTypes[vn] = vt;
    }
    if (all[k].type == GTokenType::Question &&
        ghost_is_try_question(std::vector<Token>(
            all.begin() + static_cast<long>(k + 1), all.end()))) {
      throw std::runtime_error("o operador ? dentro de lambda exige retorno "
                               "explicito '-> result<T>'");
    }
    if (all[k].type == GTokenType::KwReturn) {
      size_t e = k + 1;
      int d = 0;
      for (; e < all.size(); ++e) {
        if (all[e].type == GTokenType::LParen ||
            all[e].type == GTokenType::LBracket ||
            all[e].type == GTokenType::LBrace)
          d++;
        else if (all[e].type == GTokenType::RParen ||
                 all[e].type == GTokenType::RBracket ||
                 all[e].type == GTokenType::RBrace)
          d--;
        else if (d == 0 && all[e].type == GTokenType::Semicolon)
          break;
      }
      if (e == k + 1) {
        hasVoidReturn = true;
      } else {
        std::vector<Token> rExpr(all.begin() + static_cast<long>(k + 1),
                                 all.begin() + static_cast<long>(e));
        checkNoBareTry(rExpr);
        rets.push_back(inferExprType(rExpr, localTypes));
      }
      k = e;
    }
  }
  if (rets.empty())
    return "void";
  if (hasVoidReturn)
    throw std::runtime_error(
        "lambda mistura 'return;' vazio e 'return valor;': informe '-> tipo' e "
        "retorne valor em todos os caminhos");
  std::string chosen;
  for (const auto &r : rets) {
    if (r.empty())
      continue;
    if (chosen.empty()) {
      chosen = r;
      continue;
    }
    if (r != chosen) {
      throw std::runtime_error(
          "lambda com retornos de tipos diferentes (" + chosen + " e " + r +
          "): informe o tipo com '-> tipo' antes de '=>' (ex.: (...) -> double "
          "=> { ... } ou (...) -> result<T> => { ... })");
    }
  }
  return chosen;
}
