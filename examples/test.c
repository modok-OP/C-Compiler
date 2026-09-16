/*
 * Conformance test example from Section 28.1 of
 * C Compiler Front-End Language & Semantic Specification v1.0
 */

int sum(int a, int b) {
    return a + b;
}

int main() {
    int values[5];
    int i;
    int total = 0;

    for (i = 0; i < 5; i++) {
        values[i] = i * 10;
        total += values[i];
    }

    printf("total = %d\n", total);
    return 0;
}
