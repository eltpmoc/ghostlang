// Adaptador DAP da GhostLang: DAP (JSON com Content-Length) <-> gdb/MI.
#include "../include/GhostDAP.hpp"

#include <atomic>
#include <cctype>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace fs = std::filesystem;

namespace {

// ---------------------------------------------------------------- JSON mínimo
struct J {
    enum T { Null, Bool, Num, Str, Arr, Obj } t = Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<J> a;
    std::vector<std::pair<std::string, J>> o;

    static J str(const std::string& v) { J j; j.t = Str; j.s = v; return j; }
    static J num(double v) { J j; j.t = Num; j.n = v; return j; }
    static J boolean(bool v) { J j; j.t = Bool; j.b = v; return j; }
    static J arr() { J j; j.t = Arr; return j; }
    static J obj() { J j; j.t = Obj; return j; }
    J& set(const std::string& k, J v) {
        t = Obj;
        for (auto& p : o) if (p.first == k) { p.second = std::move(v); return *this; }
        o.emplace_back(k, std::move(v));
        return *this;
    }
    const J* get(const std::string& k) const {
        for (auto& p : o) if (p.first == k) return &p.second;
        return nullptr;
    }
    std::string text(const std::string& k, const std::string& padrao = "") const {
        const J* v = get(k);
        return v && v->t == Str ? v->s : padrao;
    }
    double number(const std::string& k, double padrao = 0) const {
        const J* v = get(k);
        if (!v) return padrao;
        if (v->t == Num) return v->n;
        if (v->t == Str) { try { return std::stod(v->s); } catch (...) {} }
        return padrao;
    }
    bool flag(const std::string& k) const {
        const J* v = get(k);
        return v && v->t == Bool && v->b;
    }
};

std::string escape(const std::string& s) {
    std::string r;
    for (unsigned char c : s) {
        switch (c) {
            case '"': r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n"; break;
            case '\r': r += "\\r"; break;
            case '\t': r += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); r += b; }
                else r += static_cast<char>(c);
        }
    }
    return r;
}

std::string dump(const J& j) {
    switch (j.t) {
        case J::Null: return "null";
        case J::Bool: return j.b ? "true" : "false";
        case J::Num: {
            std::ostringstream os;
            if (j.n == static_cast<long long>(j.n)) os << static_cast<long long>(j.n); else os << j.n;
            return os.str();
        }
        case J::Str: return "\"" + escape(j.s) + "\"";
        case J::Arr: {
            std::string r = "[";
            for (size_t i = 0; i < j.a.size(); ++i) r += (i ? "," : "") + dump(j.a[i]);
            return r + "]";
        }
        case J::Obj: {
            std::string r = "{";
            for (size_t i = 0; i < j.o.size(); ++i) r += (i ? "," : "") + std::string("\"") + escape(j.o[i].first) + "\":" + dump(j.o[i].second);
            return r + "}";
        }
    }
    return "null";
}

struct JsonParser {
    const std::string& s;
    size_t i = 0;
    explicit JsonParser(const std::string& t) : s(t) {}
    void ws() { while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i; }
    J value() {
        ws();
        if (i >= s.size()) return J();
        const char c = s[i];
        if (c == '{') {
            J o = J::obj(); ++i; ws();
            if (i < s.size() && s[i] == '}') { ++i; return o; }
            while (i < s.size()) {
                ws(); std::string k = string(); ws();
                if (i < s.size() && s[i] == ':') ++i;
                o.o.emplace_back(k, value()); ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == '}') { ++i; break; }
                break;
            }
            return o;
        }
        if (c == '[') {
            J a = J::arr(); ++i; ws();
            if (i < s.size() && s[i] == ']') { ++i; return a; }
            while (i < s.size()) {
                a.a.push_back(value()); ws();
                if (i < s.size() && s[i] == ',') { ++i; continue; }
                if (i < s.size() && s[i] == ']') { ++i; break; }
                break;
            }
            return a;
        }
        if (c == '"') return J::str(string());
        if (s.compare(i, 4, "true") == 0) { i += 4; return J::boolean(true); }
        if (s.compare(i, 5, "false") == 0) { i += 5; return J::boolean(false); }
        if (s.compare(i, 4, "null") == 0) { i += 4; return J(); }
        size_t st = i;
        while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || std::string("+-.eE").find(s[i]) != std::string::npos)) ++i;
        try { return J::num(std::stod(s.substr(st, i - st))); } catch (...) { return J(); }
    }
    std::string string() {
        std::string r;
        if (i >= s.size() || s[i] != '"') return r;
        ++i;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                const char e = s[++i];
                switch (e) {
                    case 'n': r += '\n'; break;
                    case 't': r += '\t'; break;
                    case 'r': r += '\r'; break;
                    case 'b': r += '\b'; break;
                    case 'f': r += '\f'; break;
                    case 'u': {
                        unsigned v = 0;
                        for (int k = 0; k < 4 && i + 1 < s.size(); ++k) v = v * 16 + static_cast<unsigned>(std::stoi(std::string(1, s[++i]), nullptr, 16));
                        if (v < 0x80) r += static_cast<char>(v);
                        else if (v < 0x800) { r += static_cast<char>(0xC0 | (v >> 6)); r += static_cast<char>(0x80 | (v & 0x3F)); }
                        else { r += static_cast<char>(0xE0 | (v >> 12)); r += static_cast<char>(0x80 | ((v >> 6) & 0x3F)); r += static_cast<char>(0x80 | (v & 0x3F)); }
                        break;
                    }
                    default: r += e;
                }
                ++i;
            } else {
                r += s[i++];
            }
        }
        ++i;
        return r;
    }
};

// ---------------------------------------------------------------- MI do gdb
// Valores MI: c-string "..." , tupla {a=..,b=..} , lista [..] (de valores ou de a=..)
struct MiParser {
    const std::string& s;
    size_t i = 0;
    explicit MiParser(const std::string& t, size_t pos) : s(t), i(pos) {}
    std::string cstring() {
        std::string r;
        if (i >= s.size() || s[i] != '"') return r;
        ++i;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                const char e = s[++i];
                r += e == 'n' ? '\n' : e == 't' ? '\t' : e == 'r' ? '\r' : e;
                ++i;
            } else {
                r += s[i++];
            }
        }
        ++i;
        return r;
    }
    std::string name() {
        size_t st = i;
        while (i < s.size() && s[i] != '=' && s[i] != ',' && s[i] != '}' && s[i] != ']') ++i;
        return s.substr(st, i - st);
    }
    J value() {
        if (i >= s.size()) return J();
        if (s[i] == '"') return J::str(cstring());
        if (s[i] == '{') {
            J o = J::obj(); ++i;
            while (i < s.size() && s[i] != '}') {
                std::string k = name();
                if (i < s.size() && s[i] == '=') { ++i; o.o.emplace_back(k, value()); }
                if (i < s.size() && s[i] == ',') ++i;
            }
            ++i;
            return o;
        }
        if (s[i] == '[') {
            J a = J::arr(); ++i;
            while (i < s.size() && s[i] != ']') {
                if (s[i] == '"' || s[i] == '{' || s[i] == '[') a.a.push_back(value());
                else { std::string k = name(); if (i < s.size() && s[i] == '=') { ++i; J v = value(); J w = J::obj(); w.o.emplace_back(k, v); a.a.push_back(w); } }
                if (i < s.size() && s[i] == ',') ++i;
            }
            ++i;
            return a;
        }
        return J();
    }
    // resto de um registro: ,a=..,b=..
    J results() {
        J o = J::obj();
        while (i < s.size()) {
            if (s[i] == ',') ++i;
            std::string k = name();
            if (k.empty()) break;
            if (i < s.size() && s[i] == '=') { ++i; o.o.emplace_back(k, value()); } else break;
        }
        return o;
    }
};

struct MiRecord {
    long token = -1;
    char kind = 0;          // '^' resultado, '*' exec, '=' notify, '~' '@' '&' streams
    std::string klass;      // done, running, error, stopped...
    J data;
    std::string text;       // streams
};

bool parseMi(const std::string& line, MiRecord& r) {
    size_t i = 0;
    while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) ++i;
    if (i > 0) r.token = std::stol(line.substr(0, i));
    if (i >= line.size()) return false;
    const char k = line[i];
    if (std::string("^*+=~@&").find(k) == std::string::npos) return false;
    r.kind = k;
    ++i;
    if (k == '~' || k == '@' || k == '&') {
        MiParser p(line, i);
        r.text = p.cstring();
        return true;
    }
    size_t st = i;
    while (i < line.size() && line[i] != ',') ++i;
    r.klass = line.substr(st, i - st);
    MiParser p(line, i);
    r.data = p.results();
    return true;
}

// ---------------------------------------------------------------- processo filho
struct Child {
#ifdef _WIN32
    HANDLE proc = nullptr, inW = nullptr, outR = nullptr;
#else
    pid_t pid = -1;
    int inW = -1, outR = -1;
#endif
    bool start(const std::vector<std::string>& args) {
#ifdef _WIN32
        SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
        HANDLE inR, outW;
        if (!CreatePipe(&inR, &inW, &sa, 0) || !CreatePipe(&outR, &outW, &sa, 0)) return false;
        SetHandleInformation(inW, HANDLE_FLAG_INHERIT, 0);
        SetHandleInformation(outR, HANDLE_FLAG_INHERIT, 0);
        std::string cmd;
        for (const auto& a : args) {
            if (!cmd.empty()) cmd += ' ';
            cmd += '"';
            for (char c : a) { if (c == '"') cmd += '\\'; cmd += c; }
            cmd += '"';
        }
        STARTUPINFOA si{};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = inR;
        si.hStdOutput = outW;
        si.hStdError = outW;
        PROCESS_INFORMATION pi{};
        std::vector<char> buf(cmd.begin(), cmd.end());
        buf.push_back('\0');
        const BOOL ok = CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
        CloseHandle(inR);
        CloseHandle(outW);
        if (!ok) return false;
        CloseHandle(pi.hThread);
        proc = pi.hProcess;
        return true;
#else
        int a[2], b[2];
        if (pipe(a) || pipe(b)) return false;
        pid = fork();
        if (pid < 0) return false;
        if (pid == 0) {
            dup2(a[0], 0); dup2(b[1], 1); dup2(b[1], 2);
            close(a[1]); close(b[0]);
            std::vector<char*> av;
            for (const auto& s : args) av.push_back(const_cast<char*>(s.c_str()));
            av.push_back(nullptr);
            execvp(av[0], av.data());
            _exit(127);
        }
        close(a[0]); close(b[1]);
        inW = a[1]; outR = b[0];
        return true;
#endif
    }
    void write(const std::string& s) {
#ifdef _WIN32
        DWORD n;
        WriteFile(inW, s.data(), static_cast<DWORD>(s.size()), &n, nullptr);
#else
        (void)!::write(inW, s.data(), s.size());
#endif
    }
    // lê uma linha (sem \r\n); false no fim
    bool readLine(std::string& line) {
        line.clear();
        char c;
        for (;;) {
#ifdef _WIN32
            DWORD n = 0;
            if (!ReadFile(outR, &c, 1, &n, nullptr) || n == 0) return !line.empty();
#else
            if (::read(outR, &c, 1) != 1) return !line.empty();
#endif
            if (c == '\n') { if (!line.empty() && line.back() == '\r') line.pop_back(); return true; }
            line += c;
        }
    }
    int wait() {
#ifdef _WIN32
        WaitForSingleObject(proc, INFINITE);
        DWORD code = 0;
        GetExitCodeProcess(proc, &code);
        return static_cast<int>(code);
#else
        int st = 0;
        waitpid(pid, &st, 0);
        return WIFEXITED(st) ? WEXITSTATUS(st) : 1;
#endif
    }
    void kill() {
#ifdef _WIN32
        if (proc) TerminateProcess(proc, 1);
#else
        if (pid > 0) ::kill(pid, SIGKILL);
#endif
    }
};

// ---------------------------------------------------------------- adaptador
class Adapter {
public:
    Adapter(std::string ghost, std::string gdb) : ghostExe(std::move(ghost)), gdbExe(std::move(gdb)) {}

    int run() {
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
#endif
        std::string msg;
        while (!terminado && readMessage(msg)) {
            J req = JsonParser(msg).value();
            try {
                handle(req);
            } catch (const std::exception& e) {
                respond(req, false, J::obj(), e.what());
            }
        }
        if (gdbStarted) { mi("-gdb-exit", 2000); gdb.kill(); }
        if (leitor.joinable()) leitor.detach();
        return 0;
    }

private:
    std::string ghostExe, gdbExe;
    Child gdb;
    bool gdbStarted = false;
    std::atomic<bool> terminado{false};
    std::mutex outMx, miMx;
    std::condition_variable miCv;
    std::map<long, MiRecord> respostas;
    long proxToken = 1;
    int seq = 1;
    std::thread leitor;
    std::map<std::string, std::vector<std::string>> bpPorArquivo; // arquivo -> números de breakpoint do gdb
    bool stopOnEntry = false;
    std::string programa;
    std::vector<std::string> argsPrograma;

    // --- DAP E/S
    bool readMessage(std::string& out) {
        std::string linha;
        size_t len = 0;
        for (;;) {
            linha.clear();
            int c;
            while ((c = std::getchar()) != EOF && c != '\n') linha += static_cast<char>(c);
            if (c == EOF) return false;
            if (!linha.empty() && linha.back() == '\r') linha.pop_back();
            if (linha.empty()) break;
            if (linha.rfind("Content-Length:", 0) == 0) len = static_cast<size_t>(std::stoul(linha.substr(15)));
        }
        out.assign(len, '\0');
        return std::fread(&out[0], 1, len, stdin) == len;
    }
    void send(J m) {
        std::lock_guard<std::mutex> g(outMx);
        m.set("seq", J::num(seq++));
        const std::string body = dump(m);
        std::cout << "Content-Length: " << body.size() << "\r\n\r\n" << body << std::flush;
    }
    void respond(const J& req, bool ok, J body, const std::string& erro = "") {
        J r = J::obj();
        r.set("type", J::str("response"));
        r.set("request_seq", J::num(req.number("seq")));
        r.set("success", J::boolean(ok));
        r.set("command", J::str(req.text("command")));
        if (!ok) r.set("message", J::str(erro));
        r.set("body", std::move(body));
        send(std::move(r));
    }
    void event(const std::string& nome, J body = J::obj()) {
        J e = J::obj();
        e.set("type", J::str("event"));
        e.set("event", J::str(nome));
        e.set("body", std::move(body));
        send(std::move(e));
    }
    void output(const std::string& texto, const std::string& cat = "stdout") {
        J b = J::obj();
        b.set("category", J::str(cat));
        b.set("output", J::str(texto));
        event("output", std::move(b));
    }

    // --- MI
    MiRecord mi(const std::string& cmd, int timeoutMs = 30000) {
        long tok;
        {
            std::lock_guard<std::mutex> g(miMx);
            tok = proxToken++;
        }
        gdb.write(std::to_string(tok) + cmd + "\n");
        std::unique_lock<std::mutex> lk(miMx);
        if (!miCv.wait_for(lk, std::chrono::milliseconds(timeoutMs), [&] { return respostas.count(tok) > 0 || terminado.load(); })) {
            MiRecord r; r.kind = '^'; r.klass = "error"; r.data = J::obj().set("msg", J::str("gdb nao respondeu a " + cmd));
            return r;
        }
        if (!respostas.count(tok)) { MiRecord r; r.kind = '^'; r.klass = "error"; return r; }
        MiRecord r = respostas[tok];
        respostas.erase(tok);
        return r;
    }
    MiRecord miOk(const std::string& cmd) {
        MiRecord r = mi(cmd);
        if (r.klass == "error") throw std::runtime_error(r.data.text("msg", "erro do gdb em " + cmd));
        return r;
    }
    static std::string quoteMi(const std::string& s) {
        std::string r = "\"";
        for (char c : s) { if (c == '"' || c == '\\') r += '\\'; r += c; }
        return r + "\"";
    }
    static std::string pathKey(const std::string& p) {
        std::string r = fs::path(p).lexically_normal().generic_string();
#ifdef _WIN32
        for (auto& c : r) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
#endif
        return r;
    }

    void leitorGdb() {
        std::string linha;
        while (gdb.readLine(linha)) {
            if (linha == "(gdb)" || linha == "(gdb) ") continue;
            MiRecord r;
            if (!parseMi(linha, r)) { output(linha + "\n"); continue; } // saída do programa
            if (r.kind == '^') {
                std::lock_guard<std::mutex> g(miMx);
                respostas[r.token] = r;
                miCv.notify_all();
                if (r.klass == "exit") break;
            } else if (r.kind == '*' && r.klass == "stopped") {
                onStopped(r.data);
            } else if (r.kind == '@') {
                output(r.text);
            } else if (r.kind == '~' && !r.text.empty() && r.text.find("Python") == std::string::npos) {
                // console do gdb (avisos); não vai para a saída do programa
            }
        }
        if (!terminado) { event("terminated"); terminado = true; }
        miCv.notify_all();
    }

    void onStopped(const J& d) {
        const std::string reason = d.text("reason");
        if (reason.rfind("exited", 0) == 0) {
            J b = J::obj();
            b.set("exitCode", J::num(d.number("exit-code", 0)));
            event("exited", std::move(b));
            event("terminated");
            return;
        }
        std::string motivo = "pause";
        if (reason == "breakpoint-hit") motivo = d.text("disp") == "del" ? "entry" : "breakpoint";
        else if (reason == "end-stepping-range" || reason == "function-finished") motivo = "step";
        else if (reason == "signal-received") motivo = "exception";
        J b = J::obj();
        b.set("reason", J::str(motivo));
        b.set("threadId", J::num(d.number("thread-id", 1)));
        b.set("allThreadsStopped", J::boolean(true));
        event("stopped", std::move(b));
    }

    // compila o .g com símbolos num processo filho e repassa a saída como 'output'
    std::string compilar(const std::string& fonte) {
        std::error_code ec;
        fs::path dir = fs::temp_directory_path(ec) / "ghost-dap";
        fs::create_directories(dir, ec);
        std::string exe = (dir / (fs::path(fonte).stem().string() + "_dap" +
#ifdef _WIN32
                                  ".exe"
#else
                                  ""
#endif
                                  )).string();
        Child c;
        if (!c.start({ghostExe, "debug", "--saida", exe, fonte})) throw std::runtime_error("nao consegui iniciar o ghost para compilar");
        std::string linha, log;
        while (c.readLine(linha)) { log += linha + "\n"; output(linha + "\n", "console"); }
        if (c.wait() != 0 || !fs::exists(exe)) throw std::runtime_error("falha ao compilar " + fonte + ":\n" + log);
        return exe;
    }

    void handle(const J& req) {
        const std::string cmd = req.text("command");
        const J* argsP = req.get("arguments");
        const J args = argsP ? *argsP : J::obj();

        if (cmd == "initialize") {
            J caps = J::obj();
            caps.set("supportsConfigurationDoneRequest", J::boolean(true));
            caps.set("supportsEvaluateForHovers", J::boolean(true));
            caps.set("supportsTerminateRequest", J::boolean(true));
            respond(req, true, caps);
            event("initialized");
        } else if (cmd == "launch") {
            programa = args.text("program");
            if (programa.empty()) throw std::runtime_error("launch sem 'program' (arquivo .g)");
            stopOnEntry = args.flag("stopOnEntry");
            if (const J* a = args.get("args")) for (const auto& x : a->a) argsPrograma.push_back(x.s);
            const std::string exe = compilar(programa);
            if (!gdb.start({gdbExe, "--interpreter=mi3", "-q", "-nx", exe})) throw std::runtime_error("nao consegui iniciar o gdb (" + gdbExe + ")");
            gdbStarted = true;
            leitor = std::thread([this] { leitorGdb(); });
            mi("-gdb-set confirm off");
            mi("-gdb-set pagination off");
            if (!argsPrograma.empty()) {
                std::string a = "-exec-arguments";
                for (const auto& x : argsPrograma) a += " " + quoteMi(x);
                mi(a);
            }
            respond(req, true, J::obj());
        } else if (cmd == "setBreakpoints") {
            const J* src = args.get("source");
            const std::string arquivo = src ? src->text("path") : "";
            const std::string chave = pathKey(arquivo);
            for (const auto& n : bpPorArquivo[chave]) mi("-break-delete " + n);
            bpPorArquivo[chave].clear();
            J lista = J::arr();
            if (const J* bps = args.get("breakpoints")) {
                for (const auto& bp : bps->a) {
                    const int linha = static_cast<int>(bp.number("line"));
                    MiRecord r = mi("-break-insert " + quoteMi(fs::path(arquivo).generic_string() + ":" + std::to_string(linha)));
                    J out = J::obj();
                    if (r.klass == "done") {
                        const J* bkpt = r.data.get("bkpt");
                        bpPorArquivo[chave].push_back(bkpt ? bkpt->text("number") : "");
                        out.set("verified", J::boolean(true));
                        out.set("line", J::num(bkpt ? bkpt->number("line", linha) : linha));
                    } else {
                        out.set("verified", J::boolean(false));
                        out.set("message", J::str(r.data.text("msg", "linha sem codigo")));
                        out.set("line", J::num(linha));
                    }
                    lista.a.push_back(out);
                }
            }
            respond(req, true, J::obj().set("breakpoints", lista));
        } else if (cmd == "configurationDone") {
            respond(req, true, J::obj());
            if (stopOnEntry) mi("-break-insert -t ghost_main");
            miOk("-exec-run");
        } else if (cmd == "threads") {
            MiRecord r = miOk("-thread-info");
            J lista = J::arr();
            if (const J* ts = r.data.get("threads")) {
                for (const auto& t : ts->a) {
                    J th = J::obj();
                    th.set("id", J::num(t.number("id", 1)));
                    th.set("name", J::str("thread " + t.text("id", "1")));
                    lista.a.push_back(th);
                }
            }
            if (lista.a.empty()) { J th = J::obj(); th.set("id", J::num(1)); th.set("name", J::str("main")); lista.a.push_back(th); }
            respond(req, true, J::obj().set("threads", lista));
        } else if (cmd == "stackTrace") {
            const int tid = static_cast<int>(args.number("threadId", 1));
            MiRecord r = miOk("-stack-list-frames --thread " + std::to_string(tid));
            J frames = J::arr();
            if (const J* st = r.data.get("stack")) {
                for (const auto& item : st->a) {
                    const J* f = item.get("frame");
                    if (!f) continue;
                    const int nivel = static_cast<int>(f->number("level"));
                    J fr = J::obj();
                    fr.set("id", J::num(tid * 1000 + nivel));
                    std::string nome = f->text("func", "?");
                    if (nome == "ghost_main") nome = "main";
                    fr.set("name", J::str(nome));
                    fr.set("line", J::num(f->number("line", 0)));
                    fr.set("column", J::num(1));
                    const std::string arq = f->text("fullname", f->text("file"));
                    if (!arq.empty()) fr.set("source", J::obj().set("path", J::str(arq)).set("name", J::str(fs::path(arq).filename().string())));
                    frames.a.push_back(fr);
                }
            }
            J body = J::obj();
            body.set("totalFrames", J::num(static_cast<double>(frames.a.size())));
            body.set("stackFrames", frames);
            respond(req, true, body);
        } else if (cmd == "scopes") {
            const int fid = static_cast<int>(args.number("frameId"));
            J sc = J::obj();
            sc.set("name", J::str("Locais"));
            sc.set("variablesReference", J::num(fid + 1));
            sc.set("expensive", J::boolean(false));
            J lista = J::arr();
            lista.a.push_back(sc);
            respond(req, true, J::obj().set("scopes", lista));
        } else if (cmd == "variables") {
            const int ref = static_cast<int>(args.number("variablesReference")) - 1;
            const int tid = ref / 1000, nivel = ref % 1000;
            MiRecord r = miOk("-stack-list-variables --thread " + std::to_string(tid) + " --frame " + std::to_string(nivel) + " --simple-values");
            J lista = J::arr();
            if (const J* vs = r.data.get("variables")) {
                for (const auto& v : vs->a) {
                    J var = J::obj();
                    var.set("name", J::str(v.text("name")));
                    var.set("value", J::str(v.text("value", "{...}")));
                    var.set("type", J::str(v.text("type")));
                    var.set("variablesReference", J::num(0));
                    lista.a.push_back(var);
                }
            }
            respond(req, true, J::obj().set("variables", lista));
        } else if (cmd == "continue" || cmd == "next" || cmd == "stepIn" || cmd == "stepOut") {
            const std::string m = cmd == "continue" ? "-exec-continue" : cmd == "next" ? "-exec-next" : cmd == "stepIn" ? "-exec-step" : "-exec-finish";
            respond(req, true, cmd == "continue" ? J::obj().set("allThreadsContinued", J::boolean(true)) : J::obj());
            miOk(m);
        } else if (cmd == "pause") {
            respond(req, true, J::obj());
            mi("-exec-interrupt");
        } else if (cmd == "evaluate") {
            std::string extra;
            if (const J* f = args.get("frameId")) {
                const int fid = static_cast<int>(f->n);
                extra = " --thread " + std::to_string(fid / 1000) + " --frame " + std::to_string(fid % 1000);
            }
            MiRecord r = mi("-data-evaluate-expression" + extra + " " + quoteMi(args.text("expression")));
            if (r.klass != "done") throw std::runtime_error(r.data.text("msg", "nao foi possivel avaliar"));
            J body = J::obj();
            body.set("result", J::str(r.data.text("value")));
            body.set("variablesReference", J::num(0));
            respond(req, true, body);
        } else if (cmd == "disconnect" || cmd == "terminate") {
            respond(req, true, J::obj());
            if (gdbStarted) { mi("-gdb-exit", 2000); gdb.kill(); }
            terminado = true;
            event("terminated");
        } else {
            respond(req, false, J::obj(), "comando DAP nao suportado: " + cmd);
        }
    }
};

} // namespace

namespace GhostDAP {
int run(const std::string& ghostExe, const std::string& gdb) {
    Adapter a(ghostExe, gdb);
    return a.run();
}
}
