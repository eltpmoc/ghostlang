import "libs/db.g";

function int main() {
    int db = db_open(":memory:");
    db_exec(db, "CREATE TABLE produtos (id INTEGER PRIMARY KEY, item TEXT, preco REAL);");
    db_exec(db, "INSERT INTO produtos (item, preco) VALUES ('Teclado', 150.0);");
    db_exec(db, "INSERT INTO produtos (item, preco) VALUES ('Mouse', 75.5);");
    
    string res = db_query(db, "SELECT item, preco FROM produtos ORDER BY id;");
    print(res);
    
    int last = db_last_id(db);
    print(last);
    
    db_close(db);
    print("sqlite_compilado: OK");
    return 0;
}
