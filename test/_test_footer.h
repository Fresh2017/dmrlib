int main(int argc, char **argv)
{
    DMR_UNUSED(argc);
    DMR_UNUSED(argv);

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
