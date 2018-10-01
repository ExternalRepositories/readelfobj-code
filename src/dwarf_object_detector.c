/* Copyright (c) 2018-2018, David Anderson
All rights reserved.

Redistribution and use in source and binary forms, with
or without modification, are permitted provided that the
following conditions are met:

    Redistributions of source code must retain the above
    copyright notice, this list of conditions and the following
    disclaimer.

    Redistributions in binary form must reproduce the above
    copyright notice, this list of conditions and the following
    disclaimer in the documentation and/or other materials
    provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "config.h"
#include <stdio.h>
#include <sys/types.h> /* fstat */
#include <sys/stat.h> /* fstat */
#include <fcntl.h> /* O_RDONLY */
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* lseek read close */
#endif /* HAVE_UNISTD_H */
#include <string.h> /* memcpy, strcpy */
#include "dwarf_object_detector.h"

/* This is the main() program for the object_detector executable. */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif /* TRUE */

#ifndef O_RDONLY
#define O_RDONLY 0
#endif


#define DW_DLV_NO_ENTRY -1
#define DW_DLV_OK        0
#define DW_DLV_ERROR     1

#define RO_ERR_SEEK   2
/* Must match dwarf_reading.h list */
#define RO_ERR_READ   3
#define RO_ERR_MALLOC 4
#define RO_ERR_OTHER  5
#define RO_ERR_BADOFFSETSIZE    6
#define RO_ERR_LOADSEGOFFSETBAD 7
#define RO_ERR_FILEOFFSETBAD    8
#define RO_ERR_BADTYPESIZE    9 
#define RO_ERR_TOOSMALL    10
#define RO_ERR_ELF_VERSION    11
#define RO_ERR_ELF_CLASS    12
#define RO_ERR_ELF_ENDIAN    13
#define RO_ERR_OPEN_FAIL    14
#define RO_ERR_PATH_SIZE    15



#ifndef EI_NIDENT
#define EI_NIDENT 16
#define EI_CLASS  4
#define EI_DATA   5
#define EI_VERSION 6
#define ELFCLASS32 1
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ELFDATA2MSB 2
#endif /* EI_NIDENT */

#define DSYM_SUFFIX "/.dSYM/Contents/Resources/DWARF/"
#define PATHSIZE 2000

/*  Assuming short 16 bits, unsigned 32 bits */
typedef unsigned short t16;
typedef unsigned t32;

#ifndef  MH_MAGIC
/* mach-o 32bit */
#define MH_MAGIC        0xfeedface
#define MH_CIGAM        0xcefaedfe
#endif /*  MH_MAGIC */
#ifndef  MH_MAGIC_64
/* mach-o 64bit */
#define MH_MAGIC_64 0xfeedfacf
#define MH_CIGAM_64 0xcffaedfe
#endif /*  MH_MAGIC_64 */

#define EI_NIDENT 16
/* An incomplete elf header, good for 32 and 64bit elf */
struct elf_header {
    unsigned char  e_ident[EI_NIDENT];
    t16 e_type;
    t16 e_machine;
    t32 e_version;
};

/*  For following MacOS file naming convention */
static const char *
getseparator (const char *f)
{
    const char *p, *q;
    p = NULL;
    q = f;
    char c;
    do  {
        c = *q++;
        if (c == '\\' || c == '/' || c == ':') {
            p = q;
        }
    } while (c);
    return p;
}

static const char *
getbasename (const char *f)
{
    const char *pseparator = getseparator (f);
    if (!pseparator) {
        return f;
    }
    return pseparator;
}

/*  Not a standard function, though part of GNU libc
    since 2008 (I have never examined the GNU version).  */
static char *
dw_stpcpy(char *dest,const char *src)
{
    const char *cp = src;
    char *dp = dest;

    for ( ; *cp; ++cp,++dp) {
        *dp = *cp;
    }
    *dp = 0;
    return dp;
}



/* This started like Elf, so check initial fields. */
static int
fill_in_elf_fields(struct elf_header *h,
    unsigned *endian,
    /*  Size of the object file offsets, not DWARF offset
        size. */
    unsigned *objoffsetsize,
    int *errcode)
{
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    if (h->e_version != 1 /* EV_CURRENT */) {
        *errcode = RO_ERR_ELF_VERSION;
        return DW_DLV_ERROR;
    }
    switch(h->e_ident[EI_CLASS]) {
    case ELFCLASS32:
        locoffsetsize = 32;
        break;
    case ELFCLASS64:
        locoffsetsize = 64;
        break;
    default:
        *errcode = RO_ERR_ELF_CLASS;
        return DW_DLV_ERROR;
    }
    switch(h->e_ident[EI_DATA]) {
    case ELFDATA2LSB:
        locendian = DW_ENDIAN_LITTLE;
        break;
    case ELFDATA2MSB:
        locendian = DW_ENDIAN_BIG;
        break;
    default:
        *errcode = RO_ERR_ELF_ENDIAN;
        return DW_DLV_ERROR;
    }
    *endian = locendian;
    *objoffsetsize = locoffsetsize;
    return DW_DLV_OK;
}

static int
is_mach_o_magic(struct elf_header *h,
    unsigned *endian,
    unsigned *offsetsize)
{
    t32 magicval = 0;
    unsigned locendian = 0;
    unsigned locoffsetsize = 0;

    memcpy(&magicval,h,sizeof(magicval));
    if (magicval == MH_MAGIC) {
        locendian = DW_ENDIAN_SAME;
        locoffsetsize = 32;
    } else if (magicval == MH_CIGAM) {
        locendian = DW_ENDIAN_OPPOSITE;
        locoffsetsize = 32;
    }else if (magicval == MH_MAGIC_64) {
        locendian = DW_ENDIAN_SAME;
        locoffsetsize = 64;
    } else if (magicval == MH_CIGAM_64) {
        locendian = DW_ENDIAN_OPPOSITE;
        locoffsetsize = 64;
    } else {
        return FALSE;
    }
    *endian = locendian;
    *offsetsize = locoffsetsize;
    return TRUE;
}

int
dwarf_object_detector_f(FILE *f,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int *errcode)
{
    return dwarf_object_detector_fd(fileno(f),
        ftype,endian,offsetsize,filesize,errcode);
}

int
dwarf_object_detector_fd(int fd,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int *errcode)
{
    size_t resf = 0;
    struct elf_header h;
    size_t readlen = sizeof(h);
    int res = 0;
    off_t fsize = 0;
    off_t lsval = 0;
    ssize_t readval = 0;

    if (sizeof(t32) != 4 || sizeof(t16)!= 2) {
        *errcode = RO_ERR_BADTYPESIZE;
        return DW_DLV_ERROR;
    }
    fsize = lseek(fd,0L,SEEK_END);
    if(fsize < 0) {
        *errcode = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    if (fsize <= readlen) {
        /* Not a real object file */
        *errcode = RO_ERR_TOOSMALL;
        return DW_DLV_ERROR;
    }
    lsval  = lseek(fd,0L,SEEK_SET);
    if(lsval < 0) {
        *errcode = RO_ERR_SEEK;
        return DW_DLV_ERROR;
    }
    readval = read(fd,&h,readlen);
    if (readval != readlen) {
        *errcode = RO_ERR_READ;
        return DW_DLV_ERROR;
    }
    if (h.e_ident[0] == 0x7f &&
        h.e_ident[1] == 'E' &&
        h.e_ident[2] == 'L' &&
        h.e_ident[3] == 'F') {
        /* is ELF */
        res = fill_in_elf_fields(&h,endian,offsetsize,errcode);
        if (res != DW_DLV_OK) {
            return res;
        }
        *ftype = DW_FTYPE_ELF;
        *filesize = (size_t)fsize;
        return DW_DLV_OK;
    }
    if (is_mach_o_magic(&h,endian,offsetsize)) {
        *ftype = DW_FTYPE_MACH_O;
        *filesize = (size_t)fsize;
        return DW_DLV_OK;
    }
    /* CHECK FOR  PE object. */
    return DW_DLV_NO_ENTRY;
}

int
dwarf_object_detector_path(const char  *path,
    char *outpath,size_t outpath_len,
    unsigned *ftype,
    unsigned *endian,
    unsigned *offsetsize,
    size_t   *filesize,
    int *errcode)
{
    FILE *f = 0;
    size_t plen = strlen(path);
    size_t dsprefixlen = sizeof(DSYM_SUFFIX);
    struct stat statbuf;
    size_t finallen = outpath_len;
    int fd = -1;
    int res = 0;

#if !defined(S_ISREG)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#if !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

    res = stat(path,&statbuf);
    if(res) {
        return DW_DLV_NO_ENTRY;
    }
    if(S_ISDIR(statbuf.st_mode)) {
        finallen = 2*plen + dsprefixlen + 2;
        if (finallen < outpath_len) {
            /* path + suffix + basenameofpath */
            char * p = 0;

            p = dw_stpcpy(outpath,path);
            p = dw_stpcpy(p,DSYM_SUFFIX);
            dw_stpcpy(p,getbasename(path));
        } else {
            *errcode = RO_ERR_PATH_SIZE;
            return DW_DLV_ERROR;
        }
    } else {
        finallen = strlen(path);
        if (finallen <  outpath_len) {
            strcpy(outpath,path);
        } else {
            *errcode = RO_ERR_PATH_SIZE;
            return DW_DLV_ERROR;
        }
    }
    fd = open(outpath,O_RDONLY);
    if (fd < 0) {
        return DW_DLV_NO_ENTRY;
    }
    res = dwarf_object_detector_fd(fd,
        ftype,endian,offsetsize,filesize,errcode);
    if (res == DW_DLV_OK) {
        strcpy(outpath,path);
    }
    close(fd);
    return res;
}
