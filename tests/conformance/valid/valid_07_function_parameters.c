float evaluate(int code, float weight, char flag) {
    if (flag == 'Y') {
        return (float)code * weight;
    }
    return weight;
}

int main() {
    float val = evaluate(10, 2.5, 'Y');
    return 0;
}
