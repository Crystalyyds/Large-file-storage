#include "DBConnectionPool.h"
#include "Logger.h"

DBConnectionPool &DBConnectionPool::getInstance()
{
    static DBConnectionPool instance;
    return instance;
}

void DBConnectionPool::init(const std::string &h, const std::string &u,
                            const std::string &pw, const std::string &d,
                            unsigned int p, int minConn, int maxConn)
{
    host = h,
    user = u;
    password = pw;
    db = d;
    port = p;
    minSize = minConn;
    maxSize = maxConn;

    Logger::getInstance().log("Connection pool initialization started");

    for (int i = 0; i <= minSize; i++)
    {
        DBConnection *conn = new DBConnection();
        if (!conn->connect(host, user, password, db, port))
        {
            Logger::getInstance().log("Failed to create connection.");
            delete (conn);
        }
        else
        {
            free_conn.push(i);
            conns.push_back(conn);
            currentSize += 1;
            Logger::getInstance().log("Successfully created connection " + std::to_string(i + 1));
        }
    }
}

std::shared_ptr<DBConnection> DBConnectionPool::get()
{
    std::unique_lock<std::mutex> lock(mtx);
    Logger::getInstance().log("Thread requested a connection...");

    while (free_conn.empty())
    {
        if (currentSize < maxSize)
        {
            DBConnection *conn = new DBConnection();
            if (conn->connect(host, user, password, db, port))
            {

                int index = conns.size();
                conns.push_back(conn);
                currentSize += 1;
                Logger::getInstance().log("Lazily created new connection, total connections: " + std::to_string(currentSize));
                return std::shared_ptr<DBConnection>(conn, [this, index](DBConnection *ptr)
                                                     {
                    std::lock_guard<std::mutex> lock(mtx);
                    free_conn.push(index);
                    Logger::getInstance().log("Connection returned, current pool size: " + std::to_string(conns.size()));
                    cond.notify_one(); });
            }
            else
            {
                Logger::getInstance().log("Failed to lazily create connection.");
                delete conn;
            }
        }
        else
        {
            Logger::getInstance().log("Waiting for a connection to become available...");
            cond.wait(lock);
        }
    }
    int index = free_conn.front();
    DBConnection *conn = conns[index];

    free_conn.pop();
    Logger::getInstance().log("Successfully retrieved connection, remaining pool size: " + std::to_string(free_conn.size()));

    auto delete_conn = [this, index](DBConnection *ptr)
    {
        std::lock_guard<std::mutex> lock(mtx);
        free_conn.push(index);
        Logger::getInstance().log("Connection returned, current pool size: " + std::to_string(conns.size()));
        cond.notify_one();
    };

    return std::shared_ptr<DBConnection>(conn, delete_conn);
}

DBConnectionPool::~DBConnectionPool()
{
    for (auto conn : conns)
    {
        delete (conn);
    }
    conns.clear();
}