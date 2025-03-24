#include "mprpcapplication.h"
#include <iostream>
#include <unistd.h>

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp()
{
    std::cout<< "format: command -i <configfile>" <<std::endl;
}

void MprpcApplication::Init(int argc, char **argv)
{
    if (argc < 2)
    {
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c =0;
    std::string config_file;
    /*
    argc: main 函数的参数个数

    argv: main 函数的参数字符串数组

    optstring: 选项定义字符串，格式规则：

    单字符：普通选项（如 a 对应 -a）

    字符后加冒号 :：选项需要参数（如 a: 表示 -a value）

    字符后加双冒号 ::：选项参数可选（GNU 扩展，如 a::）
    */
    while((c = getopt(argc, argv, "i:")) != -1) // -1 表示所有命令行选项已经被解析 ，getopt用于查找命令
    {
        switch (c)
        {
        case 'i': // 表示出现了 -i
            config_file = optarg; // optarg: 如果某个选项有参数，这包含当前选项的参数字符串
            break;
        case '?': // 表示出现了无效的命令
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':  // 表示出现了-i ，但缺失了参数
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    // 开始加载配置文件了  rpcserver_ip=  rpcserver_port=  zookeeper_ip=  zookeeper_port=
    m_config.LoadConfigFile(config_file.c_str());

    std::cout << "rpcserverip:" << m_config.Load("rpcserverip") <<std::endl;
    std::cout << "rpcserverport:" << m_config.Load("rpcserverport") <<std::endl;
    std::cout << "zookeeperip:" << m_config.Load("zookeeperip") <<std::endl;
    std::cout << "zookeeperport:" << m_config.Load("zookeeperport") <<std::endl;
}

// 单例模式
MprpcApplication& MprpcApplication::GetInstance()  // 若函数的返回值为引用(&)，则编译器就不为返回值创建临时变量了。直接返回那个变量的引用。
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig()
{
    return m_config;
}