#ifndef PROJECT4_VIRTUAL_MEMORY_NAIVEFS_H
#define PROJECT4_VIRTUAL_MEMORY_NAIVEFS_H

#include "stddef.h"

/* File types.  */
#define S_IFDIR 0040000  /* Directory.  */
#define S_IFCHR 0020000  /* Character device.  */
#define S_IFBLK 0060000  /* Block device.  */
#define S_IFREG 0100000  /* Regular file.  */
#define S_IFIFO 0010000  /* FIFO.  */
#define S_IFLNK 0120000  /* Symbolic link.  */
#define S_IFSOCK 0140000 /* Socket.  */

typedef unsigned long ino_t;
typedef int mode_t;
typedef int nlink_t;
typedef unsigned long uid_t;
typedef unsigned long gid_t;
typedef unsigned long off_t;

struct dirent {
  ino_t d_ino;             /* Inode number */
  off_t d_off;             /* Not an offset; see below */
  unsigned short d_reclen; /* Length of this record */
  unsigned char d_type;    /* Type of file; not supported by all filesystem types */
  char d_name[256];        /* Null-terminated filename */
};

enum {
  DT_UNKNOWN = 0,
#define DT_UNKNOWN DT_UNKNOWN
  DT_FIFO = 1,
#define DT_FIFO DT_FIFO
  DT_CHR = 2,
#define DT_CHR DT_CHR
  DT_DIR = 4,
#define DT_DIR DT_DIR
  DT_BLK = 6,
#define DT_BLK DT_BLK
  DT_REG = 8,
#define DT_REG DT_REG
  DT_LNK = 10,
#define DT_LNK DT_LNK
  DT_SOCK = 12,
#define DT_SOCK DT_SOCK
  DT_WHT = 14
#define DT_WHT DT_WHT
};

struct stat {
  ino_t st_ino;     /* File serial number.	*/
  mode_t st_mode;   /* File mode.  */
  nlink_t st_nlink; /* Link count.  */
  uid_t st_uid;     /* User ID of the file's owner.	*/
  gid_t st_gid;     /* Group ID of the file's group.*/
  size_t st_size;   /* Size of file, in bytes.  */
  long st_atime;    /* Time of last access.  */
  long st_mtime;    /* Time of last modification.  */
  long st_ctime;    /* Time of last status change.  */
};

int naive_getattr(const char *path, struct stat *buf);

int naive_mknod(const char *path, mode_t mode);

int naive_mkdir(const char *path, mode_t mode);

int naive_unlink(const char *path);

int naive_rmdir(const char *path);

int naive_chmod(const char *path, mode_t mode);

int naive_chown(const char *path, uid_t uid, gid_t gid);

int naive_truncate(const char *path, off_t new_size);

int naive_open(const char *path, int flags);

int naive_read(const char *path, char *buf, size_t size, off_t offset);

int naive_write(const char *path, const char *buf, size_t size, off_t offset);

int naive_opendir(const char *path);

int naive_readdir(const char *path, void *buf);

int naive_access(const char *path, int mask);

int naive_create(const char *path, mode_t mode);

int naive_init();

#endif // PROJECT4_VIRTUAL_MEMORY_NAIVEFS_H
