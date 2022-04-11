/*
 * This program displays the names of all files in the current directory.
 */

#include <dirent.h> 
#include <stdio.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <set>

#include "util.h"
#include "access.h"

using namespace std;

formatted_output field_cout(cout, 30);

string command_rule;
string type_rule;
string file_rule;

int main(int argc, char* argv[]) {
  int opt;
  set<string> s({"REG", "CHR", "DIR", "FIFO", "SOCK", "unknown"});
  while ((opt = getopt(argc, argv, "c:t:f:")) != -1) {
      switch (opt) {
      case 'c':
        command_rule = optarg;
        break;
      case 't':
        if (!s.count(optarg)) {
          cout << "Invalid TYPE option." << endl;
          exit(3);
        } else {
          type_rule = optarg;
        }
        break;
      default: /* '?' */
        cout << "Unkown arguments";
        exit(4);  
      }
  }
  vector<string> processes;
  // Extract all the pid of processes
  DIR *d;
  struct dirent *dir;
  d = opendir(kRootDir.c_str());
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (IsNumber(dir->d_name))
        processes.push_back(dir->d_name);
    }
    closedir(d);
  }
  // Get opened file
  field_cout << "Command" << "PID" << "USER" << "FD" << "TYPE" << "NODE";
  cout << "\tNAME" << endl;
  for (string pid : processes) {
    GetLinked(pid, "cwd");
    GetLinked(pid, "exe");
    GetLinked(pid, "root");
    GetMem(pid);
    GetFds(pid);
  }
  return(0);
}