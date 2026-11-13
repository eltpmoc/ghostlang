function result<int> f() {
    spawn {
        var r = ok(1);
        int v = r?;
    }
    return ok(0);
}
function main() {}
