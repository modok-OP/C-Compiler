int main() {
    int outer = 1;
    {
        int mid = 2;
        {
            int inner = outer + mid;
        }
    }
    return 0;
}
