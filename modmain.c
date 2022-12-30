#define VGA_COLS 80
#define VGA_ROWS 25
#define NL mputch('\n')
#define PRINT(X)    mputs((#X)); mputs(" == "); mputx((X)); NL; 
#define PRINT_EIP int32 eip; asm volatile("1: lea 1b, %0;": "=a"(eip)); PRINT(eip);

typedef unsigned char int8;
typedef unsigned short int int16;
typedef unsigned int int32;
typedef unsigned long long int64;
typedef unsigned int bool;
typedef unsigned int bit; // for bitfields

int32 cursor_x = 0;
int32 cursor_y = 0;

void outb(unsigned short int port, char data) {
   asm("outb %%al, %%dx;" : : "d"(port), "a"(data));
}

unsigned char inb(unsigned short int port) {
    unsigned char ret;
    asm ("inb %1, %0;" : "=a"(ret) : "Nd"(port));
    return ret;
}

void update_cursorpos_to_vga() {
    unsigned int position = cursor_y * 80 + cursor_x;
    outb(0x3d4, 0x0e);
    outb(0x3d5, (position >> 8) & 0xff);
    outb(0x3d4, 0x0f);
    outb(0x3d5, position & 0xff);
}


void update_cursorpos_from_vga() {
    unsigned short int offset;
    unsigned short int port_3D4 = 0x3D4;
    unsigned short int port_3D5 = 0x3D5;
    outb(port_3D4, 14);
    offset = inb(port_3D5) << 8;
    outb(port_3D4, 15);
    offset |= inb(port_3D5);

    cursor_x = offset % 80;
    cursor_y = offset / 80;
}    

void mputch(char ch) {
    volatile char *videoram = (char *)0xf0000000;
    unsigned int target = cursor_y * 80 + cursor_x;

    if (ch != '\n')
    {
        videoram[target << 1] = ch;
        videoram[(target << 1) + 1] = 0x07;
        cursor_x ++;
    } else {
        cursor_x = 0;
        cursor_y++;
    }

    // End of line
    if (cursor_x > (80 - 1))
    {
        cursor_y += cursor_x / VGA_COLS;
        cursor_x = cursor_x % VGA_COLS;
    }

    // Screen is full -- scrolling
    if (cursor_y > 24)
    {
        int line, col;
        target = 0;
        for (line = 0; line < VGA_ROWS; line++)
            for (col = 0; col < VGA_COLS; col++) {
            videoram[target] = videoram[target + 160];
            target += 2;
            }
        cursor_y = VGA_ROWS - 1;
        cursor_x = 0;
    }

    update_cursorpos_to_vga();
}

void mputs(char *s) {
    for (; *s!=0; s++)
        mputch(*s);
}

void mputd(unsigned int d) {
    // The only thing this function aims to do is the simplest
    // possible way of displaying an unsigned decimal integer, without
    // any leading zeros, alignment etc.

    char *decdigits = "0123456789";
    int d_place = 1000000000;
    int leading_zero = 1;

    while (d_place) {
        int digit = d / d_place;
        if (digit || !leading_zero || (d_place == 1))
            mputch(decdigits[d / d_place]);
        if (digit) leading_zero = 0;
        d %= d_place;
        d_place /= 10;
    }
}

void mputh(unsigned int d) {
    // "Human" printing. This is what some Unix commands do on the
    // '-h' switch. Eg. 16400 -> "16k"

    char postfix[] = "GMk "; // 0: G, 1: M, 2: k, 3: none
    int magnitude = 0;

    for (magnitude = 0; magnitude < 4; magnitude++) {
        if (d >= 1U<<(30 - 10 * magnitude))
            break;
    }

    mputd(d / (1U<<(30 - 10 * magnitude)));
    mputch(postfix[magnitude]);
}

void mputx_inner_32(unsigned int d) {
    char *hexdigits = "0123456789abcdef";
    int current = d;
    int count;

    for (count = 0; count < 8; count++)
    {
        mputch(hexdigits[(current & 0xf0000000) >> 28]);
        current <<= 4;
    }
}

void mputx(unsigned int d) {
    mputs("0x");
    mputx_inner_32(d);
}

void mputxx(unsigned long long dd) {
    // 64 bit
    mputs("0x");

    int32 d = (int32)(dd >> 32);
    mputx_inner_32(d);
    d = (int32)dd;  // truncate to 32 bits
    mputx_inner_32(d);
}

void mputb(unsigned int d) {
    // always 32 bit for now
    char *bindigits = "01";
    int current = d;
    int count;

    mputs("b");

    for (count = 0; count < 32; count++)
    {
        mputch(bindigits[(current & 0x80000000) >> 31]);
        current <<= 1;
    }
}


void modmain(void) {
    mputs("In module."); NL;
    PRINT_EIP;
    char *testvideo = (char *) 0xf0000002;

    while (1)
        testvideo[0]++;
}
