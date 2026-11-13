// qhw_*: o circuito roda no simulador local (nenhum job vai para IBM, IonQ ou Braket).
// Até 12/11/2026 as contagens eram inventadas; agora vêm da simulação de verdade.
import "../../libs/hardware.g"

function main() {
    qhw_configure("ibm", "token_nao_usado", "ibm_brisbane");

    // Bell: só 00 e 11 aparecem; a divisão exata depende da amostragem
    string jid = qhw_submit("OPENQASM 2.0; qreg q[2]; creg c[2]; h q[0]; cx q[0], q[1]; measure q -> c;", "ibm_brisbane", 1000);
    print(qhw_status(jid));
    print(qhw_get_count(jid, "00") + qhw_get_count(jid, "11"));
    print(qhw_get_count(jid, "01") + qhw_get_count(jid, "10"));
    print(qhw_get_count(jid, "00") > 400, " ", qhw_get_count(jid, "11") > 400);

    // circuito determinístico: o resumo inteiro é previsível
    string j2 = qhw_submit("OPENQASM 2.0; qreg q[2]; x q[0]; rx(pi) q[1]; measure q -> c;", "", 50);
    print(qhw_summary(j2));

    try {
        qhw_submit("OPENQASM 2.0; qreg q[1]; foo q[0];", "", 10);
    } catch (e) {
        print("erro: ", e);
    }
}
