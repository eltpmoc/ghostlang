"""Gera include/GhostBuiltins.hpp: nomes que o verificador de tipos (c41) aceita sem declaração.

O C++ gerado chama as funções do runtime direto (include/*.hpp) e as da biblioteca de matemática do C;
o REPL tem as suas (name == "..." em src/Interpreter*.cpp). Duas listas:
  - funções: aceitas quando o nome é chamado, nome(...). Generosa de propósito (inclui métodos dos
    cabeçalhos): um nome a mais só deixa de acusar um erro, um nome a menos recusaria um programa válido;
  - constantes: aceitas quando o nome é usado como valor (PI, true, macros e const dos cabeçalhos).
Nomes de uma letra vêm de código C++ dos cabeçalhos (lambdas, macros) e esconderiam variáveis não
declaradas (x, y, ...); as portas quânticas h(q), x(q) e as unidades i/theta ficam no verificador.

Uso (na raiz do projeto): python scripts/python/gen_builtins.py
"""
import glob
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SKIP = {"GhostBuiltins.hpp", "GhostAst.hpp", "GhostTypes.hpp"}

functions = set()
values = set()
for h in glob.glob(os.path.join(ROOT, "include", "*.hpp")):
    if os.path.basename(h) in SKIP:
        continue
    text = open(h, encoding="utf-8", errors="replace").read()
    text = re.sub(r"//[^\n]*", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r'"(?:[^"\\\n]|\\.)*"', '""', text)
    functions.update(re.findall(r"\b([A-Za-z_]\w*)\s*\(", text))
    values.update(re.findall(r"#\s*define\s+([A-Za-z_]\w*)", text))
    # só no nível do arquivo (sem recuo): const locais de funções dos cabeçalhos não são nomes da linguagem
    values.update(re.findall(r"^(?:static\s+|inline\s+)*(?:constexpr|const)\s+[\w:<>]+\s+([A-Za-z_]\w*)\s*=", text, flags=re.M))
for src in glob.glob(os.path.join(ROOT, "src", "Interpreter*.cpp")):
    text = open(src, encoding="utf-8", errors="replace").read()
    functions.update(re.findall(r'\bf?name\s*==\s*"([A-Za-z_]\w*)"', text))

# biblioteca do C (cmath, cstdlib, cstdio no espaço global)
functions.update("""sqrt cbrt pow exp exp2 log log2 log10 log1p expm1 sin cos tan asin acos atan atan2 sinh cosh tanh
asinh acosh atanh floor ceil round trunc fabs abs labs fmod fmin fmax hypot erf erfc tgamma lgamma rand srand
atoi atof exit time clock rename remove getenv system isnan isinf isfinite copysign nearbyint rint lround
llround""".split())
values.update("true false null PI E M_PI M_E INFINITY NAN RAND_MAX".split())

KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "catch", "defined", "decltype", "static_assert",
            "alignof", "noexcept", "operator", "template", "typename"}


def clean(names):
    return sorted(n for n in names if n not in KEYWORDS and not n.startswith("__") and (len(n) > 1 or n == "E"))


functions, values = clean(functions), clean(values)


def emit(f, fn, names, what):
    f.write("// %s\ninline const std::set<std::string>& %s() {\n    static const std::set<std::string> names = {\n" % (what, fn))
    line = "       "
    for n in names:
        piece = ' "%s",' % n
        if len(line) + len(piece) > 118:
            f.write(line + "\n")
            line = "       "
        line += piece
    f.write(line + "\n    };\n    return names;\n}\n\n")


out = os.path.join(ROOT, "include", "GhostBuiltins.hpp")
with open(out, "w", encoding="utf-8", newline="\n") as f:
    f.write("// GhostBuiltins.hpp - GERADO por scripts/python/gen_builtins.py (não editar à mão).\n")
    f.write("// Nomes do runtime e da biblioteca do C que o verificador de tipos (c41) aceita sem declaração.\n")
    f.write("#ifndef GHOST_BUILTINS_HPP\n#define GHOST_BUILTINS_HPP\n\n#include <set>\n#include <string>\n\n")
    emit(f, "ghost_builtin_functions", functions, "funções: aceitas quando chamadas, nome(...)")
    emit(f, "ghost_builtin_values", values, "constantes: aceitas como valor")
    f.write("#endif\n")
print(len(functions), "funcoes e", len(values), "constantes em", out)
