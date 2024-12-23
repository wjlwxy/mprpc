
# set(SRC_LIST userservice.cc ../user.pb.cc)
set(SRC_LIST friendservice.cc ../friend.pb.cc)

add_executable(provider ${SRC_LIST})

target_link_libraries(provider mprpc protobuf)  # provider为目标，mprpc和protobuf为需要链接的库

# # 指定可执行文件需要依赖的库文件
# target_link_libraries(protobuf)