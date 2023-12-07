/*------------------------------------------------------------------------
 * filename - unmangle.h (C++ decorated symbol unmangler)
 *
 * function(s)
 *
 *   _rtl_unmangle
 *   _rtl_setUnmangleMode
 *
 *-----------------------------------------------------------------------*/

/*
 *      C/C++ Run Time Library - Version 22.0
 *
 *      Copyright (c) 1998, 2015 by Embarcadero Technologies, Inc.
 *      All Rights Reserved.
 *
 */

/* $Revision: 23325 $        */

#ifndef _UNMANGLE_
#define _UNMANGLE_

#define _UMAPI

enum
{
    /* The kind of symbol. */

    x_UM_UNKNOWN       = 0x00000000,

    x_UM_FUNCTION      = 0x00000001,
    x_UM_CONSTRUCTOR   = 0x00000002,
    x_UM_DESTRUCTOR    = 0x00000003,
    x_UM_OPERATOR      = 0x00000004,
    x_UM_CONVERSION    = 0x00000005,

    x_UM_DATA	       = 0x00000006,
    x_UM_THUNK	       = 0x00000007,
    x_UM_TPDSC	       = 0x00000008,
    x_UM_VTABLE        = 0x00000009,
    x_UM_VRDF_THUNK    = 0x0000000a,
    x_UM_DYN_THUNK     = 0x0000000b,

    x_UM_KINDMASK      = 0x000000ff,

    /* Modifier (is it a member, template?). */

    x_UM_QUALIFIED     = 0x00000100,
    x_UM_TEMPLATE      = 0x00000200,

    x_UM_VIRDEF_FLAG   = 0x00000400,
    x_UM_FRIEND_LIST   = 0x00000800,
    x_UM_CTCH_HNDL_TBL = 0x00001000,
    x_UM_OBJ_DEST_TBL  = 0x00002000,
    x_UM_THROW_LIST    = 0x00004000,
    x_UM_EXC_CTXT_TBL  = 0x00008000,
    x_UM_LINKER_PROC   = 0x00010000,
    x_UM_SPECMASK      = 0x0001fc00,

    x_UM_MODMASK       = 0x00ffff00,

    /* Some kind of error occurred. */

    x_UM_BUFOVRFLW     = 0x01000000,
    x_UM_HASHTRUNC     = 0x02000000,
    x_UM_ERROR	       = 0x04000000,

    x_UM_ERRMASK       = 0x7f000000,

    /* This symbol is not a mangled name. */

    x_UM_NOT_MANGLED   = 0x80000000,
};

typedef int _umKind;

#define UM_UNKNOWN	 ((_umKind) x_UM_UNKNOWN)
#define UM_FUNCTION	 ((_umKind) x_UM_FUNCTION)
#define UM_CONSTRUCTOR	 ((_umKind) x_UM_CONSTRUCTOR)
#define UM_DESTRUCTOR	 ((_umKind) x_UM_DESTRUCTOR)
#define UM_OPERATOR	 ((_umKind) x_UM_OPERATOR)
#define UM_CONVERSION	 ((_umKind) x_UM_CONVERSION)
#define UM_DATA		 ((_umKind) x_UM_DATA)
#define UM_THUNK	 ((_umKind) x_UM_THUNK)
#define UM_TPDSC	 ((_umKind) x_UM_TPDSC)
#define UM_VTABLE	 ((_umKind) x_UM_VTABLE)
#define UM_VRDF_THUNK	 ((_umKind) x_UM_VRDF_THUNK)
#define UM_DYN_THUNK	 ((_umKind) x_UM_DYN_THUNK)
#define UM_KINDMASK	 ((_umKind) x_UM_KINDMASK)
#define UM_QUALIFIED	 ((_umKind) x_UM_QUALIFIED)
#define UM_TEMPLATE	 ((_umKind) x_UM_TEMPLATE)
#define UM_VIRDEF_FLAG	 ((_umKind) x_UM_VIRDEF_FLAG)
#define UM_FRIEND_LIST	 ((_umKind) x_UM_FRIEND_LIST)
#define UM_CTCH_HNDL_TBL ((_umKind) x_UM_CTCH_HNDL_TBL)
#define UM_OBJ_DEST_TBL  ((_umKind) x_UM_OBJ_DEST_TBL)
#define UM_THROW_LIST	 ((_umKind) x_UM_THROW_LIST)
#define UM_EXC_CTXT_TBL  ((_umKind) x_UM_EXC_CTXT_TBL)
#define UM_LINKER_PROC	 ((_umKind) x_UM_LINKER_PROC)
#define UM_SPECMASK	 ((_umKind) x_UM_SPECMASK)
#define UM_MODMASK	 ((_umKind) x_UM_MODMASK)
#define UM_BUFOVRFLW	 ((_umKind) x_UM_BUFOVRFLW)
#define UM_HASHTRUNC	 ((_umKind) x_UM_HASHTRUNC)
#define UM_ERROR	 ((_umKind) x_UM_ERROR)
#define UM_ERRMASK	 ((_umKind) x_UM_ERRMASK)
#define UM_NOT_MANGLED	 ((_umKind) x_UM_NOT_MANGLED)


#define _UM_MAXBUFFLEN 8192 /* maximum output length */

#define MAXBUFFLEN      _UM_MAXBUFFLEN
#define umKind          _umKind
#define UMAPI           _UMAPI

#ifdef __cplusplus
extern "C" {
#endif

_umKind _UMAPI
unmangle(char   *       src,     /* the string to be unmangled */
              char   *       dest,    /* the unmangled output string */
              unsigned       maxlen,  /* the max length of the output string */
              char   *       qualP,   /* optional additional string to hold
                                         only the qualifiers */

              char   *       baseP,   /* optional additional string to hold
                                         only the base name */
              int            doArgs); /* handle function arguments? */

//int _UMAPI _rtl_setUnmangleMode(int); /* currently not implemented */

#ifdef __cplusplus
}
#endif

#endif