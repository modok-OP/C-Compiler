int double_val(int n) {
    return n * 2;
}

int main() {
    int val = double_val(double_val(5)) + double_val(3);
    return 0;
}
