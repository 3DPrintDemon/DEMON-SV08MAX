#ifndef PROCESS_MESSAGES_H
#define PROCESS_MESSAGES_H
/**
 * 通过发送的 method id 来获得相应的回调信息，使用不同的 response type id 可以通过
 * switch 的方式，快速找到相应的解析方式
 * 
 */

void *json_parse(WebSocketClient& ws_client);

#endif
