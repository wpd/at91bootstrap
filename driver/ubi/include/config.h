#define SYS_MALLOC_SIMPLE 1
#define SYS_MALLOC_F_LEN 0x400
#define CONFIG_SYS_MALLOC_LEN (512 << 10)
#define CONFIG_MTD_UBI_BEB_LIMIT 20
#define CONFIG_MTD_UBI_WL_THRESHOLD 4096
#define CONFIG_MTD_UBI_FASTMAP
#define CONFIG_ARM_ASM_UNIFIED
#define CONFIG_SYS_CACHELINE_SIZE 32
#define CONFIG_MTD_UBI_READONLY
#define CONFIG_IS_ENABLED(x) (x)
#define CONFIG_VAL(x) (x)

// Fiddle with CONFIG_LOGLEVEL to see more output from UBI,
// but we run out of code space pretty quickly when we do that.
#define CONFIG_LOGLEVEL 0
#ifdef CONFIG_DEBUG
extern int dbg_printf(const char *fmt_str, ...);
#define printf dbg_printf
#else
#define printf(fmt, ...)
#define CONFIG_UBI_SILENCE_MSG
#endif

#define __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRCAT
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRNCMP
#define __HAVE_ARCH_STRCHR
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_STRSTR
#define __HAVE_ARCH_MEMCHR
#define __HAVE_ARCH_STRCHR
#define __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMCMP
