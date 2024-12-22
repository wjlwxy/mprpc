#pragma once
#include "mprpcconfig.h"

// mprpc框架的基础类  负责框架的初始化操作
class MprpcApplication
{
public:
    static void Init(int argc, char **argv);
    static MprpcApplication& GetInstance();
    static MprpcConfig& GetConfig();
private:
    static MprpcConfig m_config;
    // 构造函数
    MprpcApplication(){}

    // 删除拷贝构造函数
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;
    // 删除拷贝复制运算符
    MprpcApplication &operator=(const MprpcApplication&) = delete;
};