#pragma once
#include "rpcheader.pb.h"
#include <google/protobuf/service.h>

// 继承自google::protobuf::RpcChannel而不是FriendsService，所以这个 CallMethod 函数与 FriendsService的CallMethod 函数不冲突
class MprpcChannel : public google::protobuf::RpcChannel  
{
public:
    // 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据序列化和网络发送
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done);
};