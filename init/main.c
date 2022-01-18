#include <types.h>
#include <serial.h>
#include <mmu.h>
#include <pmu.h>

#ifdef SUNXI_H3
    const static pbv_t pages[4] = {{(ptr_t)0x50000000, 0x2000000}, {(ptr_t)0x60000000, 0x4000000}, {(ptr_t)0x68000000, 0xA00000}, {(ptr_t)0x70000000, 0xF00000}};
#else
    const static pbv_t pages[4] = {{(ptr_t)0x90000000, 0x2000000}, {(ptr_t)0xA0000000, 0x4000000}, {(ptr_t)0xA8000000, 0xA00000}, {(ptr_t)0xB0000000, 0xF00000}};
#endif

extern char *itoa(int32_t num, char *str, int32_t base);

const char test_string[] = "qvNqTZp1Z0yjuUXv6PHmyLWv0cUP7SHr2o2caUsvDz9oxhDQI9lLp2meDyEBvPW6lvYtuIC8nVEGgHDgLIW62zOpJKjOlem3sIoRgXz1tI3tPwIk8npEEASnMoDHYsjAebaViYDXBvU4FBBNyblmizLqaZFnSd5ebPHVD1006G4MMLFR3THV7mcJpeKHU6KVSRfKlh5ixYgoOevK59csJfpEIdEDDWNDpF95YGHzrlKUoKqia6AIonMXUZgG2AOkOg0VHmrCN7U99wONsa1NNW7KhiSskc2bzJlROKCFlgiLo4FfS1XKGLQCAxREVlKFXcG7rGemkwhdXuDM1nYxFUYn56YPvuGbrLZl0MZn6gM28nq5og2GeSCxID2XCGmoovUGmZdxuchwaInjSrHJRpLPZy5WEjLi0BNa14bHXzZpHYXOYXyX7d6uUHUlvrJYCLaiyoPppV0rKJSa6zBA6pjEplm9Sv6wYQZHskTYD3NPTj1qKkumRC6u9Ndycsp27brPtcZKNGqfH8WXjCRIDJ7TDrUtKKJZP4gESedmUsDO4U1S3fuIlMgpBTKfIjCuWV1EJJtNWtVr3dOJBMz6sW7e";

void MMU_Map1MbPages(ulong_t* pgt, ulong_t paddr, ulong_t vaddr, size_t pages, memCfg_t* memCfg);

void main()
{
#ifdef USE_EARLY_UART
    SerialOpen();
    puts("BareMetal OS!!!");
#endif

    uint32_t pmu_counter = 0;

    // Initialize PMU counters
    pmu_int_perfcounters(1, 0);

    PERFORMANCE_MONITORING_START(pmu_counter);

    pgt_t pgt = MMU_P2L(MMU_KernelPGT());

    memCfg_t memCfg = {CPOLICY_WRITEALLOC, APOLICY_RWNA, TRUE, FALSE, TRUE};

    MMU_MapPages(pgt, (vaddr_t)0xA0100000, (pbv_t*)pages, 4, &memCfg);

    uint32_t* ptr = (uint32_t*)0xA6B00000;

    *ptr = 0xDEADFACE;

    char* dst = (char*)0xA20FFFC0;
    char* src = (char*)test_string;

    while(*src != '\0')
    {
        *dst++ = *src++;
    }

    *dst = '\0';

    PERFORMANCE_MONITORING_STOP(pmu_counter);

    char str[16] = {0};
    itoa(pmu_counter, str, 10);

#ifdef USE_EARLY_UART
    puts("\n\nMMU test pass");
    puts("\nTest duration: ");
    puts(str);
    puts("\n");
    puts("\n");
    puts((char*)0xA20FFFC0);
#endif

    while(TRUE);
}
