#include <gccore.h>
#include <ogc/machine/processor.h>
#include <string.h>

extern void __exception_closeall();

typedef struct {
    u32 text_pos[7];
    u32 data_pos[11];
    u32 text_start[7];
    u32 data_start[11];
    u32 text_size[7];
    u32 data_size[11];
    u32 bss_start;
    u32 bss_size;
    u32 entry_point;
} dolheader;

static u32 load_dol_image(const void *dolstart, struct __argv *argv) {
    dolheader *dolfile = (dolheader *)dolstart;
    u32 i;
    for (i = 0; i < 7; i++) {
        if (!dolfile->text_size[i] || dolfile->text_start[i] < 0x100) continue;
        ICInvalidateRange((void *)dolfile->text_start[i], dolfile->text_size[i]);
        memmove((void *)dolfile->text_start[i], dolstart+dolfile->text_pos[i], dolfile->text_size[i]);
    }
    for (i = 0; i < 11; i++) {
        if (!dolfile->data_size[i] || dolfile->data_start[i] < 0x100) continue;
        memmove((void *)dolfile->data_start[i], dolstart+dolfile->data_pos[i], dolfile->data_size[i]);
        DCFlushRangeNoSync((void *)dolfile->data_start[i], dolfile->data_size[i]);
    }

    if (argv && argv->argvMagic == ARGV_MAGIC) {
        void *new_argv = (void *)(dolfile->entry_point + 8);
        memmove(new_argv, argv, sizeof(*argv));
        DCFlushRange(new_argv, sizeof(*argv));
    }

    return dolfile->entry_point;
}

void run_dol(const void *dol, struct __argv *argv) {
    u32 level;
    void (*ep)() = (void(*)())load_dol_image(dol, argv);
    __IOS_ShutdownSubsystems();
    _CPU_ISR_Disable(level);
    __exception_closeall();
    ep();
    _CPU_ISR_Restore(level);
}

#include <gctypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>

static bool read_from_file(u8 *buf, FILE *f) {
    while (1) {
        s32 bytes_read = fread(buf, 1, 0x8000, f);
        if (bytes_read > 0) buf += bytes_read;
        if (bytes_read < 0x8000) return feof(f);
    }
}

void load_from_buffer(u8* dol, int length, char *arg) {
    struct __argv argv;
    bzero(&argv, sizeof(argv));
    argv.argvMagic = ARGV_MAGIC;
    argv.length = strlen(arg) + 2;
    argv.commandLine = malloc(argv.length);
    if (!argv.commandLine) return;
    strcpy(argv.commandLine, arg);
    argv.commandLine[argv.length - 1] = '\x00';
    argv.argc = 1;
    argv.argv = &argv.commandLine;
    argv.endARGV = argv.argv + 1;

    //u8 *buf = (u8 *)0x92000000;
    //memcpy(buf, dol, length);

    //run_dol(buf, &argv);
	run_dol(dol, &argv);

    free(argv.commandLine);
}

