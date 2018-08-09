/*
 * Editor
 */

#include <signal.h>
#include <setjmp.h>

/* make BLKSIZE and LBSIZE 512 for smaller machines */
#define BLKSIZE 4096
#define NBLK 2047

#define NULL 0
#define FNSIZE 128
#define LBSIZE 4096
#define ESIZE 256
#define GBSIZE 256
#define NBRA 5
#define EOF -1
#define KSIZE 9

#define CBRA 1
#define CCHR 2
#define CDOT 4
#define CCL 6
#define NCCL 8
#define CDOL 10
#define CEOF 11
#define CKET 12
#define CBACK 14
#define CCIRC 15

#define STAR 01

char Q[] = "";
char T[] = "TMP";
#define READ 0
#define WRITE 1

int peekc;
int lastc;
char savedfile[FNSIZE];
char file[FNSIZE];
char linebuf[LBSIZE];   /* buffer of current line */
char expbuf[ESIZE + 4]; /* buffer to hold regular expression */
int given;
unsigned int *addr1, *addr2;
unsigned int *dot, *dol, *zero;
char genbuf[LBSIZE];
long count;
char *linebp; /* pointer to current line buffer */
int io;
int pflag; /* if true at beginning of commands loop -> print current buffer */
long lseek(int, long, int);
int open(char *, int);
int creat(char *, int);
int read(int, char *, int);
int write(int, char *, int);
int close(int);
int execl(char *, ...);
int exit(int);
int wait(int *);
int unlink(char *);

int vflag = 1;
int oflag; /* output to file */
int listf;
int listn;
int col;
char *globp;
int tfile = -1;
int tline;
char *tfname;
char *loc1;
char *loc2;
char ibuff[BLKSIZE];
int iblock = -1;
char obuff[BLKSIZE];
int oblock = -1;
int ichanged;
int nleft;
char WRERR[] = "WRITE ERROR";
int names[26];
int anymarks;
char *braslist[NBRA];
char *braelist[NBRA];
int nbra;
int subnewa;
int fchange;
int wrapp;
unsigned nlall = 128;

char *mktemp(char *);
char tmpXXXXX[50] = "/tmp/eXXXXX";
char *malloc(int);
char *realloc(char *, int);

int advance(char *lp, char *ep);
int backref(int i, char *lp);
int cclass(char *set, int c, int af);
void compile(char *eof);
void error(char *s);
int execute(unsigned int *addr);
int getchr(void);
void init(void);
void puts(char *sp);
void quit(int n);

jmp_buf savej;

typedef void (*SIG_TYP)(int);
SIG_TYP oldhup;
SIG_TYP oldquit;
/* these two are not in ansi, but we need them */
#define SIGHUP 1  /* hangup */
#define SIGQUIT 3 /* quit (ASCII FS) */

/* argc = argument count
   argv = argument vector */
int main(int argc, char *argv[])
{
  argc--;
  argv++;

  /* currently accept only regex as argument */
  if (argc != 1)
    exit(2);

  compile(*argv);

  execute((char *)NULL);

  return 0;
}

void error(char *s)
{
  int c;

  wrapp = 0;
  listf = 0;
  listn = 0;
  putchr('?');
  puts(s);
  count = 0;
  lseek(0, (long)0, 2);
  pflag = 0;
  if (globp)
    lastc = '\n';
  globp = 0;
  peekc = lastc;
  if (lastc)
    while ((c = getchr()) != '\n' && c != EOF)
      ;
  if (io > 0)
  {
    close(io);
    io = -1;
  }
  longjmp(savej, 1);
}

int getchr(void)
{
  char c;
  if (lastc = peekc)
  {
    peekc = 0;
    return (lastc);
  }
  if (globp)
  {
    if ((lastc = *globp++) != 0)
      return (lastc);
    globp = 0;
    return (EOF);
  }
  /* read on byte from stdin and store at c */
  /* if NULL or EOF set lastc to EOF and return EOF */
  if (read(0, &c, 1) <= 0)
    return (lastc = EOF);
  lastc = c & 0177;
  return (lastc);
}

void compile(char *eof)
{
  int c;
  char *ep, *sp;
  char *lastep;
  char bracket[NBRA], *bracketp;
  int cclcnt;

  ep = expbuf;
  bracketp = bracket;
  if ((c = getchr()) == '\n')
  {
    peekc = c;
    c = eof;
  }
  if (c == eof)
  {
    if (*ep == 0)
      error(Q);
    return;
  }
  nbra = 0;
  if (c == '^')
  {
    c = getchr();
    *ep++ = CCIRC;
  }
  peekc = c;
  lastep = 0;
  for (;;)
  {
    if (ep >= &expbuf[ESIZE])
      goto cerror;
    c = getchr();
    if (c == '\n')
    {
      peekc = c;
      c = eof;
    }
    if (c == eof)
    {
      if (bracketp != bracket)
        goto cerror;
      *ep++ = CEOF;
      return;
    }
    if (c != '*')
      lastep = ep;
    switch (c)
    {

    case '\\':
      if ((c = getchr()) == '(')
      {
        if (nbra >= NBRA)
          goto cerror;
        *bracketp++ = nbra;
        *ep++ = CBRA;
        *ep++ = nbra++;
        continue;
      }
      if (c == ')')
      {
        if (bracketp <= bracket)
          goto cerror;
        *ep++ = CKET;
        *ep++ = *--bracketp;
        continue;
      }
      if (c >= '1' && c < '1' + NBRA)
      {
        *ep++ = CBACK;
        *ep++ = c - '1';
        continue;
      }
      *ep++ = CCHR;
      if (c == '\n')
        goto cerror;
      *ep++ = c;
      continue;

    case '.':
      *ep++ = CDOT;
      continue;

    case '\n':
      goto cerror;

    case '*':
      if (lastep == 0 || *lastep == CBRA || *lastep == CKET)
        goto defchar;
      *lastep |= STAR;
      continue;

    case '$':
      if ((peekc = getchr()) != eof && peekc != '\n')
        goto defchar;
      *ep++ = CDOL;
      continue;

    case '[':
      *ep++ = CCL;
      *ep++ = 0;
      cclcnt = 1;
      if ((c = getchr()) == '^')
      {
        c = getchr();
        ep[-2] = NCCL;
      }
      do
      {
        if (c == '\n')
          goto cerror;
        if (c == '-' && ep[-1] != 0)
        {
          if ((c = getchr()) == ']')
          {
            *ep++ = '-';
            cclcnt++;
            break;
          }
          while (ep[-1] < c)
          {
            *ep = ep[-1] + 1;
            ep++;
            cclcnt++;
            if (ep >= &expbuf[ESIZE])
              goto cerror;
          }
        }
        *ep++ = c;
        cclcnt++;
        if (ep >= &expbuf[ESIZE])
          goto cerror;
      } while ((c = getchr()) != ']');
      lastep[1] = cclcnt;
      continue;

    defchar:
    default:
      *ep++ = CCHR;
      *ep++ = c;
    }
  }
cerror:
  expbuf[0] = 0;
  nbra = 0;
  error(Q);
}

int execute(unsigned int *addr)
{
  char *p1, *p2;
  int c;

  for (c = 0; c < NBRA; c++)
  {
    braslist[c] = 0;
    braelist[c] = 0;
  }
  p2 = expbuf;
  if (addr == (unsigned *)0)
  {
    if (*p2 == CCIRC)
      return (0);
    p1 = loc2;
  }
  else if (addr == zero)
    return (0);
  else
    p1 = getline(*addr);
  if (*p2 == CCIRC)
  {
    loc1 = p1;
    return (advance(p1, p2 + 1));
  }
  /* fast check for first character */
  if (*p2 == CCHR)
  {
    c = p2[1];
    do
    {
      if (*p1 != c)
        continue;
      if (advance(p1, p2))
      {
        loc1 = p1;
        return (1);
      }
    } while (*p1++);
    return (0);
  }
  /* regular algorithm */
  do
  {
    if (advance(p1, p2))
    {
      loc1 = p1;
      return (1);
    }
  } while (*p1++);
  return (0);
}

int advance(char *lp, char *ep)
{
  char *curlp;
  int i;

  for (;;)
    switch (*ep++)
    {

    case CCHR:
      if (*ep++ == *lp++)
        continue;
      return (0);

    case CDOT:
      if (*lp++)
        continue;
      return (0);

    case CDOL:
      if (*lp == 0)
        continue;
      return (0);

    case CEOF:
      loc2 = lp;
      return (1);

    case CCL:
      if (cclass(ep, *lp++, 1))
      {
        ep += *ep;
        continue;
      }
      return (0);

    case NCCL:
      if (cclass(ep, *lp++, 0))
      {
        ep += *ep;
        continue;
      }
      return (0);

    case CBRA:
      braslist[*ep++] = lp;
      continue;

    case CKET:
      braelist[*ep++] = lp;
      continue;

    case CBACK:
      if (braelist[i = *ep++] == 0)
        error(Q);
      if (backref(i, lp))
      {
        lp += braelist[i] - braslist[i];
        continue;
      }
      return (0);

    case CBACK | STAR:
      if (braelist[i = *ep++] == 0)
        error(Q);
      curlp = lp;
      while (backref(i, lp))
        lp += braelist[i] - braslist[i];
      while (lp >= curlp)
      {
        if (advance(lp, ep))
          return (1);
        lp -= braelist[i] - braslist[i];
      }
      continue;

    case CDOT | STAR:
      curlp = lp;
      while (*lp++)
        ;
      goto star;

    case CCHR | STAR:
      curlp = lp;
      while (*lp++ == *ep)
        ;
      ep++;
      goto star;

    case CCL | STAR:
    case NCCL | STAR:
      curlp = lp;
      while (cclass(ep, *lp++, ep[-1] == (CCL | STAR)))
        ;
      ep += *ep;
      goto star;

    star:
      do
      {
        lp--;
        if (advance(lp, ep))
          return (1);
      } while (lp > curlp);
      return (0);

    default:
      error(Q);
    }
}

int backref(int i, char *lp)
{
  char *bp;

  bp = braslist[i];
  while (*bp++ == *lp++)
    if (bp >= braelist[i])
      return (1);
  return (0);
}

int cclass(char *set, int c, int af)
{
  int n;

  if (c == 0)
    return (0);
  n = *set++;
  while (--n)
    if (*set++ == c)
      return (af);
  return (!af);
}

void puts(char *sp)
{
  col = 0;
  while (*sp)
    putchr(*sp++);
  putchr('\n');
}

char line[70];
char *linp = line;

void putchr(int ac)
{
  char *lp;
  int c;

  lp = linp;
  c = ac;
  if (listf)
  {
    if (c == '\n')
    {
      if (linp != line && linp[-1] == ' ')
      {
        *lp++ = '\\';
        *lp++ = 'n';
      }
    }
    else
    {
      if (col > (72 - 4 - 2))
      {
        col = 8;
        *lp++ = '\\';
        *lp++ = '\n';
        *lp++ = '\t';
      }
      col++;
      if (c == '\b' || c == '\t' || c == '\\')
      {
        *lp++ = '\\';
        if (c == '\b')
          c = 'b';
        else if (c == '\t')
          c = 't';
        col++;
      }
      else if (c < ' ' || c == '\177')
      {
        *lp++ = '\\';
        *lp++ = (c >> 6) + '0';
        *lp++ = ((c >> 3) & 07) + '0';
        c = (c & 07) + '0';
        col += 3;
      }
    }
  }
  *lp++ = c;
  if (c == '\n' || lp >= &line[64])
  {
    linp = line;
    write(oflag ? 2 : 1, line, lp - line);
    return;
  }
  linp = lp;
}
