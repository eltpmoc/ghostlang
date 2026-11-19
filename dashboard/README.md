# GhostLang Status — dashboard

Painel de maturidade, metas e checklist da GhostLang. Tudo vem de **`ghostlang-status.json`**.

## Abrir

```bash
dashboard\abrir-dashboard.bat
```

O script sobe um servidor apenas em `http://localhost:8777` e abre o navegador. Assim, o JSON é relido a cada atualização da página (botão ⟳).
Também é possível abrir o `index.html` direto do disco e carregar o JSON pelo botão **Carregar**. A cópia fica salva no navegador.

## Site (GitHub Pages)

O GitHub Pages publica a pasta `docs/`, e o dashboard aparece em https://eltpmoc.github.io/ghostlang/dashboard/. A fonte continua sendo esta pasta. Depois de mudar o `index.html` ou o JSON, atualize a cópia publicada:

```bash
python scripts/python/publicar_dashboard.py
```

O teste `docs_dashboard_publicado` do CTest falha se a cópia em `docs/dashboard/` ficar diferente desta pasta.

## Como preencher o JSON

| Bloco | Para que serve |
|---|---|
| `niveis` | Escala 0–5: Planejado, Conceito, Protótipo, Funcional, Estável, Avançado |
| `categorias` | Áreas do menu lateral (`id` é usado em `recursos[].categoria`) |
| `fases` | F0…F6; `status` = `concluida`, `em_andamento` ou `planejada` |
| `meta` | Meta mais avançada (GhostLang 2.0 Quantum-AI), pilares e critérios 1.0/2.0 |
| `recursos` | Um item por recurso da linguagem (veja abaixo) |
| `plataformas` | Matriz Windows/Linux/macOS com níveis por item |
| `diagnostico` | Problemas encontrados; `severidade` = `critico`, `serio`, `alerta` ou `ok` |
| `checklist` | Tarefas por fase; `feito` = true/false; `recurso` liga a um recurso |
| `linha_do_tempo` | Marcos com data |
| `snapshots` | Histórico do progresso geral (gerado pelo botão “Registrar snapshot”) |

Recurso:

```json
{
  "id": "q-medicao",              // único
  "nome": "Medição e colapso",
  "categoria": "quantico",        // id de categorias
  "nivel": 2,                     // 0–5 (situação atual)
  "alvo": 5,                      // 0–5 (onde queremos chegar)
  "fase": "F3",
  "prioridade": "alta",           // alta | media | baixa
  "desde": "2026-04-25",          // data em que o nível atual foi alcançado (alimenta o gráfico de evolução)
  "onde": ["prototipo"],          // compilador, repl, runtime, prototipo, build, aplicacao, biblioteca, ide, docs, repo
  "arquivos": ["temps_phathon/src/QuantumRegisterG.cpp"],
  "descricao": "…",
  "evidencia": "O que comprova o nível (teste, arquivo, saída)",
  "proximos_passos": ["…"]
}
```

**Progresso** = Σ min(nível, alvo) ÷ Σ alvo, calculado no total, por área e por fase.

## Fluxo sugerido

1. Ao implementar algo, suba o `nivel` do recurso, atualize `desde` e `evidencia`.
2. Marque os itens do checklist no painel e clique em **Exportar** para baixar o JSON atualizado. Depois, substitua o arquivo nesta pasta.
3. Ao fim de cada etapa, use **Linha do tempo → Registrar snapshot de hoje** para guardar o progresso no histórico.
