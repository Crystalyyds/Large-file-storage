#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger
{
private:
    Logger();
    ~Logger();
    std::ofstream logFile;
    std::mutex mtx;

public:
    static Logger &getInstance();
    void log(const std::string &message);
};