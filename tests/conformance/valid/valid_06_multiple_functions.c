int helper(int a) {
    return a + 1;
}

int compute(int x, int y) {
    return helper(x) + helper(y);
}

int main() {
    int res = compute(2, 3);
    return 0;
}
