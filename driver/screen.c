#include <sbi.h>
#include <scheduler.h>
#include <screen.h>
#include <stdio.h>
#include <string.h>

/* cursor position */
void vt100_move_cursor(int x, int y) {
  // \033[y;xH
  printf("\033[%d;%dH", y, x);
}

/* clear screen */
void vt100_screen_clear() {
  // \033[2J
  printf("\033[2J");
  vt100_move_cursor(0, 0);
}

void init_screen(void) {
  vt100_screen_clear();
  vt100_move_cursor(0, 0);
}

void screen_flush() {
  sbi_console_puts((const char *)va_to_pa((uintptr_t)current_running->buffer, (const page_table_entry_t *)current_running->page_directory));
  memset(current_running->buffer, 0, sizeof(current_running->buffer));
  current_running->buffer_pos = 0;
}

void screen_put_char(char c) {
  if (current_running->buffer_pos + 2 < BUFFER_MAX_SIZE) {
    current_running->buffer[current_running->buffer_pos++] = c;
    if (c == '\n')
      current_running->buffer[current_running->buffer_pos++] = '\r';
  } else {
    screen_flush();
    current_running->buffer[current_running->buffer_pos++] = c;
    if (c == '\n')
      current_running->buffer[current_running->buffer_pos++] = '\r';
  }
  if (c == '\n' || c == '\r') {
    screen_flush();
  }
}
