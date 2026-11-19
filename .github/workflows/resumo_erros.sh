#!/usr/bin/env bash
# Resumo dos logs do CI como anotações do GitHub (aparecem no resumo do run sem login).
# Uma anotação por log (o GitHub mostra no máximo 10 por etapa): as linhas de erro com
# 3 linhas de contexto ou, se não houver, as últimas 30 linhas. %0A quebra a linha.
# Uso: bash .github/workflows/resumo_erros.sh error|warning
nivel="${1:-error}"
for f in ctest.log build.log configure.log; do
  [ -f "$f" ] || continue
  # se a compilação começou, a configuração deu certo: não gasta anotação com ela
  if [ "$f" = configure.log ] && [ -f build.log ]; then continue; fi
  grep -E -A3 'FAILED|error:|Error |undefined reference|No such file|\*\*\*Failed|Timeout|Killed|out of memory' "$f" | head -40 > resumo.txt
  [ -s resumo.txt ] || tail -n 30 "$f" > resumo.txt
  msg=$(cut -c1-300 resumo.txt | tr -d '\r' | awk 'BEGIN{ORS="%0A"} {gsub(/%/,"%25"); print}')
  echo "::${nivel} title=${f}::${msg}"
done
exit 0
