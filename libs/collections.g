// libs/collections.g - estruturas de dados adicionais para a GhostLang (Stack, Queue, etc.)

class Stack<T> {
    list<T> itens;

    function Stack() {
        this.itens.clear();
    }

    function void push(T val) {
        this.itens.push(val);
    }

    function T pop() {
        if (this.itens.length() == 0) {
            throw "Stack vazia no pop";
        }
        return this.itens.pop();
    }

    function T peek() {
        if (this.itens.length() == 0) {
            throw "Stack vazia no peek";
        }
        return this.itens[this.itens.length() - 1];
    }

    function bool is_empty() {
        return this.itens.length() == 0;
    }

    function int size() {
        return this.itens.length();
    }

    function void clear() {
        this.itens.clear();
    }
}

class Queue<T> {
    list<T> itens;

    function Queue() {
        this.itens.clear();
    }

    function void enqueue(T val) {
        this.itens.push(val);
    }

    function T dequeue() {
        if (this.itens.length() == 0) {
            throw "Queue vazia no dequeue";
        }
        T primeiro = this.itens[0];
        list<T> resto;
        for (int i = 1; i < this.itens.length(); ++i) {
            resto.push(this.itens[i]);
        }
        this.itens = resto;
        return primeiro;
    }

    function T peek() {
        if (this.itens.length() == 0) {
            throw "Queue vazia no peek";
        }
        return this.itens[0];
    }

    function bool is_empty() {
        return this.itens.length() == 0;
    }

    function int size() {
        return this.itens.length();
    }

    function void clear() {
        this.itens.clear();
    }
}
