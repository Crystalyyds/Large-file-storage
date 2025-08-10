#include<string>
#include<iomanip>
#include"Logger.h"
#include<sstream>


Logger::Logger(){
    logFile.open("log.txt",std::ios::app);
}

Logger::~Logger(){
    if(logFile.is_open()){
        logFile.close();
    }
}

Logger& Logger::getInstance(){
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message){
    std::lock_guard<std::mutex> lock(mtx);
    std::time_t now = std::time(nullptr);
    std::tm* localtime = std::localtime(&now);

    std::ostringstream oss;
    oss<<"["<<std::put_time(localtime,"%Y-%m-%d %H:%M:%S")<<"]"<<message<<"\n";
    logFile<<oss.str();
    logFile.flush();
}
