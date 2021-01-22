/**
* @file process.cpp
* @author Bengisu Özaydın
* @date 2 Jan 2021
* @brief Each process is forked from the watchdog process and sleeps until a signal comes from executor or watchdog processes, then handles the signal.
*/
#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <csignal>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>
#include <fstream>
using namespace std;

string process_output_path; // Path of the file to write the output.
string name;
struct timespec delta = {0, 300000000};
ofstream outfile;

/**
* @brief Handle signals. Print the value of received signal to output file. If the signal is SIGTERM, terminate.
* @param [in] signal type of signal that the process has received
* @return void
*/

void signal_handler(int signal){
  outfile.open(process_output_path, ios_base::app);
  if(signal != 15) {
    outfile << name << " received signal " << signal << endl;
    outfile.close();
  }
  else{
    outfile << name << " received signal " << signal << ", terminating gracefully" << endl;
    outfile.close();
    exit(0);
  }
}


int main(int argc, char *argv[]) {

  /**
  * Get arguments. Expects 2 inputs: path to the process output file and the name of process.
  */
  process_output_path = argv[1];
  name = argv[2];

  /**
  * Write to the output file that the process has been created and is waiting for a signal.
  */
  outfile.open(process_output_path, ios_base::app);
  outfile << name << " is waiting for a signal" << endl;
  outfile.close();

  /**
  * Receive signals to be handled and call the handler method by sending the type of the signal.
  */
  signal(SIGHUP, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGILL, signal_handler);
  signal(SIGTRAP, signal_handler);
  signal(SIGFPE, signal_handler);
  signal(SIGSEGV, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGXCPU, signal_handler);

  /**
  * Sleep until a signal is received.
  */
  while(true){
    nanosleep(&delta, &delta);
  }

  return 0;
}
