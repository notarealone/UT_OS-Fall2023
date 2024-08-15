#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include "rapidcsv.h"

#define ARG_OFFSET 1

#define WRITE_PIPE 1
#define READ_PIPE 0

#define MAX_LENGTH 100
#define BUFFER_LEN 4096

using namespace std;

int main(int argc, char* argv[]){
    string building_name = string(argv[0]);
    string folder_path = "buildings/";
    vector<string> csv_path;
    for(int i = 1; i < argc - 1; i++){
        csv_path.push_back(folder_path + building_name + "/" + string(argv[i]) + ".csv");
    }
    
    string buffer(BUFFER_LEN, '\0');
    //Reading Prices from bills.o process
    string temp_name = argv[argc - 1];
    string pipe_name = "named_pipe" + temp_name;
    int fd = open(pipe_name.c_str(), O_RDONLY);
    int bytes_read = read(fd, buffer.data(), BUFFER_LEN);
    close(fd);
    unlink(pipe_name.c_str());
    buffer.resize(bytes_read);

    int building_pipes[argc - 2][2];
    pid_t pid;
    for(int i = 0; i < argc - 2; i++){
        if(pipe(building_pipes[i]) == -1){
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if(pid == 0){
            close(building_pipes[i][READ_PIPE]);
            dup2(building_pipes[i][WRITE_PIPE], STDOUT_FILENO);
            char* exec_path = const_cast<char*>("./counter.o");
            char* exec_arg[] = {const_cast<char*>(csv_path[i].c_str()), const_cast<char*>(buffer.c_str()), NULL};
            execvp(exec_path, exec_arg);
                exit(EXIT_FAILURE); // runs only when exec() runs into a problem
        }
    }
    string temp_buffer = "";
    for(int i = 0; i < argc - 2; i++){
        buffer.resize(BUFFER_LEN);
        bytes_read = read(building_pipes[i][READ_PIPE], buffer.data(), BUFFER_LEN);
        close(building_pipes[i][READ_PIPE]);
        buffer.resize(bytes_read);
        temp_buffer = temp_buffer + buffer + "|";
    }
    temp_buffer.erase(temp_buffer.size()-1,temp_buffer.size());
    cout << temp_buffer; //Sends data back to parent process through STDOUT fd

}