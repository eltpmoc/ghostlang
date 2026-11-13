import "libs/test.g";
import "libs/crypto.g";

function void test_crypto() {
    string msg = "GhostLang 2026";
    string enc = crypto_base64_encode(msg);
    assert_eq_str(enc, "R2hvc3RMYW5nIDIwMjY=", "base64 encode");
    string dec = crypto_base64_decode(enc);
    assert_eq_str(dec, msg, "base64 decode");
    print("test_crypto: OK");
}

function void main() {
    test_crypto();
    print("todos os testes de crypto passaram!");
}

