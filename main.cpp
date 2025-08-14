#include <iostream>
#include "DBConnectionPool.h"
#include "DBManager.h"
#include "md5_util.h"
#include "file_utils.h"
#include "Logger.h"
#include <thread>
#include "config.h"

int main()
{
    auto &pool = DBConnectionPool::getInstance();
    pool.init(DB_HOST, DB_USER, DB_PASSWORD, DB_NAME, DB_PORT, 10, 20);

    DBManager dbManager(std::make_shared<DBConnectionPool>(pool));

    if (!dbManager.createDatabaseAndTable())
    {
        std::cerr << "database and table faild create" << std::endl;
        Logger::getInstance().log("database and table faild create");
        return -1;
    }

    std::string filepath = "test_video.mp4";

    std::string md5;

    try
    {
        md5 = file_md5_hex(filepath);
    }
    catch (const std::exception &e)
    {
        std::cerr << " build MD5 faild: " << e.what() << std::endl;
        Logger::getInstance().log("build MD5 faild");
        return -1;
    }

    int64_t videoId = 0;
    if (dbManager.checkVideoExists(md5, videoId))
    {
        std::cout << "vedio has exit , ID=" << videoId << std::endl;
        Logger::getInstance().log("edio has exit , ID=" + videoId);
        return 0;
    }
    else
    {
        videoId = static_cast<int64_t>(time(nullptr));
        if (!dbManager.insertVideoRecord(videoId, md5))
        {
            std::cerr << "insert failed" << std::endl;
            Logger::getInstance().log("insert failed");
            return -1;
        }
    }

    size_t chunkSize = 1024 * 1024 * 4; // 4MB
    std::vector<FileChunk> chunks;
    try
    {
        chunks = sliceFile(filepath, chunkSize);
    }
    catch (const std::exception &e)
    {

        std::cerr << "chunk failed: " << e.what() << std::endl;
        return -1;
    }

    for (const auto &chunk : chunks)
    {
        if (!dbManager.insertChunk(videoId, chunk.index, chunk.data))
        {
            std::cerr << "insert chunk faild , index=" << chunk.index << std::endl;
            Logger::getInstance().log("insert chunk faild ,index=" + chunk.index );
            return -1;
        }
        std::cout << "insert ok " << chunk.index << " success\n";
        Logger::getInstance().log("insert ok");
    }
}
