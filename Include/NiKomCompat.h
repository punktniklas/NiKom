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

#endif /* NIKOMCOMPAT_H */
