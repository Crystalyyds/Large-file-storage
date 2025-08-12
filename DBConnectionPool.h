#pragma once
#include "DBConnection.h"
#include "Logger.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

class DBConnectionPool
{
private:
    DBConnectionPool() = default;
    ~DBConnectionPool();

    std::vector<DBConnection *> conns;
    std::queue<int> free_conn;
    std::mutex mtx;
    std::condition_variable cond;

    std::string host, user, password, db;
    unsigned int port{0};

    int minSize{5};
    int maxSize{10};
    int currentSize{0};

public:
    static DBConnectionPool &getInstance();
    void init(const std::string &host,
              const std::string &user,
              const std::string &password,
              const std::string &db,
              unsigned int port,
              int minConn, int maxConn);
    std::shared_ptr<DBConnection> get();
};