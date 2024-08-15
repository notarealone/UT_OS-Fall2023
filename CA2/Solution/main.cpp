#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include "tables.hpp"

using namespace std;
namespace fs = std::filesystem;

#define EFFECT_END "\033[0m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_YELLOW "\033[1;33m"
#define UNDERLINED_WHITE "\033[4;37m"

#define WRITE_PIPE 1
#define READ_PIPE 0

#define MAX_LENGTH 100
#define BUFFER_LEN 2048

void create_tables(string buf, vector<string> requests);

void extract_buildings(vector<string>&buildings, string path){
    for (const auto & entry : fs::directory_iterator(path)){
        struct stat sb;
        string s = entry.path().string();
        const char* temp_path = s.c_str();
        if ((stat(temp_path, &sb) == 0) && (sb.st_mode & S_IFDIR)){
            int pos = 0;
            while(pos < s.size()){
                pos = s.find("/");
                s.erase(0, pos+1);
            }
            buildings.push_back(s);

        }
    }
}

vector<string> get_requests(string line){
    vector<string> requests;
    int pos = 0;
    while ((pos = line.find(" ")) != string::npos) {
        requests.push_back(line.substr(0, pos));
        line.erase(0, pos + 1);
    }
    requests.push_back(line);
    return requests;
}

int main(int argc, char* argv[]){
    string buildings_path = argv[1];
    struct stat sb;
    vector<string> Buildings;
    extract_buildings(Buildings, buildings_path);

    cout << BOLD_YELLOW << "Amount of Buildings Found : " << EFFECT_END << BOLD_GREEN << Buildings.size() << EFFECT_END << endl << endl;
    cout << BOLD_RED << "Buildings' Names are as follow : " << EFFECT_END << endl;
    for(int i = 0; i < Buildings.size(); i++)
        cout << UNDERLINED_WHITE << Buildings[i] << EFFECT_END << ", ";
    cout << endl << endl << BOLD_GREEN << "Enter the resources and buildings you are looking for : " << EFFECT_END << endl;
    
    string input_line;
    getline(cin, input_line); //Gets space-seperated input from user
    vector<string> resource_requests = get_requests(input_line);
    getline(cin, input_line);
    vector<string> building_requests = get_requests(input_line); //Assumption -> resource and building requests are mapped 1 by 1

    //Creating Named Pipes
    string named_pipes = "named_pipe";
    for(int i = 0; i < building_requests.size(); i++){
        string pipe_path = named_pipes + to_string(i);
        if(mkfifo(const_cast<char*>(pipe_path.c_str()), 0666) == -1){
            cout << "FIFO error \n";
            exit(EXIT_FAILURE);
        }
    }

    //Creating child_processe and pipe for loading resource prices
    int bills_pipe[2];
    if(pipe(bills_pipe) == -1){
        exit(EXIT_FAILURE);
    }
    pid_t pid = fork();
    if(pid == 0){
        close(bills_pipe[READ_PIPE]);
        dup2(bills_pipe[WRITE_PIPE], STDOUT_FILENO);
        char* exec_path = const_cast<char*>("./bills.o");
        char pipes_num[10];
        sprintf(pipes_num, "%ld", building_requests.size());
        char* exec_arg[] = {const_cast<char*> (pipes_num), NULL};
        execvp(exec_path, exec_arg);
    }
    else{
        close(bills_pipe[WRITE_PIPE]);
    }

    //Creating child processes and their pipes for each requested building
    int main_pipes[building_requests.size()][2];
    for(int i = 0; i < building_requests.size(); i++){
        string building_num = to_string(i);
        if(pipe(main_pipes[i]) == -1){
            exit(EXIT_FAILURE);
        }
        pid = fork();
        if(pid == 0){
            close(main_pipes[i][READ_PIPE]);
            dup2(main_pipes[i][WRITE_PIPE], STDOUT_FILENO);
            if(resource_requests.size() == 1){
                char* exec_path = const_cast<char*>("./building.o");
                char* exec_arg[] = {const_cast<char*>(building_requests[i].c_str()), const_cast<char*>(resource_requests[0].c_str()), const_cast<char*>(building_num.c_str()),NULL};
                execvp(exec_path, exec_arg);
            }
            else if(resource_requests.size() == 2){
                char* exec_path = const_cast<char*>("./building.o");
                char* exec_arg[] = {const_cast<char*>(building_requests[i].c_str()), const_cast<char*>(resource_requests[0].c_str()), const_cast<char*>(resource_requests[1].c_str()), const_cast<char*>(building_num.c_str()),NULL};
                execvp(exec_path, exec_arg);
            }
            else {
                char* exec_path = const_cast<char*>("./building.o");
                char* exec_arg[] = {const_cast<char*>(building_requests[i].c_str()), const_cast<char*>(resource_requests[0].c_str()), const_cast<char*>(resource_requests[1].c_str()),  const_cast<char*>(resource_requests[2].c_str()), const_cast<char*>(building_num.c_str()),NULL};
                execvp(exec_path, exec_arg);
            }
        }
        else {
            close(main_pipes[i][WRITE_PIPE]);
        }
    }

    string buffer(BUFFER_LEN, '\0');
    string buildings_merged = "";
    for(int i = 0; i < building_requests.size(); i++){
        int bytes_read = read(main_pipes[i][READ_PIPE], buffer.data(), BUFFER_LEN);
        close(main_pipes[i][READ_PIPE]);
        buffer.resize(bytes_read);
        cout << "\n\n"; 
        cout << BOLD_GREEN << "Building Name : " << building_requests[i] << EFFECT_END << endl;
        create_tables(buffer, resource_requests);
        //buildings_merged += buffer + "~"; 
        
    }
    //buildings_merged.erase(buildings_merged.size()-1, buildings_merged.size());

    //cout << buildings_merged << endl;




    return 0;
}

void create_tables(string buf, vector<string> requests){
    vector<string> bills;
    int pos = 0;
    while((pos = buf.find("|")) != string::npos){
        bills.push_back(buf.substr(0, pos));
        buf.erase(0, pos+1);
    }
    bills.push_back(buf);

    string all_bill, all_usage, all_peak, all_avg;

    for(int i = 0; i < bills.size(); i++){
        pos = bills[i].find("#");
        all_bill = bills[i].substr(0, pos);
        bills[i].erase(0, pos+1);
        pos = bills[i].find("#");
        all_usage = bills[i].substr(0, pos);
        bills[i].erase(0, pos+1);
        pos = bills[i].find("#");
        all_peak = bills[i].substr(0, pos);
        bills[i].erase(0, pos+1);
        pos = bills[i].find("#");
        all_avg = bills[i].substr(0, pos);
        bills[i].erase(0, pos+1);

        vector<string> bill, usage, peaks, avgs;
        while((pos = all_bill.find("/")) != string::npos){
            bill.push_back((all_bill.substr(0, pos-5)));
            all_bill.erase(0, pos+1);
        }
        bill.push_back((all_bill.substr(0, all_bill.size()-5)));


        while((pos = all_usage.find("/")) != string::npos){
            usage.push_back((all_usage.substr(0, pos)));
            all_usage.erase(0, pos+1);
        }
        usage.push_back((all_usage));

        while((pos = all_peak.find("/")) != string::npos){
            peaks.push_back((all_peak.substr(0, pos)));
            all_peak.erase(0, pos+1);
        }
        peaks.push_back((all_peak));

        while((pos = all_avg.find("/")) != string::npos){
            avgs.push_back((all_avg.substr(0, pos-5)));
            all_avg.erase(0, pos+1);
        }
        avgs.push_back((all_avg.substr(0, all_avg.size()-5)));

        string headerrow[] = {"Months", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
        string headercolumn[] = {"Debt($)", "Usage", "Peak Hour", "Avg(perDay)"};

        vector<vector<string>> array;
        array.push_back(bill);
        array.push_back(usage);
        array.push_back(peaks);
        array.push_back(avgs);

        tables::options aoptions;
        aoptions.headerrow = true;
        aoptions.headercolumn = true;
        aoptions.check = false;
        aoptions.cellborder = true;
        aoptions.padding = 0;
        aoptions.alignment = nullptr;
        aoptions.style = tables::style_double;
        // or with C++20:
        // tables::options aoptions{.headerrow = true, .headercolumn = true};

        cout << UNDERLINED_WHITE << requests[i] << " Bill :" << EFFECT_END << endl;
        tables::array(array, headerrow, headercolumn, aoptions);
        cout << "\n";
    }

}