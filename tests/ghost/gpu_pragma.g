pragma auto;

function void main(string args[]) {
    print("Backend: " + device_backend());
    pragma cpu;
    print("Backend apos cpu: " + device_backend());
    print("gpu_pragma: OK");
}
