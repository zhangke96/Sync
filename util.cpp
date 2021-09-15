// Copyright [2021] zhangke
#include "util.h"

#include <string.h>

#include <iostream>

std::string error_msg(int error) {
  std::string msg(256, 0);
  char *emsg = strerror_r(errno, const_cast<char *>(msg.c_str()), 256);
  if (!emsg) {
    msg.clear();
    std::cerr << "strerror failed, errno:" << error << ", now errno:" << errno
              << std::endl;
  }
  return msg;
}
