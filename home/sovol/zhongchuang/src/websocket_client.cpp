#include "../include/websocket_client.h"

// #include "../include/websocket_client.h"
#include "../include/process_messages.h"
#include "../include/MoonrakerAPI.h"
#include "../include/mks_printer.h"

WebSocketClient::WebSocketClient() {
    // 初始化WebSocket客户端
    client_.init_asio();
    //设置日志
    client_.set_access_channels(websocketpp::log::alevel::all);
    client_.clear_access_channels(websocketpp::log::alevel::frame_payload);
    client_.clear_access_channels(websocketpp::log::alevel::frame_header);
    client_.clear_access_channels(websocketpp::log::alevel::control);
    // 绑定回调函数
    client_.set_message_handler(std::bind(&WebSocketClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
    client_.set_open_handler(std::bind(&WebSocketClient::on_open, this, std::placeholders::_1));
    client_.set_fail_handler(std::bind(&WebSocketClient::on_fail, this, std::placeholders::_1));
    client_.set_close_handler(std::bind(&WebSocketClient::on_close, this, std::placeholders::_1));
}

WebSocketClient::~WebSocketClient() {

}

void WebSocketClient::connect(const std::string& uri) {
    uri_ = uri;
    try {
        // 连接到WebSocket服务器
        websocketpp::lib::error_code ec;
        client::connection_ptr con = client_.get_connection(uri_, ec);
        client_.connect(con);
        client_.run();
    } catch (websocketpp::exception const & e) {
        std::cout << "连接失败：" << e.what() << std::endl;
        connected = false;
    }
}

void WebSocketClient::on_open(websocketpp::connection_hdl hdl) {
    // WebSocket连接打开事件
    std::cout << "WebSocket连接已打开" << std::endl;
    connected = true;
    connection_hdl_ = hdl; // 设置连接句柄

    // 在连接成功之后发送消息
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::string message = json_query_printer_object_status(subscribe_objects_status());
    // std::string message = "{\"id\":5434,\"jsonrpc\":\"2.0\",\"method\":\"printer.objects.query\",\"params\":{\"objects\":" + subscribe_objects_status() + "}}";
    send_message(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    message = json_subscribe_to_printer_object_status(subscribe_objects_status());
    // message = "{\"id\":5434,\"jsonrpc\":\"2.0\",\"method\":\"printer.objects.subscribe\",\"params\":{\"objects\":" + subscribe_objects_status() + "}}";
    send_message(message);
}

void WebSocketClient::on_fail(websocketpp::connection_hdl hdl) {
    // WebSocket连接失败事件
    std::cout << "WebSocket连接失败" << std::endl;
    connected = false;

    // 重连延迟时间（单位：毫秒）
    int reconnect_delay = 5000;

    // 重连
    std::cout << "尝试重新连接..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
    connect(uri_); // 根据实际情况修改WebSocket服务器地址
}

void WebSocketClient::on_close(websocketpp::connection_hdl hdl) {
    // WebSocket连接关闭事件
    std::cout << "WebSocket连接已关闭" << std::endl;

    // 重连延迟时间（单位：毫秒）
    int reconnect_delay = 5000;

    // 重连
    std::cout << "尝试重新连接..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay));
    connect(uri_); // 根据实际情况修改WebSocket服务器地址
}

void WebSocketClient::on_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
    // 处理接收到的消息
    std::lock_guard<std::mutex> lock(mutex_); // 加锁
    message_queue_.push(msg->get_payload());
}

std::string WebSocketClient::get_next_message() {
    std::lock_guard<std::mutex> lock(mutex_); // 加锁
    if (!message_queue_.empty()) {
        std::string message = message_queue_.front();
        message_queue_.pop();
        return message;
    }
    return "";
}

void WebSocketClient::send_message(const std::string& message) {
    websocketpp::lib::error_code ec;
    client_.send(connection_hdl_, message, websocketpp::frame::opcode::text, ec);
    if (ec) {
        std::cout << "发送消息失败: " << ec.message() << std::endl;
    } else {
        // std::cout << "发送消息成功: " << message << std::endl;
    }
}

bool WebSocketClient::get_status() {
    return connected;
}
