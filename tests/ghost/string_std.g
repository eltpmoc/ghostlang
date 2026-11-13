// tests/ghost/string_std.g - testes da biblioteca padrao de manipulacao de strings
import "libs/string.g";
import "libs/test.g";

function void test_string_ops() {
    string s = "GhostLang Quantum AI";

    assert_true(starts_with(s, "Ghost"), "starts_with Ghost");
    assert_false(starts_with(s, "Python"), "nao deve comecar com Python");

    assert_true(ends_with(s, "AI"), "ends_with AI");
    assert_false(ends_with(s, "Quantum"), "nao deve terminar com Quantum");

    string up = to_upper("ghost");
    assert_eq_str(up, "GHOST", "to_upper ghost -> GHOST");

    string low = to_lower("LANG");
    assert_eq_str(low, "lang", "to_lower LANG -> lang");

    string rep = str_repeat("ab", 3);
    assert_eq_str(rep, "ababab", "repeat ab 3x");

    string pl = pad_left("42", 5, "0");
    assert_eq_str(pl, "00042", "pad_left 42 com 5 digitos");

    string pr = pad_right("ghost", 8, ".");
    assert_eq_str(pr, "ghost...", "pad_right ghost com 8 caracteres");

    string rep_all = replace_all("banana", "a", "o");
    assert_eq_str(rep_all, "bonono", "replace_all banana a->o");

    print("test_string_ops: OK");
}

function void main() {
    test_string_ops();
    print("todos os testes de string_std passaram com sucesso!");
}
