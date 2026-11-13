import "libs/math.g";

function void main() {
    // 1. Raízes no domínio real
    double raiz2 = sqrt(144.0);             // Raiz quadrada: 12.0
    double raiz3 = cbrt(125.0);             // Raiz cúbica: 5.0
    double raiz10 = raiz_n(1024.0, 10);      // Raiz décima de 1024: 2.0
    double raiz5_neg = nth_root(-32.0, 5);   // Raiz quinta de -32: -2.0
    println(f"Raiz decima de 1024 = {raiz10}");
    println(f"Raiz quinta de -32 = {raiz5_neg}");

    // 2. Raízes de números negativos com complex nativo
    complex raiz_neg4 = csqrt(-4.0);         // Raiz quadrada de -4: 2i -> (0, 2)
    complex raiz_neg1 = csqrt(-1.0);         // Raiz quadrada de -1: 1i -> (0, 1)
    complex raiz_cplx = sqrt(3.0 + 4.0i);    // Raiz de número complexo -> (2, 1)
    complex raiz4_neg16 = raiz_n_complexa(-16.0, 4); // Raiz 4a de -16: (1.41421, 1.41421)
    complex raiz2_neg9 = raiz_n(-9.0 + 0.0i, 2);     // Sobrecarga complexa -> (0, 3)
    println(f"csqrt(-4.0) = {raiz_neg4}");
    println(f"csqrt(-1.0) = {raiz_neg1}");
    println(f"raiz_n_complexa(-16, 4) = {raiz4_neg16}");
    println(f"raiz_n(-9+0i, 2) = {raiz2_neg9}");
}
