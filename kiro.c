// --- includes ---

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// --- defines ---

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE (!FALSE)
#endif

#define CTRL_KEY(k) ((k)&0x1f)

// --- data ---

struct EditorConfig {
  int screenrows;
  int screencols;
  struct termios orig_termios;
};

struct EditorConfig E;

// --- prototypes ---

void die(const char *);
void disableRawMode();
void enableRawMode();
int getWindowSize(int *, int *);

void editorDrawRows();
void editorRefreshScreen();
void refreshScreen();
void clearScreen();
void repositionCursor();

void editorProcessKeypress();
char editorReadKey();

void initEditor();

// --- terminal ---

void die(const char *s) {
  refreshScreen();

  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) {
    die("tcsetattr");
  }
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) {
    die("tcgetattr");
  }
  atexit(disableRawMode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag &= ~(CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

int getWindowSize(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    return -1;
  }

  *cols = ws.ws_col;
  *rows = ws.ws_row;
  return 0;
}

// --- output ---

void editorRefreshScreen() {
  refreshScreen();

  editorDrawRows();

  write(STDERR_FILENO, "\x1b[H", 3);
}

void editorDrawRows() {
  for (int y = 0; y < E.screenrows; y++) {
    write(STDOUT_FILENO, "~\r\n", 3);
  }
}

void refreshScreen() {
  clearScreen();
  repositionCursor();
}

void clearScreen() { write(STDOUT_FILENO, "\x1b[2J", 4); }

void repositionCursor() { write(STDOUT_FILENO, "\x1b[H", 3); }

// --- input ---

void editorProcessKeypress() {
  char c = editorReadKey();

  switch (c) {
    case CTRL_KEY('q'):
      refreshScreen();
      exit(0);
      break;
  }
}

char editorReadKey() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN) {
      die("read");
    }
  }
  return c;
}

// --- init ---

void initEditor() {
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
    die("getWindowSize");
  }
}

int main() {
  enableRawMode();
  initEditor();

  while (TRUE) {
    editorRefreshScreen();
    editorProcessKeypress();
  }

  return 0;
}
