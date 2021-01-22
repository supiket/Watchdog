/**
* @file watchdog.cpp
* @author Bengisu Özaydın
* @date 2 Jan 2021
* @brief Serves as parent for the processes. Keeps track of all processes.
*/
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>
#include <vector>
using namespace std;

int num_of_process;
char *process_output_path;
char *watchdog_output_path;
struct timespec delta = {0, 300000000};
vector<pid_t> pid_vector(0);
string name_pid_tuple;
char *name_pid_tuple_c;
int unnamed_pipe;
ofstream outfile;

/**
* @brief Write process name and pid tuple to the pipe to be shared with the executor. Writes to the pipe as char array.
* @param [in] process_name name of process: P1, P1, ..., PN.
* @return void
*/
void write_to_pipe(string process_name, pid_t process_id){
    name_pid_tuple = process_name + " " + to_string(process_id);
    name_pid_tuple_c = &name_pid_tuple[0];
    write(unnamed_pipe, name_pid_tuple_c, sizeof(name_pid_tuple_c));
}

/**
* @brief Handle signal (specifically, SIGTERM) sent to watchdog.T erminate all child processes, close output file, close pipe, terminate itself.
* @param [in] signal type of signal received
* @return void
*/
void sigterm_handler(int signal){
  outfile << "Watchdog is terminating gracefully" << endl;
  for(int i = 0; i < num_of_process; i++){
    nanosleep(&delta, &delta);
    kill(pid_vector[i], SIGTERM);
    nanosleep(&delta, &delta);
  }
  outfile.close();
  close(unnamed_pipe);
  exit(0);
}

/**
* @brief Fork and call exec() to create a new process. Write pid of new process to pipe.
* @param [in] i number that will give its name to the process to be created.
* @return void
*/
void fork_and_exec(int i){
  string name = "P" + to_string(i+1);
  char * name_c = &name[0];
  pid_t pid = fork();
  usleep(1000);
  if(pid == -1){
    i--;
  }
  if(pid == 0){
    execl("process", "process", process_output_path, name_c, (char*) NULL);
  }
  else{
    pid_vector.at(i) = pid;
    outfile << name << " is started and it has a pid of " << pid << endl;
    write_to_pipe(name_c, pid);
  }
}

int main(int argc, char *argv[]) {

  /**
  * Listen for SIGTERM signal. When signal received, call handler method.
  */
  signal(SIGTERM, sigterm_handler);

  /**
  * Get command line arguments. Expects 3 inputs, number of processes, path to file for process output and path to file for watchdog output.
  */
  num_of_process = stoi(argv[1]);
  process_output_path = argv[2];
  watchdog_output_path = argv[3];

  /**
  * Open output file.
  */
  outfile.open(watchdog_output_path, ios_base::app);

  pid_vector.resize(num_of_process);

  /**
  * Open pipe.
  */
  char * myfifo = (char*) "/tmp/myfifo";
  unnamed_pipe = open(myfifo, O_WRONLY);
  /**
  * Write name and PID of self to pipe.
  */
  write_to_pipe("P0", getpid());

  /**
  * Create specified number of processes.
  */
  for (int i=0; i<num_of_process; i++) {
    fork_and_exec(i);
  }

  /**
  * Wait for the pid of process that is terminated.
  */
  while(true){
    pid_t terminated_pid = wait(NULL);
    /**
    * If P1 is ternimated, kill all other processes and restart all.
    */
    if(terminated_pid == pid_vector[0]){
      outfile << "P1 is killed, all processes must be killed" << endl;
      for(int i = 1; i < num_of_process; i++){
        nanosleep(&delta, &delta);
        kill(pid_vector[i], SIGTERM);
        nanosleep(&delta, &delta);
      }
      outfile << "Restarting all processes" << endl;
      for(int i = 0; i < num_of_process; i++){
        nanosleep(&delta, &delta);
        fork_and_exec(i);
        nanosleep(&delta, &delta);
      }
    }
    /**
    * If any other process is terminated, restart that process.
    */
    else{
      for(int i = 1; i < num_of_process; i++){
        if(terminated_pid == pid_vector[i]){
          outfile << "P" << i+1 << " is killed" << endl;
          outfile << "Restarting P" << i+1 << endl;
          nanosleep(&delta, &delta);
          fork_and_exec(i);
          nanosleep(&delta, &delta);
          break;
        }
      }
    }
  }

  return 0;
}
