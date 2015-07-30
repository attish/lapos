
void modmain(void) {
    char *testvideo = (char *) 0xf0000006;

    while (1)
        testvideo[0]++;
}
