#include <string>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "mprpcapplication.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "logger.h"

// 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();    // service_name
    std::string method_name = method->name(); // method_name

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string args_str;
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
    send_rpc_str.insert(0, std::string((char *)&header_size, 4)); // 以二进制的方式将header_size存入send_rpc_str的前4个字节
    send_rpc_str += rpc_header_str;                               // rpcheader
    send_rpc_str += args_str;                                     // args

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

    // 连接rpc服务端
    std::string rpc_server_ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");                // 从配置文件中获取rpcserverip
    uint16_t rpc_server_port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str()); // 从配置文件中获取rpcserverport
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
    int recv_len = recv(clientfd, recv_buf, 1024, 0);
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
