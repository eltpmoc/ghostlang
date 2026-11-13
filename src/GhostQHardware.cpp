#include "GhostQHardware.hpp"
#include "GhostQuantum.hpp"

#include <cctype>
#include <cmath>
#include <stdexcept>

// qhw_submit: até 12/11/2026 inventava as contagens (se o texto tinha "cx", metade
// "00" e metade "11"). Agora o circuito é interpretado e executado no simulador de
// vetor de estado (GhostQReg). Nada é enviado a hardware real.

namespace {

std::string trim(const std::string &s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

std::string lower(std::string s) {
  for (auto &ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return s;
}

[[noreturn]] void fail(const std::string &msg) {
  throw std::runtime_error("qhw_submit: " + msg);
}

// Expressão de ângulo: números, pi, + - * / e parênteses.
struct Angle {
  const std::string &s;
  size_t i = 0;
  explicit Angle(const std::string &t) : s(t) {}
  void ws() { while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; }
  double expr() {
    double v = term();
    for (;;) {
      ws();
      if (i < s.size() && s[i] == '+') { ++i; v += term(); }
      else if (i < s.size() && s[i] == '-') { ++i; v -= term(); }
      else return v;
    }
  }
  double term() {
    double v = unary();
    for (;;) {
      ws();
      if (i < s.size() && s[i] == '*') { ++i; v *= unary(); }
      else if (i < s.size() && s[i] == '/') {
        ++i;
        double d = unary();
        if (d == 0.0) fail("divisao por zero no angulo '" + s + "'");
        v /= d;
      } else return v;
    }
  }
  double unary() {
    ws();
    if (i < s.size() && s[i] == '-') { ++i; return -unary(); }
    if (i < s.size() && s[i] == '+') { ++i; return unary(); }
    return atom();
  }
  double atom() {
    ws();
    if (i < s.size() && s[i] == '(') {
      ++i;
      double v = expr();
      ws();
      if (i >= s.size() || s[i] != ')') fail("parentese faltando no angulo '" + s + "'");
      ++i;
      return v;
    }
    if (s.compare(i, 2, "pi") == 0) { i += 2; return 3.14159265358979323846; }
    size_t start = i;
    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' ||
                            s[i] == 'e' || s[i] == 'E' ||
                            ((s[i] == '-' || s[i] == '+') && i > start && (s[i - 1] == 'e' || s[i - 1] == 'E'))))
      ++i;
    if (start == i) fail("angulo invalido '" + s + "'");
    return std::stod(s.substr(start, i - start));
  }
  double parse() {
    double v = expr();
    ws();
    if (i != s.size()) fail("angulo invalido '" + s + "'");
    return v;
  }
};

struct Register { int offset; int size; };

} // namespace

namespace GhostQuantum {

void ghost_qhw_bootstrap() {}

std::string GhostQHardware::submit_qasm(const std::string &qasm_code,
                                        const std::string &backend_override,
                                        int shots) {
  if (shots < 1) fail("shots deve ser >= 1");
  std::string b = backend_override.empty() ? m_backend : backend_override;
  if (b.empty()) b = "local";

  // 1) Comandos separados por ';' (comentários // removidos).
  std::string clean;
  {
    size_t i = 0;
    while (i < qasm_code.size()) {
      if (qasm_code.compare(i, 2, "//") == 0) {
        while (i < qasm_code.size() && qasm_code[i] != '\n') ++i;
      } else {
        clean += qasm_code[i++];
      }
    }
  }
  std::vector<std::string> stmts;
  {
    size_t start = 0;
    for (size_t i = 0; i <= clean.size(); ++i) {
      if (i == clean.size() || clean[i] == ';') {
        std::string st = trim(clean.substr(start, i - start));
        if (!st.empty()) stmts.push_back(st);
        start = i + 1;
      }
    }
  }

  // 2) Registradores quânticos: qreg q[n]; (2.0) ou qubit[n] q; (3.0).
  std::map<std::string, Register> regs;
  int total = 0;
  auto declare = [&](const std::string &name, int n) {
    if (n < 1) fail("registrador '" + name + "' sem qubits");
    regs[name] = {total, n};
    total += n;
  };
  for (const auto &st : stmts) {
    std::string l = lower(st);
    if (l.rfind("qreg ", 0) == 0) {
      size_t a = st.find('['), c = st.find(']');
      if (a == std::string::npos || c == std::string::npos) fail("qreg invalido: " + st);
      declare(trim(st.substr(5, a - 5)), std::stoi(st.substr(a + 1, c - a - 1)));
    } else if (l.rfind("qubit[", 0) == 0) {
      size_t c = st.find(']');
      declare(trim(st.substr(c + 1)), std::stoi(st.substr(6, c - 6)));
    }
  }
  if (total == 0) fail("nenhum registrador quantico (qreg q[n]; ou qubit[n] q;)");
  if (total > 24) fail("circuito com " + std::to_string(total) + " qubits; o simulador local aceita ate 24");

  auto qubit = [&](const std::string &raw) -> int {
    std::string t = trim(raw);
    size_t a = t.find('[');
    std::string name = trim(a == std::string::npos ? t : t.substr(0, a));
    auto it = regs.find(name);
    if (it == regs.end()) fail("registrador desconhecido '" + name + "'");
    if (a == std::string::npos) {
      if (it->second.size != 1) fail("use " + name + "[i] para escolher o qubit");
      return it->second.offset;
    }
    size_t c = t.find(']', a);
    int k = std::stoi(t.substr(a + 1, c - a - 1));
    if (k < 0 || k >= it->second.size) fail("qubit fora do registrador: " + t);
    return it->second.offset + k;
  };

  // 3) Executa as portas.
  GhostQReg reg(total);
  reg.set_recording(false);
  const int job_number = ++m_job_counter;
  reg.seed(static_cast<unsigned long long>(job_number));

  for (const auto &st : stmts) {
    std::string l = lower(st);
    if (l.rfind("openqasm", 0) == 0 || l.rfind("include", 0) == 0 ||
        l.rfind("qreg ", 0) == 0 || l.rfind("qubit[", 0) == 0 ||
        l.rfind("creg ", 0) == 0 || l.rfind("bit[", 0) == 0 ||
        l.rfind("barrier", 0) == 0 || l.rfind("measure", 0) == 0 ||
        l.find("= measure") != std::string::npos || l.find("=measure") != std::string::npos)
      continue; // a medição é a amostragem final

    // nome[(ângulos)] argumentos
    size_t p = 0;
    while (p < st.size() && (std::isalnum(static_cast<unsigned char>(st[p])) || st[p] == '_')) ++p;
    std::string gate = lower(st.substr(0, p));
    std::vector<double> ang;
    size_t rest = p;
    if (rest < st.size() && st[rest] == '(') {
      int depth = 0;
      size_t close = rest;
      for (; close < st.size(); ++close) {
        if (st[close] == '(') ++depth;
        else if (st[close] == ')' && --depth == 0) break;
      }
      if (close >= st.size()) fail("parentese faltando: " + st);
      std::string inner = st.substr(rest + 1, close - rest - 1);
      size_t s0 = 0;
      for (size_t i = 0; i <= inner.size(); ++i)
        if (i == inner.size() || inner[i] == ',') {
          ang.push_back(Angle(trim(inner.substr(s0, i - s0))).parse());
          s0 = i + 1;
        }
      rest = close + 1;
    }
    std::vector<int> q;
    {
      std::string args = st.substr(rest);
      size_t s0 = 0;
      for (size_t i = 0; i <= args.size(); ++i)
        if (i == args.size() || args[i] == ',') {
          std::string a = trim(args.substr(s0, i - s0));
          if (!a.empty()) q.push_back(qubit(a));
          s0 = i + 1;
        }
    }
    auto need = [&](size_t nq, size_t na) {
      if (q.size() != nq || ang.size() != na)
        fail("porta '" + gate + "' espera " + std::to_string(nq) + " qubit(s) e " +
             std::to_string(na) + " angulo(s): " + st);
    };

    if (gate == "h") { need(1, 0); reg.h(q[0]); }
    else if (gate == "x") { need(1, 0); reg.x(q[0]); }
    else if (gate == "y") { need(1, 0); reg.y(q[0]); }
    else if (gate == "z") { need(1, 0); reg.z(q[0]); }
    else if (gate == "s") { need(1, 0); reg.s(q[0]); }
    else if (gate == "sdg") { need(1, 0); reg.sdg(q[0]); }
    else if (gate == "t") { need(1, 0); reg.t(q[0]); }
    else if (gate == "tdg") { need(1, 0); reg.tdg(q[0]); }
    else if (gate == "sx") { need(1, 0); reg.sx(q[0]); }
    else if (gate == "id") { need(1, 0); }
    else if (gate == "rx") { need(1, 1); reg.rx(q[0], ang[0]); }
    else if (gate == "ry") { need(1, 1); reg.ry(q[0], ang[0]); }
    else if (gate == "rz") { need(1, 1); reg.rz(q[0], ang[0]); }
    else if (gate == "p" || gate == "u1") { need(1, 1); reg.p(q[0], ang[0]); }
    else if (gate == "u" || gate == "u3") { need(1, 3); reg.u(q[0], ang[0], ang[1], ang[2]); }
    else if (gate == "cx" || gate == "cnot") { need(2, 0); reg.cx(q[0], q[1]); }
    else if (gate == "cy") { need(2, 0); reg.cy(q[0], q[1]); }
    else if (gate == "cz") { need(2, 0); reg.cz(q[0], q[1]); }
    else if (gate == "ch") { need(2, 0); reg.ch(q[0], q[1]); }
    else if (gate == "swap") { need(2, 0); reg.swap(q[0], q[1]); }
    else if (gate == "cp" || gate == "cu1") { need(2, 1); reg.cp(q[0], q[1], ang[0]); }
    else if (gate == "crx") { need(2, 1); reg.crx(q[0], q[1], ang[0]); }
    else if (gate == "cry") { need(2, 1); reg.cry(q[0], q[1], ang[0]); }
    else if (gate == "crz") { need(2, 1); reg.crz(q[0], q[1], ang[0]); }
    else if (gate == "rzz") { need(2, 1); reg.rzz(q[0], q[1], ang[0]); }
    else if (gate == "ccx") { need(3, 0); reg.ccx(q[0], q[1], q[2]); }
    else if (gate == "cswap") { need(3, 0); reg.cswap(q[0], q[1], q[2]); }
    else fail("instrucao QASM nao suportada pelo simulador local: " + st);
  }

  QHardwareJob job;
  job.job_id = "qjob_" + std::to_string(job_number);
  job.backend_name = b;
  job.qasm_circuit = qasm_code;
  job.shots = shots;
  job.counts = reg.counts(shots);
  job.status = "COMPLETED";
  m_jobs[job.job_id] = job;
  return job.job_id;
}

} // namespace GhostQuantum
