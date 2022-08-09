#define _POSIX_C_SOURCE 200809L

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>

#include "color.h"
#include "config.h"

struct ascii_art {
    char *row1, *row2, *row3, *row4, *row5, *row6, *row7, *row8;
};

struct uptime {
    int day, hour, minute;
};

struct memory {
    unsigned long used, total;
};

struct stats {
    char           hostname[HOST_NAME_MAX + 1];
    struct utsname sys_info;
    struct uptime  time;
    char          *pkg_cnt;
    char          *shell;
    struct memory  mem;
};

char *pipe_read(const char *exec)
{
    FILE *pipe = popen(exec, "r");
    if (pipe == NULL)
        return NULL;
    char     *return_val   = malloc(256);
    const int scanf_return = fscanf(pipe, "%[^\n]256s", return_val);
    pclose(pipe);

    return scanf_return == EOF ? NULL : return_val;
}

struct uptime get_uptime()
{
    struct uptime   up;
    struct timespec time;
#if defined(CLOCK_BOOTTIME)
#define CLOCK CLOCK_BOOTTIME
#elif defined(CLOCK_UPTIME)
#define CLOCK CLOCK_UPTIME
#elif defined(__APPLE__)
#define CLOCK CLOCK_MONOTONIC
#endif

#ifdef CLOCK
    clock_gettime(CLOCK, &time);
    up.day    = time.tv_sec / (24 * 3600);
    up.hour   = (time.tv_sec / 3600) - (time.tv_sec / (24 * 3600) * 24);
    up.minute = (time.tv_sec / 60) - (time.tv_sec / 3600 * 60);
#endif
    return up;
}

char *get_shell()
{
    char *shell = getenv("SHELL");
    char *slash = strrchr(shell, '/');
    if (slash) {
        shell = slash + 1;
    }
    return shell;
}

void get_memory_usage(struct memory *mem)
{
    unsigned long mem_free, mem_available, buffers, cached, shmem,
        s_reclaimable;
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        printf("error");
        return;
    }

    char buf[1024] = {0}, *ptr;
    fread(buf, 1024, 1, fp);

    ptr = strstr(buf, "MemTotal:");
    sscanf(ptr, "MemTotal: %lu", &mem->total);

    ptr = strstr(ptr, "MemFree:");
    sscanf(ptr, "MemFree: %lu", &mem_free);

    ptr = strstr(ptr, "MemAvailable:");
    sscanf(ptr, "MemAvailable: %lu", &mem_available);

    ptr = strstr(ptr, "Buffers:");
    sscanf(ptr, "Buffers: %lu", &buffers);

    ptr = strstr(ptr, "Cached:");
    sscanf(ptr, "Cached: %lu", &cached);

    ptr = strstr(ptr, "Shmem:");
    sscanf(ptr, "Shmem: %lu", &shmem);

    ptr = strstr(ptr, "SReclaimable:");
    sscanf(ptr, "SReclaimable: %lu", &s_reclaimable);

    mem->used =
        mem->total + shmem - mem_free - buffers - cached - s_reclaimable;

    fclose(fp);
}

void print_logo(struct ascii_art ascii)
{
    printf("%s\n", ascii.row1);
    printf("%s\n", ascii.row2);
    printf("%s\n", ascii.row3);
    printf("%s\n", ascii.row4);
    printf("%s\n", ascii.row5);
    printf("%s\n", ascii.row6);
    printf("%s\n", ascii.row7);
    printf("%s",   ascii.row8);
}

void print_stats(struct stats *s)
{
    int row = 7, shift = 0;
    printf("\033[%dA", row);

    if (PRINT_HOST) {
        shift =
            printf(BYELLOW "%s" BRED "@" BBLUE "%s", getlogin(), s->hostname);
        shift -= sizeof(BYELLOW BRED BBLUE) - 1;
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_OS) {
        shift = printf(VARIABLE_COLOR OS_TEXT TEXT_COLOR DISTRO);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_KERNEL) {
        shift = printf(VARIABLE_COLOR KERNEL_TEXT TEXT_COLOR "%s",
                       s->sys_info.release);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_UPTIME) {
        shift = printf(VARIABLE_COLOR UPTIME_TEXT TEXT_COLOR);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        if (s->time.day)
            shift += printf("%dd ", s->time.day);
        if (s->time.hour)
            shift += printf("%dh ", s->time.hour);
        shift += printf("%dm ", s->time.minute);
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_PKGS) {
        shift = printf(VARIABLE_COLOR PACKAGE_TEXT TEXT_COLOR "%s", s->pkg_cnt);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_SHELL) {
        shift = printf(VARIABLE_COLOR SHELL_TEXT TEXT_COLOR "%s", s->shell);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_MEMORY) {
        shift = printf(VARIABLE_COLOR MEMORY_TEXT TEXT_COLOR);
        shift -= sizeof(VARIABLE_COLOR TEXT_COLOR) - 1;
        shift +=
            printf("%lu/%lu MB (%d%%)", s->mem.used / 1024, s->mem.total / 1024,
                   (int)(s->mem.used / (double)s->mem.total * 100));
        printf("\033[%dD\033[1B", shift);
        row--;
    }

    if (PRINT_COLORS) {
        for (int i = 31; i < 37; i++) {
            printf("\033[0;%dm%s", i, COLOR_CHARACTER);
        } // print regular term colours
        printf("\n");
        row--;
    }

    if (row > 0)
        printf("\033[%dB", row);
    printf("%s\n", RESET);
}

int main()
{
    struct stats     s;
    struct ascii_art logo = {ASCII_ART};

    gethostname(s.hostname, HOST_NAME_MAX + 1);
    uname(&s.sys_info);
    s.time    = get_uptime();
    s.pkg_cnt = pipe_read(GET_PKG_CNT);
    s.shell   = get_shell();
    get_memory_usage(&s.mem);

    print_logo(logo);
    print_stats(&s);

    free(s.pkg_cnt);
    return 0;
}

