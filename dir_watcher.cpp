// Copyright [2021] zhangke
#include "dir_watcher.h"

#include <dirent.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <iostream>

#include "util.h"

DirWatcher::DirWatcher(std::string dir) {
  started_ = false;
  need_stop_ = false;
  // 处理目录
  if (dir.empty()) {
    return;
  }
  ready_ = true;
  if (dir[0] != '/' && dir[0] != '~') {
    // 相对路径, 获取当前工作目录
    char *cwd = get_current_dir_name();
    if (cwd) {
      dir = std::string(cwd) + "/" + dir;
    } else {
      ready_ = false;
      std::cerr << "get_current_dir_name failed, msg:" << error_msg(errno)
                << std::endl;
    }
  }
  // 处理掉最后的/
  if (dir.size() > 1 && dir.back() == '/') {
    dir.pop_back();
  }
  dir_ = dir;
  id_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (id_ < 0) {
    ready_ = false;
    std::cerr << "inotiy_init failed:" << error_msg(errno) << std::endl;
  }
  int ret = pipe(pipe_fd_);
  if (ret < 0) {
    ready_ = false;
    std::cerr << "pipe failed:" << error_msg(errno) << std::endl;
  }
}

DirWatcher::~DirWatcher() {
  need_stop_ = true;
  if (work_thread_.joinable()) {
    work_thread_.join();
  }
  // 关闭句柄
  if (id_ >= 0) {
    close(id_);
  }
}

bool DirWatcher::StartWatch() {
  if (!ready_) {
    return false;
  }
  if (started_) {
    return false;
  }
  if (dir_.empty()) {
    std::cerr << "watch dir empty" << std::endl;
    return false;
  }
  // 启动watch线程
  work_thread_ = std::thread(&DirWatcher::start_watch, this);
  return true;
}

void DirWatcher::StopWatch(bool no_wait) {
  // 考虑使用eventfd通知
}

void DirWatcher::start_watch() {
  to_travel_dirs_.push_back(dir_);
  while (!to_travel_dirs_.empty()) {
    std::string dir = to_travel_dirs_.front();
    to_travel_dirs_.pop_front();
    int wd = inotify_add_watch(id_, dir.c_str(), IN_ALL_EVENTS & ~IN_ACCESS);
    if (wd == -1) {
      std::cerr << "inotify_add_watch failed, path:" << dir
                << ", error:" << error_msg(errno) << std::endl;
      if (errno == ENOENT) {
        std::cout << dir << " not exists" << std::endl;
        continue;
      }
    }
    wd_map_.insert(std::make_pair(wd, dir));
    watch_files_.insert(dir);
    TravelDirResult result = travel_dir(dir);
    if (result.result) {
      std::cout << "travel dir:" << dir
                << " success, file count:" << result.regular_files.size()
                << ", sub dir count:" << result.sub_dirs.size()
                << ", link file count:" << result.link_files.size()
                << std::endl;
      // 监听文件
      for (auto c : result.regular_files) {
        std::string filename = dir + "/" + c;
        add_watch_file(filename);
      }
      // TODO(ke.zhang) link file handle
      // 子文件夹加到队列中
      for (auto c : result.sub_dirs) {
        if (c == "." || c == "..") {
          continue;
        }
        std::string sub_dir = dir + "/" + c;
        add_watch_dir(sub_dir);
      }
    } else {
      std::cerr << "travel dir:" << dir << " failed" << std::endl;
    }
  }
}

DirWatcher::TravelDirResult DirWatcher::travel_dir(const std::string &path) {
  TravelDirResult travel_result;
  travel_result.result = true;
  DIR *dir = opendir(path.c_str());
  if (!dir) {
    std::cerr << "opendir failed, path:" << path
              << ", error:" << error_msg(errno) << std::endl;
    travel_result.result = false;
    return travel_result;
  }
  struct dirent64 *file = nullptr;
  auto &regular_files = travel_result.regular_files;
  auto &sub_dirs = travel_result.sub_dirs;
  auto &link_files = travel_result.link_files;
  while (file = readdir64(dir)) {
    switch (file->d_type) {
      case DT_REG:
        regular_files.push_back(file->d_name);
        break;
      case DT_DIR:
        sub_dirs.push_back(file->d_name);
        break;
      case DT_LNK:
        link_files.push_back(file->d_name);
        break;
      default:
        std::cerr << "not handle file type:" << file->d_type
                  << ", filepath:" << path << " " << file->d_name << std::endl;
        break;
    }
  }

  return travel_result;
}

bool DirWatcher::add_watch_file(const std::string &filename) {
  int wd = inotify_add_watch(id_, filename.c_str(), IN_ALL_EVENTS & ~IN_ACCESS);
  if (wd == -1) {
    std::cerr << "inotify_add_watch failed, file:" << filename
              << ", error:" << error_msg(errno) << std::endl;
    if (errno == ENOENT) {
      std::cout << filename << " not exists" << std::endl;
    }
    return false;
  }
  std::cout << "add_watch success, file:" << filename << ", wd:" << wd
            << std::endl;
  wd_map_.insert(std::make_pair(wd, filename));
  watch_files_.insert(filename);
  return true;
}

bool DirWatcher::add_watch_dir(const std::string &filename) {
  int wd = inotify_add_watch(id_, filename.c_str(), IN_ALL_EVENTS & ~IN_ACCESS);
  if (wd == -1) {
    std::cerr << "inotify_add_watch dir failed, dir:" << filename
              << ", error:" << error_msg(errno) << std::endl;
    if (errno == ENOENT) {
      std::cout << filename << " not exists" << std::endl;
    }
    return false;
  }
  to_travel_dirs_.push_back(filename);
  return true;
}
