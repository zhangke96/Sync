// Copyright [2021] zhangke
#ifndef DIR_WATCHER_H_
#define DIR_WATCHER_H_

#include <atomic>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

class DirWatcher {
 public:
  explicit DirWatcher(std::string dir);
  ~DirWatcher();
  bool StartWatch();
  void StopWatch(bool no_wait = true);
  enum class FileType {
    INVALID,
    REGULAR,
    DIR,
    LINK,
  };
  enum class EventType {
    INVALID,
    ADD,
    CHANGE,
    RENAME,
    DELETE,
  };
  struct Event {
    EventType event_type;
    FileType file_type;
    std::string files;
  };
  typedef std::function<void(const Event &)> EventCbType;
  bool SetEventCb(EventCbType on_event) {
    if (event_cb_) {
      return false;
    }
    event_cb_ = on_event;
    return true;
  }

 private:
  struct TravelDirResult {
    bool result;
    std::vector<std::string> regular_files;
    std::vector<std::string> sub_dirs;
    std::vector<std::string> link_files;
  };
  void start_watch();
  TravelDirResult travel_dir(const std::string &path);
  bool add_watch_file(const std::string &filename);
  bool add_watch_dir(const std::string &dirname);
  std::string dir_;
  int id_ = -1;
  int pipe_fd_[2] = {-1, -1};
  std::thread work_thread_;
  std::deque<std::string> to_travel_dirs_;
  std::set<std::string> watch_files_;
  std::multimap<int, std::string> wd_map_;
  EventCbType event_cb_;
  bool ready_ = false;
  std::atomic_bool started_;
  std::atomic_bool need_stop_;
};

#endif  // DIR_WATCHER_H_
