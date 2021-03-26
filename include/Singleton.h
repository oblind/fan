#pragma once

class Singleton {
protected:
  static bool isSame(int pid, char *path);
public:
  struct result {
    int myPid;
    int runningPid;
    char realpath[96];
  };

  static bool running(const char *pidFile, result &r);
  static bool stop(const char *pidFile, result &r);
  //写入pid
  static void writePid(const char *pidFile, result &r);
};
