// Backends de dispositivo: só cpu e cuda existem. Vulkan, Metal e ROCm ainda não foram implementados
// (até 12/11/2026 eram fachada: device_supports dizia true e use_device fingia trocar de backend).
function main() {
    print(device_supports("cpu"));
    print(device_supports("vulkan"));
    print(device_supports("metal"));
    print(device_supports("rocm"));
    try {
        use_device("vulkan");
    } catch (e) {
        print("erro: ", e);
    }
    use_device("cpu");
    print(device_backend());
    use_device("auto");
}
