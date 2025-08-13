#include "DBManager.h"
#include "Logger.h"
#include "file_utils.h"
#include <mysql/mysql.h>
#include <sstream>
#include <cstring>

DBManager::DBManager(std::shared_ptr<DBConnectionPool> pool)
    : pool(std::move(pool))
{
}

bool DBManager::createDatabaseAndTable()
{
    auto conn = pool->get();
    if (!conn)
        return false;

    MYSQL *mysql = conn->getConnetion();

    const char *sql_create_db = "CREATE DATABASE IF NOT EXISTS media_db CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;";

    if (mysql_query(mysql, sql_create_db))
    {
        Logger::getInstance().log("Create DB failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    const char *sql_use_db = "USE media_db;";
    if (mysql_query(mysql, sql_use_db))
    {
        Logger::getInstance().log("Use DB failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    const char *sql_create_table_chunks =
        "CREATE TABLE IF NOT EXISTS video_chunks ("
        "video_id BIGINT NOT NULL,"
        "chunk_index INT NOT NULL,"
        "chunk_data LONGBLOB NOT NULL,"
        "chunk_size INT NOT NULL,"
        "PRIMARY KEY (video_id, chunk_index)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";

    if (mysql_query(mysql, sql_create_table_chunks))
    {
        Logger::getInstance().log("Create Table video_chunks failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    const char *sql_create_table_videos =
        "CREATE TABLE IF NOT EXISTS videos ("
        "video_id BIGINT PRIMARY KEY,"
        "md5 VARCHAR(32) UNIQUE NOT NULL"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;";

    if (mysql_query(mysql, sql_create_table_videos))
    {
        Logger::getInstance().log("Create Table videos failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    return true;
}

bool DBManager::checkVideoExists(const std::string &md5, int64_t &outVideoId)
{
    auto conn = pool->get();
    if (!conn)
        return false;

    MYSQL *mysql = conn->getConnetion();

    std::string query = "SELECT video_id FROM videos WHERE md5 = '" + md5 + "' LIMIT 1;";

    if (mysql_query(mysql, query.c_str()))
    {
        Logger::getInstance().log("Query video md5 failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    MYSQL_RES *res = mysql_store_result(mysql);
    if (!res)
        return false;

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row)
    {
        outVideoId = std::stoll(row[0]);
        mysql_free_result(res);
        return true;
    }

    mysql_free_result(res);
    return false;
}

bool DBManager::insertChunk(int64_t videoId, int chunkIndex, const std::vector<char> &data)
{
    auto conn = pool->get();
    if (!conn)
        return false;
    MYSQL *mysql = conn->getConnetion();

    MYSQL_STMT *stmt = mysql_stmt_init(mysql);

    if (!stmt)
    {
        Logger::getInstance().log("mysql_stmt_init failed");
        return false;
    }

    const char *sql = "INSERT INTO video_chunks(video_id, chunk_index, chunk_data, chunk_size) VALUES(?,?,?,?);";

    if (mysql_stmt_prepare(stmt, sql, strlen(sql)))
    {
        Logger::getInstance().log("mysql_stmt_prepare failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return false;
    }

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    int64_t vid = videoId;
    int cindex = chunkIndex;
    unsigned long chunk_len = static_cast<unsigned long>(data.size());
    const void *chunk_ptr = data.data();

    // video_id
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].buffer = (void *)&vid;
    bind[0].is_null = 0;
    bind[0].length = 0;

    // chunk_index
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = (void *)&cindex;
    bind[1].is_null = 0;
    bind[1].length = 0;

    // chunk_data
    bind[2].buffer_type = MYSQL_TYPE_BLOB;
    bind[2].buffer = (void *)chunk_ptr;
    bind[2].buffer_length = chunk_len;
    bind[2].is_null = 0;
    bind[2].length = &chunk_len;

    // chunk_size
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = (void *)&chunk_len;
    bind[3].is_null = 0;
    bind[3].length = 0;

    if (mysql_stmt_bind_param(stmt, bind))
    {
        Logger::getInstance().log("mysql_stmt_bind_param failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return false;
    }

    if (mysql_stmt_execute(stmt))
    {
        Logger::getInstance().log("mysql_stmt_execute failed: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        return false;
    }

    mysql_stmt_close(stmt);
    return true;
}

bool DBManager::getChunks(int64_t videoId, std::vector<FileChunk>& outChunks) {
    auto conn = pool->get();
    if (!conn) return false;
    MYSQL* mysql = conn->getConnetion();

    std::string query = "SELECT chunk_index, chunk_data, chunk_size FROM video_chunks WHERE video_id = " + std::to_string(videoId) + " ORDER BY chunk_index ASC;";
    if (mysql_query(mysql, query.c_str())) {
        Logger::getInstance().log("Query chunks failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res) return false;

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        unsigned long* lengths = mysql_fetch_lengths(res);
        int idx = std::stoi(row[0]);
        size_t size = lengths[1];

        FileChunk chunk;
        chunk.index = idx;
        chunk.size = size;
        chunk.data.resize(size);
        memcpy(chunk.data.data(), row[1], size);

        outChunks.push_back(std::move(chunk));
    }
    mysql_free_result(res);
    return true;
}

bool DBManager::deleteVideo(int64_t videoId) {
    auto conn = pool->get();
    if (!conn) return false;
    MYSQL* mysql = conn->getConnetion();

    // 先删除 chunks
    std::string sql1 = "DELETE FROM video_chunks WHERE video_id = " + std::to_string(videoId) + ";";
    if (mysql_query(mysql, sql1.c_str())) {
        Logger::getInstance().log("Delete chunks failed: " + std::string(mysql_error(mysql)));
        return false;
    }

    // 删除视频记录
    std::string sql2 = "DELETE FROM videos WHERE video_id = " + std::to_string(videoId) + ";";
    if (mysql_query(mysql, sql2.c_str())) {
        Logger::getInstance().log("Delete video failed: " + std::string(mysql_error(mysql)));
        return false;
    }
    return true;
}
