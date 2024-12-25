#pragma once
#include <google/protobuf/service.h>
#include <string>

class MprpcController : public google::protobuf::RpcController
{
public:
    MprpcController();
    ~MprpcController();

    // 以下是RpcController接口的方法
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;
    void StartCancel();
    void SetFailed(const std::string &reason);
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure *callback);

private:
    bool m_failed;           // 标识rpc调用是否失败
    std::string m_errorText; // rpc调用失败的错误信息
};