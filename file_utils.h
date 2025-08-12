#pragma once
#include<string>
#include<vector>


struct FileChunk{
    std::vector<char> data;
    size_t size;
    int index;
};


std::vector<FileChunk> sliceFile(const std::string& filePath,size_t chunk_size);