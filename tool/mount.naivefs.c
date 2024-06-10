#define FUSE_USE_VERSION 37

#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define align(a, n) (((((unsigned long)(a)) + (n)-1)) & ~((n)-1))

int fd;

typedef struct {
  char magic[8];
  int64_t sectors;
  int64_t block_bitmap_size;
  int64_t inode_bitmap_size;
  int64_t max_filename_length;
  int64_t root_inode;
  int64_t root_block;
} naivefs_super_block_t;

naivefs_super_block_t super_block;

typedef struct {
  char name[56];
  int64_t inode;
} dir_entry_t;

typedef struct {
  int16_t type;
  int16_t mode;
  int32_t size;
  int64_t access_time;
  int64_t modify_time;
  int64_t link_count;
  int64_t child_count;
  int32_t user;
  int32_t group;
  int64_t pad[1];
} file_header_t;

typedef struct {
  file_header_t header;
  dir_entry_t entry[63];
  int64_t second_inode;
} dir_t;

typedef struct {
  file_header_t header;
  int64_t block[504];
  int64_t second_inode;
} file_t;

typedef struct {
  file_header_t header;
  char target[4032];
  int64_t second_inode;
} soft_link_t;

typedef struct {
  dir_entry_t entry[63];
  int64_t pad[7];
  int64_t second_inode;
} second_dir_t;

typedef struct {
  int64_t block[504];
  int64_t pad[7];
  int64_t second_inode;
} second_file_t;

unsigned int low_bit(unsigned int i) { return i & -i; }

long inode_to_offset(long inode) { return 4096 * (super_block.root_inode + inode); }

unsigned char *inode_map;
unsigned char *block_map;

long get_free_inode() {
  long inode = 0;
  for (int i = 0; i < 4096 * super_block.inode_bitmap_size; ++i) {
    if (inode_map[i] != 0xff) {
      inode = i * 8;
      unsigned low = low_bit(~inode_map[i]);
      while (low != 1) {
        inode += 1;
        low >>= 1u;
      }
      inode_map[i] |= low_bit(~inode_map[i]);
      pwrite(fd, inode_map, 4096 * super_block.inode_bitmap_size, 4096 * (1 + super_block.block_bitmap_size));
      fsync(fd);
      return inode;
    }
  }
  return 0;
}

long free_inode(unsigned long inode) {
  unsigned char position;
  pread(fd, &position, 1, 4096 * (1 + super_block.block_bitmap_size) + inode / 8);
  position |= 1u << (inode % 8);
  pwrite(fd, &position, 1, 4096 * (1 + super_block.block_bitmap_size) + inode / 8);
  fsync(fd);
  return 0;
}

long get_free_block() {
  long block = 0;
  for (int i = 0; i < 4096 * super_block.block_bitmap_size; ++i) {
    if (block_map[i] != 0xff) {
      block = i * 8;
      unsigned low = low_bit(~block_map[i]);
      while (low != 1) {
        block += 1;
        low >>= 1u;
      }
      block_map[i] |= low_bit(~block_map[i]);
      pwrite(fd, block_map, 4096 * super_block.block_bitmap_size, 4096);
      fsync(fd);
      return block;
    }
  }
  return 0;
}

long create_subdir(char *name, long inode, mode_t mode) {
  long subdir_inode = get_free_inode();
  dir_t *subdir = malloc(sizeof(dir_t));
  memset(subdir, 0, sizeof(dir_t));
  subdir->header.type = 0;
  subdir->header.mode = mode;
  subdir->header.size = 4096;
  subdir->header.access_time = time(0);
  subdir->header.modify_time = time(0);
  subdir->header.link_count = 2;
  subdir->header.child_count = 2;
  subdir->header.user = 1000;
  subdir->header.group = 1000;
  memset(subdir->header.pad, 0, sizeof(subdir->header.pad));
  memset(subdir->entry, 0, sizeof(subdir->entry));
  memcpy(subdir->entry[0].name, ".", 2);
  subdir->entry[0].inode = subdir_inode;
  memcpy(subdir->entry[1].name, "..", 3);
  subdir->entry[1].inode = inode;
  pwrite(fd, subdir, sizeof(dir_t), inode_to_offset(subdir_inode));
  fsync(fd);
  free(subdir);
  dir_t dir;
  second_dir_t second;
  long second_inode = dir.second_inode;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = subdir_inode;
        dir.header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        second.second_inode = get_free_inode();
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        second.second_inode = dir.second_inode = get_free_inode();
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(inode));
  fsync(fd);
  return subdir_inode;
}

long create_file(char *name, long inode, mode_t mode) {
  long file_inode = get_free_inode();
  file_t *subdir = malloc(sizeof(dir_t));
  memset(subdir, 0, sizeof(dir_t));
  subdir->header.type = 1;
  subdir->header.mode = mode;
  subdir->header.size = 0;
  subdir->header.access_time = time(0);
  subdir->header.modify_time = time(0);
  subdir->header.link_count = 1;
  subdir->header.child_count = 0;
  subdir->header.user = 1000;
  subdir->header.group = 1000;
  memset(subdir->header.pad, 0, sizeof(subdir->header.pad));
  memset(subdir->block, 0, sizeof(subdir->block));
  pwrite(fd, subdir, sizeof(dir_t), inode_to_offset(file_inode));
  fsync(fd);
  free(subdir);
  dir_t dir;
  second_dir_t second;
  long second_inode = dir.second_inode;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = file_inode;
        dir.header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        second.second_inode = get_free_inode();
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        second.second_inode = dir.second_inode = get_free_inode();
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(inode));
  fsync(fd);
  return file_inode;
}

long link_file(char *name, long file_inode, long inode) {
  dir_t dir;
  second_dir_t second;
  long second_inode = dir.second_inode;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = file_inode;
        dir.header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        second.second_inode = get_free_inode();
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        second.second_inode = dir.second_inode = get_free_inode();
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(inode));
  fsync(fd);
  return file_inode;
}

long create_soft_link(char *name, long inode, char *source) {
  long file_inode = get_free_inode();
  soft_link_t *subdir = malloc(sizeof(dir_t));
  memset(subdir, 0, sizeof(dir_t));
  subdir->header.type = 2;
  subdir->header.mode = 0777;
  subdir->header.size = 0;
  subdir->header.access_time = time(0);
  subdir->header.modify_time = time(0);
  subdir->header.link_count = 1;
  subdir->header.child_count = 0;
  subdir->header.user = 1000;
  subdir->header.group = 1000;
  memset(subdir->header.pad, 0, sizeof(subdir->header.pad));
  strcpy(subdir->target, source);
  pwrite(fd, subdir, sizeof(dir_t), inode_to_offset(file_inode));
  fsync(fd);
  free(subdir);
  dir_t dir;
  second_dir_t second;
  long second_inode = dir.second_inode;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = file_inode;
        dir.header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        second.second_inode = get_free_inode();
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        second.second_inode = dir.second_inode = get_free_inode();
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(inode));
  fsync(fd);
  return file_inode;
}

long unlink_file(char *name, long parent_inode) {
  dir_t dir;
  second_dir_t second;
  pread(fd, &dir, sizeof(dir), inode_to_offset(parent_inode));
  long second_inode = dir.second_inode;
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (strcmp(name, entry[i].name) == 0) {
        found = true;
        entry[i].inode = 0;
        memset(entry[i].name, 0, sizeof(entry[i].name));
        dir.header.child_count -= 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        return -EFAULT;
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        return -EFAULT;
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(parent_inode));
  fsync(fd);
  return 0;
}

long remove_subdir(char *name, long parent_inode) {
  dir_t dir;
  second_dir_t second;
  pread(fd, &dir, sizeof(dir), inode_to_offset(parent_inode));
  long second_inode = dir.second_inode;
  dir_entry_t *entry = dir.entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (strcmp(name, entry[i].name) == 0) {
        found = true;
        entry[i].inode = 0;
        memset(entry[i].name, 0, sizeof(entry[i].name));
        dir.header.child_count -= 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir.entry) {
      if (second.second_inode == 0)
        return -EFAULT;
      pwrite(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
      fsync(fd);
    } else {
      if (dir.second_inode == 0)
        return -EFAULT;
    }
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
    second_inode = second.second_inode;
    entry = second.entry;
  }
  pwrite(fd, &dir, sizeof(dir), inode_to_offset(parent_inode));
  fsync(fd);
  return 0;
}

long free_block(unsigned long block) {
  unsigned char position;
  pread(fd, &position, 1, 4096 + block / 8);
  position |= 1u << (block % 8);
  pwrite(fd, &position, 1, 4096 + block / 8);
  fsync(fd);
  return 0;
}

long free_blocks(long inode) {
  file_t file;
  second_file_t second;
  pread(fd, &file, sizeof(file), inode_to_offset(inode));
  second.second_inode = file.second_inode;
  int64_t *blocks = file.block;
  while (true) {
    for (int i = 0; i < 504; ++i) {
      if (blocks[i] != 0) {
        free_block(blocks[i]);
      }
    }
    if (second.second_inode == 0)
      return 0;
    free_inode(second.second_inode);
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    blocks = second.block;
  }
}

long find_inode_from_path_helper(const char *path, long inode) {
  if (path[0] == '/' && path[1] == 0)
    return inode;
  else if (path[0] == '/')
    return find_inode_from_path_helper(path + 1, inode);
  char name[super_block.max_filename_length];
  char *index = strchr(path, '/');
  unsigned long length;
  if (index == NULL) {
    length = strlen(path);
  } else {
    length = index - path;
  }
  memcpy(name, path, length);
  name[length] = 0;
  dir_t dir;
  second_dir_t second;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  second.second_inode = dir.second_inode;
  dir_entry_t *entry = dir.entry;
  while (true) {
    for (int i = 0; i < 63; ++i) {
      if (entry[i].inode != 0 && strcmp(name, entry[i].name) == 0) {
        if (index == NULL)
          return entry[i].inode;
        else
          return find_inode_from_path_helper(index + 1, entry[i].inode);
      }
    }
    if (second.second_inode == 0)
      return index == NULL ? -1 : -2;
    pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second.second_inode));
    entry = second.entry;
  }
}

long find_inode_from_path(const char *path) { return find_inode_from_path_helper(path, 2); }

void resize_file(long inode, long __new_size) {
  file_t file;
  second_file_t second;
  pread(fd, &file, sizeof(file), inode_to_offset(inode));
  second.second_inode = file.second_inode;
  long second_inode = file.second_inode;
  int64_t *blocks = file.block;
  long old_size = align(file.header.size, 4096ul);
  long new_size = align(__new_size, 4096ul);
  if (new_size > old_size) {
    long current = 0;
    while (current * 4096 < new_size) {
      if (current * 4096 >= old_size) {
        long block = get_free_block();
        blocks[current % 504] = block;
      }
      if (current != 0 && current % 504 == 0) {
        if (blocks != file.block) {
          if (second.second_inode == 0)
            second.second_inode = get_free_inode();
          pwrite(fd, &second, sizeof(second_file_t), inode_to_offset(second_inode));
          fsync(fd);
          pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second.second_inode;
          blocks = second.block;
        } else {
          if (file.second_inode == 0)
            second.second_inode = file.second_inode = get_free_inode();
          pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second.second_inode;
          blocks = second.block;
        }
      }
      current++;
      printf("Current %ld finished, %ld in total.\n", current, new_size / 4096);
    }
  } else {
    long current = 0;
    while (current * 4096 <= old_size) {
      if (current * 4096 > new_size) {
        free_block(blocks[current % 504]);
        blocks[current % 504] = 0;
      }
      if (current != 0 && current % 504 == 0) {
        if (blocks != file.block) {
          if (second.second_inode == 0)
            second.second_inode = get_free_inode();
          pwrite(fd, &second, sizeof(second_file_t), inode_to_offset(second_inode));
          fsync(fd);
          pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second.second_inode;
          blocks = second.block;
        } else {
          if (file.second_inode == 0)
            second.second_inode = file.second_inode = get_free_inode();
          pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second.second_inode;
          blocks = second.block;
        }
      }
      current++;
    }
  }
  file.header.size = __new_size;
  pwrite(fd, &file, sizeof(file), inode_to_offset(inode));
}

int write_file(long inode, char *buffer, size_t size, off_t offset) {
  file_t file;
  second_file_t second;
  int length = 0;
  pread(fd, &file, sizeof(file), inode_to_offset(inode));
  second.second_inode = file.second_inode;
  long second_inode = file.second_inode;
  int64_t *blocks = file.block;
  long current = 0;
  while (current * 4096 < (long)(size + offset)) {
    if (current * 4096 <= offset && current * 4096 + 4096 > offset) {
      if (current * 4096 + 4096 >= (long)(offset + size)) {
        pwrite(fd, buffer, size, 4096 * blocks[current % 504] + offset % 4096);
        length += size;
      } else {
        pwrite(fd, buffer, 4096 - offset % 4096, 4096 * blocks[current % 504] + offset % 4096);
        length += 4096 - offset % 4096;
      }
    } else if (current * 4096 >= offset && current * 4096 + 4096 < (long)(offset + size)) {
      pwrite(fd, buffer + current * 4096 - offset, 4096, 4096 * blocks[current % 504]);
      length += 4096;
    } else if (current * 4096 < (long)(offset + size) && current * 4096 + 4096 >= (long)(offset + size)) {
      pwrite(fd, buffer + current * 4096 - offset, (offset + size) % 4096, 4096 * blocks[current % 504]);
      length += (int)(offset + size) % 4096;
    }
    if (current != 0 && current % 504 == 0) {
      if (blocks != file.block) {
        if (second.second_inode == 0)
          second.second_inode = get_free_inode();
        pwrite(fd, &second, sizeof(second_file_t), inode_to_offset(second_inode));
        fsync(fd);
        pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second.second_inode;
        blocks = second.block;
      } else {
        if (file.second_inode == 0)
          second.second_inode = file.second_inode = get_free_inode();
        pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second.second_inode;
        blocks = second.block;
      }
    }
    current += 1;
  }
  return length;
}

int read_file(long inode, char *buffer, size_t size, off_t offset) {
  file_t file;
  second_file_t second;
  int length = 0;
  pread(fd, &file, sizeof(file), inode_to_offset(inode));
  second.second_inode = file.second_inode;
  long second_inode = file.second_inode;
  int64_t *blocks = file.block;
  long current = 0;
  while (current * 4096 < (long)(size + offset)) {
    if (current * 4096 <= offset && current * 4096 + 4096 > offset) {
      if (current * 4096 + 4096 >= (long)(offset + size)) {
        pread(fd, buffer, size, 4096 * blocks[current % 504] + offset % 4096);
        length += size;
      } else {
        pread(fd, buffer, 4096 - offset % 4096, 4096 * blocks[current % 504] + offset % 4096);
        length += 4096 - offset % 4096;
      }
    } else if (current * 4096 >= offset && current * 4096 + 4096 < (long)(offset + size)) {
      pread(fd, buffer + current * 4096 - offset, 4096, 4096 * blocks[current % 504]);
      length += 4096;
    } else if (current * 4096 < (long)(offset + size) && current * 4096 + 4096 >= (long)(offset + size)) {
      pread(fd, buffer + current * 4096 - offset, (offset + size) % 4096, 4096 * blocks[current % 504]);
      length += (int)(offset + size) % 4096;
    }
    if (current != 0 && current % 504 == 0) {
      if (blocks != file.block) {
        if (second.second_inode == 0)
          return length;
        pwrite(fd, &second, sizeof(second_file_t), inode_to_offset(second_inode));
        fsync(fd);
        pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second.second_inode;
        blocks = second.block;
      } else {
        if (file.second_inode == 0)
          return length;
        pread(fd, &second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second.second_inode;
        blocks = second.block;
      }
    }
    current += 1;
  }
  return length;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored. The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given. In that case it is passed to userspace,
 * but libfuse and the kernel will still assign a different
 * inode for internal use (called the "nodeid").
 *
 * `fi` will always be NULL if the file is not currently open, but
 * may also be NULL if the file is open.
 */
int naive_getattr(const char *path, struct stat *buf, struct fuse_file_info *fi) {
  printf("getattr %s\n", path);
  memset(buf, 0, sizeof(struct stat));
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  dir_t dir;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  if (dir.header.type == 0)
    buf->st_mode = S_IFDIR | dir.header.mode;
  else if (dir.header.type == 2)
    buf->st_mode = S_IFLNK | dir.header.mode;
  else
    buf->st_mode = S_IFREG | dir.header.mode;
  buf->st_size = dir.header.size;
  buf->st_uid = dir.header.user;
  buf->st_gid = dir.header.group;
  buf->st_blocks = align(dir.header.size, 512u) / 512;
  buf->st_ino = inode;
  buf->st_nlink = dir.header.link_count;
  buf->st_atim.tv_sec = dir.header.access_time;
  buf->st_mtim.tv_sec = dir.header.modify_time;
  return 0;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.	If the linkname is too long to fit in the
 * buffer, it should be truncated.	The return value should be 0
 * for success.
 */
int naive_readlink(const char *path, char *link, size_t size) {
  printf("chmod %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  soft_link_t l;
  pread(fd, &l, sizeof(l), inode_to_offset(inode));
  if (l.header.type != 2) {
    return -EINVAL;
  }
  strncpy(link, l.target, size);
  return 0;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
int naive_mknod(const char *path, mode_t mode, dev_t dev) {
  printf("mknod %s\n", path);
  char *base_s = strdup(path);
  char *dir_s = strdup(path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_file(base, inode, mode);
  free(dir_s);
  free(base_s);
  return 0;
}

/** Create a directory */
int naive_mkdir(const char *path, mode_t mode) {
  printf("mkdir %s\n", path);
  char *base_s = strdup(path);
  char *dir_s = strdup(path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_subdir(base, inode, mode);
  free(dir_s);
  free(base_s);
  return 0;
}

/** Remove a file */
int naive_unlink(const char *path) {
  printf("rmdir %s\n", path);
  char *base_s = strdup(path);
  char *dir_s = strdup(path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  if (header.link_count == 1) {
    free_inode(inode);
    free_blocks(inode);
  }
  inode = find_inode_from_path(dir);
  if (inode == -1 || inode == -2)
    return -ENOTDIR;
  unlink_file(base, inode);
  free(dir_s);
  free(base_s);
  return 0;
}

/** Remove a directory */
int naive_rmdir(const char *path) {
  printf("rmdir %s\n", path);
  char *base_s = strdup(path);
  char *dir_s = strdup(path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(file_header_t), inode_to_offset(inode));
  if (header.child_count > 2)
    return -ENOTEMPTY;
  free_inode(inode);
  inode = find_inode_from_path(dir);
  if (inode == -1 || inode == -2)
    return -ENOTDIR;
  remove_subdir(base, inode);
  free(dir_s);
  free(base_s);
  return 0;
}

/** Create a symbolic link */
int naive_symlink(const char *path, const char *link) {
  printf("symlink %s\n", path);
  char *base_s = strdup(link);
  char *dir_s = strdup(link);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  long parent_inode = find_inode_from_path(dir);
  if (parent_inode == -1)
    return -ENOENT;
  else if (parent_inode == -2)
    return -ENOTDIR;
  create_soft_link(base, parent_inode, path);
  return 0;
}

/** Rename a file */
int naive_link(const char *path, const char *new_path);
int naive_rename(const char *path, const char *new_path, unsigned int flags) {
  int ret = naive_link(path, new_path);
  if (ret < 0)
    return ret;
  ret = naive_unlink(path);
  if (ret < 0)
    return ret;
}

/** Create a hard link to a file */
int naive_link(const char *path, const char *new_path) {
  printf("link %s\n", path);
  char *base_s = strdup(new_path);
  char *dir_s = strdup(new_path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  long parent_inode = find_inode_from_path(dir);
  if (parent_inode == -1)
    return -ENOENT;
  else if (parent_inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  header.link_count += 1;
  link_file(base, inode, parent_inode);
  pwrite(fd, &header, sizeof(header), inode_to_offset(inode));
  return 0;
}

/** Change the permission bits of a file */
int naive_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
  printf("chmod %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  header.mode = mode;
  pwrite(fd, &header, sizeof(header), inode_to_offset(inode));
  fsync(fd);
  return 0;
}

/** Change the owner and group of a file */
int naive_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
  printf("chmod %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  header.user = uid;
  header.group = gid;
  pwrite(fd, &header, sizeof(header), inode_to_offset(inode));
  fsync(fd);
  return 0;
}

/** Change the size of a file */
int naive_truncate(const char *path, off_t new_size, struct fuse_file_info *fi) {
  printf("truncate %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  resize_file(inode, new_size);
  return 0;
}

/** Open a file
 *
 * Open flags are available in fi->flags. The following rules
 * apply.
 *
 *  - Creation (O_CREAT, O_EXCL, O_NOCTTY) flags will be
 *    filtered out / handled by the kernel.
 *
 *  - Access modes (O_RDONLY, O_WRONLY, O_RDWR, O_EXEC, O_SEARCH)
 *    should be used by the filesystem to check if the operation is
 *    permitted.  If the ``-o default_permissions`` mount option is
 *    given, this check is already done by the kernel before calling
 *    open() and may thus be omitted by the filesystem.
 *
 *  - When writeback caching is enabled, the kernel may send
 *    read requests even for files opened with O_WRONLY. The
 *    filesystem should be prepared to handle this.
 *
 *  - When writeback caching is disabled, the filesystem is
 *    expected to properly handle the O_APPEND flag and ensure
 *    that each write is appending to the end of the file.
 *
 *  - When writeback caching is enabled, the kernel will
 *    handle O_APPEND. However, unless all changes to the file
 *    come through the kernel this will not work reliably. The
 *    filesystem should thus either ignore the O_APPEND flag
 *    (and let the kernel handle it), or return an error
 *    (indicating that reliably O_APPEND is not available).
 *
 * Filesystem may store an arbitrary file handle (pointer,
 * index, etc) in fi->fh, and use this in other all other file
 * operations (read, write, flush, release, fsync).
 *
 * Filesystem may also implement stateless file I/O and not store
 * anything in fi->fh.
 *
 * There are also some flags (direct_io, keep_cache) which the
 * filesystem may set in fi, to change the way the file is opened.
 * See fuse_file_info structure in <fuse_common.h> for more details.
 *
 * If this request is answered with an error code of ENOSYS
 * and FUSE_CAP_NO_OPEN_SUPPORT is set in
 * `fuse_conn_info.capable`, this is treated as success and
 * future calls to open will also succeed without being send
 * to the filesystem process.
 *
 */
int naive_open(const char *path, struct fuse_file_info *fi) {
  printf("open %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t file;
  pread(fd, &file, sizeof(file), inode_to_offset(inode));
  if (file.header.type != 1)
    return -EISDIR;
  file.header.access_time = time(NULL);
  if (fi->flags & O_TRUNC) {
    file.header.modify_time = time(NULL);
    resize_file(inode, 0);
  }
  pwrite(fd, &file, sizeof(file), inode_to_offset(inode));
  fsync(fd);
  return 0;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int naive_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  printf("read %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t root;
  pread(fd, &root, sizeof(root), inode_to_offset(inode));
  if (offset > root.header.size) {
    size = 0;
  } else if (offset + size > root.header.size) {
    size = root.header.size - offset;
  }
  int bits = read_file(inode, buf, size, offset);
  pread(fd, &root, sizeof(root), inode_to_offset(inode));
  root.header.access_time = time(0);
  pwrite(fd, &root, sizeof(root), inode_to_offset(inode));
  return bits;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int naive_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  printf("write %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t root;
  pread(fd, &root, sizeof(root), inode_to_offset(inode));
  if (root.header.size < (int32_t)(offset + size))
    resize_file(inode, (long)(offset + size));
  int bits = write_file(inode, buf, size, offset);
  pread(fd, &root, sizeof(root), inode_to_offset(inode));
  root.header.access_time = root.header.modify_time = time(0);
  pwrite(fd, &root, sizeof(root), inode_to_offset(inode));
  return bits;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int naive_statfs(const char *path, struct statvfs *statv) { return 0; }

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int naive_flush(const char *path, struct fuse_file_info *fi) {
  printf("flush %s\n", path);
  fsync(fd);
  return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int naive_release(const char *path, struct fuse_file_info *fi) { return 0; }

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int naive_fsync(const char *path, int datasync, struct fuse_file_info *fi) {
  fsync(fd);
  return 0;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int naive_opendir(const char *path, struct fuse_file_info *fi) {
  printf("opendir %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1) {
    return -ENOENT;
  } else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  if (header.type != 0)
    return -ENOTDIR;
  return 0;
}

/** Read directory
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 */
int naive_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
  printf("readdir %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  dir_t dir;
  pread(fd, &dir, sizeof(dir), inode_to_offset(inode));
  if (dir.header.type != 0)
    return -ENOTDIR;
  second_dir_t second;
  second.second_inode = dir.second_inode;
  dir_entry_t *entries = dir.entry;
  while (true) {
    for (int i = 0; i < 63; ++i) {
      if (entries[i].inode != 0) {
        filler(buf, entries[i].name, NULL, 0, 0);
      }
    }
    if (second.second_inode != 0) {
      pread(fd, &second, sizeof(second), inode_to_offset(second.second_inode));
      entries = second.entry;
    } else {
      break;
    }
  }
  return 0;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int naive_releasedir(const char *path, struct fuse_file_info *fi) { return 0; }

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
int naive_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi) {
  fsync(fd);
  return 0;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the `private_data` field of
 * `struct fuse_context` to all file operations, and as a
 * parameter to the destroy() method. It overrides the initial
 * value provided to fuse_main() / fuse_new().
 */
void *naive_init(struct fuse_conn_info *conn, struct fuse_config *conf) {
  conf->use_ino = 1;
  return NULL;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void naive_destroy(void *userdata) { fsync(fd); }

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int naive_access(const char *path, int mask) {
  printf("access %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1) {
    return -ENOENT;
  } else if (inode == -2)
    return -ENOTDIR;
  return 0;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int naive_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  printf("create %s\n", path);
  char *base_s = strdup(path);
  char *dir_s = strdup(path);
  char *base = basename(base_s);
  char *dir = dirname(dir_s);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_file(base, inode, mode);
  free(dir_s);
  free(base_s);
  return 0;
}

/**
 * Change the access and modification times of a file with
 * nanosecond resolution
 *
 * This supersedes the old utime() interface.  New applications
 * should use this.
 *
 * `fi` will always be NULL if the file is not currenlty open, but
 * may also be NULL if the file is open.
 *
 * See the utimensat(2) man page for details.
 */
int naive_utimens(const char *path, const struct timespec tv[2], struct fuse_file_info *fi) {
  printf("access %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1) {
    return -ENOENT;
  } else if (inode == -2)
    return -ENOTDIR;
  file_header_t header;
  pread(fd, &header, sizeof(header), inode_to_offset(inode));
  header.access_time = tv[0].tv_sec;
  header.modify_time = tv[1].tv_sec;
  return 0;
}

struct fuse_operations bb_oper = {.getattr = naive_getattr,
                                  .readlink = naive_readlink,
                                  .mknod = naive_mknod,
                                  .mkdir = naive_mkdir,
                                  .unlink = naive_unlink,
                                  .rmdir = naive_rmdir,
                                  .symlink = naive_symlink,
                                  .rename = naive_rename,
                                  .link = naive_link,
                                  .chmod = naive_chmod,
                                  .chown = naive_chown,
                                  .truncate = naive_truncate,
                                  .open = naive_open,
                                  .read = naive_read,
                                  .write = naive_write,
                                  /** Just a placeholder, don't set */
                                  .statfs = naive_statfs,
                                  .flush = naive_flush,
                                  .release = naive_release,
                                  .fsync = naive_fsync,
                                  .opendir = naive_opendir,
                                  .readdir = naive_readdir,
                                  .releasedir = naive_releasedir,
                                  .fsyncdir = naive_fsyncdir,
                                  .init = naive_init,
                                  .destroy = naive_destroy,
                                  .access = naive_access,
                                  .create = naive_create,
                                  .utimens = naive_utimens};

int main(int argc, char *argv[]) {
  int fuse_stat;

  if (argc < 3)
    printf("no enough param given.");
  fd = open(argv[1], O_RDWR | O_EXCL);

  if (fd == -1) {
    error(EXIT_FAILURE, errno, "%s", argv[1]);
  }

  pread(fd, &super_block, sizeof(super_block), 0);
  for (int i = 2; i < argc; ++i) {
    argv[i - 1] = argv[i];
  }
  inode_map = malloc(4096 * super_block.inode_bitmap_size);
  pread(fd, inode_map, 4096 * super_block.inode_bitmap_size, 4096 * (1 + super_block.block_bitmap_size));

  block_map = malloc(4096 * super_block.block_bitmap_size);
  pread(fd, block_map, 4096 * super_block.block_bitmap_size, 4096);

  // turn over control to fuse
  fuse_stat = fuse_main(argc - 1, argv, &bb_oper, NULL);

  return fuse_stat;
}
