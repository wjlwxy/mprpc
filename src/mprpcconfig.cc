#include "mprpcconfig.h"
#include <string>
#include <iostream>

void MprpcConfig::Trim(std::string &src_buf)
{
    // 去掉字符串前面多余的空格
    int idx = src_buf.find_first_not_of(' ');
    if (idx != -1)
    {
        // 说明字符串前面有空格
        src_buf = src_buf.substr(idx, src_buf.size() - idx);
    }
    idx = src_buf.find_last_not_of(' ');
    if (idx != -1)
    {
        // 说明字符串后面有空格
        src_buf = src_buf.substr(0, idx + 1);
    }
}

// 负责解析加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if (nullptr == pf)
    {
        std::cout << config_file << "is not exist!" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 1.注释  2.正确的配置项 =  3.去掉开头的空余的空格
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf); // 读取一行，最多512个字节

        std::string read_buf(buf);
        Trim(read_buf);

        // 判断 # 的注释
        if (read_buf[0] == '#' || read_buf.empty())
        {
            continue;
        }

        // 解析配置项
        int idx = read_buf.find('=');
        if (idx == -1)
        {
            // 配置项不合法
            continue;
        }

        std::string key;
        std::string value;
        key = read_buf.substr(0, idx);
        Trim(key);
        int endidx = read_buf.find('\n', idx);
        value = read_buf.substr(idx + 1, endidx - 1 - idx);
        Trim(value);
        // 这个对象在构造时会自动上锁，当lock_guard对象超出作用域时会自动释放锁。这样可以避免手动解锁时的错误。
        std::lock_guard<std::mutex> lock(mtx);
        m_configMap.insert({key, value}); // 这个地方线程不一定安全，每次服务端启动的时候都会插入，如果两个服务器同时那么就可能出现线程安全问题
    }
}
// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key)
{
    auto it = m_configMap.find(key); // 迭代器
    if (it == m_configMap.end())
    {
        return "";
    }
    return it->second;
}