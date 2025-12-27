fn bad_recv() {
    ch := Channel<int>();
    ch.recv(1);
}

fn main() {
}
