#include <iostream>
#include "mprpcapplication.h"
#include "user.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Login
    fixbug::UserServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;
    // 发起rpc方法的调用  同步的rpc调用过程 MprpcChannel::callmethod
    MprpcController controller;
    stub.Login(&controller, &request, &response, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次rpc调用完成，读调用的结果
    if (controller.Failed())
    {
        std::cout << "rpc login response error: " << controller.ErrorText() << std::endl;
        return 1;
    }
    else
    {
        if (0 == response.result().errcode())
        {
            LOG_INFO(" rpc login response success: %d", response.success());
            // std::cout << " rpc login response success: " << response.success() << std::endl;
            // std::cout << " rpc login response errcode: " << response.result().errcode() << std::endl;
        }
        else
        {
            LOG_ERROR("rpc login response error: %s", response.result().errmsg().c_str());
            LOG_ERROR(" rpc login response errcode: %d", response.result().errcode());
            // std::cout << "rpc login response error: " << response.result().errmsg() << std::endl;
            // std::cout << " rpc login response errcode: " << response.result().errcode() << std::endl; // 必须在response被定义为fixbug::LoginResponse类型的时候才能使用，其他情况就算是多态也不行
        }
    }

    // 演示调用远程发布的rpc方法Register
    fixbug::UserServiceRpc_Stub stub2(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::RegisterRequest request2;
    request2.set_id(11);
    request2.set_name("li si");
    request2.set_pwd("091113");
    // rpc方法的响应
    fixbug::RegisterResponse response2;
    // 发起rpc方法的调用  同步的rpc调用过程 MprpcChannel::callmethod
    stub2.Register(nullptr, &request2, &response2, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次rpc调用完成，读调用的结果
    if (0 == response2.result().errcode())
    {
        LOG_INFO(" rpc register response success: %d", response2.success());
        // std::cout << " rpc register response success: " << response2.success() << std::endl;
        // std::cout << " rpc register response errcode: " << response2.result().errcode() << std::endl;
    }
    else
    {
        LOG_ERROR("rpc register response error: %s", response2.result().errmsg().c_str());
        LOG_ERROR(" rpc register response errcode: %d", response2.result().errcode());
        // std::cout << "rpc register response error: " << response2.result().errmsg() << std::endl;
        // std::cout << " rpc register response errcode: " << response2.result().errcode() << std::endl; // 必须在response被定义为fixbug::RegisterResponse类型的时候才能使用，其他情况就算是多态也不行
    }

    return 0;
}