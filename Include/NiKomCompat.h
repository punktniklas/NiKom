#ifndef NIKOMCOMPAT_H
#define NIKOMCOMPAT_H
/*
 * Handle differences between different compilers.
 */

#ifdef __GNUC__
typedef struct LocaleBase NiKomLocaleType;
#else
typedef struct Library NiKomLocaleType;
#endif

/*
 * Support argument passing in registers.
 *
 * GCC needs the register specification after the parameter type and
 * name, while SAS/C needs it before. In the source we specify
 * registers twice, and let the preprocessor decide which one to use.
 */
#ifdef __GNUC__
# define AASM
# define __a0
# define __a1
# define __a6
# define __d0
# define __d1
# define __d2
# define AREG(reg) asm(#reg)
#else
# define AASM __asm
# define AREG(reg)
#endif

/* Header files. */
#ifndef __SASC__
# define HAVE_PROTO_ALIB_H	1
#endif

/* Far pointer handling. */
#ifdef __GNUC__
# define __far /* FIXME: Not sure what gcc does by default. */
#endif

#endif /* NIKOMCOMPAT_H */
