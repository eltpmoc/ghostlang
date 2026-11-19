// Validação automática da extensão VS Code da GhostLang (CTest vscode_extensao).
//   node test/validar.js <ghost.exe> <raiz-do-projeto>
// 1. package.json, gramática, snippets, tema e language-configuration são JSON válidos;
// 2. as palavras-chave da gramática existem no lexer (src/Lexer.cpp) e todas as do lexer
//    estão na gramática;
// 3. cada snippet, com os valores padrão dos campos, passa no `ghost check`.
'use strict';
const fs = require('fs');
const os = require('os');
const path = require('path');
const { spawnSync } = require('child_process');

const [ghost, raiz] = process.argv.slice(2);
if (!ghost || !raiz) {
    console.error('uso: node test/validar.js <ghost.exe> <raiz>');
    process.exit(2);
}
const ext = path.join(raiz, 'vscode-ghostlang');
let falhas = 0;
const confere = (ok, oQue) => {
    console.log(`${ok ? '[ok]   ' : '[FALHA]'} ${oQue}`);
    if (!ok) falhas++;
};
const lerJson = (rel) => {
    try {
        return JSON.parse(fs.readFileSync(path.join(ext, rel), 'utf8'));
    } catch (e) {
        confere(false, `${rel} e JSON valido (${e.message})`);
        return null;
    }
};

// 1. JSON
const pkg = lerJson('package.json');
const gram = lerJson('syntaxes/ghostlang.tmLanguage.json');
const snippets = lerJson('snippets/ghostlang.json');
lerJson('language-configuration.json');
lerJson('themes/ghostlang-dark-color-theme.json');
if (pkg) {
    confere(fs.existsSync(path.join(ext, pkg.main)), `main (${pkg.main}) existe`);
    const langs = (pkg.contributes && pkg.contributes.languages) || [];
    confere(langs.some((l) => (l.extensions || []).includes('.g')), 'linguagem associada a .g');
}

// depurador declarado no package.json tem de ser registrado com 'ghost debug --dap'
if (pkg && (pkg.contributes.debuggers || []).length) {
    const js = fs.readFileSync(path.join(ext, pkg.main), 'utf8');
    confere(/registerDebugAdapterDescriptorFactory\('ghost'/.test(js) && /\['debug', '--dap'\]/.test(js),
            'depurador ghost registrado com ghost debug --dap');
}

// 2. palavras-chave
const lexer = fs.readFileSync(path.join(raiz, 'src', 'Lexer.cpp'), 'utf8');
const doLexer = new Set([...lexer.matchAll(/keywords\["([A-Za-z_]+)"\]/g)].map((m) => m[1]));
const contextuais = new Set(['interface']); // reconhecidas pelo parser, não pelo lexer
const naGramatica = new Set();
if (gram) {
    for (const [nome, regra] of Object.entries(gram.repository)) {
        if (!/^keywords-|^types$/.test(nome) || !regra.match) continue;
        const m = regra.match.match(/^\\b\((.*)\)\\b$/);
        if (!m) continue;
        for (const p of m[1].split('|')) naGramatica.add(p);
    }
}
const inventadas = [...naGramatica].filter((p) => !doLexer.has(p) && !contextuais.has(p));
confere(inventadas.length === 0, `gramatica sem palavras inexistentes ${inventadas.length ? '(' + inventadas.join(', ') + ')' : ''}`);
const faltando = [...doLexer].filter((p) => !naGramatica.has(p));
confere(faltando.length === 0, `todas as ${doLexer.size} palavras do lexer na gramatica ${faltando.length ? '(faltam ' + faltando.join(', ') + ')' : ''}`);

// 3. snippets no ghost check
// Expande como o VS Code: ${1:padrao} e ${1|a,b|} viram o padrão, $1 repete o valor do
// campo 1, e \\ \$ \} são escapes do formato de snippet.
const expandir = (corpo) => {
    let t = Array.isArray(corpo) ? corpo.join('\n') : String(corpo);
    const ESC = { '\\': '\u0001', '$': '\u0002', '}': '\u0003' };
    t = t.replace(/\\([\\$}])/g, (_, c) => ESC[c]);
    const padrao = {};
    for (let i = 0; i < 5; i++) {
        t = t.replace(/\$\{(\d+)\|([^,|}]*)[^}]*\|\}/g, (_, n, v) => (padrao[n] = v))
             .replace(/\$\{(\d+):([^${}]*)\}/g, (_, n, v) => (padrao[n] = v));
    }
    t = t.replace(/\$\{(\d+)\}/g, (_, n) => padrao[n] || '').replace(/\$(\d+)/g, (_, n) => padrao[n] || '');
    return t.replace(/\u0001/g, '\\').replace(/\u0002/g, '$').replace(/\u0003/g, '}');
};
const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'ghost-vscode-'));
for (const [nome, s] of Object.entries(snippets || {})) {
    let corpo = expandir(s.body);
    const topo = /^\s*(function|import|class|interface)\b/.test(corpo);
    // Um trecho é inserido dentro de um programa: declara os nomes que ele usa sem declarar
    const contexto = {
        req: 'string req = "";', condicao: 'bool condicao = true;', valor: 'string valor = "";',
        api: 'server api = 18080;', W: 'gate W(2, 2);', lista: 'list<int> lista = [1, 2];',
    };
    const declara = (n) => new RegExp(`\\b(gate|var|string|int|double|bool|server|list<[^>]*>|function\\s+\\w+)\\s+${n}\\b|\\b${n}\\s*\\(`).test(corpo);
    const pre = Object.entries(contexto)
        .filter(([n]) => new RegExp(`\\b${n}\\b`).test(corpo) && !declara(n))
        .map(([, d]) => '    ' + d + '\n').join('');
    if (!topo) corpo = `function main() {\n${pre}${corpo}\n}\n`;
    else if (!/function\s+main\s*\(/.test(corpo)) corpo += '\nfunction main() {\n}\n';
    if (/\bhttp_\w+\(/.test(corpo) && !/import\s+"libs\/http\.g"/.test(corpo)) corpo = 'import "libs/http.g";\n' + corpo;
    const arq = path.join(tmp, 'snippet.g');
    fs.writeFileSync(arq, corpo);
    const r = spawnSync(ghost, ['check', arq], { cwd: raiz, encoding: 'utf8' });
    const ok = r.status === 0;
    confere(ok, `snippet "${nome}" passa no ghost check`);
    if (!ok) console.log((r.stdout || '') + (r.stderr || '') + '\n--- programa ---\n' + corpo);
}
fs.rmSync(tmp, { recursive: true, force: true });

console.log(falhas ? `extensao: ${falhas} falha(s)` : 'extensao: tudo certo');
process.exit(falhas ? 1 : 0);
