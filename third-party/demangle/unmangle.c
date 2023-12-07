#include "unmangle.h"

/* C++Builder Unmangler Source Code (UNMANGLE.C, UNMANGLE.H)

   This code provides a mechanism for unmangling C++Builder linker
   names into a user-readable format. It is used by TDUMP, the
   debugger and the linker (for reporting error messages).

   This code can also create a standalone executable (by building with
   -DSTANDALONE), that takes a filename containing a list of mangled
   names, one per line.

   The main entrypoint for this code is the function "unmangle". See
   documentation for that function below.


  *******************************************************************

   The basic name-mangling scheme is based on the "Type-safe Linkage"
   section of the 2.1 C++ ARM (pp 121-125).

   In general, only one case of letters is used, since case-insensitivity
   is an option.  For some newer C++ features however, upper case letters
   are used.

   A mangled name always starts with '@'. A '_' may be prefixed later
   by the object output module. If the name is that of a class member,
   or is only the class name as for the virtual table name, the class
   name is followed by '@'.

   If the function is a constructor, destructor, or an overloaded
   operator, a special code is prefixed by "$b".

   If the function is a type conversion, the type code is prefixed by
   "$o".  Otherwise, the function name appears as itself.  After one
   of these name forms comes a '$' terminator.

   The type specification follows. If the type is a tag name (class or
   enum), the name is preceded by the decimal count (in ascii) of
   letters in the name. Otherwise a type code is used, as follows:

     a  array of
     b  wchar_t
     c  char
     Cs char16_t
     Ci char32_t
     d  double
     e  ellipsis (...)
     f  float
     g  long double
     h  rvalue reference
     i  int
     j  __int64 (long long)
     k  far rvalue reference
     l  long
     m  far reference
     M  member pointer
     n  far pointer
     o  bool
     p  pointer
     q  function
     r  reference
     s  short
     t  tN refers to the type of parameter #N in the parameter list
     T  T[type$]N template parameter at index N ([type$] is optional)
     TV TV[type$]N variadic template parameter at index N ([type$] is optional)

     u  unsigned modifier, except:
     um huge/far16 reference
     up huge/far16 pointer
     ur segment pointer
     ua _cs pointer
     ue _es pointer
     uf _fs pointer
     ug _gs pointer
     uv _ss pointer
     un _cs reference
     ub _es reference
     uw _fs reference
     ux _gs reference
     uq _ss reference

     v  void
     w  volatile modifier
     x  const modifier
     y  closure
     z  signed modifier

     $  array element, template arguments or func return type
     ?  unknown or error
     @  mangled or template dependent
     %  template name start or end
        (Bcc only) also used in find-references as prefix for local symbols
     +  (Bcc only) used in find-references for local symbols
     .  multibyte replacement
     =  used in binary encoding
     {  (Bcc only) used in non-deduced non-type templates
     }  (Bcc only) used in non-deduced non-type templates

     calling conventions:
        c C
        p PASCAL
        r FASTCALL
        f FORTRAN
        s STDCALL
        i INTERRUPT
        m MSFASTCALL

   A function has a type of 'q' followed by the type specifiers of
   each of its parameters.  If a parameter contains a function type, a
   'q' introduces the function type (typically this will be a
   'pointed-to' type), followed by the types of its parameters,
   followed by '$', followed by its return type.  (There always is
   one, even if void).  Note that the main function whose name we are
   mangling doesn't include the return type.

   Examples:

     afunc(const signed char, int huge*) ==> @afunc$qxzcupi
     foo(double (*)(float,int))		 ==> @foo$qpqfi$d
     myclass::func(int,long)		 ==> @myclass@func$qil
     myclass::f2(otherclass &)		 ==> @myclass@f2$q10otherclassr
     <virtual table for class 'myclass'> ==> @myclass@

   A template name begins with a percent sign, followed by the name of
   the template, followed by a '$', and the types and values of the
   template parameters. These parameters are mangled in exactly the
   same way as for function arguments.

   Also, a template function, after its function parameters have been
   mangled, will end with another '$' and the type of the return
   value.
*/

/* #define STANDALONE 1 */
/* #define DELPHI4_COMPAT 1 */

/* Fixes to get unmangler in sync with cppmangl.c */
#define UM_FIXES

#ifdef __MT__
#ifdef __GNUC__
#define UMTHREAD
#else
/* FIXME! Bcc on Unix doesn't support threads yet. */
#ifdef POSIX
#define UMTHREAD
#else
#define UMTHREAD __declspec(thread)
#endif /* POSIX */
#endif /* __GNUC__ */
#else
#define UMTHREAD
#endif

static char * UMTHREAD	source;	      /* current input source */
static char * UMTHREAD	srcbase;      /* beginning of input source */
static int UMTHREAD	srcindx;      /* beginning of input source */
static char * UMTHREAD	target;	      /* current output location */
static char * UMTHREAD	targbase;     /* beginning of output */
static char * UMTHREAD	namebase;     /* beginning of 'name' */
static char * UMTHREAD	targend;      /* end of output */
static char * UMTHREAD	qualend;      /* qualified part of name */
static char * UMTHREAD	prevqual;     /* qualified part of name */
static char * UMTHREAD	base_name;    /* base part of name */
static char * UMTHREAD	base_end;     /* end of base name */
static int UMTHREAD	set_qual;     /* setup the qualifier name? */
static int UMTHREAD	adjust_quals; /* adjust the qualifier pos? */
static umKind UMTHREAD	kind;
static char UMTHREAD	buff[MAXBUFFLEN];
static char UMTHREAD	vtbl_flags[256];
static jmp_buf UMTHREAD jmpb;     /* for quick escapes */
static int UMTHREAD     hashstart;    /* character offset of start of hash */
static char UMTHREAD    savechar;     /* character saved during copy_name */

/* The mangler, when mangling argument types, will create
   backreferences if the type has already been seen. These take the
   form t?, where ? can be either 0-9, or a-z. */

typedef struct param_entry_t
{
    char * targpos;
    int len;
} pentry;

/* No reason this has to be small (36).  We should allow for long
 * parameter lists. */

#define PTABLE_LEN  250

/* New mangle scheme lengths:
   len == 254 ==> old hash
   len == 253 ==> new MD5 hash
   len < 253  ==> unhashed */

#define QUALIFIER   '@'
#define ARGLIST     '$'
#define TMPLCODE    '%'

#define finish()  longjmp(jmpb, 1)
#define input()   (srcindx >= hashstart ?				\
		   (kind |= UM_HASHTRUNC, finish(), (char)0) : *source)
#define advance() (source++, srcindx++, input())
#define backup()  (source--, srcindx--)

#if defined(DEBUG) && defined(STANDALONE)
/* Allow the unmangler to be debugged by exposing input() as an external
   function. */

char (input)(void)
{
    return input();
}
#undef input
#endif

static void _overflow(void)
{
    *target = 0;
    kind |= UM_BUFOVRFLW;
    finish();
}

#define copy_char(c)							\
    (target != targend ? (*target++ = c, c) : (_overflow(), 0))

static void copy_string(char *p, int len)
{
    if (!len)
	    len = (int)strlen(p);

    if (len <= targend - target)
    {
		memcpy(target, p, len);
		target += len;
    }
    else
    {
		len = (int)(targend - target);
		memcpy(target, p, len);
		target += len;
		*target = 0;
		kind |= UM_BUFOVRFLW;
		finish();
    }
}

static void _error(char *error)
{
    copy_string(error, 0);

    kind |= UM_ERROR;
    finish();
}

struct trans
{
    char * name;
    char * symbol;
};

static struct trans table[] =
{
    { "add" , "+" },	 { "adr",  "&" },      { "and" , "&" },
    { "asg",  "=" },	 { "land", "&&" },     { "lor" , "||" },
    { "call", "()" },	 { "cmp" , "~" },      { "fnc" , "()" },
    { "dec",  "--" },	 { "dele", "delete" }, { "div" , "/" },
    { "eql" , "==" },	 { "geq" , ">=" },     { "gtr" , ">" },
    { "inc",  "++" },	 { "ind" , "*" },      { "leq" , "<=" },
    { "lsh" , "<<" },	 { "lss" , "<" },      { "mod" , "%" },
    { "mul" , "*" },	 { "neq" , "!=" },     { "new" , "new" },
    { "not" , "!" },	 { "or" ,  "|" },      { "rand", "&=" },
    { "rdiv", "/=" },	 { "rlsh", "<<=" },    { "rmin", "-=" },
    { "rmod", "%=" },	 { "rmul", "*=" },     { "ror" , "|=" },
    { "rplu", "+=" },	 { "rrsh", ">>=" },    { "rsh" , ">>" },
    { "rxor", "^=" },	 { "subs", "[]" },     { "sub" , "-" },
    { "xor",  "^" },	 { "arow", "->"},
    { "nwa",  "new[]" }, { "dla",  "delete[]" },
    { 0,      0 }
};

static void copy_op(char *src)
{
    struct trans * t;

    t = table;
    while (t->name && strcmp(t->name, src))
	++t;

    if (t->name == 0)
	longjmp(jmpb, 1);	/* presumably truncated */

    copy_string(t->symbol, 0);
}

static void copy_until1(char end1)
{
    char c;

    for (c = input();
	 c && c != end1;
	 c = advance())
    {
	copy_char(c);
    }
}

static void copy_until2(char end1, char end2)
{
    char c;

    for (c = input();
	 c && c != end1 && c != end2;
	 c = advance())
    {
	copy_char(c);
    }
}

static void copy_name(int tmplname);
static void copy_args(char end, int tmplargs);
static void copy_tmpl_args(void);
static void copy_type(char *start, int arglvl);

static void copy_return_type(char * start,
			     char * callconv,
			     char * regconv,
			     int    process_return)
{
    char * ret_type;
    int    ret_len;

    /* Process the return type of a function, and shuffle the output
       text around so it looks like the return type came first. */

    ret_type = target;

    if (process_return)
    {
	copy_type(target, 0);
	copy_char(' ');
    }

    if (callconv)
	copy_string(callconv, 0);
    if (regconv)
	copy_string(regconv, 0);

    ret_len = (int)(target - ret_type);

    /* Set up the return type to have a space after it. */

    assert(ret_len < MAXBUFFLEN);
    strncpy(buff, ret_type, ret_len);
    memmove(start + ret_len, start, ret_type - start);
    memmove(start, buff, ret_len);

    /* If we are inserting this return type at the very beginning of a
       string, it means the location of all the qualifier names is
       about to move. */

    if (adjust_quals)
    {
	if (namebase)  namebase += ret_len;
	if (qualend)   qualend += ret_len;
	if (prevqual)  prevqual += ret_len;
	if (base_name) base_name+= ret_len;
	if (base_end)  base_end += ret_len;
    }
}

static void copy_type(char *start, int arglvl)
{
    char * tname       = 0;
    char   c	       = input();
    int    is_const    = 0;
    int    is_volatile = 0;
    int    is_signed   = 0;
    int    is_unsigned = 0;
#ifdef _DEBUG
	int    maxloop     = 100;
#endif
    char   savedsavechar;

    for (;;)			/* emit type qualifiers */
    {
	assert(--maxloop > 0);

	switch (c)
	{
	case 'u': is_unsigned = 1; break;
	case 'z': is_signed = 1; break;
	case 'x': is_const = 1; break;
	case 'w': is_volatile = 1; break;

	case 'y':
	    /* 'y' for closure is followed by 'f' or 'n' */
	    c = advance();
	    assert(c == 'f' || c == 'n');
	    copy_string("__closure", 9);
	    break;

	default:
	    goto HANDLE_TYPE;
	}

	c = advance();
    }

  HANDLE_TYPE:
    if (isdigit(c))		/* enum or class name */
    {
	int    i = 0;
	char * p;
	int    len;
	char   mysavechar;
#ifdef DEBUG
	char * savecharaddr;
#endif

	do			/* compute length */
	{
	    i = i * 10 + (c - '0');
	    c = advance();
	}
	while (isdigit(c));

	/* Prepare to copy the digits, but handle the case that we'll
	   run into the start of the hash before the end.  */

	for (p = source, len = i; --len >= 0; p++)
	{
	    assert(*p);
	    if ((p - srcbase) >= hashstart)
	    {
		kind |= UM_HASHTRUNC;
		finish();
	    }
	}

	/* Output whether this class name was const or volatile. */

/* These were already printed (see  [BCB-265738]) */
//#if 0
	if (is_const) copy_string("const ", 6);
	if (is_volatile) copy_string("volatile ", 9);
//#endif

	/* Now copy the digits up to the terminating '\0' that we're
	   about to smash in.  We use the global savechar here because if we
	   happen to reach the hashstart, we'll think the string is
	   finished and not print the ending '...'. */

	savedsavechar = savechar;
	savechar = *(source + i);
	mysavechar = savechar;
#ifdef DEBUG
	savecharaddr = (source + i);
#endif
	*(source + i) = '\0';

	copy_name(0);

	*source = mysavechar;
	savechar = savedsavechar;
#ifdef DEBUG
	assert(savecharaddr == source);
#endif

	return;
    }

    savechar = c;

    switch (c)
    {
    case 'v': tname = "void"; break;
    case 'c': tname = "char"; break;
    case 'b': tname = "wchar_t"; break;
    case 's': tname = "short"; break;
    case 'i': tname = "int"; break;
    case 'l': tname = "long"; break;
    case 'f': tname = "float"; break;
    case 'd': tname = "double"; break;
    case 'g': tname = "long double"; break;
    case 'j': tname = "long long"; break;
    case 'o': tname = "bool"; break;
    case 'e': tname = "..."; break;

    case 'C':			/* C++ wide char */
    {
	c = advance();
	if (c == 's')
	    tname = "char16_t";
	else if (c == 'i')
	    tname = "char32_t";
	else
	    _error("Unknown type");
	break;
    }

    case 'M':			/* member pointer */
    {
	char * name = target;
	int len;

	/* We call 'copy_type' because it knows how to extract
	   length-prefixed names. */

	(void)advance();
	copy_type(target, 0);

	len = (int)(target - name);
	if (len > MAXBUFFLEN - 1)
	    len = MAXBUFFLEN - 1;
	strncpy(buff, name, len);
	buff[len] = 0;

	target = name;

	/* fall thru ... */
    }

    case 'r':			/* reference */
    case 'h':			/* rvalue reference */
    case 'p':			/* pointer */

	if (savechar == 'M')
	{
	    c = input();	/* [BTS-??????] */
	    switch (c)
	    {
	    case 'x': is_const = 1; c = advance(); break;     /* [BCB-272500] */
	    case 'w': is_volatile = 1; c = advance(); break;
	    }
	}
	else
	{
	    c = advance();
	}

	if (c == 'q')		/* function pointer */
	{
	    copy_char('(');

	    if (savechar == 'M')
	    {
		copy_string(buff, 0);
		copy_char(':');
		copy_char(':');
	    }

	    copy_char('*');
	    copy_char(')');

	    savechar = c;
	}

	savedsavechar = savechar;	  /* [BTS-263572] */
	copy_type(start, 0);
	savechar = savedsavechar;

	if (savechar == 'r')
	{
	    copy_char('&');
	}
	else if (savechar == 'h')
	{
	    copy_char('&');
	    copy_char('&');
	}
	else if (savechar == 'p')
	{
	    copy_char(' ');
	    copy_char('*');
	}
	else if (savechar == 'M')
	{
	    assert(buff[0]);
	    copy_char(' ');
	    copy_string(buff, 0);
	    copy_char(':');
	    copy_char(':');
	    copy_char('*');
	}

	break;

    case 'a':			/* array */
    {
	char dims[90];
	unsigned i = 0;

	do {
	    c = advance();

	    dims[i++] = '[';

	    if (c == '0')
		c = advance();	/* 0 size means unspecified */

	    while (c != '$')	/* collect size, up to '$' */
	    {
		dims[i++] = c;
		c = advance();
	    }

	    assert(c == '$');
	    c = advance();

	    dims[i++] = ']';
	}
	while (c == 'a');	/* collect all dimensions */

	dims[i] = 0;

	copy_type(target, 0);
	copy_string(dims, 0);

	break;
    }

    case 'q':			/* function */
    {
	char * callconv = 0;
	char * regconv	= 0;
	int    hasret;
	int    save_adjqual;

	/* We want the return type first, but find it last. So we emit
	   all but the return type, get the return type, then shuffle
	   to get them in the right place. */

	for (;;)
	{
	    if (advance() != 'q')
		break;

	    switch (advance())
	    {
	    case 'c': callconv = "__cdecl "; break;
	    case 'p': callconv = "__pascal "; break;
	    case 'r': callconv = "__fastcall "; break;
	    case 'f': callconv = "__fortran "; break;
	    case 's': callconv = "__stdcall "; break;
	    case 'y': callconv = "__syscall "; break;
	    case 'i': callconv = "__interrupt "; break;
	    case 'g': regconv = "__saveregs "; break;
	    }
	}

	save_adjqual = adjust_quals;
	adjust_quals = 0;

	copy_char('(');
	copy_args('$', 0);
	copy_char(')');

	adjust_quals = save_adjqual;

	hasret = input() == '$';
	if (hasret)
	    (void)advance();

	if (hasret || callconv || regconv)
	    copy_return_type(start, callconv, regconv, hasret);
	break;
    }

#ifdef UM_FIXES
    case ARGLIST:			/* template arg list */
	break;
    case TMPLCODE:			/* template reference */
	break;
#endif

    default:
	_error("Unknown type");
	break;
    }

    if (tname)
    {
	if (is_const)	 copy_string("const ", 6);
	if (is_volatile) copy_string("volatile ", 9);
	if (is_signed)	 copy_string("signed ", 7);
	if (is_unsigned) copy_string("unsigned ", 9);

	if (! arglvl || savechar != 'v')
	    copy_string(tname, 0);

	(void)advance();
    }
    else
    {
	if (is_const)    copy_string(" const", 6);
	if (is_volatile) copy_string(" volatile", 9);
    }
}

//#ifdef DELPHI4_COMPAT

static void copy_delphi4args(char end, int tmplargs)
{
    char   c	    = input();
    int    first    = 1;
    char * begin;
    char * start;
    char   termchar = 0;

    while (c && c != end)
    {
	if (first)
	{
	    first = 0;
	}
	else
	{
	    copy_char(',');
	    copy_char(' ');
	}

	begin = source;
	start = target;

	(void)advance();		/* skip the kind character */

	switch (c)
	{
	case 't':
	    copy_type(target, ! tmplargs);
	    break;

	case 'T':
	    copy_string("<type ", 6);
	    termchar = '>';
	    /* fall through... */

	case 'i':
	    if (*begin == '4' &&
		strncmp(begin + 1, "bool", 4) == 0)
	    {
		if (input() == '0')
		    copy_string("false", 5);
		else
		    copy_string("true", 4);
		(void)advance();
		break;
	    } //-V

	case 'j':
	case 'g':
	case 'e':
		copy_char('(');
	    copy_type(target, ! tmplargs);
		copy_char(')');
	    assert(input() == '$'); (void)advance();

	    copy_until2('$', TMPLCODE);
	    if (termchar)
		copy_char(termchar);
	    break;

	case 'm':
		copy_char('(');
	    copy_type(target, ! tmplargs);
		copy_char(')');
	    assert(input() == '$'); (void)advance();

	    copy_until1('$');
	    copy_char(':');
	    copy_char(':');
	    copy_char('*');
	    copy_until2('$', TMPLCODE);
	    break;

	default:
	    _error("Unknown template arg kind");
	    break;
	}

	c = input();
	if (c != end)
	{
	    assert(c == '$');
	    c = advance();
	}
    }
}

//#endif

static void copy_args(char end, int tmplargs)
{
    char   c	       = input();
    int    first       = 1;
    char * begin;
    char * start;
    int    startidx;
    int    param_index = 0;
    pentry param_table[PTABLE_LEN];
	//int    isDelphi4name = 0;
#ifdef DELPHI4_COMPAT
    int    scanned;
#endif

    memset(param_table, 0, sizeof(pentry) * PTABLE_LEN);

    while (c && c != end)
    {
	if (first)
	{
	    first = 0;
	}
	else
	{
	    copy_char(',');
	    copy_char(' ');
	}

	begin	 = source;
	startidx = srcindx;
	start	 = target;

	param_table[param_index].targpos = target;

#ifdef DELPHI4_COMPAT
	scanned = 0;

	while (c == 'x' || c == 'w')
	{
	    /* Decode 'const'/'volatile' modifiers [BCB-265738] */

	    switch (c)
	    {
	    case 'x': copy_string("const ", 6); break;
	    case 'w': copy_string("volatile ", 9); break;
	    }
	    scanned = 1;
	    c = advance();
	}

	if (scanned && c != 't')
	{
	    source = begin;
	    srcindx = startidx;
	}
#endif

	if (c != 't')
	{
	    copy_type(target, ! tmplargs);
	}
	else
	{
	    int ptindex;

	    c = advance();

	    if (isdigit(c))
		ptindex = c - '0';
	    else
		ptindex = (c - 'a') + 10;
	    ptindex--;

	    assert(ptindex >= 0 && ptindex < PTABLE_LEN);
	    assert(param_table[ptindex].targpos);
	    assert(param_table[ptindex].len > 0);

	    strncpy(buff, param_table[ptindex].targpos,
		    param_table[ptindex].len);
	    buff[param_table[ptindex].len] = 0;

	    copy_string(buff, 0);

		(void)advance();
	}

	param_table[param_index].len
	    = (int)(target - param_table[param_index].targpos);

	param_index++;

	c = input();

	if (tmplargs && c == '$') /* non-type template argument */
	{
	    char termchar = 0;

		*target = 0;
		target = start + 1;
		copy_string(start, 0);
		*start = '(';
		copy_char(')');

	    c = advance();
	    (void)advance();

	    switch (c)
	    {
	    case 'T':
		copy_string("<type ", 6);
		termchar = '>';

		/* fall through... */

	    case 'i':
		if (*begin == '4' &&
		    strncmp(begin + 1, "bool", 4) == 0)
		{
		    if (input() == '0')
			copy_string("false", 5);
		    else
			copy_string("true", 4);
			(void)advance();
		    break;
		}

		/* fall through... */

	    case 'j':
	    case 'g':
	    case 'e':
		copy_until1('$');
		if (termchar)
		    copy_char(termchar);
		break;

	    case 'm':
		copy_until1('$');
		copy_char(':');
		copy_char(':');
		copy_char('*');
		copy_until1('$');
		break;

	    default:
		_error("Unknown template arg kind");
		break;
	    }

	    assert(input() == '$');
	    c = advance();
	}
    }
}

/* parse template name and arguments according to the grammar:
 *     tmpl_args:
 *         % generic_name args %
 *     args:
 *         $ new_args
 *         bcb3_args
 */
static void copy_tmpl_args(void)
{
    char   c	       = input();
    int    save_setqual;
//#ifdef DELPHI4_COMPAT
    int    isDelphi4name = 0;
//#endif

//#ifdef DELPHI4_COMPAT
    if (c == 'S' || c == 'D')
    {
	if (! strncmp(source, "Set$", 4) ||
	    ! strncmp(source, "DynamicArray$", 13) ||
	    ! strncmp(source, "SmallString$", 12) ||
	    ! strncmp(source, "DelphiInterface$", 16))
	{
	    isDelphi4name = 1;
	}
    }
//#endif

    /* Output the base name of the template.  We use
	'copy_name' instead of 'copy_until', since this could
	be a template constructor name, for example. */

    copy_name(1);

    assert(input() == ARGLIST); (void)advance();

    if (*(target - 1) == '<')
	copy_char(' ');

    copy_char('<');

    /* Copy the template arguments over.  Also, save the
	'set_qual' variable, since we don't want to mix up the
	status of the currently known qualifier name with a
	name from a template argument, for example. */

    save_setqual = set_qual;
    set_qual = 0;

	if (isDelphi4name) {
		c = input();
		if (c == 'u' || c == 'z' || c == 'x' || c == 'w' || c == 'y' || isdigit(c))
			isDelphi4name = 0;
	}
	if (isDelphi4name)
		copy_delphi4args(TMPLCODE, 1);
	else
		copy_args(TMPLCODE, 1);

    set_qual = save_setqual;

    if (*(target - 1) == '>')
	copy_char(' ');
    copy_char('>');

    assert(input() == TMPLCODE); (void)advance();
}

static void copy_name(int tmplname)
{
    char   c		 = input();
    char * start;
    int    startidx;
    int    save_setqual;

    /* Start outputting the qualifier names and the base name. */

    for (;;)
    {
	if (set_qual)
	    base_name = target;

	/* Examine the string to see what this is.  Either it's a
	   qualifier name, a member name, a function name, a template
	   name, or a special name. We wouldn't be here if this were a
	   regular name. */

	if (isdigit(c))
	{
	    int flags;

	    /* If there's a number at the beginning of a name, it
	       could only be a vtable symbol flag. */

	    flags = c - '0' + 1;

	    vtbl_flags[0] = 0;

	    if (flags & 0x01)
		strcat(vtbl_flags, "huge");

	    if (flags & 0x02)
	    {
		if (vtbl_flags[0])
		    strcat(vtbl_flags, ", ");
		strcat(vtbl_flags, "fastthis");
	    }

	    if (flags & 0x04)
	    {
		if (vtbl_flags[0])
		    strcat(vtbl_flags, ", ");
		strcat(vtbl_flags, "rtti");
	    }

	    kind = (kind & ~UM_KINDMASK) | UM_VTABLE;

	    c = advance();
	    assert(c == 0 || c == '$');
	}

	switch (c)
	{
	case '#':		/* special symbol used for cond syms */

	    c = advance();

	    if (c == '$')
	    {
		assert(advance() == 'c');
		assert(advance() == 'f');
		assert(advance() == '$');
		assert(advance() == '@');

		copy_string("__vdflg__ ", 10);
	    (void)advance();
		copy_name(0);

		kind |= UM_VIRDEF_FLAG;
	    }
	    return;

	case QUALIFIER:		/* virdef flag or linker proc */

	    (void)advance();

	    copy_string("__linkproc__ ", 13);
	    copy_name(0);

	    kind |= UM_LINKER_PROC;

	    return;

	case TMPLCODE:		/* template name */

	    (void)advance();

	    copy_tmpl_args();

	    if (input() != QUALIFIER)
		kind |= UM_TEMPLATE;
	    break;


	case ARGLIST:		/* special name, or arglist */
	    if (tmplname)
		return;

	    c = advance();
	    if (c == 'x')
	    {
		c = advance();
		if (c == 'p' || c == 't')
		{
		    assert(advance() == ARGLIST);
		    (void)advance();

		    copy_string("__tpdsc__ ", 10);
		    copy_type(target, 0);

		    kind = (kind & ~UM_KINDMASK) | UM_TPDSC;
		    return;
		}
		else
		{
		    _error("What happened?");
		}
	    }

	    if (c == 'b')
	    {
		c = advance();
		start = source;
		startidx = srcindx;

		if ((c == 'c' || c == 'd') &&
		    advance() == 't' &&
		    advance() == 'r')
		{
		    assert(advance() == ARGLIST);

		    /* The actual outputting of the name will happen
		       outside of this function, to be sure that we
		       don't include any special name characters. */

		    if (c == 'c')
			kind = (kind & ~UM_KINDMASK) | UM_CONSTRUCTOR;
		    else
			kind = (kind & ~UM_KINDMASK) | UM_DESTRUCTOR;

		    break;
		}

		source = start;
		srcindx = startidx;

		if (c == 'c')
		{
			c = advance();
			if ((c == 'c' || c == 'd') &&
				advance() == 't' &&
				advance() == 'r')
			{
				assert(advance() == ARGLIST);

				if (c == 'c')
					copy_string("`class constructor`", 0); //-V666
				else
					copy_string("`class destructor`", 0); //-V666
				break;
			}
		}

		source = start;
		srcindx = startidx;

		copy_string("operator ", 9);

		start = target;

		copy_until1(ARGLIST);

		*target = 0;
		target = start;

		copy_op(start);

		kind = (kind & ~UM_KINDMASK) | UM_OPERATOR;
	    }
	    else if (c == 'o')
	    {
	    (void)advance();
		copy_string("operator ", 9);

		save_setqual = set_qual;
		set_qual = 0;

		copy_type(target, 0);

		set_qual = save_setqual;

		assert(input() == ARGLIST);
		kind = (kind & ~UM_KINDMASK) | UM_CONVERSION;
	    }
	    else if (c == 'v' || c == 'd')
	    {
		char tkind = c;

		c = advance();
		if (tkind == 'v' && c == 's')
		{
		    c = advance();
		    assert(c == 'f' || c == 'n');
		    (void)advance();

		    copy_string("__vdthk__", 9);
		    kind = (kind & ~UM_KINDMASK) | UM_VRDF_THUNK;
		}
		else if (c == 'c')
		{
		    c = advance();
		    assert(isdigit(c));
		    c = advance();
		    assert(c == '$');
		    c = advance();

		    copy_string("__thunk__ [", 11);
		    kind = (kind & ~UM_KINDMASK) |
		    (tkind == 'v' ? UM_THUNK : UM_DYN_THUNK);

		    copy_char(c);
		    copy_char(',');

		    while ((c = advance()) != '$')
			copy_char(c);

		    copy_char(',');

		    while ((c = advance()) != '$')
			copy_char(c);

		    copy_char(',');

		    while ((c = advance()) != '$')
			copy_char(c);

		    copy_char(']');

		    (void)advance();	/* skip last '$' */
		    return;
		}
	    }
	    else
	    {
		_error("Unknown special name");
	    }
	    break;

	case '_':

	    start = source;
	    startidx = srcindx;

	    if (advance() == '$')
	    {
		c = advance();

		/* At the moment there are five kind of special names:

		     frndl FL friend list
		     chtbl CH catch handler table
		     odtbl DC object destructor table
		     thrwl TL throw list
		     ectbl ETC exception context table */

		copy_char('_');
		copy_char('_');

		switch ((source[0]<<8)|source[1])
		{
		case 0x464c:	/* FL */
		    copy_string("frndl", 5);
		    kind |= UM_FRIEND_LIST;
		    break;

		case 0x4348:	/* CH */
		    copy_string("chtbl", 5);
		    kind |= UM_CTCH_HNDL_TBL;
		    break;

		case 0x4443:	/* DC */
		    copy_string("odtbl", 5);
		    kind |= UM_OBJ_DEST_TBL;
		    break;

		case 0x544c:	/* TL */
		    copy_string("thrwl", 5);
		    kind |= UM_THROW_LIST;
		    break;

		case 0x4543:	/* EC(T) */
		    copy_string("ectbl", 5);
		    kind |= UM_EXC_CTXT_TBL;
		    break;
		}

		copy_char('_');
		copy_char('_');
		copy_char(' ');

		while (c >= 'A' && c <= 'Z')
		    c = advance();

		assert(c == '$');
		assert(advance() == '@');
		(void)advance();

		copy_name(0);

		return;
	    }

	    source = start;
	    srcindx = startidx;

	    /* fall through... */

	default:		/* qualifier, member, plain */
	    copy_until2(QUALIFIER, ARGLIST);
	    break;
	}

	/* If we're processing a template name, then '$' is allowed to
	   end the name. */

	c = input();

	assert(c == 0 || c == QUALIFIER || c == ARGLIST);

	if (c == QUALIFIER)
	{
	    c = advance();

	    if (set_qual)
	    {
		prevqual = qualend;
		qualend = target;
	    }

	    copy_char(':');
	    copy_char(':');

	    if (c == 0)
		kind = (kind & ~UM_KINDMASK) | UM_VTABLE;
	}
	else
	{
	    break;
	}
    }
}

/* umKind unmangle(src, dest, maxlen, qualP, baseP, doArgs)

   This is the main entry-point for the unmangler code. To use it,
   pass the following arguments:

   src
     the source buffer, NULL terminated, which contains the mangled
     name. If this pointer is NULL, unmangle() will return
     UM_NOT_MANGLED.

   dest
     the destination buffer. If this pointer is NULL, unmangle() will
     return UM_ERROR.

   maxlen
     the maximum number of bytes which should be output to the
     destination buffer. Remember to account for the NULL that will be
     output at the end of the mangled name.

     It is impossible to know beforehand exactly how long a mangled
     name should be, but due to restrictions in the length of linker
     names, imposed by the OMF format, a buffer of at least 2K bytes
     or longer should always be sufficient.

     If the size of the buffer is insufficient, unmangle() will return
     with the flag UM_BUFOVRFLW set in the return code. Any other
     flags set in the return code will reflect whatever information
     unmangle() was able to determine before the overflow occurred.

   qualP
     if non-NULL, this argument should point to the address of a
     buffer large enough to contain the qualifier part of the
     unmangled name. For example, if the unmangled name is
     "foo::bar::baz", then the qualifier would be "foo::bar".

     Thus, this buffer should always be at least as large as the
     destination buffer, in order to ensure that memory overwrites
     never occur.

   baseP
     if non-NULL, this argument should point to the address of a
     buffer large enough to contain the basename part of the unmangled
     name. For example, if the unmangled name is "foo::bar::baz", then
     the basename would be "baz". See the documentation of "qualP" for
     further notes on the required length of this buffer.

   doArgs
     if this argument is non-0 (aka TRUE), it means that when
     unmangling a function name, its arguments should also be
     unmangled as part of the output name.  Otherwise, only the name
     will be unmangled, and not the arguments.

   The return code of this function contains a series of flags, some
   of which are mutually exclusive, and some of which represent the
   status of the unmangled name. These flags are:

   UM_NOT_MANGLED
     If the return value equals this flag, then it is the only flag
     which will be set, all other values being irrelevant.

     The kind of symbol (mutually exclusive)

     UM_UNKNOWN     Symbol of unknown type
     UM_FUNCTION    Global function, or member function
     UM_CONSTRUCTOR Class donstructor function
     UM_DESTRUCTOR  Class destructor function
     UM_OPERATOR    Global operator, or member operator
     UM_CONVERSION  Member conversion operator
     UM_DATA        Class static data member
     UM_THUNK       (16-bit only, no longer used)
     UM_TPDSC       Type descriptor object (RTTI)
     UM_VTABLE      Class virtual table
     UM_VRDF_THUNK  Virtual table thunk (special)

   UM_KINDMASK
     This mask can be used to exclude all other flags from the return
     type, except the symbol kind.

   Modifiers (not mutually exclusive)

     UM_QUALIFIED
       A member symbol, either of a class or of a namespace

     UM_TEMPLATE
       A template specialization symbol

   Modifiers (mutually exclusive)

     UM_VIRDEF_FLAG   Virdef flag (special)
     UM_FRIEND_LIST   Friend list (special)
     UM_CTCH_HNDL_TBL Catch handler table (exception handling)
     UM_OBJ_DEST_TBL  Object destructor table (exception handling)
     UM_THROW_LIST    Throw list (exception handling)
     UM_EXC_CTXT_TBL  Exception context table (exception handling)
     UM_LINKER_PROC   Special linker procedure (#pragma package)

     UM_SPECMASK
       Special flags mask. Use this to extract only these special,
       mutually exclusive, flags.

     UM_MODMASK
       This mask can be used to access any of the symbol modifiers,
       whether mutually exclusive or not.

   Error flags (not mutually exclusive)

     UM_BUFOVRFLW  The output buffer has been overflowed
     UM_HASHTRUNC  The input name was truncated by a hash code
     UM_ERROR      Some other error has occurred

     UM_ERRMASK     Use this mask to examine only the error flags

   Note on exceptional conditions: Sometimes a mangled name does not
   have the correct format. This can happen if garbage code is passed
   in, or a mangled name from a different, or older product, is used.
   In this case, you will notice a number enclosed in curly-braces at
   the point in the name where the fault was detected.

   For example, a false name like "@foo@$z" will generate an error
   like "foo::{853}...", because "$z" does not represent a valid
   special function name. In this case, the number 853 represents the
   line in UM.C where the error was found.

   If you are debugging a problem with unmangling in a case where
   examining the mangled name under the debugger is not convenient,
   you can tell the unmangler to output the mangled form of the name
   in the output buffer by setting the environment variable
   SHOW_TROUBLED_NAME to any textual value. In that case, the output
   buffer for the example above would contain the string "foo::{853:
   @foo@$z}".

   Lastly, this code is subject to change at any time. Although
   borland.com intends to keep the API and function signature intact
   from release to release, nothing is guaranteed. Making this source
   code visible in no way implies any guarantee as to its
   functionality or accuracy.  Caveat Programmor.
*/

umKind UMAPI unmangle (char *	src,
		       char *	dest,
		       unsigned maxlen,
		       char *	qualP,
		       char *	baseP,
		       int	doArgs)
{
    char * p;
    int    len;
#if defined(PASCAL_LOWERCASE)
    int    i;
#endif

    assert(maxlen <= MAXBUFFLEN);

    /* Quick check to see whether this name is even mangled or not. */

    if (src == 0)
	return UM_NOT_MANGLED;

    if (dest == 0)
	return UM_ERROR;

    if (*src != '@')
    {
	strncpy(dest, src, maxlen);
	dest[maxlen - 1] = 0;
	return UM_NOT_MANGLED;
    }

    /* Initialize globals */

    len = (int)strlen(src);

    if (len > 254)
        return UM_ERROR;
    else if (len == 254)  /* old hash for bcc version 0x600 and earlier */
        hashstart = 250;
    else if (len == 253)  /* new hash for bcc version 0x610 and later */
        hashstart = 231;  /* 253 - 22 bytes of based 64 encoded MD5 */
    else
        hashstart = MAXBUFFLEN;

    savechar = 0;
    srcindx = 1;
    srcbase = src;

    /* All mangled names begin with an '@' character. */

    src++;			/* skip the initial '@' */
    len--;

    /* Now that we have the length, let's check for Microsoft
       compatible fastcall names, which are of the form:
       @funcName@<one or more digits indicating size of all
       parameters> */

    if (isdigit(src[len-1]))
    {
	p = src + len - 2;
	while (isdigit(*p))
	    p--;
	if (*p == '@' && p != (src-1))
	{
	    /* We have a Microsoft fastcall name */

	    strncpy(dest, src-1, maxlen);
	    dest[maxlen - 1] = 0;
	    return UM_NOT_MANGLED;
	}
    }

#if defined(PASCAL_LOWERCASE)
	/* Slightly ugly code for turning an uppercase pascal name
           into a lowercase equivalent. */

    for (p = src, i = len; --i >= 0; p++)
    {
	if (*p >= 'a' && *p <= 'z')
	    goto NOT_PASCAL;
    }

    for (p = src, i = len; --i >= 0; p++)
	*p = (char)tolower(*p);

  NOT_PASCAL:
#endif

    /* This is at LEAST a member name, if not a fully mangled template
       or function name. So, begin outputting the subnames. We set up
       the pointers in globals so that we don't have to pass
       everything around all the time. */

    kind      = UM_UNKNOWN;
    source    = src;
    prevqual  =
    qualend   =
    base_name =
    base_end  = 0;
    set_qual  = 1;
    target    =
    targbase  = dest;
    targend   = targbase + (maxlen - 1);

    /* If anyone long jumps, it means a hash code was reached, the
       destination buffer reached its end, or the source buffer was
       terminated. */

    if (setjmp(jmpb))
    {
	/* If we reached this exit point because the target did not
	   contain enough space, or a hash code was reached, then
	   output a trailer to let the user know that there was more
	   data in the source string. */

	if (*source != 0 || savechar)
	{
	    if (target + 3 < targend)
	    {
		copy_char('.');
		copy_char('.');
		copy_char('.');
	    }
	    else
	    {
		*--target = '.';
		*--target = '.';
		*--target = '.';
	    }
	}

	goto FINISH;
    }

    /* Start outputting the qualifier names and the base name. */

    namebase = target;

    copy_name(0);
    set_qual = 0;
    base_end = target;

    if ((kind & UM_KINDMASK) == UM_TPDSC ||
	kind & UM_SPECMASK)
    {
	p = strchr(namebase, ' ');
	if (p == NULL) return UM_ERROR;
	namebase = p + 1;
    }

    if ((kind & UM_KINDMASK) == UM_CONSTRUCTOR ||
	(kind & UM_KINDMASK) == UM_DESTRUCTOR)
    {
	char * start;

	if ((kind & UM_KINDMASK) == UM_DESTRUCTOR)
	    copy_char('~');

#ifndef UM_FIXES
	assert(qualend);
#else
	if (!qualend)
	{
	    /* It's a bcc-created static constructor??
	       give it a name. */

	    copy_string("unknown", 7);
	}
	else
#endif
	{

	if (! prevqual)
	    start = namebase;
	else
	    start = prevqual + 2;

	len = (int)(qualend - start);

	strncpy(buff, start, len);
	buff[len] = 0;
	copy_string(buff, len);
	}
    }

    /* If there's a function argument list, copy it over in expanded
       form. */

    if (input() == ARGLIST && doArgs) /* function args */
    {
	char c;

	c = advance();
	assert(c == 'q' || c == 'x' || c == 'w');

	/* Output the function parameters, and return type in the case
	   of template function specializations. */

	set_qual = 0;
	adjust_quals = 1;

	copy_type(namebase, 0);

	if ((kind & UM_KINDMASK) == UM_UNKNOWN)
	    kind |= UM_FUNCTION;
    }
    else if ((kind & UM_KINDMASK) == UM_UNKNOWN)
    {
	kind |= UM_DATA;
    }
    else if (vtbl_flags[0])
    {
	copy_char(' ');
	copy_char('(');
	copy_string(vtbl_flags, 0);
	copy_char(')');
    }

  FINISH:
    /* Put some finishing touches on the kind of this entity. */

    if (qualend)
	kind |= UM_QUALIFIED;

    /* Put a terminator on the target. */

    *target = 0;

    /* If the user wanted the qaulifier and base name saved, then do
       it now. */

    if (! (kind & UM_ERRMASK))
    {
	if (qualP && qualend)
	{
	    len = (int)(qualend - namebase);
	    strncpy(qualP, namebase, len);
	    qualP[len] = 0;
	}

	if (baseP && base_name)
	{
	    len = (int)(base_end - base_name);
	    strncpy(baseP, base_name, len);
	    baseP[len] = 0;
	}
    }

    return kind;
}
/*
int UMAPI setUnmangleMode(int mode)
{
    return 0;
}*/

#ifdef STANDALONE

int main(int argc, char *argv[])
{
    char   name[8192];
    char   result[1024];
    char   buff1[1024];
    char   buff2[1024];
    int    i;
    FILE * fp;
    int    code;

    if (argc == 1)
    {
	printf("Usage: %s file.lst\n", argv[0]);
	printf("where file.lst contains mangled strings, one per line.\n");
	return 1;
    }

    fp = fopen(argv[1], "r");
    if (! fp)
    {
	printf("file %s not found\n", argv[1]);
	return 1;
    }

    while (fgets(name, 8192, fp) != 0)
    {
        i = strlen(name);
        if (name[0] == '#')
            continue;      /* ignore comments */
        if (name[i-1] == '\n')
            name[--i] = 0; /* strip return */
        if (name[i-1] == '\r')
            name[--i] = 0; /* strip dos-style return */
        if (i <= 1)
            continue;      /* nothing to unmangle */
	printf("%-40s\n", name);
	result[0] = buff1[0] = buff2[0] = 0;

	code = unmangle(name, result, sizeof(result), buff1, buff2, 1);

	if (code == UM_NOT_MANGLED)
	{
	    printf("(not mangled)\n\n");
	    continue;
	}

	if (code & UM_BUFOVRFLW)
	    printf("[buffer overflowed] ");
	if (code & UM_HASHTRUNC)
	    printf("[truncated by hash] ");
	if (code & UM_ERROR)
	    printf("[error occurred] ");

	if (code & UM_QUALIFIED)
	    printf("qualified ");
	if (code & UM_TEMPLATE)
	    printf("template ");

	if (code & UM_VIRDEF_FLAG)
	    printf("virdef_flag ");
	if (code & UM_FRIEND_LIST)
	    printf("friend_list ");
	if (code & UM_CTCH_HNDL_TBL)
	    printf("catch_handler_table ");
	if (code & UM_OBJ_DEST_TBL)
	    printf("object_destructor_table ");
	if (code & UM_THROW_LIST)
	    printf("throw_list ");
	if (code & UM_EXC_CTXT_TBL)
	    printf("exception_context_table ");

	code &= UM_KINDMASK;

	switch (code)
	{
	case UM_FUNCTION:    printf("function"); break;
	case UM_CONSTRUCTOR: printf("constructor"); break;
	case UM_DESTRUCTOR:  printf("destructor"); break;
	case UM_OPERATOR:    printf("operator"); break;
	case UM_CONVERSION:  printf("conv_operator"); break;
	case UM_DATA:	     printf("data"); break;
	case UM_THUNK:	     printf("thunk"); break;
	case UM_TPDSC:	     printf("typedesc"); break;
	case UM_VTABLE:      printf("vtable"); break;
	case UM_VRDF_THUNK:  printf("virdef_thunk"); break;
	case UM_UNKNOWN:     printf("unknown"); break;
	default:
	    printf("not mangled");
	    break;
	}

	printf(" '%s' '%s' '%s'\n\n", result, buff1, buff2);
    }

    fclose(fp);

    return 0;
}

#endif
