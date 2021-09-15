// Copyright [2021] zhangke

#include <unistd.h>

#include <iostream>

#include "dir_watcher.h"

using namespace std;

void print_usage(char *cmd) { cerr << "Usage:" << cmd << " watch_dir" << endl; }

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    _exit(-1);
    return 0;
  }
  DirWatcher dir_watcher(argv[1]);
  dir_watcher.StartWatch();
}
