// See LICENSE for license details.

#include <assert.h>
#include <fdt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define MAX_HARTS 8

static inline uint32_t bswap(uint32_t x) {
  uint32_t y = (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
  uint32_t z = (y & 0x0000FFFF) << 16 | (y & 0xFFFF0000) >> 16;
  return z;
}

static inline int isstring(char c) {
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 'a' && c <= 'z')
    return 1;
  if (c >= '0' && c <= '9')
    return 1;
  if (c == '\0' || c == ' ' || c == ',' || c == '-')
    return 1;
  return 0;
}

static uint32_t *fdt_scan_helper(uint32_t *lex, const char *strings, struct fdt_scan_node *node, const struct fdt_cb *cb) {
  struct fdt_scan_node child;
  struct fdt_scan_prop prop;
  int last = 0;

  child.parent = node;
  // these are the default cell counts, as per the FDT spec
  child.address_cells = 2;
  child.size_cells = 1;
  prop.node = node;

  while (1) {
    switch (bswap(lex[0])) {
    case FDT_NOP: {
      lex += 1;
      break;
    }
    case FDT_PROP: {
      assert(!last);
      prop.name = strings + bswap(lex[2]);
      prop.len = bswap(lex[1]);
      prop.value = lex + 3;
      if (node && !strcmp(prop.name, "#address-cells")) {
        node->address_cells = bswap(lex[3]);
      }
      if (node && !strcmp(prop.name, "#size-cells")) {
        node->size_cells = bswap(lex[3]);
      }
      lex += 3 + (prop.len + 3) / 4;
      cb->prop(&prop, cb->extra);
      break;
    }
    case FDT_BEGIN_NODE: {
      uint32_t *lex_next;
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      last = 1;
      child.name = (const char *)(lex + 1);
      if (cb->open)
        cb->open(&child, cb->extra);
      lex_next = fdt_scan_helper(lex + 2 + strlen(child.name) / 4, strings, &child, cb);
      if (cb->close && cb->close(&child, cb->extra) == -1)
        while (lex != lex_next)
          *lex++ = bswap(FDT_NOP);
      lex = lex_next;
      break;
    }
    case FDT_END_NODE: {
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      return lex + 1;
    }
    default: { // FDT_END
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      return lex;
    }
    }
  }
}

void fdt_scan(uintptr_t fdt, const struct fdt_cb *cb) {
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC || bswap(header->last_comp_version) > FDT_VERSION)
    return;

  const char *strings = (const char *)(fdt + bswap(header->off_dt_strings));
  uint32_t *lex = (uint32_t *)(fdt + bswap(header->off_dt_struct));

  fdt_scan_helper(lex, strings, 0, cb);
}

uint32_t fdt_size(uintptr_t fdt) {
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC || bswap(header->last_comp_version) > FDT_VERSION)
    return 0;
  return bswap(header->totalsize);
}

const uint32_t *fdt_get_address(const struct fdt_scan_node *node, const uint32_t *value, uint64_t *result) {
  *result = 0;
  for (int cells = node->address_cells; cells > 0; --cells)
    *result = (*result << 32u) + bswap(*value++);
  return value;
}

const uint32_t *fdt_get_size(const struct fdt_scan_node *node, const uint32_t *value, uint64_t *result) {
  *result = 0;
  for (int cells = node->size_cells; cells > 0; --cells)
    *result = (*result << 32u) + bswap(*value++);
  return value;
}

int fdt_string_list_index(const struct fdt_scan_prop *prop, const char *str) {
  const char *list = (const char *)prop->value;
  const char *end = list + prop->len;
  int index = 0;
  while (end - list > 0) {
    if (!strcmp(list, str))
      return index;
    ++index;
    list += strlen(list) + 1;
  }
  return -1;
}

//////////////////////////////////////////// PRINT
/////////////////////////////////////////////////

#define FDT_PRINT_MAX_DEPTH 32

struct fdt_print_info {
  int depth;
  const struct fdt_scan_node *stack[FDT_PRINT_MAX_DEPTH];
};

void fdt_print_printm(struct fdt_print_info *info, const char *format, ...) {
  va_list vl;

  for (int i = 0; i < info->depth; ++i)
    printf("  ");

  va_start(vl, format);
  vprintf(format, vl);
  va_end(vl);
}

static void fdt_print_open(const struct fdt_scan_node *node, void *extra) {
  struct fdt_print_info *info = (struct fdt_print_info *)extra;

  while (node->parent != NULL && info->stack[info->depth - 1] != node->parent) {
    info->depth--;
    fdt_print_printm(info, "}\r\n");
  }

  fdt_print_printm(info, "%s {\r\n", node->name);
  info->stack[info->depth] = node;
  info->depth++;
}

static void fdt_print_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct fdt_print_info *info = (struct fdt_print_info *)extra;
  int asstring = 1;
  char *char_data = (char *)(prop->value);

  fdt_print_printm(info, "%s", prop->name);

  if (prop->len == 0) {
    printf(";\r\n");
    return;
  } else {
    printf(" = ");
  }

  /* It appears that dtc uses a hueristic to detect strings so I'm using a
   * similar one here. */
  for (int i = 0; i < prop->len; ++i) {
    if (!isstring(char_data[i]))
      asstring = 0;
    if (i > 0 && char_data[i] == '\0' && char_data[i - 1] == '\0')
      asstring = 0;
  }

  if (asstring) {
    for (int i = 0; i < prop->len; i += strlen(char_data + i) + 1) {
      if (i != 0)
        printf(", ");
      printf("\"%s\"", char_data + i);
    }
  } else {
    printf("<");
    for (int i = 0; i < prop->len / 4; ++i) {
      if (i != 0)
        printf(" ");
      printf("0x%08x", bswap(prop->value[i]));
    }
    printf(">");
  }

  printf(";\r\n");
}

static void fdt_print_done(__attribute__((unused)) const struct fdt_scan_node *node, __attribute__((unused)) void *extra) {}

static int fdt_print_close(__attribute__((unused)) const struct fdt_scan_node *node, __attribute__((unused)) void *extra) { return 0; }

void fdt_print(uintptr_t fdt) {
  struct fdt_print_info info;
  struct fdt_cb cb;

  info.depth = 0;

  memset(&cb, 0, sizeof(cb));
  cb.open = fdt_print_open;
  cb.prop = fdt_print_prop;
  cb.done = fdt_print_done;
  cb.close = fdt_print_close;
  cb.extra = &info;

  fdt_scan(fdt, &cb);

  while (info.depth > 0) {
    info.depth--;
    fdt_print_printm(&info, "}\r\n");
  }
}

struct fdt_get_prop_info {
  const char *path;
  char *buff;
  const uint32_t *value;
  int len;
};

static void fdt_get_prop_dummy(__attribute__((unused)) const struct fdt_scan_node *node, __attribute__((unused)) void *extra) {}
static int fdt_get_prop_close(__attribute__((unused)) const struct fdt_scan_node *node, __attribute__((unused)) void *extra) { return 0; }

static void path_name_helper(const struct fdt_scan_node *node, char *buff) {
  if (node->parent == NULL) {
    return;
  }

  path_name_helper(node->parent, buff);
  strcat(buff, "/");
  strcat(buff, node->name);
}

static void fdt_get_prop_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct fdt_get_prop_info *info = (struct fdt_get_prop_info *)extra;
  if (info->value != NULL)
    return;

  char *buff = info->buff;
  buff[0] = '\0';

  path_name_helper(prop->node, buff);
  strcat(buff, "/");
  strcat(buff, prop->name);

  if (strcmp(buff, info->path) == 0) {
    info->len = prop->len;
    info->value = prop->value;
  }
}

int get_prop_u32(uintptr_t fdt, const char *path, uint32_t *value) {
  char buff[MAX_PATH_SIZE];
  struct fdt_get_prop_info info;
  info.path = path;
  info.buff = buff;
  info.value = NULL;
  info.len = 0;
  struct fdt_cb cb;
  memset(&cb, 0, sizeof(cb));

  cb.open = fdt_get_prop_dummy;
  cb.prop = fdt_get_prop_prop;
  cb.done = fdt_get_prop_dummy;
  cb.close = fdt_get_prop_close;
  cb.extra = &info;

  fdt_scan(fdt, &cb);

  assert(info.len / 4 == 1);
  *value = bswap(info.value[0]);
  return 0;
}
