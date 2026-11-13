// ciclo_a.g e ciclo_b.g se importam: o import repetido gera um aviso e é ignorado
import "ciclo_b.g";

function int deA() {
    return deB() + 1;
}
