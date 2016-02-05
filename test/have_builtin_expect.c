#define test(x) __builtin_expect(!!(x), 1)

int main() {
    if (test(1))
        return 0;
    return 1;
}

