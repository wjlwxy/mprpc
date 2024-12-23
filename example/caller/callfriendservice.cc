#include <iostream>
#include "mprpcapplication.h"
#include "friend.pb.h"
#include "mprpcchannel.h"

int main(int argc, char **argv)
{
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法Getfriendlist
    fixbug::FriendsServiceRpc_Stub stub(new MprpcChannel());
    // rpc方法的请求参数
    fixbug::GetfriendslistRequest request;
    request.set_userid(1000);
    // rpc方法的响应
    fixbug::GetfriendslistResponse response;
    // 发起rpc方法的调用  同步的rpc调用过程 MprpcChannel::callmethod
    stub.Getfriendlist(nullptr, &request, &response, nullptr); // RpcChannel->RpcChannel::callMethod 集中来做所有rpc方法调用的参数序列化和网络发送

    // 一次rpc调用完成，读调用的结果
    if (0 == response.result().errcode())
    {
        std::cout << " rpc Getfriendlist response success: " << std::endl;
        for (int i = 0; i < response.friends_size(); i++)
        {
            std::cout << "friend " << i << ": " << response.friends(i) << std::endl;
        }
    }
    else
    {
        std::cout << "rpc Getfriendlist response error: " << response.result().errmsg() << std::endl;
        std::cout << " rpc Getfriendlist response errcode: " << response.result().errcode() << std::endl; // 必须在response被定义为fixbug::GetfriendlistResponse类型的时候才能使用，其他情况就算是多态也不行
    }

    return 0;
}