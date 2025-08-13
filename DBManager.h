#pragma once

#include <string>
#include <vector>
#include <memory>
#include "DBConnectionPool.h"

struct FileChunk;

class DBManager
{
public:
    explicit DBManager(std::shared_ptr<DBConnectionPool> pool);

    bool createDatabaseAndTable();

    bool checkVideoExists(const std::string &md5, int64_t &outVideoId);

    bool insertVideoRecord(int64_t videoId, const std::string &md5);

    bool insertChunk(int64_t videoId, int chunkIndex, const std::vector<char> &data);

    bool getChunks(int64_t videoId, std::vector<FileChunk> &outChunks);

    bool deleteVideo(int64_t videoId);

private:
    std::shared_ptr<DBConnectionPool> pool;
};
