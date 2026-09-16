int sum_array(int a, int b, int c) {
    return a + b + c;
}

int main() {
    int data[3];
    int i;
    for (i = 0; i < 3; i++) {
        data[i] = (i + 1) * 10;
    }
    int total = sum_array(data[0], data[1], data[2]);
    printf("Calculated total: %d\n", total);
    return 0;
}
