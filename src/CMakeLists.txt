aux_source_directory(. SRC_LIST) # 当前目录下的所有源文件放入SRC_LIST
add_library(mprpc ${SRC_LIST}) # 生成名为mprpc的动态库
target_link_libraries(mprpc muduo_net muduo_base pthread) # muduo_base需要依赖muduo_net(net是核心)，所以muduo_net需要在muduo_base前面