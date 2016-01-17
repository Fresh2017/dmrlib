char *_basename(char *prog)
{
    uint8_t pos = 0, sep = 0;
    while (prog[pos] != 0) {
        if ((prog[pos] == '/' || prog[pos] == '\\' ) && prog[pos + 1] != 0) {
            sep = pos + 1;
        }
        pos++;
    }
    return prog + sep;
}

int main(int argc, char **argv)
{
    DMR_UNUSED(argc);
    prog = _basename(argv[0]);

    srand(time(NULL));
    dmr_error_clear();
    dmr_log_priority_set(DMR_LOG_PRIORITY_TRACE);

    uint8_t i;
    for (i = 0; tests[i].test != NULL; i++) {
        printf("testing %s... ", tests[i].name);
        if (tests[i].test()) {
            printf("ok\n");
        } else {
            printf(" ... failed\n");
            return 1;
        }
    }
    return 0;
}
