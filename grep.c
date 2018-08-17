#define NULL 0
#define LBSIZE 4096
#define ESIZE 256
#define NBRA 5
#define EOF -1

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

#define READ 0
#define WRITE 1

int peekc;
int lastc;
char linebuf[LBSIZE];
char expbuf[ESIZE + 4];
int open(char *, int);
int read(int, char *, int);
int write(int, char *, int);
int close(int);
int exit(int);

int listf;
int col;
char *braslist[NBRA];
char *braelist[NBRA];
int nbra;

int advance(char *lp, char *ep);
int backref(int i, char *lp);
int cclass(char *set, int c, int af);
void compile(char *eof);
int execute(char *file);
int getchr(int f);
void putchr(int ac);
void print(char *sp);
void puts(char *sp);
void print_matched_line(char *file);

int main(int argc, char *argv[])
{
  argc--;
  argv++;

  compile(*argv);
  --argc;
  if (argc <= 0) {
    execute((char *)NULL);
  } else while (--argc >= 0) {
    argv++;
    execute(*argv);
  }
  return 0;
}

int getchr(int f)
{
  char c;
  if (read(f, &c, 1) <= 0)
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

  sp = eof;

  nbra = 0;
    peekc = c;
  lastep = 0;

  if (sp == "^")
  {
    sp++;
    *ep++ = CCIRC;
  }

  for (;;)
  {
    if (ep >= &expbuf[ESIZE])
      goto cerror;
    if ((c = *sp++) != '*')
      lastep = ep;
    switch (c)
    {
    case '\0':
      *ep++ = CEOF;
      return;

    case '\\':
      if ((c = *sp++) == '(')
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
      if ((peekc = *sp++) != eof && peekc != '\n')
        goto defchar;
      *ep++ = CDOL;
      continue;

    case '[':
      *ep++ = CCL;
      *ep++ = 0;
      cclcnt = 1;
      if ((c = *sp++) == '^')
      {
        c = *sp++;
        ep[-2] = NCCL;
      }
      do
      {
        if (c == '\n')
          goto cerror;
        if (c == '-' && ep[-1] != 0)
        {
          if ((c = *sp++) == ']')
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
      } while ((c = *sp++) != ']');
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
  exit(2);
}

int execute(char *file)
{
  char *p1, *p2;
  int c;
  int f;

  if (file) {
    if ((f = open(file, READ)) == -1) {
      exit(2);
    }
  } else {
    f = 0;
  }

  for (c = 0; c < NBRA; c++)
  {
    braslist[c] = 0;
    braelist[c] = 0;
  }
  for (;;)
  {
    p1 = linebuf;

    while ((c = getchr(f)) != '\n')
    {
      if (c == EOF)
        return;

      *p1++ = c;
      /* ensure at least one space left in buffer for null-termination */
      if (p1 >= &linebuf[LBSIZE - 1])
        break;
    }
    /* null terminate input */
    *p1++ = '\0';
    p1 = linebuf;
    p2 = expbuf;
    if (*p2 == CCIRC)
    {
        if (advance(p1, p2 + 1))
          print_matched_line(file);
        continue;
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
            print_matched_line(file);
            break;
        }
      } while (*p1++);
        continue;
    }
    /* regular algorithm */
    do
    {
      if (advance(p1, p2))
      {
          print_matched_line(file);
          break;
      }
    } while (*p1++);
  }
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
        return(0);
      if (backref(i, lp))
      {
        lp += braelist[i] - braslist[i];
        continue;
      }
      return (0);

    case CBACK | STAR:
      if (braelist[i = *ep++] == 0)
        return(0);
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
      exit(2);
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

void print(char *sp)
{
  while (*sp)
    putchr(*sp++);
}

void puts(char *sp)
{
  col = 0;
  print(sp);
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
    write(1, line, lp - line);
    return;
  }
  linp = lp;
}

void print_matched_line(char *file)
{
  /* TODO: If multiple files -> print filename (*file) first */
  puts(linebuf);
}