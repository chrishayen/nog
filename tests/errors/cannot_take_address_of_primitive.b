Person :: struct { name str }

fn process(int *p) {
    print(p);
}

fn main() {
    int x = 5;
    process(&x);
}
