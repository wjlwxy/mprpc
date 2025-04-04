#include "mprpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"
#include "mprpccontroller.h"
#include <unistd.h>

/*
service_name =>  service服务描述
                        => service* 记录服务对象
                        method => method方法对象
                        method_name => method方法对象名
*/

// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口    ::PROTOBUF_NAMESPACE_ID::Service是所有service的基类
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
    ServiceInfo service_info; // 头文件中定义的一个结构体，用于记录服务对象和方法的映射关系

    // 获取了服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务对象service的方法的数量
    int methodCnt = pserviceDesc->method_count();

    LOG_INFO("service_name:%s methodCnt:%d", service_name.c_str(), methodCnt);

    for (int i = 0; i < methodCnt; i++)
    {
        // 获取了服务对象指定下标的服务方法的描述 （抽象描述）
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i); // 其实就是方法
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        LOG_INFO("method_name:%s", method_name.c_str());
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
    // std::string portt = MprpcApplication::GetInstance().GetConfig().Load("rpcserverport");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
    // std::cout<< portt <<std::endl;
    // std::cout<<  port << std::endl;
    muduo::net::InetAddress address(ip, port);

    // 创建TcpServer对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 绑定连接回调和消息读写回调方法  分离了网络代码和业务代码
    server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 把当前rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    // session timeout 30s zkclient 网络I/O线程  1/3 * timeout时间发送ping消息
    ZkClient zkCli;
    zkCli.Start();
    // service_name为永久性节点  method_name为临时性节点
    for (auto &sp : m_serviceMap)
    {
        /// service_name 如/UserServiceRpc 或者/FriendsServiceRpc
        std::string service_path = "/" + sp.first;
        zkCli.Create(service_path.c_str(), nullptr, 0); // 第四个参数有默认值0，为永久性节点
        for (auto &mp : sp.second.m_methodMap)
        {
            // /service_name/method_name   如/UserServiceRpc/Login 或者/FriendsServiceRpc/GetFriendList  用于存储当前这个rpc方法服务节点主机的ip和port
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示znode是一个临时性节点
            zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    // 将当前rpc节点上要发布的服务全部注册到muduo的TcpServer服务中
    LOG_INFO("RpcProvider::Run server start at ip:%s port:%d", ip.c_str(), port);
    // std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 和 rpc client的连接断开了，于是关闭连接
        conn->shutdown();
    }
}

/*
在框架内部，EpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
 定义proto的message类型，进行数据头的序列化和反序列化
*/
// 已建立连接用户的读写事件回调  如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                            muduo::net::Buffer *buffer,
                            muduo::Timestamp)
{
    // 网络上接收的远程rpc调用请求的字符流  Login args
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    /*
    size_t copy(char* dest, size_t len, size_t pos = 0) const;
    参数
    dest: 指向目标字符数组的指针，该数组必须足够大以容纳复制的字符。

    len: 要复制的字符的最大数量。

    pos: 从 std::string 中的哪一个位置开始复制，默认值为 0（即从字符串的开头开始复制）。
    */
    recv_buf.copy((char *)&header_size, 4, 0); // 为什么不在调用的时候将header_size一并序列化进去？

    // 从字符流中读出服务名和方法名的字符流，并反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if (rpcHeader.ParseFromString(rpc_header_str))
    {
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size(); // 这里的args_size是指rpc方法的参数的长度，不是指rpc方法的参数个数
    }
    else
    {
        // 数据头反序列化失败
        LOG_ERROR("rpc_header_str:%s parse error!", rpc_header_str.c_str());
        // std::cout << "rpc_header_str:" << rpc_header_str << " parse error!" << std::endl;
        return;
    }

    // 获取rpc方法参数的字符流数据  header_size + 4 跳过前4个字节的内容，header_size是指rpc_header_str的长度，4是指rpc_args_str的长度
    std::string rpc_args_str = recv_buf.substr(header_size + 4, args_size);

    // 获取controller对象指针，用于rpc方法的调用
    // MprpcController *controller;
    MprpcController *controller;
    recv_buf.copy((char *)&controller, 4, 4 + header_size + args_size); // 写入到&controller指针的内存地址,即给controller赋值

    // 打印调试信息
    std::cout << "=======================================" << std::endl;
    std::cout << "header_size:" << header_size << std::endl;
    std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "rpc_args_str: " << rpc_args_str << std::endl;
    std::cout << "=======================================" << std::endl;

    // 获取service对象和方法
    auto it = m_serviceMap.find(service_name);
    if (it == m_serviceMap.end())
    {
        LOG_ERROR("%s is not exist!", service_name.c_str());
        // std::cout << service_name << " is not exist!" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if (mit == it->second.m_methodMap.end())
    {
        LOG_ERROR("%s:%s is not exist!", service_name.c_str(), method_name.c_str());
        // std::cout << service_name << ":" << method_name << " is not exist!" << std::endl;
        return;
    }
    google::protobuf::Service *service = it->second.m_service;      // 获取service对象，如 UserService
    const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象 如 Login

    // 生成rpc方法调用的请求request和响应response参数
    /*
    service->GetRequestPrototype(method)：通过 service 对象和 method 描述符，
    可以获取该方法的请求消息类型（即输入参数类型的原型）。
    具体地，GetRequestPrototype(method) 返回一个 Message 对象的原型，这个原型表示该方法的请求消息类型。

    .New()：在原型上调用 New() 方法，动态地创建一个新的消息对象（即请求对象）。
    这个请求对象的类型是 google::protobuf::Message，它是所有 Protobuf 消息类的基类，
    具体的请求类型会根据 method 中定义的请求消息类型来确定。
    */
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if (!request->ParseFromString(rpc_args_str)) // 反序列化rpc_args_str到request
    {
        LOG_ERROR("request parse error!");
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();
    // std::cout << "errcode:" << response->result().errcode() << std::endl; // 调试代码，response可能存在问题，并未具备fixbug::LoginResponse的属性

    // 给下面的method方法的调用，绑定一个Closure的回调函数  
    // 参数前两个指代RpcProvider的成员函数， 后面两个是此成员函数的参数  <>里的RpcProvider指代要绑定的函数是RpcProvider的成员函数
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider,
            const muduo::net::TcpConnectionPtr &, google::protobuf::Message *>(this, &RpcProvider::SendRpcResponse, conn, response); // 生成一个Closure* done对象

    // 在框架上根据远端rpc请求， 调用当前rpc节点上发布的方法
    // new UserService().Login()(controller, request, response, done)
    service->CallMethod(method, controller, request, response, done); // mprpc/example/friend.pb.cc的806行
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response) // done->run()就是运行这个函数
{
    std::string response_str;
    if (response->SerializeToString(&response_str)) // response进行序列化
    {
        // 序列化成功后，通过网络把rpc方法执行的结果发送给rpc的调用方
        conn->send(response_str);
        // std::cout << "errcode:" << response->result().errcode() << std::endl;
        // std::cout << "response_str: " << response_str << std::endl;
    }
    else
    {
        LOG_ERROR("serialize response_str error!");
        // std::cout << "serialize response_str error!" << std::endl;
    }
    conn->shutdown(); // 模拟http的短链接服务，有rpcprovider主动断开连接
}