/*
 *  Demangle VC++ symbols into C function prototypes
 *
 *  Copyright 2000 Jon Griffiths
 *            2004 Eric Pouech
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "undname.h"

#ifdef VMP_GNU
#define _strdup strdup
#define TRUE  (1)
#define FALSE (0)
#endif

#ifndef UND_TRACE
#define TRACE(...) do {} while(0)
#else
#define TRACE(...) printf(__VA_ARGS__)
#endif
#define WARN TRACE
#define ERR TRACE

typedef void* (*malloc_func_t)(size_t);
typedef void  (*free_func_t)(void*);

/* TODO:
 * - document a bit (grammar + functions)
 * - back-port this new code into tools/winedump/msmangle.c
 */

/* How data types modifiers are stored:
 * M (in the following definitions) is defined for 
 * 'A', 'B', 'C' and 'D' as follows
 *      {<A>}:  ""
 *      {<B>}:  "const "
 *      {<C>}:  "volatile "
 *      {<D>}:  "const volatile "
 *
 *      in arguments:
 *              P<M>x   {<M>}x*
 *              Q<M>x   {<M>}x* const
 *              A<M>x   {<M>}x&
 *      in data fields:
 *              same as for arguments and also the following
 *              ?<M>x   {<M>}x
 *              
 */

struct array
{
    unsigned            start;          /* first valid reference in array */
    unsigned            num;            /* total number of used elts */
    unsigned            max;
    unsigned            alloc;
    char**              elts;
};

/* Structure holding a parsed symbol */
struct parsed_symbol
{
    unsigned            flags;          /* the UNDNAME_ flags used for demangling */
    malloc_func_t       mem_alloc_ptr;  /* internal allocator */
    free_func_t         mem_free_ptr;   /* internal deallocator */

    const char*         current;        /* pointer in input (mangled) string */
    char*               result;         /* demangled string */

    struct array        names;          /* array of names for back reference */
    struct array        stack;          /* stack of parsed strings */

    void*               alloc_list;     /* linked list of allocated blocks */
    unsigned            avail_in_first; /* number of available bytes in head block */
	size_t*             name_pos;
};
BOOL symbol_demangle(struct parsed_symbol* sym);

/* Type for parsing mangled types */
struct datatype_t
{
    const char*         left;
    const char*         right;
};

/******************************************************************
 *		und_alloc
 *
 * Internal allocator. Uses a simple linked list of large blocks
 * where we use a poor-man allocator. It's fast, and since all
 * allocation is pool, memory management is easy (esp. freeing).
 */
static void*    und_alloc(struct parsed_symbol* sym, unsigned int len)
{
    void*       ptr;

#define BLOCK_SIZE      1024
#define AVAIL_SIZE      (1024 - sizeof(void*))

    if (len > AVAIL_SIZE)
    {
        /* allocate a specific block */
        ptr = sym->mem_alloc_ptr(sizeof(void*) + len);
        if (!ptr) return NULL;
        *(void**)ptr = sym->alloc_list;
        sym->alloc_list = ptr;
        sym->avail_in_first = 0;
        ptr = (char*)sym->alloc_list + sizeof(void*);
    }
    else 
    {
        if (len > sym->avail_in_first)
        {
            /* add a new block */
            ptr = sym->mem_alloc_ptr(BLOCK_SIZE);
            if (!ptr) return NULL;
            *(void**)ptr = sym->alloc_list;
            sym->alloc_list = ptr;
            sym->avail_in_first = AVAIL_SIZE;
        }
        /* grab memory from head block */
        ptr = (char*)sym->alloc_list + BLOCK_SIZE - sym->avail_in_first;
        sym->avail_in_first -= len;
    }
    return ptr;
#undef BLOCK_SIZE
#undef AVAIL_SIZE
}

/******************************************************************
 *		und_free
 * Frees all the blocks in the list of large blocks allocated by
 * und_alloc.
 */
static void und_free_all(struct parsed_symbol* sym)
{
    void*       next;

    while (sym->alloc_list)
    {
        next = *(void**)sym->alloc_list;
        if(sym->mem_free_ptr) sym->mem_free_ptr(sym->alloc_list);
        sym->alloc_list = next;
    }
    sym->avail_in_first = 0;
}

/******************************************************************
 *		str_array_init
 * Initialises an array of strings
 */
static void str_array_init(struct array* a)
{
    a->start = a->num = a->max = a->alloc = 0;
    a->elts = NULL;
}

/******************************************************************
 *		str_array_push
 * Adding a new string to an array
 */
static BOOL str_array_push(struct parsed_symbol* sym, const char* ptr, int len,
                           struct array* a)
{
    char**      newc;

    assert(ptr);
    assert(a);
	if (ptr == NULL || a == NULL) 
		return FALSE;

    if (!a->alloc)
    {
        newc = und_alloc(sym, (a->alloc = 32) * sizeof(a->elts[0]));
        if (!newc) return FALSE;
        a->elts = newc;
    }
    else if (a->max >= a->alloc)
    {
        newc = und_alloc(sym, (a->alloc * 2) * sizeof(a->elts[0]));
        if (!newc) return FALSE;
        memcpy(newc, a->elts, a->alloc * sizeof(a->elts[0]));
        a->alloc *= 2;
        a->elts = newc;
    }
    if (len == -1) len = (int)strlen(ptr);
    a->elts[a->num] = und_alloc(sym, len + 1);
    assert(a->elts[a->num]);
    memcpy(a->elts[a->num], ptr, len);
    a->elts[a->num][len] = '\0'; 
    if (++a->num >= a->max) a->max = a->num;
    {
        int i;
        char c;

        for (i = a->max - 1; i >= 0; i--)
        {
            c = '>';
            if ((unsigned int)i < a->start) c = '-';
            else if ((unsigned int)i >= a->num) c = '}';
            TRACE("%p\t%d%c %s\n", a, i, c, a->elts[i]);
        }
    }

    return TRUE;
}

/******************************************************************
 *		str_array_get_ref
 * Extracts a reference from an existing array (doing proper type
 * checking)
 */
static char* str_array_get_ref(struct array* cref, unsigned idx)
{
    assert(cref);
    if (cref->start + idx >= cref->max)
    {
        WARN("Out of bounds: %p %d + %d >= %d\n", 
              cref, cref->start, idx, cref->max);
        return NULL;
    }
    TRACE("Returning %p[%d] => %s\n", 
          cref, idx, cref->elts[cref->start + idx]);
    return cref->elts[cref->start + idx];
}

/******************************************************************
 *		str_printf
 * Helper for printf type of command (only %s and %c are implemented) 
 * while dynamically allocating the buffer
 */
static char* str_printf(struct parsed_symbol* sym, const char* format, ...)
{
    va_list      args;
    unsigned int len = 1, i, sz;
    char*        tmp;
    char*        p;
    char*        t;

    va_start(args, format);
    for (i = 0; format[i]; i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 's': t = va_arg(args, char*); if (t) len += (unsigned int)strlen(t); break;
            case 'c': (void)va_arg(args, int); len++; break;
            default: i--; /* fall through */
            case '%': len++; break;
            }
        }
        else len++;
    }
    va_end(args);
    if (!(tmp = und_alloc(sym, len))) return NULL;
    va_start(args, format);
    for (p = tmp, i = 0; format[i]; i++)
    {
        if (format[i] == '%')
        {
            switch (format[++i])
            {
            case 's':
                t = va_arg(args, char*);
                if (t)
                {
                    sz = (unsigned int)strlen(t);
                    memcpy(p, t, sz);
                    p += sz;
                }
                break;
            case 'c':
                *p++ = (char)va_arg(args, int);
                break;
            default: i--; /* fall through */
            case '%': *p++ = '%'; break;
            }
        }
        else *p++ = format[i];
    }
    va_end(args);
    *p = '\0';
    return tmp;
}

/* forward declaration */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              struct array* pmt, BOOL in_args);

static char* get_number(struct parsed_symbol* sym)
{
    char*       ptr;
    BOOL        sgn = FALSE;

    if (*sym->current == '?')
    {
        sgn = TRUE;
        sym->current++;
    }
    if (*sym->current >= '0' && *sym->current <= '8')
    {
        ptr = und_alloc(sym, 3);
		if (!ptr) return ptr;
        if (sgn) ptr[0] = '-';
        ptr[sgn ? 1 : 0] = *sym->current + 1;
        ptr[sgn ? 2 : 1] = '\0';
        sym->current++;
    }
    else if (*sym->current == '9')
    {
        ptr = und_alloc(sym, 4);
		if (!ptr) return ptr;
        if (sgn) ptr[0] = '-';
        ptr[sgn ? 1 : 0] = '1';
        ptr[sgn ? 2 : 1] = '0';
        ptr[sgn ? 3 : 2] = '\0';
        sym->current++;
    }
    else if (*sym->current >= 'A' && *sym->current <= 'P')
    {
        int ret = 0;

        while (*sym->current >= 'A' && *sym->current <= 'P')
        {
            ret *= 16;
            ret += *sym->current++ - 'A';
        }
        if (*sym->current != '@') return NULL;

        ptr = und_alloc(sym, 17);
		if (!ptr) return ptr;
        sprintf(ptr, "%s%u", sgn ? "-" : "", ret);
        sym->current++;
    }
    else return NULL;
    return ptr;
}

/******************************************************************
 *		get_args
 * Parses a list of function/method arguments, creates a string corresponding
 * to the arguments' list.
 */
static char* get_args(struct parsed_symbol* sym, struct array* pmt_ref, BOOL z_term, 
                      char open_char, char close_char)

{
    struct datatype_t   ct;
    struct array        arg_collect;
    char*               args_str = NULL;
    char*               last;
    unsigned int        i;

    str_array_init(&arg_collect);

    /* Now come the function arguments */
    while (*sym->current)
    {
        /* Decode each data type and append it to the argument list */
        if (*sym->current == '@')
        {
            sym->current++;
            break;
        }
		if (close_char == '>' && sym->current[0] == '$' && sym->current[1] == '$') {
			char c = sym->current[2];
			if (c == 'V') {
				sym->current += 3;
				continue;
			} else if (c == 'Z') {
				sym->current += 3;
				continue;
			}
		}

        if (!demangle_datatype(sym, &ct, pmt_ref, TRUE))
            return NULL;
        /* 'void' terminates an argument list in a function */
        if (z_term && !strcmp(ct.left, "void")) break;
        if (!str_array_push(sym, str_printf(sym, "%s%s", ct.left, ct.right), -1,
                            &arg_collect))
            return NULL;
        if (!strcmp(ct.left, "...")) break;
    }
    /* Functions are always terminated by 'Z'. If we made it this far and
     * don't find it, we have incorrectly identified a data type.
     */
    if (z_term && *sym->current++ != 'Z') return NULL;

    if (arg_collect.num == 0 || 
        (arg_collect.num == 1 && !strcmp(arg_collect.elts[0], "void")))        
        return str_printf(sym, "%cvoid%c", open_char, close_char);
    for (i = 1; i < arg_collect.num; i++)
    {
        args_str = str_printf(sym, "%s,%s", args_str, arg_collect.elts[i]);
    }

    last = args_str ? args_str : arg_collect.elts[0];
    if (close_char == '>' && last[strlen(last) - 1] == '>')
        args_str = str_printf(sym, "%c%s%s %c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    else
        args_str = str_printf(sym, "%c%s%s%c", 
                              open_char, arg_collect.elts[0], args_str, close_char);
    
    return args_str;
}

/******************************************************************
 *		get_modifier
 * Parses the type modifier. Always returns static strings.
 */
static BOOL get_modifier(struct parsed_symbol *sym, const char **ret, const char **ptr_modif)
{
    *ptr_modif = NULL;
    if (*sym->current == 'E')
    {
        sym->current++;
		if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS)) {
	        *ptr_modif = "__ptr64";
			if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
				*ptr_modif += 2;
		}
    }
    switch (*sym->current++)
    {
    case 'A': *ret = NULL; break;
    case 'B': *ret = "const"; break;
    case 'C': *ret = "volatile"; break;
    case 'D': *ret = "const volatile"; break;
    default: return FALSE;
    }
    return TRUE;
}

static BOOL get_modified_type(struct datatype_t *ct, struct parsed_symbol* sym,
                              struct array *pmt_ref, char modif, BOOL in_args)
{
    const char* modifier;
    const char* str_modif;
	const char* ptr_modif = "";

    if (*sym->current == 'E')
    {
        sym->current++;
		if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS)) {
	        ptr_modif = "__ptr64";
			if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
				ptr_modif += 2;
			ptr_modif = str_printf(sym, " %s", ptr_modif);
		}
    }
	if (*sym->current == 'I')
	{
		sym->current++;
		if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS)) {
			const char *tmp_modif = "__restrict";
			if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
				tmp_modif += 2;
			ptr_modif = str_printf(sym, "%s %s", ptr_modif, tmp_modif);
		}
	}

    switch (modif)
    {
    case 'A': str_modif = str_printf(sym, " &%s", ptr_modif); break;
    case 'B': str_modif = str_printf(sym, " &%s volatile", ptr_modif); break;
    case 'P': str_modif = str_printf(sym, " *%s", ptr_modif); break;
    case 'Q': str_modif = str_printf(sym, " *%s const", ptr_modif); break;
    case 'R': str_modif = str_printf(sym, " *%s volatile", ptr_modif); break;
    case 'S': str_modif = str_printf(sym, " *%s const volatile", ptr_modif); break;
    case '?': str_modif = ""; break;
    default: return FALSE;
    }
	if (*sym->current == 'F')
	{
		sym->current++;
		if (!(sym->flags & UNDNAME_NO_MS_KEYWORDS)) {
			const char *tmp_modif = "__unaligned";
			if (sym->flags & UNDNAME_NO_LEADING_UNDERSCORES)
				tmp_modif += 2;
			str_modif = str_printf(sym, " %s%s", tmp_modif, str_modif);
		}
	}

	if (str_modif == NULL) return FALSE;

    if (get_modifier(sym, &modifier, &ptr_modif))
    {
        unsigned            mark = sym->stack.num;
        struct datatype_t   sub_ct;

        /* multidimensional arrays */
        if (*sym->current == 'Y')
        {
            const char* n1;
            int num;

            sym->current++;
            if (!(n1 = get_number(sym))) return FALSE;
            num = atoi(n1);
			if (num > 128) num = 128; //should be enough

            if (str_modif[0] == ' ' && !modifier)
                str_modif++;

            if (modifier)
            {
                str_modif = str_printf(sym, " (%s%s)", modifier, str_modif);
                modifier = NULL;
            }
            else
                str_modif = str_printf(sym, " (%s)", str_modif);

            while (num--)
                str_modif = str_printf(sym, "%s[%s]", str_modif, get_number(sym));
        }

        /* Recurse to get the referred-to type */
        if (!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
            return FALSE;
        if (modifier)
            ct->left = str_printf(sym, "%s %s%s", sub_ct.left, modifier, str_modif );
        else
        {
            /* don't insert a space between duplicate '*' */
            if (!in_args && str_modif && str_modif[0] && str_modif[1] == '*' && sub_ct.left[strlen(sub_ct.left)-1] == '*')
                str_modif++;
            ct->left = str_printf(sym, "%s%s", sub_ct.left, str_modif );
        }
        ct->right = sub_ct.right;
        sym->stack.num = mark;
    }
    return TRUE;
}

/******************************************************************
 *             get_literal_string
 * Gets the literal name from the current position in the mangled
 * symbol to the first '@' character. It pushes the parsed name to
 * the symbol names stack and returns a pointer to it or NULL in
 * case of an error.
 */
static char* get_literal_string(struct parsed_symbol* sym)
{
    const char *ptr = sym->current;

    do {
		char c = *sym->current;
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '$' || c == '<' || c == '>')) {
            TRACE("Failed at '%c' in %s\n", *sym->current, ptr);
            return NULL;
        }
    } while (*++sym->current != '@');
    sym->current++;
    if (!str_array_push(sym, ptr, (int)(sym->current - 1 - ptr), &sym->names))
        return NULL;

    return str_array_get_ref(&sym->names, sym->names.num - sym->names.start - 1);
}

/******************************************************************
 *		get_template_name
 * Parses a name with a template argument list and returns it as
 * a string.
 * In a template argument list the back reference to the names
 * table is separately created. '0' points to the class component
 * name with the template arguments.  We use the same stack array
 * to hold the names but save/restore the stack state before/after
 * parsing the template argument list.
 */
static char* get_template_name(struct parsed_symbol* sym)
{
    char *name, *args;
    unsigned num_mark = sym->names.num;
    unsigned start_mark = sym->names.start;
    unsigned stack_mark = sym->stack.num;
    struct array array_pmt;

    sym->names.start = sym->names.num;
    if (!(name = get_literal_string(sym)))
        return FALSE;
    str_array_init(&array_pmt);
    args = get_args(sym, &array_pmt, FALSE, '<', '>');
    if (args != NULL)
        name = str_printf(sym, "%s%s", name, args);
    sym->names.num = num_mark;
    sym->names.start = start_mark;
    sym->stack.num = stack_mark;
    return name;
}

/******************************************************************
 *		get_class
 * Parses class as a list of parent-classes, terminated by '@' and stores the
 * result in 'a' array. Each parent-classes, as well as the inner element
 * (either field/method name or class name), are represented in the mangled
 * name by a literal name ([a-zA-Z0-9_]+ terminated by '@') or a back reference
 * ([0-9]) or a name with template arguments ('?$' literal name followed by the
 * template argument list). The class name components appear in the reverse
 * order in the mangled name, e.g aaa@bbb@ccc@@ will be demangled to
 * ccc::bbb::aaa
 * For each of these class name components a string will be allocated in the
 * array.
 */
static BOOL get_class(struct parsed_symbol* sym)
{
    char* name = NULL;

    while (*sym->current != '@')
    {
        switch (*sym->current)
        {
        case '\0': return FALSE;

        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9':
            name = str_array_get_ref(&sym->names, *sym->current++ - '0');
            break;
        case '?':
            switch (*++sym->current)
            {
            case '$':
                sym->current++;
                if ((name = get_template_name(sym)) &&
                    !str_array_push(sym, name, -1, &sym->names))
                    return FALSE;
                break;
            case '?':
                {
                    struct array stack = sym->stack;
                    unsigned int start = sym->names.start;
                    unsigned int num = sym->names.num;

                    str_array_init( &sym->stack );
                    if (symbol_demangle( sym )) name = str_printf( sym, "`%s'", sym->result );
                    sym->names.start = start;
                    sym->names.num = num;
                    sym->stack = stack;
                }
                break;
			case 'A':
				if (!get_literal_string(sym))
					return FALSE;
				name = str_printf(sym, "`anonymous namespace'");
				sym->names.elts[sym->names.num - 1] = name;
				break;
            default:
                if (!(name = get_number( sym ))) return FALSE;
                name = str_printf( sym, "`%s'", name );
                break;
            }
            break;
        default:
            name = get_literal_string(sym);
            break;
        }
        if (!name || !str_array_push(sym, name, -1, &sym->stack))
            return FALSE;
    }
    sym->current++;
    return TRUE;
}

/******************************************************************
 *		get_class_string
 * From an array collected by get_class in sym->stack, constructs the
 * corresponding (allocated) string
 */
static char* get_class_string(struct parsed_symbol* sym, int start)
{
    int          i;
    unsigned int len, sz;
    char*        ret;
    struct array *a = &sym->stack;

    for (len = 0, i = start; i < (int)a->num; i++)
    {
        assert(a->elts[i]);
        len += (unsigned int)(2 + strlen(a->elts[i]));
    }
    if (!(ret = und_alloc(sym, len - 1))) return NULL;
    for (len = 0, i = a->num - 1; i >= start; i--)
    {
        sz = (unsigned int)strlen(a->elts[i]);
        memcpy(ret + len, a->elts[i], sz);
        len += sz;
        if (i > start)
        {
            ret[len++] = ':';
            ret[len++] = ':';
        }
    }
    ret[len] = '\0';
    return ret;
}

/******************************************************************
 *            get_class_name
 * Wrapper around get_class and get_class_string.
 */
static char* get_class_name(struct parsed_symbol* sym)
{
    unsigned    mark = sym->stack.num;
    char*       s = NULL;

    if (get_class(sym))
        s = get_class_string(sym, mark);
    sym->stack.num = mark;
    return s;
}

/******************************************************************
 *		get_calling_convention
 * Returns a static string corresponding to the calling convention described
 * by char 'ch'. Sets export to TRUE iff the calling convention is exported.
 */
static BOOL get_calling_convention(char ch, const char** call_conv,
                                   const char** exported, unsigned flags)
{
    *call_conv = *exported = NULL;

    if (!(flags & (UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ALLOCATION_LANGUAGE)))
    {
        if ((ch - 'A') & 1) *exported = "__dll_export ";
        switch (ch)
        {
        case 'A': case 'B': *call_conv = "__cdecl"; break;
        case 'C': case 'D': *call_conv = "__pascal"; break;
        case 'E': case 'F': *call_conv = "__thiscall"; break;
        case 'G': case 'H': *call_conv = "__stdcall"; break;
        case 'I': case 'J': *call_conv = "__fastcall"; break;
        case 'K': case 'L': break;
        case 'M': *call_conv = "__clrcall"; break;
        default: ERR("Unknown calling convention %c\n", ch); return FALSE;
        }

		if (flags & UNDNAME_NO_LEADING_UNDERSCORES) {
			if (*exported)
				*exported += 2;
			if (*call_conv)
				*call_conv += 2;
		}
    }
    return TRUE;
}

/*******************************************************************
 *         get_simple_type
 * Return a string containing an allocated string for a simple data type
 */
static const char* get_simple_type(char c)
{
    const char* type_string;
    
    switch (c)
    {
    case 'C': type_string = "signed char"; break;
    case 'D': type_string = "char"; break;
    case 'E': type_string = "unsigned char"; break;
    case 'F': type_string = "short"; break;
    case 'G': type_string = "unsigned short"; break;
    case 'H': type_string = "int"; break;
    case 'I': type_string = "unsigned int"; break;
    case 'J': type_string = "long"; break;
    case 'K': type_string = "unsigned long"; break;
    case 'M': type_string = "float"; break;
    case 'N': type_string = "double"; break;
    case 'O': type_string = "long double"; break;
    case 'X': type_string = "void"; break;
    case 'Z': type_string = "..."; break;
    default:  type_string = NULL; break;
    }
    return type_string;
}

/*******************************************************************
 *         get_extended_type
 * Return a string containing an allocated string for a simple data type
 */
static const char* get_extended_type(char c)
{
    const char* type_string;
    
    switch (c)
    {
    case 'D': type_string = "__int8"; break;
    case 'E': type_string = "unsigned __int8"; break;
    case 'F': type_string = "__int16"; break;
    case 'G': type_string = "unsigned __int16"; break;
    case 'H': type_string = "__int32"; break;
    case 'I': type_string = "unsigned __int32"; break;
    case 'J': type_string = "__int64"; break;
    case 'K': type_string = "unsigned __int64"; break;
    case 'L': type_string = "__int128"; break;
    case 'M': type_string = "unsigned __int128"; break;
    case 'N': type_string = "bool"; break;
    case 'W': type_string = "wchar_t"; break;
    default:  type_string = NULL; break;
    }
    return type_string;
}

/*******************************************************************
 *         demangle_datatype
 *
 * Attempt to demangle a C++ data type, which may be datatype.
 * a datatype type is made up of a number of simple types. e.g:
 * char** = (pointer to (pointer to (char)))
 */
static BOOL demangle_datatype(struct parsed_symbol* sym, struct datatype_t* ct,
                              struct array* pmt_ref, BOOL in_args)
{
    char                dt;
    BOOL                add_pmt = TRUE;

    assert(ct);
    ct->left = ct->right = NULL;
    
    switch (dt = *sym->current++)
    {
    case '_':
        /* MS type: __int8,__int16 etc */
        ct->left = get_extended_type(*sym->current++);
        break;
    case 'C': case 'D': case 'E': case 'F': case 'G':
    case 'H': case 'I': case 'J': case 'K': case 'M':
    case 'N': case 'O': case 'X': case 'Z':
        /* Simple data types */
        ct->left = get_simple_type(dt);
        add_pmt = FALSE;
        break;
    case 'T': /* union */
    case 'U': /* struct */
    case 'V': /* class */
    case 'Y': /* cointerface */
        /* Class/struct/union/cointerface */
        {
            const char* struct_name = NULL;
            const char* type_name = NULL;

            if (!(struct_name = get_class_name(sym)))
                goto done;
            if (!(sym->flags & UNDNAME_NO_COMPLEX_TYPE)) 
            {
                switch (dt)
                {
                case 'T': type_name = "union ";  break;
                case 'U': type_name = "struct "; break;
                case 'V': type_name = "class ";  break;
                case 'Y': type_name = "cointerface "; break;
                }
            }
            ct->left = str_printf(sym, "%s%s", type_name, struct_name);
        }
        break;
    case '?':
        /* not all the time is seems */
        if (in_args)
        {
            const char*   ptr;
            if (!(ptr = get_number(sym))) goto done;
            ct->left = str_printf(sym, "`template-parameter-%s'", ptr);
        }
        else
        {
            if (!get_modified_type(ct, sym, pmt_ref, '?', in_args)) goto done;
        }
        break;
    case 'A': /* reference */
    case 'B': /* volatile reference */
        if (!get_modified_type(ct, sym, pmt_ref, dt, in_args)) goto done;
        break;
    case 'Q': /* const pointer */
		if (isdigit(*sym->current))
		{
			switch (*sym->current++) {
			case '6':
				{
					char*                   args = NULL;
					const char*             call_conv;
					const char*             exported;
					struct datatype_t       sub_ct;
					unsigned                mark = sym->stack.num;

					if (!get_calling_convention(*sym->current++,
						&call_conv, &exported,
						sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE) ||
						!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
						goto done;

					args = get_args(sym, pmt_ref, TRUE, '(', ')');
					if (!args) goto done;
					sym->stack.num = mark;

					ct->left = str_printf(sym, "%s%s (%s*const",
						sub_ct.left, sub_ct.right, call_conv);
					ct->right = str_printf(sym, ")%s", args);
				}
				break;
			default:
				goto done;
			}
		} else if (!get_modified_type(ct, sym, pmt_ref, in_args ? dt : 'P', in_args)) goto done;
		break;
    case 'R': /* volatile pointer */
    case 'S': /* const volatile pointer */
        if (!get_modified_type(ct, sym, pmt_ref, in_args ? dt : 'P', in_args)) goto done;
        break;
    case 'P': /* Pointer */
        if (isdigit(*sym->current))
		{
			/* FIXME:
			*   P6 = Function pointer
			*   P8 = Member function pointer
			*   others who knows.. */
			switch (*sym->current++) {
			case '6':
				{
					char*                   args = NULL;
					const char*             call_conv;
					const char*             exported;
					struct datatype_t       sub_ct;
					unsigned                mark = sym->stack.num;

					if (!get_calling_convention(*sym->current++,
						&call_conv, &exported,
						sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE) ||
						!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
						goto done;

					args = get_args(sym, pmt_ref, TRUE, '(', ')');
					if (!args) goto done;
					sym->stack.num = mark;

					ct->left = str_printf(sym, "%s%s (%s*",
						sub_ct.left, sub_ct.right, call_conv);
					ct->right = str_printf(sym, ")%s", args);
				}
				break;
			case '8':
				{
					char*                   args = NULL;
					const char*             call_conv;
					const char*             exported;
					struct datatype_t       sub_ct;
					unsigned                mark = sym->stack.num;
					const char*             class;
					const char*             modifier;
					const char*             ptr_modif;

					if (!(class = get_class_name(sym)))
						goto done;
					if (!get_modifier(sym, &modifier, &ptr_modif))
						goto done;
					if (modifier && ptr_modif) modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
					else if (!modifier) modifier = ptr_modif;
					if (!get_calling_convention(*sym->current++,
						&call_conv, &exported,
						sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE))
						goto done;
					if (!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
						goto done;

					args = get_args(sym, pmt_ref, TRUE, '(', ')');
					if (!args) goto done;
					sym->stack.num = mark;

					ct->left = str_printf(sym, "%s%s (%s%s%s::*",
						sub_ct.left, sub_ct.right, call_conv, call_conv ? " " : NULL, class);
					ct->right = str_printf(sym, ")%s%s%s", args, modifier ? " " : NULL, modifier);
				}
				break;
			default:
				goto done;
			}
		}
		else if (!get_modified_type(ct, sym, pmt_ref, 'P', in_args)) goto done;
        break;
    case 'W':
        if (*sym->current == '4')
        {
            char*               enum_name;
            sym->current++;
            if (!(enum_name = get_class_name(sym)))
                goto done;
            if (sym->flags & UNDNAME_NO_COMPLEX_TYPE)
                ct->left = enum_name;
            else
                ct->left = str_printf(sym, "enum %s", enum_name);
        }
        else goto done;
        break;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        /* Referring back to previously parsed type */
        /* left and right are pushed as two separate strings */
		if(pmt_ref == NULL) goto done;
        ct->left = str_array_get_ref(pmt_ref, (dt - '0') * 2);
        ct->right = str_array_get_ref(pmt_ref, (dt - '0') * 2 + 1);
        if (!ct->left) goto done;
        add_pmt = FALSE;
        break;
    case '$':
        switch (*sym->current++)
        {
        case '0':
            if (!(ct->left = get_number(sym))) goto done;
            break;
		case '1': 
			{
				struct array stack = sym->stack;
				unsigned int start = sym->names.start;
				unsigned int num = sym->names.num;

				str_array_init(&sym->stack);
				if (symbol_demangle(sym)) ct->left = str_printf(sym, "&%s", sym->result);
				sym->names.start = start;
				sym->names.num = num;
				sym->stack = stack;
			}
			break;
        case 'D':
            {
                const char*   ptr;
                if (!(ptr = get_number(sym))) goto done;
                ct->left = str_printf(sym, "`template-parameter%s'", ptr);
            }
            break;
        case 'F':
            {
                const char*   p1;
                const char*   p2;
                if (!(p1 = get_number(sym))) goto done;
                if (!(p2 = get_number(sym))) goto done;
                ct->left = str_printf(sym, "{%s,%s}", p1, p2);
            }
            break;
        case 'G':
            {
                const char*   p1;
                const char*   p2;
                const char*   p3;
                if (!(p1 = get_number(sym))) goto done;
                if (!(p2 = get_number(sym))) goto done;
                if (!(p3 = get_number(sym))) goto done;
                ct->left = str_printf(sym, "{%s,%s,%s}", p1, p2, p3);
            }
            break;
        case 'Q':
            {
                const char*   ptr;
                if (!(ptr = get_number(sym))) goto done;
                ct->left = str_printf(sym, "`non-type-template-parameter%s'", ptr);
            }
            break;
        case '$':
			switch (*sym->current) {
			case 'A':
				{
					sym->current++;
					char c = (*sym->current);
					if (c == '6') {
						char*                   args = NULL;
						const char*             call_conv;
						const char*             exported;
						struct datatype_t       sub_ct;
						unsigned                mark = sym->stack.num;

						sym->current++;
						if (!get_calling_convention(*sym->current++,
							&call_conv, &exported,
							sym->flags & ~UNDNAME_NO_ALLOCATION_LANGUAGE) ||
							!demangle_datatype(sym, &sub_ct, pmt_ref, FALSE))
							goto done;

						args = get_args(sym, pmt_ref, TRUE, '(', ')');
						if (!args) goto done;
						sym->stack.num = mark;

						ct->left = str_printf(sym, "%s%s%s%s%s%s%s",
							sub_ct.left, (sub_ct.left && !sub_ct.right) ? " " : NULL,
							call_conv, call_conv ? " " : NULL, exported,
							args, sub_ct.right);
					} else 
						goto done;
				}
				break;
			case 'B':
				{
					const char *ptr = NULL;

					sym->current++;
					if (*sym->current == 'Y')
					{
						const char* n1;
						int num;

						sym->current++;
						if (!(n1 = get_number(sym))) goto done;
						num = atoi(n1);

						while (num--)
							ptr = str_printf(sym, "%s[%s]", ptr, get_number(sym));
					}

					if (!demangle_datatype(sym, ct, pmt_ref, in_args)) goto done;
					ct->left = str_printf(sym, "%s %s", ct->left, ptr);
				}
				break;
			case 'C':
				{
					const char *ptr, *ptr_modif;

					sym->current++;
					if (!get_modifier(sym, &ptr, &ptr_modif)) goto done;
					if (!demangle_datatype(sym, ct, pmt_ref, in_args)) goto done;
					ct->left = str_printf(sym, "%s %s", ct->left, ptr);
				}
				break;
			case 'Q':
				{
					sym->current++;
					if (!get_modified_type(ct, sym, pmt_ref, '?', in_args)) goto done;
					ct->left = str_printf(sym, "%s &&", ct->left);
				}
				break;
			case 'T':
				{
					sym->current++;
					ct->left = "std::nullptr_t";
				}
				break;
			default:
				ERR("Unknown type %c\n", *sym->current);
				goto done;
			}
            break;
        }
        break;
    default :
        ERR("Unknown type %c\n", dt);
        break;
    }
    if (add_pmt && pmt_ref && in_args)
    {
        /* left and right are pushed as two separate strings */
        if (!str_array_push(sym, ct->left ? ct->left : "", -1, pmt_ref) ||
            !str_array_push(sym, ct->right ? ct->right : "", -1, pmt_ref))
            return FALSE;
    }
done:
    
    return ct->left != NULL;
}

/******************************************************************
 *		handle_data
 * Does the final parsing and handling for a variable or a field in
 * a class.
 */
static BOOL handle_data(struct parsed_symbol* sym)
{
    const char*         access = NULL;
    const char*         member_type = NULL;
    const char*         modifier = NULL;
    const char*         ptr_modif;
    struct datatype_t   ct;
    char*               name = NULL;
    BOOL                ret = FALSE;

    /* 0 private static
     * 1 protected static
     * 2 public static
     * 3 private non-static
     * 4 protected non-static
     * 5 public non-static
     * 6 ?? static
     * 7 ?? static
     */

    if (!(sym->flags & UNDNAME_NO_ACCESS_SPECIFIERS))
    {
        /* we only print the access for static members */
        switch (*sym->current)
        {
        case '0': access = "private: "; break;
        case '1': access = "protected: "; break;
        case '2': access = "public: "; break;
        } 
    }

    if (!(sym->flags & UNDNAME_NO_MEMBER_TYPE))
    {
        if (*sym->current >= '0' && *sym->current <= '2')
            member_type = "static ";
    }

    name = get_class_string(sym, 0);

    switch (*sym->current++)
    {
    case '0': case '1': case '2':
    case '3': case '4': case '5':
        {
            unsigned mark = sym->stack.num;
            struct array pmt;

            str_array_init(&pmt);

            if (!demangle_datatype(sym, &ct, &pmt, FALSE)) goto done;
            if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
            if (modifier && ptr_modif) modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
            else if (!modifier) modifier = ptr_modif;
            sym->stack.num = mark;
        }
        break;
    case '6' : /* compiler generated static */
    case '7' : /* compiler generated static */
        ct.left = ct.right = NULL;
        if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
        if (*sym->current != '@')
        {
            char*       cls = NULL;

            if (!(cls = get_class_name(sym)))
                goto done;
            ct.right = str_printf(sym, "{for `%s'}", cls);
        }
        break;
    case '8':
    case '9':
        modifier = ct.left = ct.right = NULL;
        break;
    default: goto done;
    }
    if (sym->flags & UNDNAME_NAME_ONLY) ct.left = ct.right = modifier = NULL;

    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s", access,
                             member_type, ct.left, 
                             modifier && ct.left ? " " : NULL, modifier, 
                             modifier || ct.left ? " " : NULL, name, ct.right);
    ret = TRUE;
done:
    return ret;
}

/******************************************************************
 *		handle_method
 * Does the final parsing and handling for a function or a method in
 * a class.
 */
static BOOL handle_method(struct parsed_symbol* sym, BOOL cast_op, BOOL no_return)
{
    char                accmem;
    const char*         access = NULL;
    const char*         member_type = NULL;
    struct datatype_t   ct_ret;
    const char*         call_conv;
    const char*         modifier = NULL;
    const char*         exported;
    const char*         args_str = NULL;
    const char*         name = NULL;
    BOOL                ret = FALSE;
    unsigned            mark;
    struct array        array_pmt;
	unsigned short flags = 0;

    /* FIXME: why 2 possible letters for each option?
     * 'A' private:
     * 'B' private:
     * 'C' private: static
     * 'D' private: static
     * 'E' private: virtual
     * 'F' private: virtual
     * 'G' private: thunk
     * 'H' private: thunk
     * 'I' protected:
     * 'J' protected:
     * 'K' protected: static
     * 'L' protected: static
     * 'M' protected: virtual
     * 'N' protected: virtual
     * 'O' protected: thunk
     * 'P' protected: thunk
     * 'Q' public:
     * 'R' public:
     * 'S' public: static
     * 'T' public: static
     * 'U' public: virtual
     * 'V' public: virtual
     * 'W' public: thunk
     * 'X' public: thunk
     * 'Y'
     * 'Z'
     * "$0" private: thunk vtordisp
     * "$1" private: thunk vtordisp
     * "$2" protected: thunk vtordisp
     * "$3" protected: thunk vtordisp
     * "$4" public: thunk vtordisp
     * "$5" public: thunk vtordisp
	 * "$B" thunk vcall
     * "$R0" private: thunk vtordispex
     * "$R1" private: thunk vtordispex
     * "$R2" protected: thunk vtordispex
     * "$R3" protected: thunk vtordispex
     * "$R4" public: thunk vtordispex
     * "$R5" public: thunk vtordispex
     */

	enum {
		FLAG_PRIVATE = 0x1,
		FLAG_PROTECTED = 0x2,
		FLAG_PUBLIC = 0x4,
		FLAG_STATIC = 0x8,
		FLAG_VIRTUAL = 0x10,
		FLAG_THUNK = 0x20,
		FLAG_THIS = 0x40,
		FLAG_VTORDISP = 0x80,
		FLAG_VTORDISPEX = 0x100,
		FLAG_VCALL = 0x200,
		FLAG_RETURN = 0x400,
		FLAG_ARGS = 0x800,
	};

	accmem = *sym->current++;
	if (accmem == '$') {
		accmem = *sym->current++;
		flags |= FLAG_THUNK;
		if (accmem == 'B') {
			flags |= FLAG_VCALL;
		} else {
			if (accmem == 'R') {
  				flags |= FLAG_VTORDISPEX;
				accmem = *sym->current++;
			}
			if (accmem >= '0' && accmem <= '5') {
			  flags |= FLAG_VIRTUAL | FLAG_THIS | FLAG_RETURN | FLAG_ARGS;
			  if ((flags & FLAG_VTORDISPEX) == 0)
				  flags |= FLAG_VTORDISP;
			  switch ((accmem - '0') & 6) {
			  case 0: flags |= FLAG_PRIVATE; break;
			  case 2: flags |= FLAG_PROTECTED; break;
			  case 4: flags |= FLAG_PUBLIC; break;
			  }
			} else goto done;
		}
	} else if (accmem >= 'A' && accmem <= 'Z') {
		flags |= FLAG_RETURN | FLAG_ARGS;
        switch ((accmem - 'A') / 8) {
        case 0: flags |= FLAG_PRIVATE; break;
        case 1: flags |= FLAG_PROTECTED; break;
        case 2: flags |= FLAG_PUBLIC; break;
        }
		if (accmem <= 'X') {
		  flags |= FLAG_THIS;
          switch ((accmem - 'A') & 6) {
          case 2: flags |= FLAG_STATIC; flags &= ~FLAG_THIS; break;
          case 4: flags |= FLAG_VIRTUAL; break;
          case 6: flags |= FLAG_THUNK | FLAG_VIRTUAL; break;
          }
		}
	} else goto done;

	if (!(sym->flags & UNDNAME_NO_ACCESS_SPECIFIERS)) {
		if (flags & FLAG_PRIVATE)
			access = "private: ";
		else if (flags & FLAG_PROTECTED)
			access = "protected: ";
		else if (flags & FLAG_PUBLIC)
			access = "public: ";
	}

	if (!(sym->flags & UNDNAME_NO_MEMBER_TYPE)) {
		if (flags & FLAG_STATIC)
			member_type = "static ";
		else if (flags & FLAG_VIRTUAL)
			member_type = "virtual ";
	}

    name = get_class_string(sym, 0);

    if (flags & FLAG_THUNK) {
		access = str_printf(sym, "[thunk]:%s", access);
		if (flags & FLAG_VTORDISPEX) {
			const char *num1;
			const char *num2;
			const char *num3;
			const char *num4;
			if (!(num1 = get_number(sym))) goto done;
			if (!(num2 = get_number(sym))) goto done;
			if (!(num3 = get_number(sym))) goto done;
			if (!(num4 = get_number(sym))) goto done;
			name = str_printf(sym, "%s`vtordispex{%s,%s,%s,%s}' ", name, num1, num2, num3, num4);
		} else if (flags & FLAG_VTORDISP) {
			const char *num1;
			const char *num2;
			if (!(num1 = get_number(sym))) goto done;
			if (!(num2 = get_number(sym))) goto done;
			name = str_printf(sym, "%s`vtordisp{%s,%s}' ", name, num1, num2);
		} else if (flags & FLAG_VCALL) {
			const char *num1;
			const char *type;
			if (!(num1 = get_number(sym))) goto done;
			if (*sym->current++ == 'A') {
				type = "{flat}";
			} else goto done;
			name = str_printf(sym, "%s{%s%s}' ", name, num1, type);
		} else {
		    name = str_printf(sym, "%s`adjustor{%s}' ", name, get_number(sym));
		}
	}

    if (flags & FLAG_THIS) {
		const char *ptr_modif;
        /* Implicit 'this' pointer */
        /* If there is an implicit this pointer, const modifier follows */
        if (!get_modifier(sym, &modifier, &ptr_modif)) goto done;
        if (sym->flags & UNDNAME_NO_THISTYPE) modifier = NULL;
		else if (modifier || ptr_modif) modifier = str_printf(sym, "%s %s", modifier, ptr_modif);
    }

    if (!get_calling_convention(*sym->current++, &call_conv, &exported,
                                sym->flags))
        goto done;

	if (flags & FLAG_RETURN) {
		str_array_init(&array_pmt);

		/* Return type, or @ if 'void' */
		if (*sym->current == '@')
		{
			ct_ret.left = "void";
			ct_ret.right = NULL;
			sym->current++;
		}
		else
		{
			if (!demangle_datatype(sym, &ct_ret, &array_pmt, FALSE))
				goto done;
		}
	} else ct_ret.left = ct_ret.right = NULL;

    if (no_return)
        ct_ret.left = ct_ret.right = NULL;
    if (cast_op)
    {
        name = str_printf(sym, "%s%s%s", name, ct_ret.left, ct_ret.right);
        ct_ret.left = ct_ret.right = NULL;
    }

	if (flags & FLAG_ARGS) {
		mark = sym->stack.num;
		if (!(args_str = get_args(sym, &array_pmt, TRUE, '(', ')'))) goto done;
		if (sym->flags & UNDNAME_NAME_ONLY) args_str = modifier = NULL;
		sym->stack.num = mark;
	}

    /* Note: '()' after 'Z' means 'throws', but we don't care here
     * Yet!!! FIXME
     */
    sym->result = str_printf(sym, "%s%s%s%s%s%s%s%s%s%s%s",
                             access, member_type, ct_ret.left, 
                             (ct_ret.left && !ct_ret.right) ? " " : NULL,
                             call_conv, call_conv ? " " : NULL, exported,
                             name, args_str, modifier, ct_ret.right);
	if (sym->name_pos) {
		size_t len = 0;
		len += name ? strlen(name) : 0;
		len += args_str ? strlen(args_str) : 0;
		len += modifier ? strlen(modifier) : 0;
		len += ct_ret.right ? strlen(ct_ret.right) : 0;
		*sym->name_pos = strlen(sym->result) - len;
	}

    ret = TRUE;
done:
    return ret;
}

/******************************************************************
 *		handle_template
 * Does the final parsing and handling for a name with templates
 */
static BOOL handle_template(struct parsed_symbol* sym)
{
    const char* name;
    const char* args;

    assert(*sym->current == '$');
    sym->current++;
    if (!(name = get_literal_string(sym))) return FALSE;
    if (!(args = get_args(sym, NULL, FALSE, '<', '>'))) return FALSE;
    sym->result = str_printf(sym, "%s%s", name, args);
    return TRUE;
}

/*******************************************************************
 *         symbol_demangle
 * Demangle a C++ linker symbol
 */
BOOL symbol_demangle(struct parsed_symbol* sym)
{
    BOOL                ret = FALSE;
    unsigned            do_after = 0;
    static char         dashed_null[] = "--null--";
	BOOL                no_function_returns = ((sym->flags & UNDNAME_NO_FUNCTION_RETURNS) != 0);

    /* FIXME seems wrong as name, as it demangles a simple data type */
    if (sym->flags & UNDNAME_NO_ARGUMENTS)
    {
        struct datatype_t   ct;

        if (demangle_datatype(sym, &ct, NULL, FALSE))
        {
            sym->result = str_printf(sym, "%s%s", ct.left, ct.right);
            ret = TRUE;
        }
        goto done;
    }

    /* MS mangled names always begin with '?' */
    if (*sym->current != '?') return FALSE;
    sym->current++;

	BOOL is_template_args = FALSE;
    /* Then function name or operator code */
    if (*sym->current == '?' && (sym->current[1] != '$' || sym->current[2] == '?'))
    {
        const char* function_name = NULL;

        if (sym->current[1] == '$')
        {
			is_template_args = TRUE;
            sym->current += 2;
        }

        /* C++ operator code (one character, or two if the first is '_') */
        switch (*++sym->current)
        {
        case '0': do_after = 1; break;
        case '1': do_after = 2; break;
        case '2': function_name = "operator new"; break;
        case '3': function_name = "operator delete"; break;
        case '4': function_name = "operator="; break;
        case '5': function_name = "operator>>"; break;
        case '6': function_name = "operator<<"; break;
        case '7': function_name = "operator!"; break;
        case '8': function_name = "operator=="; break;
        case '9': function_name = "operator!="; break;
        case 'A': function_name = "operator[]"; break;
        case 'B': function_name = "operator "; do_after = 3; break;
        case 'C': function_name = "operator->"; break;
        case 'D': function_name = "operator*"; break;
        case 'E': function_name = "operator++"; break;
        case 'F': function_name = "operator--"; break;
        case 'G': function_name = "operator-"; break;
        case 'H': function_name = "operator+"; break;
        case 'I': function_name = "operator&"; break;
        case 'J': function_name = "operator->*"; break;
        case 'K': function_name = "operator/"; break;
        case 'L': function_name = "operator%"; break;
        case 'M': function_name = "operator<"; break;
        case 'N': function_name = "operator<="; break;
        case 'O': function_name = "operator>"; break;
        case 'P': function_name = "operator>="; break;
        case 'Q': function_name = "operator,"; break;
        case 'R': function_name = "operator()"; break;
        case 'S': function_name = "operator~"; break;
        case 'T': function_name = "operator^"; break;
        case 'U': function_name = "operator|"; break;
        case 'V': function_name = "operator&&"; break;
        case 'W': function_name = "operator||"; break;
        case 'X': function_name = "operator*="; break;
        case 'Y': function_name = "operator+="; break;
        case 'Z': function_name = "operator-="; break;
        case '_':
            switch (*++sym->current)
            {
            case '0': function_name = "operator/="; break;
            case '1': function_name = "operator%="; break;
            case '2': function_name = "operator>>="; break;
            case '3': function_name = "operator<<="; break;
            case '4': function_name = "operator&="; break;
            case '5': function_name = "operator|="; break;
            case '6': function_name = "operator^="; break;
            case '7': function_name = "`vftable'"; break;
            case '8': function_name = "`vbtable'"; break;
            case '9': function_name = "`vcall'"; break;
            case 'A': function_name = "`typeof'"; break;
            case 'B': function_name = "`local static guard'"; break;
            case 'C': function_name = "`string'"; do_after = 4; break;
            case 'D': function_name = "`vbase destructor'"; break;
            case 'E': function_name = "`vector deleting destructor'"; break;
            case 'F': function_name = "`default constructor closure'"; break;
            case 'G': function_name = "`scalar deleting destructor'"; break;
            case 'H': function_name = "`vector constructor iterator'"; break;
            case 'I': function_name = "`vector destructor iterator'"; break;
            case 'J': function_name = "`vector vbase constructor iterator'"; break;
            case 'K': function_name = "`virtual displacement map'"; break;
            case 'L': function_name = "`eh vector constructor iterator'"; break;
            case 'M': function_name = "`eh vector destructor iterator'"; break;
            case 'N': function_name = "`eh vector vbase constructor iterator'"; break;
            case 'O': function_name = "`copy constructor closure'"; break;
            case 'R':
				no_function_returns = TRUE;
                switch (*++sym->current)
                {
                case '0':
                    {
                        struct datatype_t       ct;
                        struct array pmt;

                        sym->current++;
                        str_array_init(&pmt);
                        demangle_datatype(sym, &ct, &pmt, FALSE);
                        function_name = str_printf(sym, "%s%s `RTTI Type Descriptor'",
                                                   ct.left, ct.right);
                        sym->current--;
                    }
                    break;
                case '1':
                    {
                        const char* n1, *n2, *n3, *n4;
                        sym->current++;
                        n1 = get_number(sym);
                        n2 = get_number(sym);
                        n3 = get_number(sym);
                        n4 = get_number(sym);
                        sym->current--;
                        function_name = str_printf(sym, "`RTTI Base Class Descriptor at (%s,%s,%s,%s)'",
                                                   n1, n2, n3, n4);
                    }
                    break;
                case '2': function_name = "`RTTI Base Class Array'"; break;
                case '3': function_name = "`RTTI Class Hierarchy Descriptor'"; break;
                case '4': function_name = "`RTTI Complete Object Locator'"; break;
                default:
                    ERR("Unknown RTTI operator: _R%c\n", *sym->current);
                    return FALSE;
                }
                break;
            case 'S': function_name = "`local vftable'"; break;
            case 'T': function_name = "`local vftable constructor closure'"; break;
            case 'U': function_name = "operator new[]"; break;
            case 'V': function_name = "operator delete[]"; break;
            case 'X': function_name = "`placement delete closure'"; break;
            case 'Y': function_name = "`placement delete[] closure'"; break;
			case '_': 
				switch (*++sym->current) {
				case 'A': function_name = "`managed vector constructor iterator'"; break;
				case 'B': function_name = "`managed vector destructor iterator'"; break;
				case 'C': function_name = "`eh vector copy constructor iterator'"; break;
				case 'D': function_name = "`eh vector vbase copy constructor iterator'"; break;
				case 'E':
					{
						char* result = NULL;
						if (*++sym->current == '?') {
							struct array stack = sym->stack;
							unsigned int start = sym->names.start;
							unsigned int num = sym->names.num;
							str_array_init(&sym->stack);
							if (symbol_demangle(sym)) result = sym->result;
							sym->names.start = start;
							sym->names.num = num;
							sym->stack = stack;
							if (!result)
								return FALSE;
						} else {
							if (!(result = get_literal_string(sym)))
								return FALSE;
							sym->current--;
						}
						function_name = str_printf(sym, "`dynamic initializer for '%s''", result);
					}
					break;
				case 'F':
					{
						char* result = NULL;
						if (*++sym->current == '?') {
							struct array stack = sym->stack;
							unsigned int start = sym->names.start;
							unsigned int num = sym->names.num;
							str_array_init(&sym->stack);
							if (symbol_demangle(sym)) result = sym->result;
							sym->names.start = start;
							sym->names.num = num;
							sym->stack = stack;
							if (!result)
								return FALSE;
						}
						else {
							if (!(result = get_literal_string(sym)))
								return FALSE;
							sym->current--;
						}
						function_name = str_printf(sym, "`dynamic atexit destructor for '%s''", result);
					}
					break;
				case 'G': function_name = "`vector copy constructor iterator'"; break;
				default:
					ERR("Unknown operator: __%c\n", *sym->current);
					return FALSE;
				}
				break;
            default:
                ERR("Unknown operator: _%c\n", *sym->current);
                return FALSE;
            }
            break;
        default:
            ERR("Unknown operator: %c\n", *sym->current);
            return FALSE;
        }
        sym->current++;

		if (is_template_args) {
			char *args;
			struct array array_pmt;
			str_array_init(&array_pmt);
			args = get_args(sym, &array_pmt, FALSE, '<', '>');
			if (args != NULL) function_name = str_printf(sym, "%s%s", function_name, args);
			sym->names.num = 0;
		}

        switch (do_after)
        {
        case 1: case 2:
            if (!str_array_push(sym, dashed_null, -1, &sym->stack))
                return FALSE;
            break;
        case 4:
            sym->result = (char*)function_name;
            ret = TRUE;
            goto done;
		}

		if (function_name != NULL) {
			if (!str_array_push(sym, function_name, -1, &sym->stack))
				return FALSE;
		}
    }
    else if (*sym->current == '$')
    {
        /* Strange construct, it's a name with a template argument list
           and that's all. */
        sym->current++;
        ret = (sym->result = get_template_name(sym)) != NULL;
        goto done;
    }
    else if (*sym->current == '?' && sym->current[1] == '$')
        do_after = 5;

    /* Either a class name, or '@' if the symbol is not a class member */
    switch (*sym->current)
    {
    case '@': sym->current++; break;
    case '$': break;
    default:
        /* Class the function is associated with, terminated by '@@' */
        if (!get_class(sym)) goto done;
        break;
    }

    switch (do_after)
    {
    case 0: default: break;
    case 1: case 2:
        /* it's time to set the member name for ctor & dtor */
        if (sym->stack.num <= 1) goto done;
        if (do_after == 1)
            sym->stack.elts[0] = sym->stack.elts[1];
        else
            sym->stack.elts[0] = str_printf(sym, "~%s", sym->stack.elts[1]);
        /* ctors and dtors don't have return type */
		no_function_returns = TRUE;
        break;
    case 3:
		no_function_returns = FALSE;
        break;
    case 5:
        sym->names.start++;
        break;
    }

    /* Function/Data type and access level */
    if (*sym->current >= '0' && *sym->current <= '9')
        ret = handle_data(sym);
    else if ((*sym->current >= 'A' && *sym->current <= 'Z') || (sym->current[0] == '$' && (sym->current[1] == 'R' || sym->current[1] == 'B' || (sym->current[1] >= '0' && sym->current[1] <= '5'))))
        ret = handle_method(sym, do_after == 3, no_function_returns);
    else if (*sym->current == '$')
        ret = handle_template(sym);
    else ret = FALSE;
done:
    if (ret) assert(sym->result);
    else WARN("Failed at %s\n", sym->current);

    return ret;
}

char* undname(const char* mangled, unsigned short int flags, size_t* name_pos)
{
    struct parsed_symbol        sym;
    char*                 result = NULL;
   
    /* The flags details is not documented by MS. However, it looks exactly
     * like the UNDNAME_ manifest constants from imagehlp.h and dbghelp.h
     * So, we copied those (on top of the file)
     */
    memset(&sym, 0, sizeof(struct parsed_symbol));
    if (flags & UNDNAME_NAME_ONLY)
        flags |= UNDNAME_NO_FUNCTION_RETURNS | UNDNAME_NO_ACCESS_SPECIFIERS |
            UNDNAME_NO_MEMBER_TYPE | UNDNAME_NO_ALLOCATION_LANGUAGE |
            UNDNAME_NO_COMPLEX_TYPE;

    sym.flags         = flags;
    sym.mem_alloc_ptr = malloc;
    sym.mem_free_ptr  = free;
    sym.current       = mangled;
	sym.name_pos      = name_pos;
    str_array_init( &sym.names );
    str_array_init( &sym.stack );

    if (symbol_demangle(&sym))
		result = _strdup(sym.result);

    und_free_all(&sym);

    return result;
}