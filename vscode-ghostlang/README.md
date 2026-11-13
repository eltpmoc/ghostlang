# GhostLang VSCode Extension 👻

Extensão oficial do VSCode para a linguagem **GhostLang** — syntax highlighting completo, tema escuro personalizado e snippets produtivos.

## Funcionalidades

- ✅ **Syntax Highlighting** completo para arquivos `.g` e `.ghost`
- 🎨 **Tema GhostLang Dark** — paleta cuidadosamente escolhida
- ⚡ **Snippets** para funções, rotas HTTP, gates, loops e mais
- 📁 **Ícone de arquivo** para `.g` / `.ghost` no explorador

## Paleta de Cores

| Token | Cor | Exemplo |
|---|---|---|
| Palavras-chave de controle (`if`, `for`, `return`) | 🔴 `#F97583` | `return`, `if` |
| Declarações (`function`, `class`, `import`) | 🟣 `#C678DD` | `function`, `var` |
| Quantum (`state`, `gate`, `optimize`) | 🔵 `#79C0FF` | `gate`, `state` |
| HTTP (`server`, `route`, `listen`) | 🟡 `#FFD700` | `server`, `listen` |
| Tipos (`string`, `int`, `void`) | 🩵 `#56B6C2` | `string`, `int` |
| Nomes de funções | 🟠 `#E5C07B` | `get_status` |
| Chamadas de função | 🔵 `#61AFEF` | `http_ok()` |
| Strings | 🟢 `#A8CC8C` | `"texto"` |
| Números | 🟠 `#D19A66` | `8080`, `3.14` |
| Comentários | ⬛ `#4A5568` (italic) | `// comentário` |

## Como Instalar

### Método 1 — Copiar para extensões do VSCode (Desenvolvimento)

```bash
# Windows
xcopy /E /I vscode-ghostlang %USERPROFILE%\.vscode\extensions\ghostlang-1.0.0

# Reinicie o VSCode
```

### Método 2 — VSIX Package

```bash
# Instale o vsce
npm install -g @vscode/vsce

# Dentro da pasta vscode-ghostlang:
vsce package

# Instale o .vsix gerado
code --install-extension ghostlang-1.0.0.vsix
```

### Método 3 — Modo Desenvolvimento (F5)

1. Abra a pasta `vscode-ghostlang/` no VSCode
2. Pressione `F5` para abrir uma nova janela com a extensão carregada
3. Abra qualquer arquivo `.g` — o highlight já estará ativo!

## Snippets Disponíveis

| Prefixo | O que gera |
|---|---|
| `fn` / `function` | Declaração de função com tipo de retorno |
| `get` | Handler de rota GET retornando JSON |
| `post` | Handler de rota POST retornando 201 |
| `server` | Scaffold completo de servidor HTTP |
| `gate` | Declaração de matriz de pesos |
| `state` | Declaração de vetor quântico |
| `for` | Loop for com var |
| `if` | Condicional if |
| `print` | Print no console |
| `import` | Import de biblioteca |
| `http_ok` | Resposta 200 JSON |
| `serve` | Rota de arquivos estáticos |

## Ativar o Tema GhostLang Dark

1. `Ctrl+Shift+P` → "Color Theme"
2. Selecione **"GhostLang Dark"**
