#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "mprpcapplication.h"
#include "mprpcchannel.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "logger.h"
#include "zookeeperutil.h"

/*
void FriendsServiceRpc_Stub::Getfriendlist(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
                              const ::fixbug::GetfriendslistRequest* request,
                              ::fixbug::GetfriendslistResponse* response,
                              ::google::protobuf::Closure* done) {
  channel_->CallMethod(descriptor()->method(0),
                       controller, request, response, done);
}
*/

/*
class GetfriendslistRequest :
    public ::PROTOBUF_NAMESPACE_ID::Message
*/

// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
{
    // method是google::protobuf::MethodDescriptor *类型，sd是google::protobuf::ServiceDescriptor *类型
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();    // service_name
    std::string method_name = method->name(); // method_name

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string args_str;
    // SerializeToString 仅仅进行了序列化，没有额外的压缩，不过序列化本身通过使用Variant编码和标签id而拥有一定的压缩效果（效果也远优于json）
    if (request->SerializeToString(&args_str))
    {
        args_size = args_str.size();
    }
    else
    {
        controller->SetFailed("serialize request error!");
        return;
    }
    // 定义rpc的请求header
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    uint32_t header_size = 0;
    std::string rpc_header_str;
    if (rpcHeader.SerializeToString(&rpc_header_str))
    {
        header_size = rpc_header_str.size();
    }
    else
    {
        controller->SetFailed("serialize rpc header error!");
        return;
    }

    // 组织待发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char *)&header_size, 4));                         // 以二进制的方式将header_size存入send_rpc_str的前4个字节,二进制的目的是可以直接发送到网络上，而且不用反序列化就能读取到数据
    send_rpc_str += rpc_header_str;                                                       // rpcheader，调用的方法名
    send_rpc_str += args_str;                                                             // args，调用方法的参数
    send_rpc_str += std::string((char *)controller);                           // 控制器，用于回调

    // 打印调试信息
    std::cout << "=======================================" << std::endl;
    std::cout << "header_size:" << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "=======================================" << std::endl;

    // 使用tcp编程，完成rpc方法的远程调用  客户端，不需要高并发，不需要使用连接池，但也可以自己使用muduo库实现一下
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        char error[128] = {0};
        sprintf(error, "create socket error! errno:%d", errno); // 将错误信息写入error 函数用于将格式化的数据写入字符串。其用法类似于 printf，但输出目标是字符串而不是标准输出。
        controller->SetFailed(error);
        return;
    }

    /* 连接rpc服务端 */

    // std::string rpc_server_ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");                // 从配置文件中获取rpcserverip
    // uint16_t rpc_server_port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str()); // 从配置文件中获取rpcserverport

    // rpc调用方想调用service_name的method_name服务方法（使用未序列化的），需要查询zk上该服务所在的host信息
    // 这里使用zookeeper来实现服务发现，zk上存储了服务的host信息，客户端根据服务名和方法名查询zk上存储的host信息，然后连接rpc服务端进行rpc调用
    ZkClient zkCli;
    zkCli.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkCli.GetData(method_path.c_str());
    if (host_data == "")
    {
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(':');
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string rpc_server_ip = host_data.substr(0, idx);
    uint16_t rpc_server_port = atoi(host_data.substr(idx + 1, host_data.size() - idx - 1).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(rpc_server_port);
    server_addr.sin_addr.s_addr = inet_addr(rpc_server_ip.c_str());
    if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) // 连接rpc服务端
    {
        char error[128] = {0};
        sprintf(error, "connect error! errno:%d", errno); // 将错误信息写入error 函数用于将格式化的数据写入字符串。其用法类似于 printf，但输出目标是字符串而不是标准输出。
        controller->SetFailed(error);
        close(clientfd);
        return;
    }

    // 发送rpc请求数据
    if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
    {
        char error[128] = {0};
        sprintf(error, "send error! errno:%d", errno); // 将错误信息写入error 函数用于将格式化的数据写入字符串。其用法类似于 printf，但输出目标是字符串而不是标准输出。
        controller->SetFailed(error);
        close(clientfd);
        return;
    }

    // 接收rpc响应数据
    char recv_buf[900] = {0};
    int recv_len = recv(clientfd, recv_buf, 900, 0);
    if (-1 == recv_len)
    {
        char error[128] = {0};
        sprintf(error, "recv error! errno:%d", errno); // 将错误信息写入error 函数用于将格式化的数据写入字符串。其用法类似于 printf，但输出目标是字符串而不是标准输出。
        controller->SetFailed(error);
        close(clientfd);
        return;
    }

    // 通过string反序列化时如果出现\0000这种字符，那么反序列化就会失败，所以这里需要改为数组
    // std::string response_str(recv_buf, recv_len); // 此处等同于response(recv_buf, 0, recv_len)  从recv_buf中取出recv_len长度的字符串并赋值给response_str
    // std::cout << "response_str: " << response_str << std::endl;
    if (response->ParseFromArray(recv_buf, recv_len))
    {
        LOG_INFO("rpc response parse success!");
        // std::cout << "rpc response parse success!" << std::endl;
        // std::cout << "errcode: " << response->result().errcode() << std::endl;
    }
    else
    {
        char error[1024] = {0};
        sprintf(error, "rpc response parse error! response_str:%s", recv_buf); // 将错误信息写入error 函数用于将格式化的数据写入字符串。其用法类似于 printf，但输出目标是字符串而不是标准输出。
        controller->SetFailed(error);
        close(clientfd);
        return;
    }
}
