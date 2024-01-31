#include "socket.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#define SCRATCH_LENGTH 256

static char scratch_buffer[SCRATCH_LENGTH];

int send_format(SOCKET socket, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  int length = vsnprintf(scratch_buffer, SCRATCH_LENGTH, fmt, va);
  if (length < 0 || length >= SCRATCH_LENGTH) {
    return -1;
  }
  scratch_buffer[length] = '\0';
  send_string(socket, scratch_buffer);
  return 0;
}

int await_message(int sfd, char **buffer) {
  int recv_result;
  do {
    recv_result = recv_message(sfd, buffer);
  } while (recv_result == 0);
  return recv_result;
}

