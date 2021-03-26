#include <stdio.h>
#include <string.h>
#include <map>
#include <unistd.h>
#include <signal.h>
#include <sys/shm.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <SimpleIni.h>
#include <wiringPi.h>
#include "Singleton.h"

using namespace std;

enum Command {cmdStart, cmdStop, cmdOn, cmdOff, cmdPWM};

int pin;
const map<string, Command> Commands = {
  {"start", cmdStart}, {"stop", cmdStop}, {"on", cmdOn}, {"off", cmdOff},
  {"pwm", cmdPWM},
};
const char *pidFile = "/var/run/fan.pid";

int running = true;

void onClose(int sig) {
  remove(pidFile);
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
  int startTemp, stopTemp, base;
  if(argc > 1) {
    int l = strlen(argv[1]);  //, state;
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
    cout << "fan " << argv[1] << '\n';
    return 0;
  }
  { //读取配置
    const char *sec = "fan";
    char s[64];
    CSimpleIni ini;
    //readlink("/proc/self/exe", s, sizeof(s));
    getcwd(s, sizeof(s));
    string path(s);
    //int p = path.find_last_of('/');
    //if(p != string::npos)
    //  path = path.substr(0, p);
    path.append("/fan.ini");
    ini.LoadFile(path.c_str());
    pin = ini.GetLongValue(sec, "pin", 9);
    startTemp = ini.GetLongValue(sec, "start", 48);
    stopTemp = ini.GetLongValue(sec, "stop", 45);
    if(startTemp < stopTemp)
      swap(startTemp, stopTemp);
    if(startTemp - stopTemp < 1)
      stopTemp = startTemp - 1;
    base = startTemp - stopTemp;
    cout << "pin: " << pin << ", from " << stopTemp << " to " << startTemp << endl << endl;
  }
  ifstream f;
  {
    Singleton::result r;
    if(Singleton::running(pidFile, r)) {
      if(cmd == cmdStop)
        cout << (Singleton::stop(pidFile, r) ? "program stopped\n\n" : "stop failed\n\n");
      else
        cout << "program is already running: " << r.runningPid << endl << endl;
      return 0;
    } else if(cmd == cmdStop) {
      cout << "program is not running\n\n";
      return 0;
    }
    //写入pid
    Singleton::writePid(pidFile, r);
  }

  //设置信号处理
  signal(SIGTERM, onClose);
  signal(SIGKILL, onClose);
  signal(SIGINT, onClose);

  cout << (cmd == cmdPWM ? "pwm\n" : "normal\n");

  setup();
  int state = 0;
  digitalWrite(pin, 1); //高电平关闭

  const char *fmt = "{\"temp\":%3.1f,\"start\":%d,\"stop\":%d,\"state\":%d}";
  char *shmjson;
  int shmid = shmget((key_t)4404, 50, 0666 | IPC_CREAT);
  shmjson = (char*)shmat(shmid, nullptr, 0);
  sprintf(shmjson, fmt, 0, startTemp, stopTemp, state);

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
    cout << "cpu temperature: " << setprecision(3) << t << "°C\n";
    f.close();

    if(cmd == cmdPWM) {
      //PWM占空比
      int r = t > startTemp ? 10 : t < stopTemp ? 0 : (t - stopTemp) * 10 / base;
      if(r) {
        int a = 0, b = 0, c = 1000 / r; //龟兔赛跑
        if(r < 3) //最小占空比，否则风扇不转
          r = 3;
        cout << "r: " << r << endl;
        for(int i = 0; i < c; i++) {
          while(a < b) {
            a += r;
            digitalWrite(pin, 1); //关闭
            usleep(1000);
            //cout << "-" << flush;
            //usleep(100000);
          }
          b += 10;
          digitalWrite(pin, 0); //开启
          usleep(1000);
          //cout << "|" << flush;
          //usleep(100000);
        }
      } else {
        digitalWrite(pin, 1); //关闭
        sleep(1);
      }
    } else {
      int s = state;
      if(state) {
        if(t < stopTemp)
          s = 0;
      } else if(t > startTemp)
        s = 1;
      if(s != state) {
        state = s;
        digitalWrite(pin, 1 - state);
        cout << "turn " << (state ? "on\n" : "off\n");
      }
      sprintf(shmjson, fmt, t, startTemp, stopTemp, state);
      sleep(10);
    }
  }
  if(state) //退出关闭
    digitalWrite(pin, 1);
  shmdt(shmjson);
  shmctl(shmid, IPC_RMID, 0);
}
