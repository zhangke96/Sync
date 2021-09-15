// Copyright [2021] zhangke

#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <unistd.h>

#include <deque>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

using namespace std;

void print_usage(char *cmd) { cerr << "Usage:" << cmd << " watch_dir" << endl; }

string error_msg(int error) { return strerror(error); }

// int inotify_init(void);
// int inotify_init1(int flags);
// int inotify_add_watch(int fd, const char *pathname, uint32_t mask);
// int inotify_rm_watch(int fd, int wd);

typedef tuple<bool, std::vector<std::string>, std::vector<std::string>>
    TravelDirResult;
TravelDirResult travel_dir(const std::string path);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    _exit(-1);
    return 0;
  }
  int id = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
  if (id < 0) {
    cerr << "inotify_init failed:" << error_msg(errno) << endl;
    return 0;
  }
  // wd => filename
  std::multimap<int, std::string> wd_map;
  // 保存监听的所有的文件
  std::set<std::string> watch_files;
  // int wd = inotify_add_watch(id, argv[1], IN_ALL_EVENTS & ~IN_ACCESS);
  // if (wd == -1) {
  //   cerr << "inotify_add_watch failed, path:" << argv[1] << ", error:" <<
  //   error_msg(errno) << endl; return 0;
  // }
  // 开始遍历目录
  // TODO(zhangke) 起始目录转换为绝对路径
  std::string root_dir(argv[1]);
  if (root_dir.empty()) {
    cerr << "root dir empty" << endl;
    _exit(-1);
    return 0;
  }
  if (root_dir[0] != '/' && root_dir[0] != '~') {
  }
  std::deque<std::string> to_travel_dirs;
  to_travel_dirs.push_back(argv[1]);
  while (!to_travel_dirs.empty()) {
    std::string dir = to_travel_dirs.front();
    to_travel_dirs.pop_front();
    int wd = inotify_add_watch(id, dir.c_str(), IN_ALL_EVENTS & ~IN_ACCESS);
    if (wd == -1) {
      cerr << "inotify_add_watch failed, path:" << dir
           << ", error:" << error_msg(errno) << endl;
      if (errno == ENOENT) {
        cout << dir << " not exists";
        continue;
      }
    }
    wd_map.insert(std::make_pair(wd, dir));
    watch_files.insert(dir);
    TravelDirResult result = travel_dir(dir);
    if (std::get<0>(result)) {
      cout << "travel dir:" << dir
           << " success, file count:" << std::get<1>(result).size()
           << ", sub dir count:" << std::get<2>(result).size() << endl;
      // 监听文件
      for (auto c : std::get<1>(result)) {
        std::string filename = dir + "/" + c;
        int wd =
            inotify_add_watch(id, filename.c_str(), IN_ALL_EVENTS & ~IN_ACCESS);
        if (wd == -1) {
          cerr << "inotify_add_watch failed, file:" << filename
               << ", error:" << error_msg(errno) << endl;
          if (errno == ENOENT) {
            cout << filename << " not exists";
            continue;
          }
        }
        cout << "add_watch success, file:" << filename << ", wd:" << wd << endl;
        wd_map.insert(std::make_pair(wd, filename));
        watch_files.insert(filename);
        // TODO(zhangke) 读取文件内容
      }
      // 子文件夹加到队列中
      for (auto c : std::get<2>(result)) {
        if (c == "." || c == "..") {
          continue;
        }
        std::string sub_dir = dir + "/" + c;
        to_travel_dirs.push_back(sub_dir);
      }
    } else {
      cerr << "travel dir:" << dir << " failed" << endl;
    }
  }
}

TravelDirResult travel_dir(const std::string path) {
  DIR *dir = opendir(path.c_str());
  if (!dir) {
    cerr << "opendir failed, path:" << path << ", error:" << error_msg(errno)
         << endl;
    return make_tuple(false, std::vector<std::string>(),
                      std::vector<std::string>());
  }
  struct dirent64 *file = nullptr;
  std::vector<std::string> regular_files;
  std::vector<std::string> sub_dirs;
  while (file = readdir64(dir)) {
    switch (file->d_type) {
      case DT_REG:
        regular_files.push_back(file->d_name);
        break;
      case DT_DIR:
        sub_dirs.push_back(file->d_name);
        break;
      default:
        cerr << "not handle file type:" << file->d_type << ", filepath:" << path
             << " " << file->d_name;
        break;
    }
  }
  return make_tuple(true, std::move(regular_files), std::move(sub_dirs));
}
