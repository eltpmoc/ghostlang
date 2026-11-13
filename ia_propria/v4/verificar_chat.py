#!/usr/bin/env python3
"""Smoke tests de integracao, sem alterar a memoria de uso real."""
import json
import re
from datetime import datetime, timezone
from zoneinfo import ZoneInfo
import os
from pathlib import Path
import subprocess
import tempfile
HERE=Path(__file__).resolve().parent
APP=HERE.parent
def main():
    device=os.environ.get("GHOST_TEST_DEVICE","cpu")
    with tempfile.TemporaryDirectory(prefix="ghost-v4-chat-") as memory:
        env=dict(os.environ,GHOST_DEVICE=device,GHOST_STREAM_MS="0",OMP_NUM_THREADS="2",GHOST_KNOWLEDGE_DIR=memory)
        def run(lines, zone="America/Sao_Paulo"):
            r=subprocess.run([str(APP/"build/v4/ia"),"chat"],cwd=APP,env=dict(env,TZ=zone),input=lines,text=True,capture_output=True,timeout=90,check=True)
            return r.stdout,r.stderr
        out,err=run("/internet off\nQuem é você?\nDescreva GhostLang e crie um exemplo.\n/stream\n/limpar\n/sair\n")
        assert "Sou a IA V4" in out and "O dobro de 5 e 10" in out and "Contexto da conversa limpo" in out,out
        assert "[Pesquisa automatica]" not in out,out
        # Network is ON: temporal questions must use the system clock, never a search.
        queries=["Que horas são?", "Que horas são agora?", "Estou perguntado se você tem nossão de tempo e qual é o Horário e data de hoje", "Olá, quantas horas?"]
        for zone in ("America/Sao_Paulo", "UTC"):
            before=datetime.now(timezone.utc).timestamp()
            out,err=run("/stream off\n"+"\n".join(queries)+"\n/sair\n", zone)
            after=datetime.now(timezone.utc).timestamp()
            clocks=re.findall(r"Data: (\d{2}/\d{2}/\d{4})\. Hora: (\d{2}:\d{2}:\d{2})\. Fuso do sistema: .*?\(UTC([+-]\d{4})\)",out)
            assert len(clocks)==len(queries),(out,err)
            for day,hour,offset in clocks:
                instant=datetime.strptime(day+" "+hour+" "+offset,"%d/%m/%Y %H:%M:%S %z")
                assert before-1 <= instant.timestamp() <= after,(out,before,after)
                assert instant.strftime("%z")==instant.astimezone(ZoneInfo(zone)).strftime("%z"),out
            assert "[Pesquisa automatica]" not in out and "[Geracao neural V4]" not in out,out
            assert "truncado" not in err,err
        out,err=run("/internet off\n/fontes\nQue dia é hoje?\n/neural\nOlá, quantas horas?\n/agora\n/sair\n")
        assert out.count("[Relogio local]")==1 and "[Calendario local]" in out and "[Ferramenta necessaria]" in out,out
        assert "[Geracao neural V4]" not in out,out
        out,err=run("/internet off\nDescreva GhostLang e crie um exemplo.\n/neural\nOlá!\n/sair\n")
        assert "truncado" not in err,(out,err)
        out,err=run("/neural\n"+"á"*250+"\n/sair\n")
        assert "[Limite de contexto]" in out and "[Geracao neural V4]" not in out and "truncado" not in err,(out,err)
        temporal_questions=["Você tem percepção de tempo?", "Há quanto tempo estamos conversando?", "Quanto tempo passou desde minha mensagem anterior?", "Que dia será amanhã?", "Que dia foi ontem?", "Que horas serão daqui a 15 minutos?"]
        out,err=run("/stream off\n"+"\n".join(temporal_questions)+"\n/tempo\n/limpar\nQuanto tempo passou desde minha mensagem anterior?\n/sair\n")
        for marker in ("nocao operacional", "Tempo decorrido", "contador monotonico", "amanha:", "ontem:", "[Calculo temporal]", "Ainda nao ha uma mensagem anterior"):
            assert marker in out,(marker,out)
        assert "[Pesquisa automatica]" not in out and "[Geracao neural V4]" not in out,out
        project="Vamos contruir um trabalho juntos. Qual a sua ideia?"
        out,err=run("/stream off\n"+project+"\n/sair\n")
        assert "[Planejamento local]" in out and "Você quer um trabalho de pesquisa" in out,out
        assert "[Pesquisa automatica]" not in out and "[Geracao neural V4]" not in out,out
        for streaming in ("on", "off"):
            out,err=run("/neural\n/stream "+streaming+"\n"+project+"\n/sair\n")
            assert "[Geracao neural recusada]" in out,out
            assert "mo to r" not in out and "[Planejamento local]" not in out and "[Geracao neural V4]" not in out,out
        question="Diga somente a palavra casa."
        raw,_=run("/internet off\n/neural\n/stream off\n"+question+"\n/sair\n")
        stream,_=run("/internet off\n/neural\n/stream on\n"+question+"\n/sair\n")
        def answer(text):
            for label in ("[Geracao neural V4] ", "[Geracao neural recusada] "):
                if label in text: return label+text.rsplit(label,1)[1].split("Voce> ",1)[0]
            raise AssertionError(text)
        assert answer(raw)==answer(stream),(raw,stream)
        cleared,_=run("/internet off\n/neural\n/stream off\nOlá!\n/limpar\n"+question+"\n/sair\n")
        assert answer(cleared)==answer(raw),cleared
        after_failure,_=run("/neural\n/stream off\n"+project+"\n"+question+"\n/sair\n")
        assert answer(after_failure)==answer(raw),after_failure
        exotic,err=run("/internet off\n/neural\n/stream off\nExplique a palavra vocẽ e o símbolo 🧠.\n/sair\n")
        assert "fora do vocabulario" not in err and ("[Geracao neural V4]" in exotic or "[Geracao neural recusada]" in exotic),(exotic,err)
        exotic.encode("utf-8",errors="strict")
        eof,_=run("")
        assert "IA V4" in eof
        assert not list(Path(memory).glob("*.gkm")),"Testes offline alteraram a memoria."
    report=dict(dispositivo=device,identidade="ok",fontes="ok",streaming="mesmo texto que geracao completa",
                contexto_limpar="ok",relogio_e_fusos="ok",nocao_temporal="ok",degeneracao_bloqueada="ok",planejamento_local="ok",roteamento_sem_pesquisa="ok",contexto_sem_corte="ok",utf8_sem_unk="ok",eof="ok",memoria_preservada=True)
    (HERE/"relatorios"/f"CHAT_{device}.json").write_text(json.dumps(report,ensure_ascii=False,indent=2)+"\n")
    print(json.dumps(report,ensure_ascii=False,indent=2))
if __name__=="__main__": main()
