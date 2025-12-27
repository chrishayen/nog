fn bad_send() {
    ch := Channel<int>();
    ch.send("hello");
}

fn main() {
}
