/* A library for fixing programs which object to 64 bit inodes on Linux.
 *
 * (c) 15/8/2014, MJ Rutter
 *
 * Can be distributed under the Gnu Public Licence version 2
 * ( http://www.gnu.org/licenses/gpl-2.0.html )
 *
 * WARNING!! USE AT OWN RISK -- AUTHOR ACCEPTS NO RESPONSIBILITY
 * IF THIS DESTROYS YOUR DATA, OR CAUSES ANY OTHER FORM OF INCONVENIENCE
 * OR DISASTER. AUTHOR RECOMMENDS YOU DO NOT USE IT.
 * 
 *
 * Problem: if a filesystem has 64 bit inodes, 32 bit programs calling
 *  *stat() or readdir() will get EOVERFLOW unless they have been compiled
 *  with large file support. However, it is likely that the only field which
 *  overflows is the inode number, which is usually ignored.
 *
 * This library forces the inode number to 32 bits, and returns the rest
 *   of the structure correctly. It is similar to mounting an NFS filesystem
 *   with the kernel parameter nfs.enable_ino64=0
 *
 * To build:
 *
 * 1/ Create a linker script called "vers" containing:
 *
GLIBC_2.0 {
  global:
    readdir;
    __fxstat;
    __xstat;
    __lxstat;
};
 *
 * 2/ compile: 
 *
 *  gcc -c -fPIC -m32 inode64.c
 *
 * 3/ link
 *
 * ld -shared -melf_i386 --version-script vers -o inode64.so inode64.o
 *
 * Then use by pointing LD_PRELOAD to the library
 *
 *
 * Bugs:
 *
 * 1/ No attempt is made to fix readdir_r()
 *
 * 2/ readdir() overwrites its buffer on every call, even on calls to
 *      different directory streams
 *
 * If compiled with INPLACE defined (the default), then the returned structure
 *   from readdir64() is modified in place, and this bug does not apply. Does
 *   POSIX permit modifying the returned structure? Unclear to me...
 *
 */

#define INPLACE

#include<stdlib.h>
#include<bits/types.h>
#include<stdint.h>
#include<unistd.h>
#include<errno.h>

struct dirent32 {
  uint32_t       d_ino;       /* inode number */
  uint32_t       d_off;       /* not an offset; see NOTES */
  unsigned short d_reclen;    /* length of this record */
  unsigned char  d_type;      /* type of file; not supported
                                 by all file system types */
  char           d_name[256]; /* filename */
};

#ifdef INPLACE
struct dirent64 {
  uint64_t       d_ino;       /* inode number */
  union {
    uint64_t       d_off;       /* not an offset; see NOTES */
    uint32_t       i32[2];
  } fudge;
  unsigned short d_reclen;    /* length of this record */
  unsigned char  d_type;      /* type of file; not supported
                                 by all file system types */
  char           d_name[256]; /* filename */
};

#else
struct dirent64 {
  uint64_t       d_ino;       /* inode number */
  uint64_t       d_off;       /* not an offset; see NOTES */
  unsigned short d_reclen;    /* length of this record */
  unsigned char  d_type;      /* type of file; not supported
                                 by all file system types */
  char           d_name[256]; /* filename */
};
#endif

struct stat32 {
  dev_t     st_dev;     /* ID of device containing file */
  unsigned short int pad1;
  uint32_t  st_ino;     /* inode number */
  mode_t    st_mode;    /* protection */
  nlink_t   st_nlink;   /* number of hard links */
  uid_t     st_uid;     /* user ID of owner */
  gid_t     st_gid;     /* group ID of owner */
  dev_t     st_rdev;    /* device ID (if special file) */
  unsigned short int   pad2;
  off_t     st_size;    /* total size, in bytes */
  blksize_t st_blksize; /* blocksize for file system I/O */
  blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
  struct timespec    st_atim;   /* time of last access */
  struct timespec    st_mtim;   /* time of last modification */
  struct timespec    st_ctim;   /* time of last status change */
  uint32_t unused1;
  uint32_t unused2;
};

struct stat64 {
  dev_t     st_dev;     /* ID of device containing file */
  unsigned short int pad1;
  uint32_t  st_ino32;     /* inode number */
  mode_t    st_mode;    /* protection */
  nlink_t   st_nlink;   /* number of hard links */
  uid_t     st_uid;     /* user ID of owner */
  gid_t     st_gid;     /* group ID of owner */
  dev_t     st_rdev;    /* device ID (if special file) */
  //  unsigned short int   pad2;
  unsigned int pad2;
  uint64_t  st_size;    /* total size, in bytes */
  blksize_t st_blksize; /* blocksize for file system I/O */
  uint64_t  st_blocks;  /* number of 512B blocks allocated */
  struct timespec    st_atim;   /* time of last access */
  struct timespec    st_mtim;   /* time of last modification */
  struct timespec    st_ctim;   /* time of last status change */
  uint64_t  st_ino;
};


typedef struct __dirstream DIR;
struct dirent64 *readdir64(DIR *dirp);

#ifdef INPLACE
struct dirent32 *readdir(DIR *dirp){
  struct dirent64 *ptr;
  int inode,ioff;

  ptr=readdir64(dirp);

  if (!ptr) return NULL;

  inode=ptr->d_ino^(ptr->d_ino>>32);
  ioff=ptr->fudge.d_off;
  ptr->fudge.i32[0]=inode;
  ptr->fudge.i32[1]=ioff;
  ptr->d_reclen-=8;

  return (struct dirent32 *)&(ptr->fudge);
}
#else
struct dirent32 *readdir(DIR *dirp){
  static struct dirent32 d32;
  struct dirent64 *ptr;
  int i;

  ptr=readdir64(dirp);

  if (!ptr) return NULL;

  d32.d_ino=ptr->d_ino^(ptr->d_ino>>32);
  d32.d_off=ptr->d_off;
  d32.d_reclen=ptr->d_reclen-8;
  d32.d_type=ptr->d_type;
  for(i=0;i<256;i++) d32.d_name[i]=ptr->d_name[i];

  return &d32;
}
#endif

int __fxstat64 (int ver, int fd, struct stat64 *buf);

int __fxstat (int ver, int fd, struct stat32 *buf){
  struct stat64 s64;
  int i;

  i=__fxstat64(ver,fd,&s64);
  if (i) return i;

  buf->st_dev=s64.st_dev;
  buf->st_ino=s64.st_ino^(s64.st_ino>>32);
  buf->st_mode=s64.st_mode;
  buf->st_nlink=s64.st_nlink;
  buf->st_uid=s64.st_uid;
  buf->st_gid=s64.st_gid;
  buf->st_rdev=s64.st_rdev;
  if (s64.st_size>>32) {errno=EOVERFLOW; return -1;} 
  buf->st_size=s64.st_size;
  buf->st_blksize=s64.st_blksize;
  buf->st_blocks=s64.st_blocks;
  buf->st_atim=s64.st_atim;
  buf->st_mtim=s64.st_mtim;
  buf->st_ctim=s64.st_ctim;

  return 0;
}

int __xstat64 (int ver, const char *path, struct stat64 *buf);

int __xstat (int ver, const char* path, struct stat32 *buf){
  struct stat64 s64;
  int i;

  i=__xstat64(ver,path,&s64);
  if (i) return i;

  buf->st_dev=s64.st_dev;
  buf->st_ino=s64.st_ino^(s64.st_ino>>32);
  buf->st_mode=s64.st_mode;
  buf->st_nlink=s64.st_nlink;
  buf->st_uid=s64.st_uid;
  buf->st_gid=s64.st_gid;
  buf->st_rdev=s64.st_rdev;
  if (s64.st_size>>32) {errno=EOVERFLOW; return -1;} 
  buf->st_size=s64.st_size;
  buf->st_blksize=s64.st_blksize;
  buf->st_blocks=s64.st_blocks;
  buf->st_atim=s64.st_atim;
  buf->st_mtim=s64.st_mtim;
  buf->st_ctim=s64.st_ctim;

  return 0;
}

int __lxstat64 (int ver, const char *path, struct stat64 *buf);

int __lxstat (int ver, const char* path, struct stat32 *buf){
  struct stat64 s64;
  int i;

  i=__lxstat64(ver,path,&s64);
  if (i) return i;

  buf->st_dev=s64.st_dev;
  buf->st_ino=s64.st_ino^(s64.st_ino>>32);
  buf->st_mode=s64.st_mode;
  buf->st_nlink=s64.st_nlink;
  buf->st_uid=s64.st_uid;
  buf->st_gid=s64.st_gid;
  buf->st_rdev=s64.st_rdev;
  if (s64.st_size>>32) {errno=EOVERFLOW; return -1;} 
  buf->st_size=s64.st_size;
  buf->st_blksize=s64.st_blksize;
  buf->st_blocks=s64.st_blocks;
  buf->st_atim=s64.st_atim;
  buf->st_mtim=s64.st_mtim;
  buf->st_ctim=s64.st_ctim;

  return 0;
}

