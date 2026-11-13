function result<int> f() {
    try {}
    finally {
        var r = ok(1);
        int v = r?;
    }
    return ok(0);
}
function main() {}
