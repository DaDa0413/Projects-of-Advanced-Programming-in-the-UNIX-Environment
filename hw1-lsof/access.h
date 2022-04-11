#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#include <fstream>     
#include <sstream>
#include <unordered_map>
#include <regex>

#include "util.h"

#define kFileNotExist 1
#define kPermissionDenied 2
#define kBufferSize 4096

extern formatted_output field_cout;
extern string command_rule;
extern string type_rule;
extern string file_rule;

const string kRootDir("/proc/");

int CheckAccess(string path, struct stat* buf) {
  memset(buf, 0, sizeof(buf));
  if (stat(path.c_str(), buf) < 0) {
    if (errno == ENOENT) {  // File is no longer existed
      return kFileNotExist;
    } else if (errno == EACCES) { // Permission denied
      return kPermissionDenied;
    }else {
      cerr << "stat failed with: " << strerror(errno) << endl;
      exit(1);
    }
  }
  return 0;
}

string GetFileType(mode_t mode) {
  string type;
  if (S_ISREG(mode)) {
    type = "REG";
  } else if (S_ISDIR(mode)) {
    type = "DIR";
  } else if (S_ISCHR(mode)) {
    type = "CHR";
  } else if (S_ISFIFO(mode)) {
    type = "FIFO";
  } else if (S_ISSOCK(mode)) {
    type = "SOCK";
  } else {
    type = "unknown";
  }
  return type;
}

string GetCommand(string pid) {
  string path("/proc/");
  path += pid + "/comm";
  ifstream command_file(path) ;
  string command_name;
  getline(command_file, command_name);
  return command_name;
}

string GetUserName(uid_t uid) {
  struct passwd *p;
  if ((p = getpwuid(uid)) == NULL)
    cerr << "Can not get the username of pid" << endl;
  return string(p->pw_name);
}

void PrintOut(string command, string pid, string user, string target, string type, string inode, string name) {
  if (command_rule.size()) {
    regex e ("([^ ]*)(" + command_rule + ")([^ ]*)");
    if (!regex_match(command, e))
      return;
  }
  if (type_rule.size()) {
    if (type.compare(type_rule))
      return;
  }
  if (file_rule.size()) {
    regex e ("([^ ]*)(" + file_rule + ")([^ ]*)");
    if (!regex_match(name, e))
      return;
  }
  // if (file_rule.size() && name.find)
  field_cout << command << pid << user << target << type << inode;
  cout << "\t" << name << endl;
}
void GetLinked(string pid, string target) {
  string path = kRootDir + pid + "/" + target;
  // Command
  string command = GetCommand(pid);
  // Check the accessibility
  struct stat buf;
  int access_status = CheckAccess(path, &buf);
  if (access_status == kFileNotExist)
    return;
  // User
  string user;
  if (access_status != kPermissionDenied) {
    user = GetUserName(buf.st_uid);
  } else {
    user = "??????";
  }
  // File type
  string type = GetFileType(buf.st_mode);

  // i-node
  string inode;
  if (access_status != kPermissionDenied) {
    inode = to_string(buf.st_ino);
  } else {
    inode = "";
  }

  // Name
  string name;
  if (access_status == kPermissionDenied) {
    name = path + " (Permission denied)";
  } else {
    char linked_buf[kBufferSize];
    memset(linked_buf, 0, kBufferSize);
    readlink(path.c_str(), linked_buf, sizeof(linked_buf));
    name = linked_buf;
  }
  // Check Deleted
  int index;
  if ((index = name.find("(deleted)")) != string::npos) {
    target = "DEL";
    name = name.substr(0, index);
  } 
  // Print out
  PrintOut(command, pid, user, target, type, inode, name);
}

void GetMem(string pid) {
  string path = kRootDir + pid + "/maps";
  // Command
  string command = GetCommand(pid);

  // Check the accessibility
  struct stat buf;
  int access_status = CheckAccess(path, &buf);
  if (access_status == kFileNotExist || access_status == kPermissionDenied)
    return;
  // User
  string user = GetUserName(buf.st_uid);
  // Read maps
  ifstream mem_file(path) ;
  string line;
  unordered_map<string, bool> files;

  while (getline(mem_file, line))
  {
    // cout << line;
    size_t index;
    if ((index = line.find_first_of('/')) == string::npos)
      continue;
    string file_name = line.substr(index);
    if (files.count(file_name)) {
      continue;
    } else {
      files[file_name];
    }
    // We should not encouter issue of permission denied here
    struct stat file_buf;
    access_status = CheckAccess(file_name, &file_buf);
    if (access_status == kFileNotExist) {
      continue;
    } else if (access_status == kPermissionDenied) {
      cerr << "Linked file permission denied" << endl;
      exit(1);
    }
    // File type
    string type = GetFileType(file_buf.st_mode);
    // i-node
    string inode = to_string(buf.st_ino);

    // Check Deleted
    string target("mem");
    if ((index = file_name.find("(deleted)")) != string::npos) {
      target = "DEL";
      file_name = file_name.substr(0, index);
    } 
    // Print out
    PrintOut(command, pid, user, target, type, inode, file_name);
  }
}

void GetFds(string pid) {
  string path = kRootDir + pid + "/fd";
  // Command
  string command = GetCommand(pid);

  // Check the accessibility
  struct stat buf;
  int access_status = CheckAccess(path, &buf);
  if (access_status == kFileNotExist) {
    return;
  }
  // User
  string user;
  if (access_status != kPermissionDenied) {
    user = GetUserName(buf.st_uid);
  } else {
    user = "?????";
  }
  // Early stopping
  if (access_status == kPermissionDenied) {
    field_cout << command << pid << user << "NOFD" << "" << "" << path + " (Permission denied)";
    return;
  }
  // Extract all the fd of the processes
  vector<string> fds;
  DIR *d;
  struct dirent *dir;
  d = opendir(path.c_str());
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      if (IsNumber(dir->d_name))
        fds.push_back(dir->d_name);
    }
    closedir(d);
  }
  // Info of each fd
  for (string fd : fds) {
    string fd_path = path + "/" + fd;
    struct stat fd_buf;
    access_status = CheckAccess(fd_path, &fd_buf);
    if (access_status == kFileNotExist) {
      continue;
    } else if (access_status == kPermissionDenied) {
      cerr << "Fd permission denied" << endl;
      exit(1);
    }
    // Target
    string target;
    if (!access(fd_path.c_str(), F_OK)) {
      continue;
    } else if (!access(fd_path.c_str(), R_OK | W_OK)) {
      target = fd + "u";
    } else if (!access(fd_path.c_str(), W_OK)) {
      target = fd + "w";
    } else if (!access(fd_path.c_str(), R_OK)) {
      target = fd + "r";
    } else {
      cerr << "Fd " << fd << " has no read and write right" << endl;
      exit(1);
    }
    // File type
    string type = GetFileType(fd_buf.st_mode);
    // i-node
    string inode = to_string(buf.st_ino);
    // Name
    string name;
    char fd_link_buf[kBufferSize];
    memset(fd_link_buf, 0, kBufferSize);
    readlink(fd_path.c_str(), fd_link_buf, sizeof(fd_link_buf));
    name = fd_link_buf;
    // Print out
    PrintOut(command, pid, user, target, type, inode, name);
  }
}