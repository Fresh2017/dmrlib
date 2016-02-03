#include <sys/socket.h>

int main() {
    int v = 0;
    setsockopt(0, 0, 0, &v, sizeof(v));
    return 0;
}
