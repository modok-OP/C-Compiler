int main() {
    int i = 0;
    int count = 0;
    for (i = 0; i < 10; i++) {
        if (i == 5) {
            continue;
        }
        count++;
    }
    return 0;
}
