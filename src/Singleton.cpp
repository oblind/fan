#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <sstream>
#include <fstream>
#include "Singleton.h"

using namespace std;

bool Singleton::isSame(int pid, char *path) {
  stringstream ss;
  ss << "/proc/" << pid << "/exe";
  char p[96];
  int l = readlink(ss.str().c_str(), p, sizeof(p));
  if(l < 0)
    return false;
  else {
    p[l] = 0;
    return !strcmp(p, path);
  }
}

bool Singleton::running(const char *pidFile, Singleton::result &r) {
  int pid;
  r.myPid = getpid();
  bool found = false;
  stringstream ss;

  int l = readlink("/proc/self/exe", r.realpath, sizeof(r.realpath));
  r.realpath[l] = 0;

  ifstream fi(pidFile);
  if(fi.is_open()) {
    fi >> pid;
    fi.close();
    ss << "/proc/" << pid;
    found = !access(ss.str().c_str(), F_OK) && isSame(pid, r.realpath);
    if(found)
      r.runningPid = pid;
    else
      remove(pidFile);
  }
  if(!found) {
    dirent *p = nullptr;
    struct stat st;
    DIR *dir = opendir("/proc");
    while((p = readdir(dir)) ) {
      if(!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
        continue;
      char s[64];
      sprintf(s, "/proc/%s", p->d_name);
      if(!lstat(s, &st) && (st.st_mode & S_IFDIR)) {
        pid = atoi(p->d_name);
        if(pid > 400 && isSame(pid, r.realpath) && pid != r.myPid) {
          r.runningPid = pid;
          found = true;
          break;
        }
      }
    }
    closedir(dir);
  }
  return found;
}

bool Singleton::stop(const char *pidFile, Singleton::result &r) {
  bool running;
  stringstream ss;
  ss << "/proc/" << r.runningPid;
  const auto &&s = ss.str();
  clock_t t = clock();
  kill(r.runningPid, SIGINT);
  do usleep(200000);
  while((running = !access(s.c_str(), F_OK)) && clock() - t < 3);
  return !running;
}

void Singleton::writePid(const char *pidFile, Singleton::result &r) {
  ofstream fo(pidFile);
  if(fo.is_open()) {
    fo << r.myPid << endl;
    fo.close();
  }
}
