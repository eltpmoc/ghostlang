function void main(string args[]) {
    print(device_info());
    textmodel primeiro = text_model(4, 4, 8, 2, 64, 19);
    primeiro.self_test();
    if (args.size() == 1 && args[0] == "ok") {
        print("OK: textmodel e operadores logicos nativos.");
    } else {
        text_error("Argumento esperado: ok");
    }
}
