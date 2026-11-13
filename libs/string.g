// ============================================================
// Biblioteca Padrao: GhostString v2.0
// Modulo de Manipulacao de Strings (starts_with, ends_with, to_upper, to_lower, pad, replace)
// ============================================================

function bool starts_with(string text, string prefix) {
    if (prefix.length() > text.length()) {
        return false;
    }
    return text.substr(0, prefix.length()) == prefix;
}

function bool ends_with(string text, string suffix) {
    if (suffix.length() > text.length()) {
        return false;
    }
    int start = text.length() - suffix.length();
    return text.substr(start, suffix.length()) == suffix;
}

function string to_upper(string text) {
    string out = "";
    for (int i = 0; i < text.length(); ++i) {
        string c = text.substr(i, 1);
        switch (c) {
            case "a": out = out + "A"; break;
            case "b": out = out + "B"; break;
            case "c": out = out + "C"; break;
            case "d": out = out + "D"; break;
            case "e": out = out + "E"; break;
            case "f": out = out + "F"; break;
            case "g": out = out + "G"; break;
            case "h": out = out + "H"; break;
            case "i": out = out + "I"; break;
            case "j": out = out + "J"; break;
            case "k": out = out + "K"; break;
            case "l": out = out + "L"; break;
            case "m": out = out + "M"; break;
            case "n": out = out + "N"; break;
            case "o": out = out + "O"; break;
            case "p": out = out + "P"; break;
            case "q": out = out + "Q"; break;
            case "r": out = out + "R"; break;
            case "s": out = out + "S"; break;
            case "t": out = out + "T"; break;
            case "u": out = out + "U"; break;
            case "v": out = out + "V"; break;
            case "w": out = out + "W"; break;
            case "x": out = out + "X"; break;
            case "y": out = out + "Y"; break;
            case "z": out = out + "Z"; break;
            default: out = out + c; break;
        }
    }
    return out;
}

function string to_lower(string text) {
    string out = "";
    for (int i = 0; i < text.length(); ++i) {
        string c = text.substr(i, 1);
        switch (c) {
            case "A": out = out + "a"; break;
            case "B": out = out + "b"; break;
            case "C": out = out + "c"; break;
            case "D": out = out + "d"; break;
            case "E": out = out + "e"; break;
            case "F": out = out + "f"; break;
            case "G": out = out + "g"; break;
            case "H": out = out + "h"; break;
            case "I": out = out + "i"; break;
            case "J": out = out + "j"; break;
            case "K": out = out + "k"; break;
            case "L": out = out + "l"; break;
            case "M": out = out + "m"; break;
            case "N": out = out + "n"; break;
            case "O": out = out + "o"; break;
            case "P": out = out + "p"; break;
            case "Q": out = out + "q"; break;
            case "R": out = out + "r"; break;
            case "S": out = out + "s"; break;
            case "T": out = out + "t"; break;
            case "U": out = out + "u"; break;
            case "V": out = out + "v"; break;
            case "W": out = out + "w"; break;
            case "X": out = out + "x"; break;
            case "Y": out = out + "y"; break;
            case "Z": out = out + "z"; break;
            default: out = out + c; break;
        }
    }
    return out;
}

function string str_repeat(string text, int count) {
    string out = "";
    for (int i = 0; i < count; ++i) {
        out = out + text;
    }
    return out;
}

function string pad_left(string text, int width, string pad = " ") {
    if (text.length() >= width) {
        return text;
    }
    int faltam = width - text.length();
    return str_repeat(pad, faltam) + text;
}

function string pad_right(string text, int width, string pad = " ") {
    if (text.length() >= width) {
        return text;
    }
    int faltam = width - text.length();
    return text + str_repeat(pad, faltam);
}

function string replace_all(string text, string de, string para) {
    if (de.length() == 0 || text.length() == 0) {
        return text;
    }
    string out = "";
    int i = 0;
    while (i < text.length()) {
        if (i + de.length() <= text.length() && text.substr(i, de.length()) == de) {
            out = out + para;
            i = i + de.length();
        } else {
            out = out + text.substr(i, 1);
            i = i + 1;
        }
    }
    return out;
}
