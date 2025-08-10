#pragma once
#include <mysql/mysql.h>
#include <string>

class DBConnection
{
private:
    MYSQL *conn;

public:
    DBConnection();
    ~DBConnection();

    bool connect(const std::string &host, const std::string &user,
                 const std::string &password, const std::string &db, unsigned int port);

    MYSQL *getConnetion();
};