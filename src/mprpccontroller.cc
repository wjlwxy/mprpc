#include "mprpccontroller.h"

MprpcController::MprpcController()
    : m_failed(false), m_errorText("")
{
}

MprpcController::~MprpcController()
{
}

void MprpcController::Reset()
{
    m_failed = false;
    m_errorText = "";
}

bool MprpcController::Failed() const
{
    return m_failed;
}

std::string MprpcController::ErrorText() const
{
    return m_errorText;
}

void MprpcController::StartCancel()
{
}

void MprpcController::SetFailed(const std::string &reason)
{
    m_failed = true;
    m_errorText = reason;
}

bool MprpcController::IsCanceled() const
{
    return false;
}

void MprpcController::NotifyOnCancel(google::protobuf::Closure *callback)
{
}