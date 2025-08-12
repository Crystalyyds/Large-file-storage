#include "DBConnection.h"
#include "Logger.h"

DBConnection::DBConnection()
{
    conn = mysql_init(nullptr);
}

DBConnection::~DBConnection()
{
    if (conn)
    {
        mysql_close(conn);
    }
}

bool DBConnection::connect(const std::string &host, const std::string &user,
                           const std::string &password, const std::string &db, unsigned int port)
{
    conn = mysql_init(nullptr);
    if (!conn)
    {
        Logger::getInstance().log("mysql_init failed.");
        return false;
    }

    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(), db.c_str(), port, nullptr, 0))
    {
        Logger::getInstance().log("MySQL connection error: " + std::string(mysql_error(conn)));
        return false;
    }

    return true;
}


MYSQL* DBConnection::getConnetion(){
    return conn;
}