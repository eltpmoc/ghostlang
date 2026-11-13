function main() {
    mutex mtx;
    var contador = 0;

    spawn {
        lock(mtx);
        contador = contador + 10;
        unlock(mtx);
    }

    sleep(50);

    lock(mtx);
    print("Contador:", contador);
    unlock(mtx);
}
