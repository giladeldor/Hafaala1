#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <memory>
#include <string>
#include <vector>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_ARGV_LENGTH (2 * COMMAND_MAX_ARGS + 5)

class SmallShell;

class Command {
protected:
  const std::string command_line;
  char **argv;
  const int argc;
  bool background_command_flag;
  // TODO: Add your data members
public:
  Command(const char *cmd_line, bool background_command_flag);
  virtual ~Command();
  virtual void execute(SmallShell *smash) = 0;
  const std::string getCommandLine() const;
  bool isBackgroundCommand() const;
  // virtual void prepare();
  // virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
  BuiltInCommand(const char *cmd_line) : Command(cmd_line, false) {}
  virtual ~BuiltInCommand() {}
};

class ChangePromptCommand : public BuiltInCommand {
public:
  ChangePromptCommand(const char *cmd_line);
  virtual ~ChangePromptCommand() {}
  void execute(SmallShell *smash) override;
};

class ExternalCommand : public Command {
public:
  ExternalCommand(const char *cmd_line);
  virtual ~ExternalCommand() {}
  void execute(SmallShell *smash) override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
public:
  PipeCommand(const char *cmd_line);
  virtual ~PipeCommand() {}
  void execute(SmallShell *smash) override;
};

class RedirectionCommand : public Command {
  // TODO: Add your data members
public:
  explicit RedirectionCommand(const char *cmd_line);
  virtual ~RedirectionCommand() {}
  void execute(SmallShell *smash) override;
  // void prepare() override;
  // void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
  ChangeDirCommand(const char *cmd_line);

  virtual ~ChangeDirCommand() {}

  void execute(SmallShell *smash) override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
  GetCurrDirCommand(const char *cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute(SmallShell *smash) override;
};

class ShowPidCommand : public BuiltInCommand {
public:
  ShowPidCommand(const char *cmd_line);
  virtual ~ShowPidCommand() {}
  void execute(SmallShell *smash) override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  QuitCommand(const char *cmd_line, JobsList *jobs);
  virtual ~QuitCommand() {}
  void execute(SmallShell *smash) override;
};

class JobsList {
public:
  int id;
  class JobEntry {
    // TODO: Add your data members
  };
  // TODO: Add your data members
public:
  JobsList();
  ~JobsList();
  void addJob(Command *cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry *getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  JobsCommand(const char *cmd_line, JobsList *jobs);
  virtual ~JobsCommand() {}
  void execute(SmallShell *smash) override;
};

class ForegroundCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  ForegroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~ForegroundCommand() {}
  void execute(SmallShell *smash) override;
};

class BackgroundCommand : public BuiltInCommand {
  // TODO: Add your data members
public:
  BackgroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~BackgroundCommand() {}
  void execute(SmallShell *smash) override;
};

class TimeoutCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
public:
  explicit TimeoutCommand(const char *cmd_line);
  virtual ~TimeoutCommand() {}
  void execute(SmallShell *smash) override;
};

class FareCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
public:
  FareCommand(const char *cmd_line);
  virtual ~FareCommand() {}
  void execute(SmallShell *smash) override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
public:
  SetcoreCommand(const char *cmd_line);
  virtual ~SetcoreCommand() {}
  void execute(SmallShell *smash) override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
  // TODO: Add your data members
public:
  KillCommand(const char *cmd_line, JobsList *jobs);
  virtual ~KillCommand() {}
  void execute(SmallShell *smash) override;
};

class SmallShell {
private:
  const std::string default_display_prompt;
  const pid_t smash_pid;
  std::string current_display_prompt;
  std::string last_dir;
  bool is_working;

  /*
  add more data members
  */
  // TODO: Add your data members
  SmallShell();

public:
  std::shared_ptr<Command> CreateCommand(const char *cmd_line);
  SmallShell(SmallShell const &) = delete;     // disable copy ctor
  void operator=(SmallShell const &) = delete; // disable = operator
  static SmallShell &getInstance()             // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char *cmd_line);
  void setDisplayPrompt(std::string new_display_line);
  void setLastDir(const std::string &last_dir);

  const pid_t getPid() const;
  std::string getDisplayPrompt() const;
  const std::string &getLastDir() const;

  void syscallError(const std::string &syscall);
  bool isSmashWorking() const;
  void disableSmash();
  void killAllJobs();
};

#endif // SMASH_COMMAND_H_
