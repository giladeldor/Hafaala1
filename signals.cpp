#include "signals.h"
#include "Commands.h"
#include <iostream>
#include <signal.h>

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << endl;

  // TODO: Add your implementation
}

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  // TODO: Add your implementation
}

void alarmHandler(int sig_num) {
  cout << "smash: got an alarm" << endl;
  // TODO: Add your implementation
}
