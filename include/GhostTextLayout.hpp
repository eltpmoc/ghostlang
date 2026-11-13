#pragma once
struct GhostTextLayout {
    int context, embedding, hidden, vocab, batch;
    int width, w1, b1, w2, b2, count;
    GhostTextLayout(int c=64, int e=32, int h=128, int v=128, int b=32)
        : context(c), embedding(e), hidden(h), vocab(v), batch(b), width(c*e),
          w1(v*e), b1(w1+h*c*e), w2(b1+h), b2(w2+v*h), count(b2+v) {}
};

