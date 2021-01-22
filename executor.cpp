/*
* Created by Misra Yavuz.
* Do not edit this program. 
* Compile: g++ executor.cpp -o executor
* ./executor processNum instructionsPath
*/
#include <iostream>
#include <string>
#include <unistd.h>
#include <csignal>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>

using namespace std;
map<string,int> sigMap;
bool finished=false;
int processNum;
string inputPath;
struct timespec delta = {0 /*secs*/, 300000000 /*nanosecs*/}; //0.3 sec

int main(int argc, char *argv[]) {
    /*
    * Map string of signal name to signal int.
    */
    sigMap["SIGINT"] = SIGINT;
    sigMap["SIGHUP"] = SIGHUP;
    sigMap["SIGILL"] = SIGILL;
    sigMap["SIGTRAP"] = SIGTRAP;
    sigMap["SIGBUS"] = SIGBUS;
    sigMap["SIGFPE"] = SIGFPE;
    sigMap["SIGSEGV"] = SIGSEGV;
    sigMap["SIGXCPU"] = SIGXCPU;
    sigMap["SIGTERM"] = SIGTERM;

    /*
    * Get command line arguments. Expects 2 inputs, number of processes and a path to the instructions.txt file.
    */
    processNum = stoi(argv[1]) ;
    inputPath = argv[2];
    pid_t pidList[processNum+1]; // Keep PID of watchdog at 0, PID of P1 at 1, ...

    /*
    * Create the named file (FIFO) under /tmp/myfifo directory.
    * executor only reads the newly written PIDs from the pipe.
    * Take a look at the example usage of named pipes for the watchdog part. 
    * (For ex., https://www.geeksforgeeks.org/named-pipe-fifo-example-c-program/)
    */
    int unnamedPipe;
    char * myfifo = (char*) "/tmp/myfifo";
    mkfifo(myfifo, 0644);
    char temp[30];
    unnamedPipe = open(myfifo,O_RDONLY);

    /*
    * Read PIDs that are written by the watchdog.
    * Format: "P# <pid>"
    * Example: "P1 3564"
    * P0 = watchdog, P1, P2, .... PN
    */
    string p_index, p_pid;
    int pid,id;
    for (int i=0; i<=processNum; i++) {
        read(unnamedPipe, temp, 30);
        stringstream splitmsg(temp);
        splitmsg >> p_index >> p_pid;
        id = stoi(p_index.substr(1));
        pid = stoi(p_pid);
        pidList[id] = pid;
    }

    /*
    * Read instructions from the input file.
    */
    string inst, token1, token2;
    int tempID, tempSig;
    ifstream inst_file (inputPath);
    if (inst_file.is_open()) {
        while ( getline(inst_file, inst)){
            stringstream splitline(inst);
            splitline >> token1 >> token2 ;

            /*
            * If the instruction is a signal:
            */
            if (token1.substr(0,3) == "SIG" ) {
                tempID =  stoi(token2.substr(1));
                tempSig = sigMap.at(token1);
                nanosleep(&delta, &delta);  // Deal with writing delays
                kill(pidList[tempID] , tempSig);

                /*
                * If SIGTERM is sent to head process, i.e., P1, read processNum amount of PIDs from pipe and update the list.
                */
                if (tempID == 1 && tempSig == 15) {
                    for (int i= 1; i<=processNum; i++){
                        read(unnamedPipe, temp, 30);
                        stringstream splitmsg(temp);
                        splitmsg >> p_index >> p_pid;
                        id = stoi(p_index.substr(1));
                        pid = stoi(p_pid);
                        pidList[id] = pid;
                    }
                }
                
                /*
                * If SIGTERM is sent to processes except P1, read only one PID from the pipe.
                */
                else if (tempSig == 15){
                    read(unnamedPipe, temp, 30);
                    stringstream splitmsg(temp);
                    splitmsg >> p_index >> p_pid;
                    id = stoi(p_index.substr(1));
                    pid = stoi(p_pid);
                    pidList[id] = pid;
                }
            }
            /*
            * If the instruction is wait:
            */
            else sleep(stoi(token2));
        }
        /*
        * Instruction file is read completely.
        */
        close(unnamedPipe);
        finished = true;
    }
    /*
    * Kill all processes and exit.
    */
    if (finished) {
        for (int i=0; i<=processNum; i++){
            kill(pidList[i] , SIGTERM);
            nanosleep(&delta, &delta);
        }
        return 0;
    }
}