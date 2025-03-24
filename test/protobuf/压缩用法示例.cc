#include <zlib.h>
#include <google/protobuf/message.h>

// 序列化 + 压缩
std::string SerializeWithCompression(const google::protobuf::Message &message)
{
    std::string serialized;
    message.SerializeToString(&serialized);

    // 使用 zlib 压缩
    uLongf dest_len = compressBound(serialized.size());
    std::string compressed(dest_len, 0);
    compress2(
        (Bytef *)compressed.data(), &dest_len,
        (const Bytef *)serialized.data(), serialized.size(),
        Z_BEST_SPEED // 平衡速度与压缩率
    );
    compressed.resize(dest_len);
    return compressed;
}

// 解压 + 反序列化
bool ParseFromCompressed(const std::string &compressed, google::protobuf::Message *message)
{
    std::string serialized;
    uLongf dest_len = /* 已知原始数据大小或通过协议传递 */;
    serialized.resize(dest_len);
    uncompress(
        (Bytef *)serialized.data(), &dest_len,
        (const Bytef *)compressed.data(), compressed.size());
    return message->ParseFromString(serialized);
}