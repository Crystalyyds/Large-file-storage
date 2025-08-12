#include "file_utils.h"
#include <fstream>
#include <stdexcept>
#include "Logger.h"

std::vector<FileChunk> sliceFile(const std::string &filePath, size_t chunk_size)
{
    std::ifstream in(filePath, std::ios::binary);
    if (!in)
    {
        Logger::getInstance().log("Cannot open file: " + filePath);
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    std::vector<FileChunk> chunks;
    int index = 0;

    while (!in.eof())
    {
        FileChunk chunk;
        chunk.data.resize(chunk_size);
        in.read(chunk.data.data(), chunk_size);
        std::streamsize readSize = in.gcount();
        if (readSize <= 0)
            break;

        chunk.size = static_cast<size_t>(readSize);
        chunk.index = index++;
        chunk.data.resize(chunk.size);

        chunks.push_back(std::move(chunk));
    }

    return chunks;
}