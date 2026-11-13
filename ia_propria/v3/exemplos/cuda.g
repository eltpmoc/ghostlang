function void main() {
    pragma auto;
    print(device_info());
    textmodel ia = text_model(64, 32, 192, 32, 4096, 42);
    ia.self_test();
}
