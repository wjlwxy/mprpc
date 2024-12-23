#include <iostream>
#include <string>
#include "friend.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include <vector>

class FriendService : public fixbug::FriendsServiceRpc
{
public:
    std::vector<std::string> Getfriendlist(uint32_t user_id)
    {
        std::cout << "doing local service: GetFriendLists" << std::endl;
        std::cout << "user_id:" << user_id << std::endl;

        std::vector<std::string> vec;
        vec.push_back("zhang san");
        vec.push_back("li si");
        vec.push_back("wang wu");

        return vec;
    }

    // 重写基类FriendsServiceRpc的虚函数 下面这些方法都是框架直接调用的
    void Getfriendlist(::google::protobuf::RpcController *controller,
                       const ::fixbug::GetfriendslistRequest *request,
                       ::fixbug::GetfriendslistResponse *response,
                       ::google::protobuf::Closure *done)
    {
        // 框架给业务上报了请求参数GetFriendListRequest, 应用获取相应数据做本地业务
        uint32_t user_id = request->userid();

        // 做本地业务-------------------------------------
        std::vector<std::string> friendsList = Getfriendlist(user_id);

        // 把响应写入
        for (std::string &name : friendsList)
        {
            response->add_friends(name);
        }

        // 或者
        // for (std::vector<std::string>::iterator it = friendsList.begin(); it != friendsList.end(); it++)
        // {
        //     response->add_friends(*it);
        // }

        fixbug::ResultCode *code = response->mutable_result();
        code->set_errmsg("");
        code->set_errcode(0);

        // 业务处理完成，执行回调操作
        done->Run();
    }
};

int main(int argc, char **argv)
{
    // 整个程序启动以后，想使用mprpc框架来享受rpc服务调用，一定需要先调用框架的初始化函数（只初始化一次）
    MprpcApplication::Init(argc, argv);

    // 演示调用远程发布的rpc方法GetFriendLists
    FriendService service;
    RpcProvider provider;
    provider.NotifyService(&service);

    // 启动一个rpc服务发布节点
    provider.Run();

    return 0;
}