#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <zconf.h>

#define align(a, n) (((((unsigned long)(a)) + (n)-1)) & ~((n)-1))

typedef struct {
  char magic[8];
  int64_t sectors;
  int64_t block_bitmap_size;
  int64_t inode_bitmap_size;
  int64_t max_filename_length;
  int64_t root_inode;
  int64_t root_block;
} naivefs_super_block_t;

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

int main(int argc, char **argv) {
  if (argc == 1) {
    error(EXIT_FAILURE, 0, "drive not specified");
  }
  char *path = argv[1];
  int fd = open(path, O_RDWR | O_EXCL);
  if (fd == -1) {
    error(EXIT_FAILURE, errno, "%s", path);
  }
  long num_blocks;
  ioctl(fd, BLKGETSIZE, &num_blocks);
  long block_bitmap_size = align(num_blocks, 8 * 8 * 4096ul) / 8 / 4096;
  long inode_bitmap_size = align(block_bitmap_size, 32ul) / 32;
  naivefs_super_block_t super_block = {.magic = "NaiveFS",
                                       .sectors = num_blocks,
                                       .block_bitmap_size = block_bitmap_size,
                                       .inode_bitmap_size = inode_bitmap_size,
                                       .max_filename_length = 56,
                                       .root_inode = block_bitmap_size + inode_bitmap_size - 2,
                                       .root_block = block_bitmap_size + inode_bitmap_size + inode_bitmap_size * 8 * 4096};
  printf("The root node is at %ld\n", super_block.root_inode);
  pwrite(fd, &super_block, sizeof(super_block), 0);
  unsigned char *buf = malloc(4096 * super_block.block_bitmap_size);
  memset(buf, 0, 4096 * super_block.block_bitmap_size);
  long used_block_count = 1 + block_bitmap_size + inode_bitmap_size + inode_bitmap_size * 8 * 4096;
  memset(buf, -1, used_block_count / 8);
  int remain = (int)(used_block_count % 8);
  while (remain > 0) {
    buf[used_block_count / 8 + 1] <<= 1u;
    buf[used_block_count / 8 + 1] |= 1u;
    remain -= 1;
  }
  pwrite(fd, buf, 4096 * super_block.block_bitmap_size, 4096);
  free(buf);
  buf = malloc(4096 * super_block.inode_bitmap_size);
  memset(buf, 0, 4096 * super_block.inode_bitmap_size);
  buf[0] = 7;
  pwrite(fd, buf, 4096, 4096 * super_block.block_bitmap_size + 4096);
  free(buf);
  dir_t *root = malloc(sizeof(dir_t));
  memset(root, 0, sizeof(dir_t));
  root->header.type = 0;
  root->header.mode = 0755;
  root->header.size = 4096;
  root->header.access_time = time(NULL);
  root->header.modify_time = time(NULL);
  root->header.link_count = 2;
  root->header.child_count = 2;
  root->header.user = 1000;
  root->header.group = 1000;
  memset(root->header.pad, 0, sizeof(root->header.pad));
  memset(root->entry, 0, sizeof(root->entry));
  memcpy(root->entry[0].name, ".", 2);
  root->entry[0].inode = 2;
  memcpy(root->entry[1].name, "..", 3);
  root->entry[1].inode = 2;
  pwrite(fd, root, sizeof(dir_t), 4096 * (super_block.root_inode + 2));
  printf("root directory create succeed.");
  free(root);
  fsync(fd);
  close(fd);
}
