#include <stdlib.h>
#include <string.h>

int main()
{
    char str[] = "hello world", *saveptr = NULL;
    strtok_r(str, " ", &saveptr);
    return 0;
}
