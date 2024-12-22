#include "test.pb.h"
#include <iostream>
#include <string>
using namespace fixbug;

int main()
{
    GetFriendListsResponse rsp;
    ResultCode *rc = rsp.mutable_result(); // 如果message中的成员也是一个message类型，那么需要使用mutable方法获取其指针再使用
    rc->set_errcode(1);
    rc->set_errmsg("获取好友列表失败！");

    User *user1 = rsp.add_friend_list(); // friend_list是一个列表，要修改列表中的内容，必须依靠add创建一个指针对象
    user1->set_name("chen weili");
    user1->set_age(24);
    user1->set_sex(User::WOMAN);

    std::cout << rsp.friend_list_size() << std::endl;

    User *user2 = rsp.add_friend_list();
    user2->set_name("zhou jia");
    user2->set_age(23);
    user2->set_sex(User::WOMAN);

    std::cout << rsp.friend_list_size() << std::endl;

    User user3 = rsp.friend_list(1); // 传入下标，获取列表中相应索引的对象

    std::cout << user3.name().c_str() << std::endl;

    return 0;
}

// int main1()
// {
//     // 封装了login请求对象的数据
//     LoginRequest req;
//     req.set_name("zhang san");
//     req.set_pwd("123456");

//     std::string send_str;
//     // 对象数据序列化 =》 char*
//     if (req.SerializeToString(&send_str))
//     {
//         std::cout << send_str.c_str() << std::endl;
//     }

//     // 从send_str反序列化一个login请求对象
//     LoginRequest reqB;
//     if (reqB.ParseFromString(send_str))
//     {
//         std::cout << reqB.name() << std::endl;
//         std::cout << req.pwd() << std::endl;
//     }

//     return 0;
// }