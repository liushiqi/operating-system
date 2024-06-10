#include <naivefs.h>

#include <errno.h>
#include <page_table.h>
#include <sbi.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int fd = 2048;

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
  dir_entry_t entry[63];
  int64_t pad[7];
  int64_t second_inode;
} second_dir_t;

typedef struct {
  int64_t block[504];
  int64_t pad[7];
  int64_t second_inode;
} second_file_t;

static int pread(int fd, void *__buf, size_t size, off_t offset) {
  char *buf = __buf;
  int blocks = (int)(size / 512);
  int start = (int)(offset / 512);
  int i;
  for (i = 0; i + 64 <= blocks; i += 64) {
    sbi_sd_read(ka_to_pa((uintptr_t)(buf + 512 * i)), 64, fd + start + i);
  }
  if (i != blocks)
    sbi_sd_read(ka_to_pa((uintptr_t)buf + 512 * i), blocks - i, fd + start + i);
  return size;
}

static int pwrite(int fd, const void *__buf, size_t size, off_t offset) {
  const char *buf = __buf;
  int blocks = (int)(size / 512);
  int start = (int)(offset / 512 + 2048);
  int i;
  for (i = 0; i + 64 <= blocks; i += 64) {
    sbi_sd_write(ka_to_pa((uintptr_t)(buf + 512 * i)), 64, start + i);
  }
  if (i != blocks)
    sbi_sd_write(ka_to_pa((uintptr_t)buf + 512 * i), blocks - i, start + i);
  return size;
}

unsigned int low_bit(unsigned int i) { return i & -i; }

long inode_to_offset(long inode) { return 4096 * (super_block.root_inode + inode); }

long get_free_inode() {
  unsigned char *inode_map = (unsigned char *)alloc_page(super_block.inode_bitmap_size);
  long inode = 0;
  pread(fd, inode_map, 4096 * super_block.inode_bitmap_size, 4096 * (1 + super_block.block_bitmap_size));
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
      return inode;
    }
  }
  return 0;
}

long free_inode(unsigned long inode) {
  unsigned char *position = (unsigned char *)alloc_page(super_block.inode_bitmap_size);
  pread(fd, position, 4096 * super_block.inode_bitmap_size, 4096 * (1 + super_block.block_bitmap_size));
  position[inode / 8] |= 1u << (inode % 8);
  pwrite(fd, position, 4096 * super_block.inode_bitmap_size, 4096 * (1 + super_block.block_bitmap_size));
  free_page((uintptr_t)position);
  return 0;
}

long get_free_block() {
  unsigned char *block_map = (unsigned char *)alloc_page(super_block.block_bitmap_size);
  long block = 0;
  pread(fd, block_map, 4096 * super_block.block_bitmap_size, 4096);
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
      return block;
    }
  }
  free_page((uintptr_t)block_map);
  return 0;
}

long create_subdir(char *name, long inode, int mode) {
  long subdir_inode = get_free_inode();
  dir_t *subdir = (dir_t *)alloc_page(1);
  memset(subdir, 0, sizeof(dir_t));
  subdir->header.type = 0;
  subdir->header.mode = mode;
  subdir->header.size = 4096;
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
  free_page((uintptr_t)subdir);
  dir_t *dir = (dir_t *)alloc_page(1);
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  long second_inode = dir->second_inode;
  pread(fd, dir, sizeof(*dir), inode_to_offset(inode));
  dir_entry_t *entry = dir->entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = subdir_inode;
        dir->header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir->entry) {
      if (second->second_inode == 0)
        second->second_inode = get_free_inode();
      pwrite(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    } else {
      if (dir->second_inode == 0)
        second->second_inode = dir->second_inode = get_free_inode();
    }
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second->second_inode));
    second_inode = second->second_inode;
    entry = second->entry;
  }
  pwrite(fd, dir, sizeof(*dir), inode_to_offset(inode));
  free_page((uintptr_t)dir);
  free_page((uintptr_t)second);
  return subdir_inode;
}

long create_file(char *name, long inode, int mode) {
  long file_inode = get_free_inode();
  file_t *subdir = (file_t *)alloc_page(1);
  memset(subdir, 0, sizeof(dir_t));
  subdir->header.type = 1;
  subdir->header.mode = mode;
  subdir->header.size = 0;
  subdir->header.link_count = 2;
  subdir->header.child_count = 0;
  subdir->header.user = 1000;
  subdir->header.group = 1000;
  memset(subdir->header.pad, 0, sizeof(subdir->header.pad));
  memset(subdir->block, 0, sizeof(subdir->block));
  pwrite(fd, subdir, sizeof(dir_t), inode_to_offset(file_inode));
  free_page((uintptr_t)subdir);
  dir_t *dir = (dir_t *)alloc_page(1);
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  long second_inode = dir->second_inode;
  pread(fd, dir, sizeof(*dir), inode_to_offset(inode));
  dir_entry_t *entry = dir->entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (entry[i].name[0] == 0) {
        found = true;
        strncpy(entry[i].name, name, 48);
        entry[i].inode = file_inode;
        dir->header.child_count += 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir->entry) {
      if (second->second_inode == 0)
        second->second_inode = get_free_inode();
      pwrite(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    } else {
      if (dir->second_inode == 0)
        second->second_inode = dir->second_inode = get_free_inode();
    }
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second->second_inode));
    second_inode = second->second_inode;
    entry = second->entry;
  }
  pwrite(fd, dir, sizeof(*dir), inode_to_offset(inode));
  free_page((uintptr_t)dir);
  free_page((uintptr_t)second);
  return file_inode;
}

long unlink_file(char *name, long parent_inode) {
  dir_t *dir = (dir_t *)alloc_page(1);
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  pread(fd, dir, sizeof(*dir), inode_to_offset(parent_inode));
  long second_inode = dir->second_inode;
  dir_entry_t *entry = dir->entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (strcmp(name, entry[i].name) == 0) {
        found = true;
        entry[i].inode = 0;
        memset(entry[i].name, 0, sizeof(entry[i].name));
        dir->header.child_count -= 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir->entry) {
      if (second->second_inode == 0) {
        free_page((uintptr_t)dir);
        free_page((uintptr_t)second);
        return -EFAULT;
      }
      pwrite(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    } else {
      if (dir->second_inode == 0) {
        free_page((uintptr_t)dir);
        free_page((uintptr_t)second);
        return -EFAULT;
      }
    }
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    second_inode = second->second_inode;
    entry = second->entry;
  }
  pwrite(fd, dir, sizeof(*dir), inode_to_offset(parent_inode));
  free_page((uintptr_t)dir);
  free_page((uintptr_t)second);
  return 0;
}

long remove_subdir(char *name, long parent_inode) {
  dir_t *dir = (dir_t *)alloc_page(1);
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  pread(fd, dir, sizeof(*dir), inode_to_offset(parent_inode));
  long second_inode = dir->second_inode;
  dir_entry_t *entry = dir->entry;
  while (true) {
    bool found = false;
    for (int i = 0; i < 63; ++i) {
      if (strcmp(name, entry[i].name) == 0) {
        found = true;
        entry[i].inode = 0;
        memset(entry[i].name, 0, sizeof(entry[i].name));
        dir->header.child_count -= 1;
        break;
      }
    }
    if (found)
      break;
    if (entry != dir->entry) {
      if (second->second_inode == 0) {
        free_page((uintptr_t)dir);
        free_page((uintptr_t)second);
        return -EFAULT;
      }
      pwrite(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    } else {
      if (dir->second_inode == 0) {
        free_page((uintptr_t)dir);
        free_page((uintptr_t)second);
        return -EFAULT;
      }
    }
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
    second_inode = second->second_inode;
    entry = second->entry;
  }
  free_page((uintptr_t)dir);
  free_page((uintptr_t)second);
  pwrite(fd, dir, sizeof(*dir), inode_to_offset(parent_inode));
  return 0;
}

long free_block(unsigned long block) {
  unsigned char *position = (unsigned char *)alloc_page(1);
  pread(fd, position, 4096 * super_block.block_bitmap_size, 4096);
  position[block / 8] |= 1u << (block % 8);
  pwrite(fd, position, 4096 * super_block.block_bitmap_size, 4096);
  free_page((uintptr_t)position);
  return 0;
}

long free_blocks(long inode) {
  file_t *file = (file_t *)alloc_page(1);
  second_file_t *second = (second_file_t *)alloc_page(1);
  pread(fd, file, sizeof(*file), inode_to_offset(inode));
  second->second_inode = file->second_inode;
  int64_t *blocks = file->block;
  while (true) {
    for (int i = 0; i < 504; ++i) {
      if (blocks[i] != 0) {
        free_block(blocks[i]);
      }
    }
    if (second->second_inode == 0) {
      free_page((uintptr_t)file);
      return 0;
    }
    free_inode(second->second_inode);
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second->second_inode));
    blocks = second->block;
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
  dir_t *dir = (dir_t *)alloc_page(1);
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  pread(fd, dir, sizeof(*dir), inode_to_offset(inode));
  second->second_inode = dir->second_inode;
  dir_entry_t *entry = dir->entry;
  while (true) {
    for (int i = 0; i < 63; ++i) {
      if (entry[i].inode != 0 && strcmp(name, entry[i].name) == 0) {
        if (index == NULL)
          return entry[i].inode;
        else
          return find_inode_from_path_helper(index + 1, entry[i].inode);
      }
    }
    if (second->second_inode == 0)
      return index == NULL ? -1 : -2;
    pread(fd, second, sizeof(second_dir_t), inode_to_offset(second->second_inode));
    entry = second->entry;
  }
}

long find_inode_from_path(const char *path) { return find_inode_from_path_helper(path, 2); }

void resize_file(long inode, long __new_size) {
  file_t *file = (file_t *)alloc_page(1);
  second_file_t *second = (second_file_t *)alloc_page(1);
  pread(fd, file, sizeof(*file), inode_to_offset(inode));
  second->second_inode = file->second_inode;
  long second_inode = file->second_inode;
  int64_t *blocks = file->block;
  long old_size = align(file->header.size, 4096ul);
  long new_size = align(__new_size, 4096ul);
  if (new_size > old_size) {
    long current = 0;
    while (current * 4096 < new_size) {
      if (current * 4096 >= old_size) {
        long block = get_free_block();
        blocks[current % 504] = block;
      }
      if (current != 0 && current % 504 == 0) {
        if (blocks != file->block) {
          if (second->second_inode == 0)
            second->second_inode = get_free_inode();
          pwrite(fd, second, sizeof(second_file_t), inode_to_offset(second_inode));
          pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second->second_inode;
          blocks = second->block;
        } else {
          if (file->second_inode == 0)
            second->second_inode = file->second_inode = get_free_inode();
          pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second->second_inode;
          blocks = second->block;
        }
      }
      current++;
    }
  } else {
    long current = 0;
    while (current * 4096 <= old_size) {
      if (current * 4096 > new_size) {
        free_block(blocks[current % 504]);
        blocks[current % 504] = 0;
      }
      if (current != 0 && current % 504 == 0) {
        if (blocks != file->block) {
          if (second->second_inode == 0)
            second->second_inode = get_free_inode();
          pwrite(fd, second, sizeof(second_file_t), inode_to_offset(second_inode));
          pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second->second_inode;
          blocks = second->block;
        } else {
          if (file->second_inode == 0)
            second->second_inode = file->second_inode = get_free_inode();
          pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
          second_inode = second->second_inode;
          blocks = second->block;
        }
      }
      current++;
    }
  }
  file->header.size = __new_size;
  pwrite(fd, file, sizeof(*file), inode_to_offset(inode));
}

int write_file(long inode, const char *buffer, size_t size, off_t offset) {
  file_t *file = (file_t *)alloc_page(1);
  second_file_t *second = (second_file_t *)alloc_page(1);
  int length = 0;
  pread(fd, file, sizeof(*file), inode_to_offset(inode));
  second->second_inode = file->second_inode;
  long second_inode = file->second_inode;
  int64_t *blocks = file->block;
  long current = 0;
  while (current * 4096 < (long)(size + offset)) {
    if (current * 4096 <= (long)offset && current * 4096 + 4096 > (long)offset) {
      if (current * 4096 + 4096 >= (long)(offset + size)) {
        char *part_buffer = (char *)alloc_page(1);
        pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        memcpy(part_buffer + offset % 4096, buffer, size);
        pwrite(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        free_page((uintptr_t)part_buffer);
        length += size;
      } else {
        char *part_buffer = (char *)alloc_page(1);
        pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        memcpy(part_buffer + offset % 4096, buffer, 4096 - offset % 4096);
        pwrite(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        free_page((uintptr_t)part_buffer);
        length += 4096 - offset % 4096;
      }
    } else if (current * 4096 >= (long)offset && current * 4096 + 4096 < (long)(offset + size)) {
      pwrite(fd, buffer + current * 4096 - offset, 4096, 4096 * blocks[current % 504]);
      length += 4096;
    } else if (current * 4096 < (long)(offset + size) && current * 4096 + 4096 >= (long)(offset + size)) {
      char *part_buffer = (char *)alloc_page(1);
      pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
      memcpy(part_buffer + offset % 4096, buffer + current * 4096 - offset, (offset + size) % 4096);
      pwrite(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
      free_page((uintptr_t)part_buffer);
      length += (int)(offset + size) % 4096;
    }
    if (current != 0 && current % 504 == 0) {
      if (blocks != file->block) {
        if (second->second_inode == 0)
          second->second_inode = get_free_inode();
        pwrite(fd, second, sizeof(second_file_t), inode_to_offset(second_inode));
        pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second->second_inode;
        blocks = second->block;
      } else {
        if (file->second_inode == 0)
          second->second_inode = file->second_inode = get_free_inode();
        pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second->second_inode;
        blocks = second->block;
      }
    }
    current += 1;
  }
  free_page((uintptr_t)file);
  free_page((uintptr_t)second);
  return length;
}

int read_file(long inode, char *buffer, size_t size, off_t offset) {
  file_t *file = (file_t *)alloc_page(1);
  second_file_t *second = (second_file_t *)alloc_page(1);
  int length = 0;
  pread(fd, file, sizeof(*file), inode_to_offset(inode));
  second->second_inode = file->second_inode;
  long second_inode = file->second_inode;
  int64_t *blocks = file->block;
  long current = 0;
  while (current * 4096 < (long)(size + offset)) {
    if (current * 4096 <= (long)offset && current * 4096 + 4096 > (long)offset) {
      if (current * 4096 + 4096 >= (long)(offset + size)) {
        char *part_buffer = (char *)alloc_page(1);
        pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        memcpy(buffer, part_buffer + offset % 4096, size);
        free_page((uintptr_t)part_buffer);
        length += size;
      } else {
        char *part_buffer = (char *)alloc_page(1);
        pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
        memcpy(buffer, part_buffer + offset % 4096, 4096 - offset % 4096);
        free_page((uintptr_t)part_buffer);
        length += 4096 - offset % 4096;
      }
    } else if (current * 4096 >= (long)offset && current * 4096 + 4096 < (long)(offset + size)) {
      pread(fd, buffer + current * 4096 - offset, 4096, 4096 * blocks[current % 504]);
      length += 4096;
    } else if (current * 4096 < (long)(offset + size) && current * 4096 + 4096 >= (long)(offset + size)) {
      char *part_buffer = (char *)alloc_page(1);
      pread(fd, part_buffer, 4096, 4096 * blocks[current % 504]);
      memcpy(buffer + current * 4096 - offset, part_buffer + offset % 4096, (offset + size) % 4096);
      free_page((uintptr_t)part_buffer);
      length += (int)(offset + size) % 4096;
    }
    if (current != 0 && current % 504 == 0) {
      if (blocks != file->block) {
        if (second->second_inode == 0) {
          free_page((uintptr_t)file);
          free_page((uintptr_t)second);
          return length;
        }
        pwrite(fd, second, sizeof(second_file_t), inode_to_offset(second_inode));
        pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second->second_inode;
        blocks = second->block;
      } else {
        if (file->second_inode == 0) {
          free_page((uintptr_t)file);
          free_page((uintptr_t)second);
          return length;
        }
        pread(fd, second, sizeof(second_dir_t), inode_to_offset(second_inode));
        second_inode = second->second_inode;
        blocks = second->block;
      }
    }
    current += 1;
  }
  free_page((uintptr_t)file);
  free_page((uintptr_t)second);
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
int naive_getattr(const char *path, struct stat *buf) {
  printf("getattr %s\n", path);
  memset(buf, 0, sizeof(struct stat));
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  if (buf != NULL) {
    dir_t *dir = (dir_t *)alloc_page(1);
    pread(fd, dir, sizeof(*dir), inode_to_offset(inode));
    if (dir->header.type == 0)
      buf->st_mode = S_IFDIR | dir->header.mode;
    else
      buf->st_mode = S_IFREG | dir->header.mode;
    buf->st_size = dir->header.size;
    buf->st_uid = dir->header.user;
    buf->st_gid = dir->header.group;
    buf->st_ino = inode;
    buf->st_nlink = dir->header.link_count;
    free_page((uintptr_t)dir);
  }
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
int naive_readlink(const char *path, char *link, size_t size) { return -ENOSYS; }

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
int naive_mknod(const char *path, mode_t mode) {
  printf("mknod %s\n", path);
  size_t len = strlen(path);
  char __base[len + 1];
  char __dir[len + 1];
  strcpy(__base, path);
  strcpy(__dir, path);
  char *base = basename(__base);
  char *dir = dirname(__dir);
  printf("%s\n%s\n", base, dir);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_file(base, inode, mode);
  return 0;
}

/** Create a directory */
int naive_mkdir(const char *path, mode_t mode) {
  printf("mkdir %s\n", path);
  size_t len = strlen(path);
  char __base[len + 1];
  char __dir[len + 1];
  strcpy(__base, path);
  strcpy(__dir, path);
  char *base = basename(__base);
  char *dir = dirname(__dir);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_subdir(base, inode, mode);
  return 0;
}

/** Remove a file */
int naive_unlink(const char *path) {
  printf("rmdir %s\n", path);
  size_t len = strlen(path);
  char __base[len + 1];
  char __dir[len + 1];
  strcpy(__base, path);
  strcpy(__dir, path);
  char *base = basename(__base);
  char *dir = dirname(__dir);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t *header = (file_header_t *)alloc_page(1);
  pread(fd, header, sizeof(*header), inode_to_offset(inode));
  if (header->link_count == 1) {
    free_inode(inode);
    free_blocks(inode);
  }
  inode = find_inode_from_path(dir);
  if (inode == -1 || inode == -2) {
    free_page((uintptr_t)header);
    return -ENOTDIR;
  }
  unlink_file(base, inode);
  free_page((uintptr_t)header);
  return 0;
}

/** Remove a directory */
int naive_rmdir(const char *path) {
  printf("rmdir %s\n", path);
  size_t len = strlen(path);
  char __base[len + 1];
  char __dir[len + 1];
  strcpy(__base, path);
  strcpy(__dir, path);
  char *base = basename(__base);
  char *dir = dirname(__dir);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t *header = (file_header_t *)alloc_page(1);
  pread(fd, header, sizeof(file_header_t), inode_to_offset(inode));
  if (header->child_count > 2)
    return -ENOTEMPTY;
  free_inode(inode);
  inode = find_inode_from_path(dir);
  if (inode == -1 || inode == -2)
    return -ENOTDIR;
  remove_subdir(base, inode);
  free_page((uintptr_t)header);
  return 0;
}

/** Create a symbolic link */
int naive_symlink(const char *path, const char *link) { return -ENOSYS; }

/** Rename a file */
int naive_rename(const char *path, const char *new_path, unsigned int flags) { return -ENOSYS; }

/** Create a hard link to a file */
int naive_link(const char *path, const char *new_path) { return -ENOSYS; }

/** Change the permission bits of a file */
int naive_chmod(const char *path, mode_t mode) {
  printf("chmod %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t *header = (file_header_t *)alloc_page(1);
  pread(fd, header, sizeof(*header), inode_to_offset(inode));
  header->mode = mode;
  pwrite(fd, header, sizeof(*header), inode_to_offset(inode));
  free_page((uintptr_t)header);
  return 0;
}

/** Change the owner and group of a file */
int naive_chown(const char *path, uid_t uid, gid_t gid) {
  printf("chmod %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_header_t *header = (file_header_t *)alloc_page(1);
  pread(fd, header, sizeof(*header), inode_to_offset(inode));
  header->user = uid;
  header->group = gid;
  pwrite(fd, header, sizeof(*header), inode_to_offset(inode));
  free_page((uintptr_t)header);
  return 0;
}

/** Change the size of a file */
int naive_truncate(const char *path, off_t new_size) {
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
int naive_open(const char *path, int flags) {
  printf("open %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t *file = (file_t *)alloc_page(1);
  pread(fd, file, sizeof(*file), inode_to_offset(inode));
  if (file->header.type != 1)
    return -EISDIR;
  free_page((uintptr_t)file);
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
int naive_read(const char *path, char *buf, size_t size, off_t offset) {
  printf("read %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t *root = (file_t *)alloc_page(1);
  pread(fd, root, sizeof(*root), inode_to_offset(inode));
  if (offset > (unsigned long)root->header.size) {
    size = 0;
  } else if (offset + size > (unsigned long)root->header.size) {
    size = root->header.size - offset;
  }
  int bits = read_file(inode, buf, size, offset);
  free_page((uintptr_t)root);
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
int naive_write(const char *path, const char *buf, size_t size, off_t offset) {
  printf("write %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  file_t *root = (file_t *)alloc_page(1);
  pread(fd, root, sizeof(*root), inode_to_offset(inode));
  if (root->header.size < (int32_t)(offset + size))
    resize_file(inode, (long)(offset + size));
  int bits = write_file(inode, buf, size, offset);
  free_page((uintptr_t)root);
  return bits;
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int naive_opendir(const char *path) {
  printf("opendir %s\n", path);
  long inode = find_inode_from_path(path);
  if (inode == -1) {
    return -ENOENT;
  } else if (inode == -2)
    return -ENOTDIR;
  file_header_t *header = (file_header_t *)alloc_page(1);
  pread(fd, header, sizeof(*header), inode_to_offset(inode));
  if (header->type != 0) {
    free_page((uintptr_t)header);
    return -ENOTDIR;
  }
  free_page((uintptr_t)header);
  return 0;
}

/** Read directory
 */
int naive_readdir(const char *path, void *buf) {
  printf("readdir %s\n", path);
  struct dirent *entry = buf;
  char full[1024];
  strcpy(full, path);
  long inode = find_inode_from_path(path);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  dir_t *dir = (dir_t *)alloc_page(1);
  pread(fd, dir, sizeof(*dir), inode_to_offset(inode));
  if (dir->header.type != 0)
    return -ENOTDIR;
  second_dir_t *second = (second_dir_t *)alloc_page(1);
  second->second_inode = dir->second_inode;
  dir_entry_t *entries = dir->entry;
  struct stat stat;
  int count = 0;
  while (true) {
    for (int i = 0; i < 63; ++i) {
      if (entries[i].inode != 0) {
        if (full[strlen(full) - 1] != '/')
          strcat(full, "/");
        strcat(full, entries[i].name);
        naive_getattr(full, &stat);
        entry[count].d_ino = stat.st_ino;
        entry[count].d_type = (stat.st_mode & S_IFDIR) ? DT_DIR : DT_REG;
        entry[count].d_off = count;
        strcpy(entry[count].d_name, entries[count].name);
        strcpy(full, path);
        count += 1;
      }
    }
    if (second->second_inode != 0) {
      pread(fd, second, sizeof(*second), inode_to_offset(second->second_inode));
      entries = second->entry;
    } else {
      break;
    }
  }
  free_page((uintptr_t)dir);
  free_page((uintptr_t)second);
  return 0;
}

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
int naive_create(const char *path, mode_t mode) {
  printf("create %s\n", path);
  size_t len = strlen(path);
  char __base[len + 1];
  char __dir[len + 1];
  strcpy(__base, path);
  strcpy(__dir, path);
  char *base = basename(__base);
  char *dir = dirname(__dir);
  long inode = find_inode_from_path(path);
  if (inode != -1)
    return -EEXIST;
  inode = find_inode_from_path(dir);
  if (inode == -1)
    return -ENOENT;
  else if (inode == -2)
    return -ENOTDIR;
  create_file(base, inode, mode);
  return 0;
}

int naive_init() {
  naivefs_super_block_t *page = (naivefs_super_block_t *)alloc_page(1);
  pread(fd, page, 4096, 0);
  if (strcmp(page->magic, "NaiveFS") == 0) {
    printf("%s\r\n", page->magic);
    memcpy(&super_block, page, sizeof(naivefs_super_block_t));
    free_page((uintptr_t)page);
    return 0;
  }
  printf("%s\r\n", page->magic);
  free_page((uintptr_t)page);
  return -1;
}
