;/*
; * $Id: amigadate.c,v 1.1.1.1 1999/02/27 19:54:51 tkarki Exp $
; *
; * Author: Tomi Ollila <too@cs.hut.fi>
; *
; * Created: Wed Mar 30 23:53:45 1994 too
; * Last modified: Thu Mar 31 16:22:23 1994 too
; *
; *
echo gcc -v -O2 -nostdlib -D__AMIGADATE__... + -s -o amigadate amigadate.c
gcc -v -O2 -nostdlib -D__AMIGADATE__="(31.3.94)" -s -o amigadate amigadate.c
quit 0
; *
; */

#define NULL 0

asm(".text; jmp pc@(_start-.+2);");

static const char version[] = "$VER: amigadate 1.0 " __AMIGADATE__ "\n";

struct Library;

typedef unsigned char u_char;
typedef long BPTR;

struct dts {
  long		ds_x1;
  long		ds_x2;
  long		ds_x3;
  u_char	dt_Format;
  u_char	dt_Flags;
  char *	dt_x1;
  char *	dt_StrDate;
  char *	dt_x2;
};

/**************** Needed Exec library inlines (modified) *******************/

static __inline struct Library *
OpenLibrary (struct Library * SysBase, char * libName, unsigned long version)
{
  register struct Library * _res __asm("d0");
  register struct Library *a6 __asm("a6") = SysBase;
  register char *a1 __asm("a1") = libName;
  register unsigned long d0 __asm("d0") = version;
  __asm __volatile ("jsr a6@(-0x228)"
  : "=r" (_res)
  : "r" (a6), "r" (a1), "r" (d0)
  : "a0","a1","d0","d1", "memory");
  return _res;
}
static __inline void
CloseLibrary (struct Library * SysBase, struct Library * library)
{
  register struct Library *a6 __asm("a6") = SysBase;
  register struct Library *a1 __asm("a1") = library;
  __asm __volatile ("jsr a6@(-0x19e)"
  : /* no output */
  : "r" (a6), "r" (a1)
  : "a0","a1","d0","d1", "memory");
}

/**************** Needed DOS library inlines (modified) ********************/

static __inline struct dts *
DateStamp (struct Library * DOSBase, struct dts * date)
{
  register struct dts * _res  __asm("d0");
  register struct Library *a6 __asm("a6") = DOSBase;
  register struct dts *d1 __asm("d1") = date;
  __asm __volatile ("jsr a6@(-0xc0)"
  : "=r" (_res)
  : "r" (a6), "r" (d1)
  : "a0","a1","d0","d1", "memory");
  return _res;
}
static __inline long
DateToStr (struct Library * DOSBase, struct dts *datetime)
{
  register long  _res  __asm("d0");
  register struct Library *a6 __asm("a6") = DOSBase;
  register struct dts *d1 __asm("d1") = datetime;
  __asm __volatile ("jsr a6@(-0x2e8)"
  : "=r" (_res)
  : "r" (a6), "r" (d1)
  : "a0","a1","d0","d1", "memory");
  return _res;
}
static __inline long
Write (struct Library * DOSBase, BPTR file, char * buffer, long length)
{
  register long  _res  __asm("d0");
  register struct Library *a6 __asm("a6") = DOSBase;
  register BPTR d1 __asm("d1") = file;
  register char * d2 __asm("d2") = buffer;
  register long d3 __asm("d3") = length;
  __asm __volatile ("jsr a6@(-0x30)"
  : "=r" (_res)
  : "r" (a6), "r" (d1), "r" (d2), "r" (d3)
  : "a0","a1","d0","d1","d2","d3", "memory");
  return _res;
}
static __inline BPTR
Output (struct Library * DOSBase)
{
  register BPTR  _res  __asm("d0");
  register struct Library *a6 __asm("a6") = DOSBase;
  __asm __volatile ("jsr a6@(-0x3c)"
  : "=r" (_res)
  : "r" (a6)
  : "a0","a1","d0","d1", "memory");
  return _res;
}

/*************************************************************************/

int start()
{
  struct Library * SysBase;
  struct Library * DOSBase;
  struct dts date;
  char buf[17];
  int i;

  date.dt_x1 = date.dt_x2 = 0;
  date.dt_Flags = 0;
  date.dt_StrDate = buf + 1;
  date.dt_Format = 3;

  SysBase = *(struct Library **)4;

  if ((DOSBase = OpenLibrary(SysBase, "dos.library", 36)) == NULL)
    return 20;

  DateStamp(DOSBase, &date);
  DateToStr(DOSBase, &date);

  /* buf = " dd-mm-yy "
	    0123456789 */

  buf[6] = '.';                 /* " xd-xm.yy" */
  buf[9] = ')';                 /* " xd-xm.yy)" */
  buf[10] = '\n';

  if (buf[4] == '0') {          /* " xd-0m.yy)" */
    i = 1;
    buf[4] = '.';               /* " xd-.m.yy)" */
    buf[3] = buf[2];		/* " xxd.m.yy)" */
    if (buf[1] == '0') {        /* " 0xd.m.yy)" */
      i = 2;
      buf[2] = '(';             /* "  (d.m.yy)" ***/
    }
    else {
      buf[2] = buf[1];		/* "  dd.m.yy)" */
      buf[1] = '(';             /* " (dd.m.yy)" ***/
    }
  }
  else {
    buf[3] = '.';               /* " xd.mm.yy)" */
    if (buf[1] == '0') {        /* " 0d.mm.yy)" */
      i = 1;
      buf[1] = '(';             /* " (d.mm.yy)" ***/
    }
    else {
      i = 0;
      buf[0] = '(';             /* "(dd.mm.yy)" ***/
    }
  }

  Write(DOSBase, Output(DOSBase), buf + i, 11 - i);

  CloseLibrary(SysBase, DOSBase);

  return 0;
}
