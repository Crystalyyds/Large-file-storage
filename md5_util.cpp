#include "md5_util.h"
#include <openssl/md5.h>
#include <fstream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>

std::string file_md5_hex(const std::string &filename)
{
    // 第一步：准备缓冲区和打开文件
    // 定义一个 4MB 大小的缓冲区 用来分块读取文件。
    // 以二进制模式打开目标文件。
    // 如果打不开文件，直接抛异常。
    constexpr size_t BUF_SIZE = 4 * 1024 * 1024;
    std::vector<char> buffer(BUF_SIZE);

    std::ifstream in(filename, std::ios::binary);
    if (!in)
    {
        throw std::runtime_error("cannot open file for MD5 " + filename);
    }

    // 初始化 MD5 计算上下文
    // 创建一个 MD5_CTX 结构体来保存 MD5 的计算状态。
    // MD5_Init 把它初始化为“空哈希”的状态，准备接收数据
    MD5_CTX ctx;
    MD5_Init(&ctx);

    // 第三步：分块读取并更新哈希
    // 循环读取文件，每次最多读取 4MB。
    // in.gcount() 获取实际读到的字节数。
    // 每读一块，就用 MD5_Update 把这块数据加入 MD5 计算。

    while (in)
    {
        in.read(buffer.data(), (std::streamsize)buffer.size());
        std::streamsize s = in.gcount();
        if (s > 0)
        {
            MD5_Update(&ctx, buffer.data(), s);
        }
    }

    // 第四步：生成最终的 MD5 值
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5_Final(digest, &ctx);

    // 转成 32 个十六进制字符的小写字符串
    // 把每个字节转成两位小写十六进制（不足两位补 0）。
    // 拼接成一个 32 字符的字符串返回。

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        oss << std::setw(2) << static_cast<int>(digest[i]);
    }

    return oss.str();
}