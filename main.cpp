#include <stdio.h>
#include <string.h>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <wiringPi.h>

using namespace std;

enum Command {cmdStart, cmdStop, cmdOn, cmdOff};

const int pin = 1;
const map<string, Command> Commands = {
  {"start", cmdStart}, {"stop", cmdStop}, {"on", cmdOn}, {"off", cmdOff}
};
const char *pidPath = "/var/run/fan.pid";

int running = true;

void onClose(int sig) {
  remove(pidPath);
  running = false;
  if(sig == SIGINT)
    cout << endl;
}

void setup() {
  wiringPiSetup();
  pinMode(pin, OUTPUT);
}

int main(int argc, char *argv[]) {
  //8550 PNP型三极管，低电平导通
  Command cmd;
  if(argc > 1) {
    int l = strlen(argv[1]), state;
    transform(argv[1], argv[1 + l], argv[1], ::tolower);
    auto i = Commands.find(argv[1]);
    if(i == Commands.end()) {
      cout << "unknown command\n";
      return 0;
    }
    cmd = i->second;
  } else
    cmd = cmdStart;
  if(cmd == cmdOn || cmd == cmdOff) {
    setup();
    digitalWrite(pin, cmd == cmdOn ? 0 : 1);
    return 0;
  }
  ifstream f;
  {
    f.open(pidPath, ios::in);
    if(f.is_open()) {
      int pid = 0;
      f >> pid;
      f.close();
      string s("/proc/");
      s.append(to_string(pid));
      if(!access(s.c_str(), F_OK)) {
        if(cmd == cmdStop) {
          if(kill(pid, SIGTERM))
            cout << "on permission\n";
        } else
          cout << "fan is running\n";
        return 0;
      }
    } else if(cmd == cmdStop) {
      cout << "fan is not running\n";
      return 0;
    }

    int pid = getpid();
    ofstream fo(pidPath);
    fo << pid;
    fo.close();
  }
  //设置信号处理
  signal(SIGTERM, onClose);
  signal(SIGKILL, onClose);
  signal(SIGINT, onClose);

  setup();
  int state = 0;
  digitalWrite(pin, 1);
  while(running) {
    /*digitalWrite(pin, state);
    cout << "state: " << state << endl;
    getchar();
    state = 1 - state;*/
    f.open("/sys/class/thermal/thermal_zone0/temp");
    if(!f.is_open()) {
      cout << "get cpu temperature faild\n";
      return -1;
    }
    float t;
    f >> t;
    t /= 1000.0;
    cout << "cpu temperature: " << t << "°C\n";
    f.close();
    int s = state;
    if(state) {
      if(t < 46)
        s = 0;
    } else if(t > 48)
      s = 1;
    if(s != state) {
      state = s;
      digitalWrite(pin, 1 - state);
      cout << "turn " << (state ? "on\n" : "off\n");
    }
    sleep(10);
  }
  if(state) //退出关闭
    digitalWrite(pin, 1);
}
