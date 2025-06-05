#include "nlohmann/json.hpp"

#include "../include/websocket_client.h"
#include "../include/MakerbaseCommand.h"
#include "../include/MoonrakerAPI.h"
#include "../include/mks_log.h"
#include "../include/process_messages.h"
#include "../include/event.h"
#include "../include/mks_error.h"
#include "../include/mks_printer.h"
#include "../include/mks_file.h"
#include "../include/mks_gcode.h"

bool get_status_flag = false;
int response_type_id;
nlohmann::json response;
nlohmann::json res;

std::string method;

void *json_parse(WebSocketClient &ws_client)
{
    while (true)
    {
        std::string message = ws_client.get_next_message();
        if (!message.empty())
        {
            try
            {
                response = string2json(message);
                res = response;
            }
            catch (const std::exception &e)
            {
                std::cerr << e.what() << '\n';
            }
            if (res["id"] != nlohmann::detail::value_t::null)
            {
                // std::cout << response["id"] << std::endl;
                int id = response["id"];
                switch (id)
                {
                case 3545:
                    std::cout << "3545: 解析gcode返回数据" << std::endl;
                    // MKSLOG("%s", message.c_str());
                    if (response["error"] != nlohmann::detail::value_t::null)
                    {
                        if (response["error"]["message"] != nlohmann::detail::value_t::null)
                        {
                            std::string err_msg = response["error"]["message"];
                            if (err_msg.length() > 22)
                            {
                                if (err_msg.substr(0, 23) == "Metadata not available")
                                {
                                    // mks_file_parse_finished = true;
                                }
                            }
                        }
                    }
                    if (response["result"] != nlohmann::detail::value_t::null)
                    {
                        // std::cout << response["result"] << std::endl;
                        parse_gcode_metadata(response);
                        // mks_file_parse_finished = true;
                    }
                    break;

                // 解析订阅参数汇总
                case 4654:
                    if (response["result"] == "ok")
                    {
                        std::cout << response << std::endl;
                    }
                    else if (response["result"] != nlohmann::detail::value_t::null)
                    {
                        if (response["result"]["status"] != nlohmann::detail::value_t::null)
                        {
                            parse_subscribe_objects_status(response["result"]["status"]);
                        }
                    }
                    break;

                // 解析获取版本信息
                case 5445:
                    if (response["result"] != nlohmann::detail::value_t::null)
                    {
                        parse_printer_info(response["result"]);
                    }
                    break;

                // 解析获取总打印时间
                case 5656:
                    std::cout << response << std::endl;
                    if (response["result"] != nlohmann::detail::value_t::null)
                    {
                        if (response["result"]["job_totals"] != nlohmann::detail::value_t::null)
                        {
                            parse_server_history_totals(response["result"]["job_totals"]);
                        }
                    }
                    break;

                default:
                    break;
                }
            }
            if (res["error"] != nlohmann::detail::value_t::null)
            {
                parse_error(response["error"]);
            }
            else
            {
                if (response["method"] != nlohmann::detail::value_t::null)
                {
                    method = response["method"];

                    if (response["method"] == "notify_proc_stat_update")
                    {
                    }
                    // gcode回应
                    else if (method == "notify_gcode_response")
                    {
                        if (response["params"] != nlohmann::detail::value_t::null)
                        {
                            parse_gcode_response(response["params"]);
                        }
                    }
                    // 解析订阅
                    else if (method == "notify_status_update")
                    {
                        if (response["params"] != nlohmann::detail::value_t::null)
                        {
                            if (response["params"][0] != nlohmann::detail::value_t::null)
                                parse_subscribe_objects_status(response["params"][0]);
                        }
                    }
                    else if (method == "notify_klippy_ready")
                    {
                        webhooks_state = "ready";
                        // 在这里进行订阅操作
                        usleep(4000);
                        sub_object_status();
                    }
                    else if (method == "notify_klippy_shutdown")
                    {
                        webhooks_state = "shutdown";
                    }
                    else if (method == "notify_klippy_disconnected")
                    {
                        webhooks_state = "disconnected";
                    }
                    else if (method == "notify_filelist_changed")
                    {
                        MKSLOG_BLUE("文件列表已更改");
                    }
                    else if (method == "notify_update_response")
                    {
                        MKSLOG_BLUE("更新管理器响应");
                    }
                    else if (method == "notify_update_refreshed")
                    {
                        MKSLOG_BLUE("更新管理器已刷新");
                    }
                    else if (method == "notify_cpu_throttled")
                    {
                        MKSLOG_BLUE("Moonraker 进程统计更新");
                    }
                    else if (method == "notify_history_changed")
                    {
                        MKSLOG_BLUE("历史改变");
                    }
                    else if (method == "notify_user_created")
                    {
                        MKSLOG_BLUE("授权用户创建");
                    }
                    else if (method == "notify_user_deleted")
                    {
                        MKSLOG_BLUE("已删除授权用户");
                    }
                    else if (method == "notify_service_state_changed")
                    {
                        // std::cout << response << std::endl;
                        // MKSLOG_BLUE("服务状态已更改");
                    }
                    else if (method == "notify_job_queue_changed")
                    {
                        MKSLOG_BLUE("作业队列已更改");
                    }
                    else if (method == "notify_button_event")
                    {
                        MKSLOG_BLUE("按钮事件");
                    }
                    else if (method == "notify_announcement_update")
                    {
                        MKSLOG_BLUE("公告更新事件");
                    }
                    else if (method == "notify_announcement_dismissed")
                    {
                        MKSLOG_BLUE("公告驳回事件");
                    }
                    else if (method == "notify_announcement_wake")
                    {
                        MKSLOG_BLUE("公告唤醒事件");
                    }
                    else if (method == "notify_agent_event")
                    {
                        MKSLOG_BLUE("代理事件");
                    }
                    else if (method == "notify_power_changed")
                    {
                        MKSLOG_BLUE("notify_power_changed");
                        // std::cout << response << std::endl;
                    }
                }
            }
            response.clear();
            res.clear();
        }
        usleep(5);
    }
    pthread_exit(NULL);
}
