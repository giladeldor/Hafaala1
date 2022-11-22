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
// Table of Content:
// Utillities functions ------------------------------------------- Line 30
// Small Shell Functions ------------------------------------------ Line 90
// Command Functions ---------------------------------------------- Line 208
// Joblist Functions ---------------------------------------------- Line 390

//                                                                      //
//------------------------- Utility functions --------------------------//
//                                                                      //
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

static void syscallError(const std::string &syscall) {
  std::string msg =
      std::string("smash error: " + syscall + std::string(" failed"));
  perror(msg.c_str());
}

//                                                                     //
//------------------------Small Shell functions------------------------//
//                                                                     //
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
void SmallShell::killAllJobs() { jobs.killAllJobs(); }
JobsList *SmallShell::getJobList() { return &jobs; }
pid_t SmallShell::getCurrnetCommandPid() const { return current_command_pid; }

void SmallShell::stopCurrentCommand() {
  if (current_command_pid == -1) {
    return;
  }

  kill(current_command_pid, SIGSTOP);
}

void SmallShell::killCurrentCommand() {
  if (current_command_pid == -1) {
    return;
  }

  kill(current_command_pid, SIGKILL);
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
    return std::make_shared<ChangePromptCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("showpid") == 0) {
    return std::make_shared<ShowPidCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("pwd") == 0) {
    return std::make_shared<GetCurrDirCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("cd") == 0) {
    return std::make_shared<ChangeDirCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("quit") == 0) {
    return std::make_shared<QuitCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("jobs") == 0) {
    return std::make_shared<JobsCommand>(cmd_line, cmd_s);
  } else if (firstWord.compare("fg") == 0) {
    return std::make_shared<ForegroundCommand>(cmd_line, cmd_s);
  } else {
    return std::make_shared<ExternalCommand>(cmd_line, cmd_s, background_flag);
  }

  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  jobs.removeFinishedJobs();

  auto command = CreateCommand(cmd_line);

  // Check if builtin or external
  bool isExternal = dynamic_cast<ExternalCommand *>(command.get()) != nullptr;
  if (isExternal) {
    // TODO: pipe + redirect
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
      if (command->isBackgroundCommand()) {
        jobs.addJob(command, pid, false);
      } else {
        current_command_pid = pid;

        int waitStatus;
        waitpid(pid, &waitStatus, WUNTRACED);

        if (WIFSTOPPED(waitStatus)) {
          jobs.addJob(command, pid, true);
          std::cout << "smash: process " << pid << " was stopped" << std::endl;
        }
      }
    }

  } else {
    command->execute(this);
  }
}

//                                                                 //
//------------------------Command functions------------------------//
//                                                                 //
Command::Command(const std::string &cmd_line,
                 const std::string &cmd_line_stripped,
                 bool background_command_flag)
    : command_line(cmd_line), argv(new char *[MAX_ARGV_LENGTH]),
      argc(_parseCommandLine(cmd_line_stripped, argv)),
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

ChangePromptCommand::ChangePromptCommand(const std::string &cmd_line,
                                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ChangePromptCommand::execute(SmallShell *smash) {
  if (argc == 1) {
    smash->setDisplayPrompt("smash");
  } else {
    smash->setDisplayPrompt(std::string(argv[1]));
  }
}

ShowPidCommand::ShowPidCommand(const std::string &cmd_line,
                               const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ShowPidCommand::execute(SmallShell *smash) {
  std::cout << "smash pid is " << smash->getPid() << std::endl;
}

GetCurrDirCommand::GetCurrDirCommand(const std::string &cmd_line,
                                     const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void GetCurrDirCommand::execute(SmallShell *smash) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    std::cout << cwd << std::endl;
  } else {
    syscallError("getcwd");
  }
}

ChangeDirCommand::ChangeDirCommand(const std::string &cmd_line,
                                   const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ChangeDirCommand::execute(SmallShell *smash) {
  if (argc > 2) {
    std::cerr << "smash error: cd: too many arguments" << std::endl;
    return;
  }

  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    syscallError("getcwd");
  }

  if (argc == 1) {
    if (chdir(getenv("HOME")) != 0) {
      syscallError("chdir");
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
          syscallError("chdir");
          return;
        }
      }
    }

    // Handle general cd.
    else {
      if (chdir(target) != 0) {
        syscallError("chdir");
        return;
      }
    }
  }

  smash->setLastDir(cwd);
}

JobsCommand::JobsCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}
void JobsCommand::execute(SmallShell *smash) {
  smash->getJobList()->printJobsList();
}

QuitCommand::QuitCommand(const std::string &cmd_line,
                         const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void QuitCommand::execute(SmallShell *smash) {
  smash->disableSmash();
  if (argc >= 2 && std::string(argv[1]).compare("kill") == 0) {
    smash->killAllJobs();
  }
  // kill the jobs
}

ForegroundCommand::ForegroundCommand(const std::string &cmd_line,
                                     const std::string &cmd_line_stripped)
    : BuiltInCommand(cmd_line, cmd_line_stripped) {}

void ForegroundCommand::execute(SmallShell *smash) {
  JobsList::JobEntry *job;
  if (argc == 1) {
    job = smash->getJobList()->getLastJob();

    if (!job) {
      std::cerr << "smash error: fg: jobs list is empty" << std::endl;
      return;
    }
  } else if (argc == 2) {
    try {
      int id = std::stoi(argv[1]);
      job = smash->getJobList()->getJobById(id);

      if (!job) {
        std::cerr << "smash error: fg: job-id " << id << " does not exist"
                  << std::endl;
        return;
      }
    } catch (const std::exception &e) {
      std::cerr << "smash error: fg: invalid arguments" << std::endl;
      return;
    }
  } else {
    std::cerr << "smash error: fg: invalid arguments" << std::endl;
    return;
  }

  job->state = JobsList::JobState::Running;
  std::cout << *job << std::endl;

  if (kill(job->pid, SIGCONT) == -1) {
    syscallError("kill");
  }
  if (waitpid(job->pid, nullptr, 0) == -1) {
    syscallError("waitpid");
  }

  smash->getJobList()->removeJobById(job->id);
}

ExternalCommand::ExternalCommand(const std::string &cmd_line,
                                 const std::string &cmd_line_stripped,
                                 bool background_command_flag)
    : Command(cmd_line, cmd_line_stripped, background_command_flag) {}

void ExternalCommand::execute(SmallShell *smash) {
  // First change group ID to prevent shell signals from being received.
  if (setpgrp() != 0) {
    syscallError("setpgrp");
  }

  if (execv(argv[0], argv) != 0) {
    syscallError("execv");
    exit(1);
  };
}

//                                                                 //
//------------------------JobList functions------------------------//
//                                                                 //
std::ostream &operator<<(std::ostream &os, const JobsList::JobEntry &job) {
  auto now = time(nullptr);
  int delta = (int)difftime(now, job.startTime);

  os << "[" << job.id << "] " << job.command->getCommandLine() << " : "
     << job.pid << " " << delta << " secs"
     << (job.state == JobsList::JobState::Stopped ? " (stopped)" : "");

  return os;
}

void JobsList::addJob(std::shared_ptr<Command> cmd, pid_t pid, bool isStopped) {
  removeFinishedJobs();
  jobs.push_back(std::make_shared<JobEntry>(cmd, getFreeID(), pid,
                                            isStopped ? JobState::Stopped
                                                      : JobState::Running));
}

void JobsList::printJobsList() {
  // TODO: mask alarm signal when travesing joblist.
  removeFinishedJobs();

  for (auto &&job : jobs) {
    std::cout << (*job) << std::endl;
  }
}

void JobsList::killAllJobs() {
  // TODO: mask alarm signal when travesing joblist.

  for (auto &&job : jobs) {
    if (kill(job->pid, SIGKILL) == -1) {
      syscallError("kill");
    }
    if (waitpid(job->pid, nullptr, 0) == -1) {
      syscallError("waitpid");
    }
  }
  jobs.clear();
}

void JobsList::removeFinishedJobs() {
  // TODO: mask alarm signal when travesing joblist.

  auto it = jobs.begin();
  while (it != jobs.end()) {
    auto job = *it;

    int waitStatus;
    if (waitpid(job->pid, &waitStatus, WNOHANG) == -1) {
      syscallError("waitpid");
    }

    if (WIFEXITED(waitStatus)) {
      auto current = it++;
      jobs.erase(current);
    } else {
      ++it;
    }
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId) {
  // TODO: mask alarm signal when travesing joblist.
  for (auto &&job : jobs) {
    if (job->id == jobId) {
      return job.get();
    }
  }

  return nullptr;
}

void JobsList::removeJobById(int jobId) {
  // TODO: mask alarm signal when travesing joblist.
  auto it = jobs.begin();
  while (it != jobs.end() && (*it)->id != jobId) {
    ++it;
  }

  if (it != jobs.end()) {
    jobs.erase(it);
  }
}

JobsList::JobEntry *JobsList::getLastJob() {
  // TODO: mask alarm signal when travesing joblist.

  if (jobs.empty()) {
    return nullptr;
  }

  auto lastJob = jobs.back();
  return lastJob.get();
}

JobsList::JobEntry *JobsList::getLastStoppedJob() {
  // TODO: mask alarm signal when travesing joblist.

  auto it = jobs.rbegin();
  while (it != jobs.rend() && (*it)->state != JobState::Stopped) {
    ++it;
  }

  if (it == jobs.rend()) {
    return nullptr;
  }

  return it->get();
}

int JobsList::getFreeID() const {
  // TODO: mask alarm signal when travesing joblist.

  if (jobs.empty()) {
    return 0;
  }

  return jobs.back()->id + 1;
}
