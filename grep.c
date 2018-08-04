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

char *getblock(unsigned int atl, int iof);
char *getline(unsigned int tl);
void add(int i);
int advance(char *lp, char *ep);
int append(int (*f)(void), unsigned int *a);
int backref(int i, char *lp);
void blkio(int b, char *buf, int (*iofcn)(int, char *, int));
int cclass(char *set, int c, int af);
void commands(void);
void compile(int eof);
void error(char *s);
int execute(unsigned int *addr);
void gdelete(void);
int getchr(void);
int getnum(void);
int gettty(void);
void global(int k);
void init(void);
unsigned int *address(void);
void newline(void);
void nonzero(void);
void onhup(int n);
void onintr(int n);
void print(void);
void putchr(int ac);
void putd(void);
void putfile(void);
int putline(void);
void puts(char *sp);
void quit(int n);
void setwide(void);
void setnoaddr(void);
void squeeze(int i);

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
  char *p1, *p2;
  SIG_TYP oldintr;

  oldquit = signal(SIGQUIT, SIG_IGN);
  oldhup = signal(SIGHUP, SIG_IGN);
  oldintr = signal(SIGINT, SIG_IGN);
  if (signal(SIGTERM, SIG_IGN) == SIG_DFL)
    signal(SIGTERM, quit);
  argv++;
  /* process options i.e. '-foo' */
  while (argc > 1 && **argv == '-')
  {
    switch ((*argv)[1])
    {

    case '\0':
      vflag = 0;
      break;

    case 'q':
      signal(SIGQUIT, SIG_DFL);
      vflag = 1;
      break;

    case 'o': /* output to file i.e. `ed -o out.txt` */
      oflag = 1;
      break;
    }
    argv++;
    argc--;
  }
  if (oflag)
  {
    p1 = "/dev/stdout";
    p2 = savedfile;
    /*
      copies a null-terminated string, with p1 ending up pointing to the end
      of the source string and p2 pointing to the end of the destination string
    */
    while (*p2++ = *p1++)
      ;
  }
  if (argc > 1)
  {
    p1 = *argv;
    p2 = savedfile;
    while (*p2++ = *p1++)
      if (p2 >= &savedfile[sizeof(savedfile)])
        p2--;
    globp = "r";
  }
  zero = (unsigned *)malloc(nlall * sizeof(unsigned));
  tfname = mktemp(tmpXXXXX);
  init();
  if (oldintr != SIG_IGN)
    signal(SIGINT, onintr);
  if (oldhup != SIG_IGN)
    signal(SIGHUP, onhup);
  setjmp(savej);
  commands();
  quit(0);
  return 0;
}

void commands(void)
{
  unsigned int *a1;
  int c;
  char lastsep;
  /*
    main editor loop, repeatedly reads commands from stdin until `q` is entered
  */
  for (;;)
  {
    /* some commands will set pflag to print current buffer */
    if (pflag)
    {
      pflag = 0;
      addr1 = addr2 = dot;
      print();
    }
    c = '\n';
    for (addr1 = 0;;)
    {
      lastsep = c;
      a1 = address();
      c = getchr();
      if (c != ',' && c != ';')
        break;
      if (lastsep == ',')
        error(Q);
      if (a1 == 0)
      {
        a1 = zero + 1;
        if (a1 > dol)
          a1--;
      }
      addr1 = a1;
      if (c == ';')
        dot = a1;
    }
    if (lastsep != '\n' && a1 == 0)
      a1 = dol;
    if ((addr2 = a1) == 0)
    {
      given = 0;
      addr2 = dot;
    }
    else
      given = 1;
    if (addr1 == 0)
      addr1 = addr2;
    switch (c)
    {
    /* append text */
    case 'a':
      add(0);
      continue;

    /* global regex */
    case 'g':
      global(1);
      continue;

    /* insert text into buffer before addressed line */
    case 'i':
      add(-1);
      continue;

    /* number - print addressed lines, preceding each line by its number */
    case 'n':
      listn++;
      newline();
      print();
      continue;

    case '\n':
      if (a1 == 0)
      {
        a1 = dot + 1;
        addr2 = a1;
        addr1 = a1;
      }
      if (lastsep == ';')
        addr1 = a1;
      print();
      continue;

    /* print addressed lines */
    case 'p':
    case 'P':
      newline();
      print();
      continue;

    /* unconditional quit */
    case 'Q':
      fchange = 0;

    /* quit with warning if changes unwritten to file */
    case 'q':
      setnoaddr();
      newline();
      quit(0);

    /* print line number of addressed line */
    case '=':
      setwide();
      squeeze(0);
      newline();
      count = addr2 - zero;
      putd();
      putchr('\n');
      continue;

    /* null command */
    case EOF:
      return;
    }
    error(Q);
  }
}

void print(void)
{
  unsigned int *a1;

  nonzero();
  a1 = addr1;
  do
  {
    if (listn)
    {
      count = a1 - zero;
      putd();
      putchr('\t');
    }
    puts(getline(*a1++));
  } while (a1 <= addr2);
  dot = addr2;
  listf = 0;
  listn = 0;
  pflag = 0;
}

unsigned int *
address(void)
{
  int sign;
  unsigned int *a, *b;
  int opcnt, nextopand;
  int c;

  nextopand = -1;
  sign = 1;
  opcnt = 0;
  a = dot;
  do
  {
    do
      c = getchr();
    while (c == ' ' || c == '\t');
    if ('0' <= c && c <= '9')
    {
      peekc = c;
      if (!opcnt)
        a = zero;
      a += sign * getnum();
    }
    else
      switch (c)
      {
      case '$':
        a = dol;
        /* fall through */
      case '.':
        if (opcnt)
          error(Q);
        break;
      case '\'':
        c = getchr();
        if (opcnt || c < 'a' || 'z' < c)
          error(Q);
        a = zero;
        do
          a++;
        while (a <= dol && names[c - 'a'] != (*a & ~01));
        break;
      case '?':
        sign = -sign;
        /* fall through */
      case '/':
        compile(c);
        b = a;
        for (;;)
        {
          a += sign;
          if (a <= zero)
            a = dol;
          if (a > dol)
            a = zero;
          if (execute(a))
            break;
          if (a == b)
            error(Q);
        }
        break;
      default:
        if (nextopand == opcnt)
        {
          a += sign;
          if (a < zero || dol < a)
            continue; /* error(Q); */
        }
        if (c != '+' && c != '-' && c != '^')
        {
          peekc = c;
          if (opcnt == 0)
            a = 0;
          return (a);
        }
        sign = 1;
        if (c != '+')
          sign = -sign;
        nextopand = ++opcnt;
        continue;
      }
    sign = 1;
    opcnt++;
  } while (zero <= a && a <= dol);
  error(Q);
  /*NOTREACHED*/
  return 0;
}

int getnum(void)
{
  int r, c;

  r = 0;
  while ((c = getchr()) >= '0' && c <= '9')
    r = r * 10 + c - '0';
  peekc = c;
  return (r);
}

void setwide(void)
{
  if (!given)
  {
    addr1 = zero + (dol > zero);
    addr2 = dol;
  }
}

void setnoaddr(void)
{
  if (given)
    error(Q);
}

void nonzero(void)
{
  squeeze(1);
}

void squeeze(int i)
{
  if (addr1 < zero + i || addr2 > dol || addr1 > addr2)
    error(Q);
}

void newline(void)
{
  int c;

  if ((c = getchr()) == '\n' || c == EOF)
    return;
  if (c == 'p' || c == 'l' || c == 'n')
  {
    pflag++;
    if (c == 'l')
      listf++;
    else if (c == 'n')
      listn++;
    if ((c = getchr()) == '\n')
      return;
  }
  error(Q);
}

void onintr(int n)
{
  signal(SIGINT, onintr);
  putchr('\n');
  lastc = '\n';
  error(Q);
}

void onhup(int n)
{
  signal(SIGINT, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  if (dol > zero)
  {
    addr1 = zero + 1;
    addr2 = dol;
    io = creat("ed.hup", 0600);
    if (io > 0)
      putfile();
  }
  fchange = 0;
  quit(0);
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

int gettty(void)
{
  int rc;

  if (rc = gety())
    return (rc);
  /* entered '.' followed by enter -> finish add command */
  if (linebuf[0] == '.' && linebuf[1] == 0)
    return (EOF);
  return (0);
}

int gety(void)
{
  int c;
  char *gf;
  char *p;

  p = linebuf;
  gf = globp;
  while ((c = getchr()) != '\n')
  {
    if (c == EOF)
    {
      if (gf)
        peekc = c;
      return (c);
    }
    if ((c &= 0177) == 0)
      continue;
    *p++ = c; /* put c into linebuf array */
    if (p >= &linebuf[LBSIZE - 2])
      error(Q);
  }

  *p++ = 0; /* null terminate linebuf */
  return (0);
}

int append(int (*f)(void), unsigned int *a)
{
  unsigned int *a1, *a2, *rdot;
  int nline, tl;

  nline = 0; /* number of new lines added */
  dot = a;
  /* read one line at a time */
  while ((*f)() == 0)
  {
    if ((dol - zero) + 1 >= nlall)
    {
      unsigned *ozero = zero;

      nlall += 1024;
      if ((zero = (unsigned *)realloc((char *)zero, nlall * sizeof(unsigned))) == NULL)
      {
        error("MEM?");
        onhup(0);
      }
      dot += zero - ozero;
      dol += zero - ozero;
    }
    tl = putline();
    nline++;
    a1 = ++dol;
    a2 = a1 + 1;
    rdot = ++dot;
    while (a1 > rdot)
      *--a2 = *--a1;
    *rdot = tl;
  }
  return (nline);
}

void putfile(void)
{
  unsigned int *a1;
  int n;
  char *fp, *lp;
  int nib;

  nib = BLKSIZE;
  fp = genbuf;
  a1 = addr1;
  do
  {
    lp = getline(*a1++);
    for (;;)
    {
      if (--nib < 0)
      {
        n = fp - genbuf;
        if (write(io, genbuf, n) != n)
        {
          puts(WRERR);
          error(Q);
        }
        nib = BLKSIZE - 1;
        fp = genbuf;
      }
      count++;
      if ((*fp++ = *lp++) == 0)
      {
        fp[-1] = '\n';
        break;
      }
    }
  } while (a1 <= addr2);
  n = fp - genbuf;
  if (write(io, genbuf, n) != n)
  {
    puts(WRERR);
    error(Q);
  }
}

void add(int i)
{
  if (i && (given || dol > zero))
  {
    addr1--;
    addr2--;
  }
  squeeze(0);
  newline();
  append(gettty, addr2);
}

void quit(int n)
{
  if (vflag && fchange && dol != zero)
  {
    fchange = 0;
    error(Q);
  }
  unlink(tfname);
  exit(0);
}

char *
getline(unsigned int tl)
{
  char *bp, *lp;
  int nl;

  lp = linebuf;
  bp = getblock(tl, READ);
  nl = nleft;
  tl &= ~((BLKSIZE / 2) - 1);
  while (*lp++ = *bp++)
    if (--nl == 0)
    {
      bp = getblock(tl += (BLKSIZE / 2), READ);
      nl = nleft;
    }
  return (linebuf);
}

int putline(void)
{
  char *bp, *lp;
  int nl;
  unsigned int tl;

  fchange = 1;
  lp = linebuf;
  tl = tline;
  bp = getblock(tl, WRITE);
  nl = nleft;
  tl &= ~((BLKSIZE / 2) - 1);
  while (*bp = *lp++)
  {
    if (*bp++ == '\n')
    {
      *--bp = 0;
      linebp = lp;
      break;
    }
    if (--nl == 0)
    {
      bp = getblock(tl += (BLKSIZE / 2), WRITE);
      nl = nleft;
    }
  }
  nl = tline;
  tline += (((lp - linebuf) + 03) >> 1) & 077776;
  return (nl);
}

char *
getblock(unsigned int atl, int iof)
{
  int bno, off;

  bno = (atl / (BLKSIZE / 2));
  off = (atl << 1) & (BLKSIZE - 1) & ~03;
  if (bno >= NBLK)
  {
    lastc = '\n';
    error(T);
  }
  nleft = BLKSIZE - off;
  if (bno == iblock)
  {
    ichanged |= iof;
    return (ibuff + off);
  }
  if (bno == oblock)
    return (obuff + off);
  if (iof == READ)
  {
    if (ichanged)
      blkio(iblock, ibuff, write);
    ichanged = 0;
    iblock = bno;
    blkio(bno, ibuff, read);
    return (ibuff + off);
  }
  if (oblock >= 0)
    blkio(oblock, obuff, write);
  oblock = bno;
  return (obuff + off);
}

void blkio(int b, char *buf, int (*iofcn)(int, char *, int))
{
  lseek(tfile, (long)b * BLKSIZE, 0);
  if ((*iofcn)(tfile, buf, BLKSIZE) != BLKSIZE)
  {
    error(T);
  }
}

void gdelete(void)
{
  unsigned int *a1, *a2, *a3;

  a3 = dol;
  for (a1 = zero; (*a1 & 01) == 0; a1++)
    if (a1 >= a3)
      return;
  for (a2 = a1 + 1; a2 <= a3;)
  {
    if (*a2 & 01)
    {
      a2++;
      dot = a1;
    }
    else
      *a1++ = *a2++;
  }
  dol = a1 - 1;
  if (dot > dol)
    dot = dol;
  fchange = 1;
}

void init(void)
{
  int *markp;

  close(tfile);
  tline = 2;
  for (markp = names; markp < &names[26];)
    *markp++ = 0;
  subnewa = 0;
  anymarks = 0;
  iblock = -1;
  oblock = -1;
  ichanged = 0;
  close(creat(tfname, 0600));
  tfile = open(tfname, 2);
  dot = dol = zero;
}

void global(int k)
{
  char *gp;
  int c;
  unsigned int *a1;
  char globuf[GBSIZE];

  if (globp)
    error(Q);
  setwide();
  squeeze(dol > zero);
  if ((c = getchr()) == '\n')
    error(Q);
  compile(c);
  gp = globuf;
  while ((c = getchr()) != '\n')
  {
    if (c == EOF)
      error(Q);
    if (c == '\\')
    {
      c = getchr();
      if (c != '\n')
        *gp++ = '\\';
    }
    *gp++ = c;
    if (gp >= &globuf[GBSIZE - 2])
      error(Q);
  }
  if (gp == globuf)
    *gp++ = 'p';
  *gp++ = '\n';
  *gp++ = 0;
  for (a1 = zero; a1 <= dol; a1++)
  {
    *a1 &= ~01;
    if (a1 >= addr1 && a1 <= addr2 && execute(a1) == k)
      *a1 |= 01;
  }
  /*
	 * Special case: g/.../d (avoid n^2 algorithm)
	 */
  if (globuf[0] == 'd' && globuf[1] == '\n' && globuf[2] == '\0')
  {
    gdelete();
    return;
  }
  for (a1 = zero; a1 <= dol; a1++)
  {
    if (*a1 & 01)
    {
      *a1 &= ~01;
      dot = a1;
      globp = globuf;
      commands();
      a1 = zero;
    }
  }
}

void compile(int eof)
{
  int c;
  char *ep;
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

void putd(void)
{
  int r;

  r = count % 10;
  count /= 10;
  if (count)
    putd();
  putchr(r + '0');
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
