#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
#include "rapidcsv.h"

#define HOUR_OFFSET 3
#define DAY_OFFSET 2


using namespace std;


vector<vector<float>> msg_parser(string msg){
    vector<vector<float>> prices;
    for(int i = 0; i < 3; i++){
        vector<float> temp_prices;
        int pos = 0;
        while((pos = msg.find("#")) != string::npos){
            temp_prices.push_back(stof(msg.substr(0, pos)));
            msg.erase(0, pos+1);
            if(temp_prices.size() == 12) {
                break;
            }
        }
        if(i == 2){
            temp_prices.push_back(stof(msg));
        }
        prices.push_back(temp_prices);
    }
    return prices;
}


vector<int> cal_peak(vector<vector<int>> list){
    vector<int> out;
    int time[6] = {0, 0, 0, 0, 0, 0};
    int temp_max = 0;
    int temp_max_index = 0;

    for(int i = 0; i < list.size(); i++){
        temp_max = list[i][0 + HOUR_OFFSET];
        temp_max_index = 0;
        for(int j = 0; j < 6; j++){
            if(list[i][j] > temp_max){
                temp_max = list[i][j + HOUR_OFFSET];
                temp_max_index = j;
            }
        }
        time[temp_max_index] += 1;
        if((i+1) % 30 == 0){
            temp_max = 0;
            temp_max_index = 0;
            for(int i = 0; i < 6; i++){
                if(temp_max < time[i]){
                    temp_max = time[i];
                    temp_max_index = i;
                }
            }
            out.push_back(temp_max_index);
        }
    }

    return out;
}

vector<float> cal_avg(vector<vector<int>> list){

    vector<float> avg_per_month;
    float sum = 0;

    for(int i = 1; i < list.size(); i++){
        for(int j = 0; j < 6; j++){
            sum += list[i][j + HOUR_OFFSET];
        }
        if(list[i][DAY_OFFSET] == 30){
            avg_per_month.push_back(sum/30);
            sum = 0;
        }
    }

    return avg_per_month;
}

vector<int> cal_usage(vector<vector<int>> list){
    vector<int> usage;
    int temp = 0;
    for(int i = 0; i < list.size(); i++){
        for(int j = 0; j < 6; j++){
            temp += list[i][j + HOUR_OFFSET];
        }
        if((i+1)%30 == 0){
            usage.push_back(temp);
            temp = 0;
        }
    }

    return usage;
}


string find_name(string str){
    int pos = 0;
    str.erase(str.size()-4, str.size());
    while((pos = str.find("/")) != string::npos){
        
        str.erase(0, pos+1);
    }
    return str;
}


vector<float> cal_bill(vector<vector<int>> usage, vector<int> usage_per_month, vector<vector<float>> price, vector<int> peak, string file_type){
    vector<float> out;
    if(file_type == "Gas"){
        for(int j = 0; j < 12; j++){
            out.push_back(price[0][j] * usage_per_month[j]);
        }
    } else if(file_type == "Electricity"){
        float temp = 0;
        for(int j = 0; j < 360; j++){
            for(int i = 0; i < 6; i++){
                if(peak[usage[j][1] - 1] == i){
                    temp += (price[1][usage[j][1] - 1] * 1.25 * usage[j][i + HOUR_OFFSET]);
                }
                else {
                    temp += (price[1][usage[j][1] - 1] * usage[j][i + HOUR_OFFSET]);
                }
            }
            if((j + 1) % 30 == 0){
                out.push_back(temp);
                temp = 0;
            }
        }
    } else if(file_type == "Water"){
        float temp = 0;
        for(int j = 0; j < 360; j++){
            for(int i = 0; i < 6; i++){
                if(peak[usage[j][1] - 1] == i){
                    temp += (price[2][usage[j][1] - 1] * 1.25 * usage[j][i + HOUR_OFFSET]);
                }
                else {
                    temp += (price[2][usage[j][1] - 1] * usage[j][i + HOUR_OFFSET]);
                }
            }
            if((j + 1) % 30 == 0){
                out.push_back(temp);
                temp = 0;
            }
        }
    }
    

    return out;    
}


int main(int argc, char* argv[]){
    rapidcsv::Document counter(argv[0], rapidcsv::LabelParams(-1, -1));
    string file_name = find_name(string(argv[0]));
    vector<vector<int>> usage_stats;
    vector<vector<float>> prices = msg_parser(argv[1]);
    
    for(int i = 1; i <= 360; i++){
        usage_stats.push_back(counter.GetRow<int>(i));
    }
    
    vector<int> peak_hour = cal_peak(usage_stats);              //Calculated per month peak hour
    vector<float> avg_usage = cal_avg(usage_stats);     //Calculates per month average usage
    vector<int> total_usage = cal_usage(usage_stats);
    vector<float> total_price = cal_bill(usage_stats, total_usage, prices, peak_hour, file_name);

    string all_avg = "";
    string all_hour = "";
    string all_usage = "";
    string all_bill = "";
    for(int i = 0; i < 12; i++){
        all_avg += to_string(avg_usage[i]) + "/"; 
    }
    all_avg.erase(all_avg.size()-1, all_avg.size());
    for(int i = 0; i < 12; i++){
        all_hour += to_string(peak_hour[i]) + "/";
    }
    all_hour.erase(all_hour.size()-1, all_hour.size());
    for(int i = 0; i < 12; i++){
        all_usage += to_string(total_usage[i]) + "/";
    }
    all_usage.erase(all_usage.size()-1, all_usage.size());
    for(int i = 0; i < 12; i++){
        all_bill += to_string(total_price[i]) + "/";
    }
    all_bill.erase(all_bill.size()-1, all_bill.size());




    cout << all_bill + "#" + all_usage + "#" + all_hour + "#" + all_avg; 
}