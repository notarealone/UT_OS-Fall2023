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

using namespace std;

int main(int argc, char* argv[]){
    int pipes_num = atoi(argv[0]);
    rapidcsv::Document bills("buildings/bills.csv", rapidcsv::LabelParams(0, 0));
    vector<string> gas_price = bills.GetColumn<string>("gas");
    vector<string> elec_price = bills.GetColumn<string>("electricity");
    vector<string> water_price = bills.GetColumn<string>("water");

    string temp_out = "";
    for(int j = 0; j < 12; j++){
        temp_out = temp_out + "#" + gas_price[j];
    }
    for(int j = 0; j < 12; j++){
        temp_out = temp_out + "#" + elec_price[j];
    }
    for(int j = 0; j < 12; j++){
        temp_out = temp_out + "#" + water_price[j];
    }
    temp_out.erase(0, 1); //erases the first #
    for(int i = 0; i < pipes_num; i++){
        string pipe_name = "named_pipe" + to_string(i);
        int fd = open(pipe_name.c_str(), O_WRONLY);
        write(fd, temp_out.c_str(), temp_out.size());
        close(fd);
    }
}