#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>

typedef websocketpp::client<websocketpp::config::asio_client> client;

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    void connect(const std::string& uri);
    void on_open(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, client::message_ptr msg);
    std::string get_next_message();
    void send_message(const std::string& message);
    bool get_status();

private:
    client client_;
    std::string uri_;
    std::queue<std::string> message_queue_;
    std::mutex mutex_;
    websocketpp::connection_hdl connection_hdl_;
    bool connected = false;
};

#endif