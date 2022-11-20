#include "Commands.h"
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() cout << __PRETTY_FUNCTION__ << " --> " << std::endl;

#define FUNC_EXIT() cout << __PRETTY_FUNCTION__ << " <-- " << std::endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

std::string _ltrim(const std::string &s) {
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string _rtrim(const std::string &s) {
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string _trim(const std::string &s) { return _rtrim(_ltrim(s)); }

int _parseCommandLine(const std::string &cmd_line, char **args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(cmd_line).c_str());
  for (std::string s; iss >> s;) {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
  const std::string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
  const std::string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == std::string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing
  // spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

// Not finished
SmallShell::SmallShell()
    : smash_pid(getpid()), current_display_prompt("smash"), last_dir(""),
      default_display_prompt("smash"), is_working(true) {}

// TODO: add your implementation

SmallShell::~SmallShell() {
  // TODO: add your implementation
}

const std::string &SmallShell::getLastDir() const { return last_dir; }

void SmallShell::setDisplayPrompt(std::string new_display_line) {
  current_display_prompt = new_display_line;
}
void SmallShell::setLastDir(const std::string &last_dir) {
  this->last_dir = last_dir;
}

const pid_t SmallShell::getPid() const { return smash_pid; }
std::string SmallShell::getDisplayPrompt() const {
  return current_display_prompt;
}
bool SmallShell::isSmashWorking() const { return is_working; }
void SmallShell::disableSmash() { is_working = false; }
void SmallShell::killAllJobs() {
  // TODO: Implement killing all jobs
}

/**
 * Creates and returns a pointer to Command class which matches the given
 * command line (cmd_line)
 */
std::shared_ptr<Command>
SmallShell::CreateCommand(const std::string &cmd_line) {
  // For example:

  std::string cmd_s = _trim(cmd_line);
  bool background_flag = cmd_s.back() == '&';
  if (background_flag) {
    cmd_s.pop_back();
  }
  std::string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  /* check if special command I.E pipe*/

  if (firstWord.compare("chprompt") == 0) {
    return std::make_shared<ChangePromptCommand>(cmd_s);
  } else if (firstWord.compare("showpid") == 0) {
    return std::make_shared<ShowPidCommand>(cmd_s);
  } else if (firstWord.compare("pwd") == 0) {
    return std::make_shared<GetCurrDirCommand>(cmd_s);
  } else if (firstWord.compare("cd") == 0) {
    return std::make_shared<ChangeDirCommand>(cmd_s);
  } else if (firstWord.compare("quit") == 0) {
    return std::make_shared<QuitCommand>(cmd_s, nullptr);
  } else {
    return std::make_shared<ExternalCommand>(cmd_s, background_flag);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  auto command = CreateCommand(cmd_line);

  // Check if builtin or external
  bool isExternal = dynamic_cast<ExternalCommand *>(command.get()) != nullptr;
  if (isExternal) {
    // TODO: fork + execv + pipe + redirect
    int pid = fork();
    if (pid == -1) {
      syscallError("fork");
      return;
    }

    if (pid == 0) {
      // Forked child
      command->execute(this);
    } else {
      // Parent
      wait(nullptr);
    }

  } else {
    command->execute(this);
  }
}

void SmallShell::syscallError(const std::string &syscall) {
  std::string msg =
      std::string("smash error: " + syscall + std::string(" failed"));
  perror(msg.c_str());
}

Command::Command(const std::string &cmd_line, bool background_command_flag)
    : command_line(cmd_line), argv(new char *[MAX_ARGV_LENGTH]),
      argc(_parseCommandLine(cmd_line, argv)),
      background_command_flag(background_command_flag) {}

Command::~Command() {
  for (int i = 0; i < argc; i++) {
    delete argv[i];
    argv[i] = NULL;
  }
  delete[] argv;
}

const std::string Command::getCommandLine() const { return command_line; }
bool Command::isBackgroundCommand() const { return background_command_flag; }

ChangePromptCommand::ChangePromptCommand(const std::string &cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChangePromptCommand::execute(SmallShell *smash) {
  if (argc == 1) {
    smash->setDisplayPrompt("smash");
  } else {
    smash->setDisplayPrompt(std::string(argv[1]));
  }
}

ShowPidCommand::ShowPidCommand(const std::string &cmd_line)
    : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute(SmallShell *smash) {
  std::cout << "smash pid is " << smash->getPid() << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const std::string &cmd_line)
    : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute(SmallShell *smash) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    std::cout << cwd << std::endl;
  } else {
    smash->syscallError("getcwd");
  }
}

ChangeDirCommand::ChangeDirCommand(const std::string &cmd_line)
    : BuiltInCommand(cmd_line) {}

void ChangeDirCommand::execute(SmallShell *smash) {
  if (argc > 2) {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    smash->syscallError("getcwd");
  }

  if (argc == 1) {
    if (chdir(getenv("HOME")) != 0) {
      smash->syscallError("chdir");
      return;
    }
  } else {
    const char *target = argv[1];

    // Handle cd to previous directory.
    if (strcmp(target, "-") == 0) {
      auto lastDir = smash->getLastDir();

      if (lastDir.empty()) {
        std::cerr << "smash error: cd: OLDPWD not set" << std::endl;
        return;
      } else {
        if (chdir(lastDir.c_str()) != 0) {
          smash->syscallError("chdir");
          return;
        }
      }
    }

    // Handle general cd.
    else {
      if (chdir(target) != 0) {
        smash->syscallError("chdir");
        return;
      }
    }
  }

  smash->setLastDir(cwd);
}

QuitCommand::QuitCommand(const std::string &cmd_line, JobsList *jobs)
    : BuiltInCommand(cmd_line) {}

void QuitCommand::execute(SmallShell *smash) {
  smash->disableSmash();
  if (argc >= 2 && std::string(argv[1]).compare("kill") == 0) {
    smash->killAllJobs();
  }
  // kill the jobs
}

ExternalCommand::ExternalCommand(const std::string &cmd_line,
                                 bool background_command_flag)
    : Command(cmd_line, background_command_flag) {}

void ExternalCommand::execute(SmallShell *smash) {
  // First change group ID to prevent shell signals from being received.
  if (setpgrp() != 0) {
    smash->syscallError("setpgrp");
  }

  if (execv(argv[0], argv) != 0) {
    smash->syscallError("execv");
    exit(1);
  };
}