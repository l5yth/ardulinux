//
// Created by kevinh on 9/1/20.
//

#include "Utility.h"
#include <csignal>
#include <stdio.h>
#include <stdarg.h>

void notImplemented(const char *msg) { printf("%s is not implemented\n", msg); }

void meshduinoError(const char *msg, ...) {
  char msgBuffer[256];
  va_list args;
  va_start(args, msg);
  vsnprintf(msgBuffer, sizeof msgBuffer, msg, args);
  va_end(args);
  printf("Meshduino critical error: %s\n", msgBuffer);
  throw Exception(msgBuffer);
}

int meshduinoCheckNotNeg(int result, const char *msg, ...) {
  if (result < 0) {
    printf("Meshduino notneg errno=%d: %s\n", errno, msg);
    throw Exception(msg);
  }
  return result;
}


int meshduinoCheckZero(int result, const char *msg, ...) {
  if (result != 0) {
    printf("Meshduino checkzero %d: %s\n", result, msg);
    throw Exception(msg);
  }
  return result;
}

void meshduinoDebug() {
  // Generate an interrupt
  std::raise(SIGINT);
}
