fn main() {
    x := 5;

    select {
        case v := x.recv() {
        }
    }
}
