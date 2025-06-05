#include <pthread.h>
#include <iomanip>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include "mks_log.h"
#include "mks_error.h"
#include "sovol_http.h"
#include "refresh_ui.h"

namespace fs = std::filesystem;

typedef struct 
{
    char url[256];
    char download_info[256];
    char output_filename[128];
    char output_filepath[256];
}OTA_Download_Str;

fs::path binPath = "/home/sovol/printer_data/build/";
fs::path binTmpPath = "/home/sovol/printer_data/build/tmp/";
int ota_progress;
std::string cur_remote_version = "";

std::string newVersion = "";
std::string curVersion = "";

const char *get_ota_info()
{
    return (const char *)OTA_URL;
}

//val 0-100
void set_ota_progress(int16_t val) 
{
    if(val < 0) {
        val = 0;
    } else if(val > 100) {
        val = 100;
    }
    int16_t end_angle = (360 * val) / 100;
}

static void update_progress_bar(double percentage) 
{
    // set_ota_progress((int16_t)percentage);
    MKSLOG_YELLOW("percentage: %d\n", (int) percentage);
    ota_progress = (int)percentage;
}

std::string exec(const char* cmd)
{
    std::array<char, 128> buf;
    std::string res;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if(pipe) {
        while(fgets(buf.data(), buf.size(), pipe.get()) != nullptr) {
            res += buf.data();
        }
    }
    return res;
}

std::string calculateMD5(const std::string& filePath) 
{
    std::string cmd = "md5sum " + filePath;
    std::string result = exec(cmd.c_str());
    return result.substr(0, result.find(' '));
}

// 回调函数，用于处理libcurl获取的数据
static size_t WriteCallback2(void* contents, size_t size, size_t nmemb, void* userp) 
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 使用libcurl进行HTTP请求，获取JSON数据
std::string get_json_data(const std::string& url) 
{
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback2);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);  // 5秒超时
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_DNS_SERVERS, "8.8.8.8,8.8.4.4"); // 使用Google DNS
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if(res != CURLE_OK) 
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
    }
    return readBuffer;
}

using json = nlohmann::json;
std::map<std::string, std::string> nlohmann_parse_json(const std::string& json_data) {
    json json_msg = json::parse(json_data);
    std::map<std::string, std::string> json_kv;
    if (json_msg.contains("success") && json_msg["success"].is_boolean() && json_msg["success"].get<bool>()) {
        if (json_msg.contains("rows") && json_msg["rows"].is_object()) {
            for (auto& [key, value] : json_msg["rows"].items()) {
                if (value.is_string()) {
                    json_kv[key] = value.get<std::string>();
                } else if (value.is_number()) {
                    json_kv[key] = std::to_string(value.get<double>());
                } else if (value.is_boolean()) {
                    json_kv[key] = value.get<bool>() ? "true" : "false";
                } else {
                    json_kv[key] = value.dump(); // For other types, use dump()
                }
            }
        }
    }
    return json_kv;
}

static const char* get_url_filename(const char *url)
{
    const char *filename = strrchr(url, '/');
    if(filename) {
        return filename + 1;
    }
    return url;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) 
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static int progress_callback(void *p, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) 
{
    if (dltotal > 0) 
    {
        CURL* easy_handle = static_cast<CURL*>(p);
        double percentage = ((double)dlnow / (double)dltotal) * 100.0;

        // pthread_mutex_lock(&mutex_kock);
        update_progress_bar(percentage);
        // pthread_mutex_unlock(&mutex_kock);
    }
    return 0;
}

static bool fs_create_dir(fs::path& dirpath)
{
    bool ret = fs::create_directory(dirpath);
    MKSLOG_YELLOW("dir:[%s] delete %s!\n", dirpath.c_str(), ret == -1 ? "failed" : "succeed");
    return ret;
}

void create_guide_file(const std::string& file_path)
{
    if (file_exists(file_path)) {
        std::cout << "文件已存在: " << file_path << std::endl;
        return;
    }

    std::ofstream outfile(file_path, std::ios::out);
    if (!outfile) {
        std::cerr << "错误: 无法创建文件: " << file_path << std::endl;
        perror("系统错误");  // 输出系统错误信息
        return;
    }
    outfile.close();
    system("sync");
}

bool file_exists(const std::string& file_path)
{
    std::ifstream file(file_path);
    return file.good();
}

void deleteOneFile(const std::string& file_path)
{
    if (std::remove(file_path.c_str()) != 0) {
        MKSLOG_YELLOW("Error deleting file");
    } else {
        MKSLOG_YELLOW("successfully deleted: %s", file_path);
    }
}

std::map<std::string, std::string> readConfigFile(const std::string &filename) {
    std::map<std::string, std::string> config;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Unable to open file " << filename << std::endl;
        return config;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream lineStream(line);
        std::string key;
        if (std::getline(lineStream, key, '=')) {
            std::string value;
            if (std::getline(lineStream, value)) {
                config[key] = value;
            }
        }
    }

    file.close();
    return config;
}

void writeConfigFile(const std::string &filename, const std::map<std::string, std::string> &config) {
    std::ofstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Unable to open file " << filename << std::endl;
        return;
    }

    for (const auto &pair : config) {
        file << pair.first << "=" << pair.second << std::endl;
    }

    file.close();
}

void deb_install(fs::path binPath) {
    std::string cmd = "sudo dpkg -i --force-overwrite ";
    cmd += binPath;
    if(system(cmd.c_str()) == 0) {
        MKSLOG_YELLOW("+++++deb_install success!!!+++++");
        system("sync");
        writeConfigFile(VERSION_PATH, {{"version", newVersion}});
        deleteOneFile(binPath);
        deleteOneFile(GUIDE_PATH);
        page_to(TJC_PAGE_STARTING);
        reset_firmware();
        system("sudo systemctl restart crowsnest");
        system("sudo systemctl restart moonraker-obico");
        system("sudo systemctl restart makerbase-client");
    }else {
        ota_progress = -2;
        mkslog_print("deb_install error!");
        MKSLOG_YELLOW("+++++deb_install failed!!!+++++");
    }
}

void* download_file(void *arg)
{
    OTA_Download_Str *a = (OTA_Download_Str *)arg;
    const char *otaUrl = (const char *)a->url;
    const char *otaInfo = (const char *)a->download_info;
    std::string otafullpath = otaUrl;
    std::string ota_json_data = get_json_data(std::string(otaInfo)); // 获取json
    if (ota_json_data == "")
    {
        newVersion = "error";
        return NULL;
    }
    
    std::map<std::string, std::string> json_kv;

    json_kv = nlohmann_parse_json(ota_json_data);
    newVersion = json_kv["versionCode"];
    otafullpath = json_kv["path"];
    MKSLOG_YELLOW("+++++versionCode:%s, firmwareMD5:%s", json_kv["versionCode"].c_str(), json_kv["firmwareMD5"].c_str());
    MKSLOG_YELLOW("+++++otafullpath: %s", otafullpath.c_str());

    strcpy(a->output_filename, get_url_filename(otafullpath.c_str()));
    sprintf(a->output_filepath, "%s%s", binTmpPath.c_str(), a->output_filename);

    if(access(binTmpPath.c_str(),F_OK) == -1)
    {
        fs_create_dir(binTmpPath);
    } else {
        bool ret = fs::remove_all(binTmpPath);
        fprintf(stdout, "dir:[%s] delete %s!\n", binTmpPath.c_str(), ret == -1 ? "failed" : "succeed");
        if(ret != -1) {
            fs_create_dir(binTmpPath);
        }
    }
    fprintf(stdout, "downFilePath: %s\nfilename: %s\n", a->output_filepath, a->output_filename);

    CURL *curl = curl_easy_init();
    FILE *fp = fopen((char *)a->output_filepath, "wb");
    CURLcode res;

    if (curl && fp) 
    {
        curl_easy_setopt(curl, CURLOPT_URL, otafullpath.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        fclose(fp);

        if (res == CURLE_OK) 
        {
            std::string md5res = calculateMD5(a->output_filepath);
            MKSLOG_YELLOW("md5res:%s, firmwareMD5:%s", md5res.c_str(), json_kv["firmwareMD5"].c_str());
            if(strcmp(md5res.c_str(), json_kv["firmwareMD5"].c_str()) == 0) {
                binPath += fs::path(a->output_filename);
                fs::rename(fs::path(a->output_filepath), binPath); 
                fs::remove(binTmpPath);
                fprintf(stdout, "%s download succeed!\n", a->output_filename);
                deb_install(binPath);
            } else { //校验失败处理
                mkslog_print("md5 error!");
                fs::remove_all(binTmpPath);
                ota_progress = -1;
            }
        }
        else
        {
            mkslog_print("curl_easy_perform download error!");
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
    } 
    else 
    {
        fprintf(stderr, "初始化失败\n");
        if (fp) 
            fclose(fp);
        if (curl) 
            curl_easy_cleanup(curl);
    }
    return NULL;
}

void create_download_thread()
{
    OTA_Download_Str *args = (OTA_Download_Str *)malloc(sizeof(OTA_Download_Str));
    strcpy((char *)args->url, OTA_URL);
    strcpy((char *)args->download_info, get_ota_info());

    pthread_t download_thread;
    if (pthread_create(&download_thread, NULL, download_file, (void*)args) != 0) 
    {
        fprintf(stderr, "Failed to create download thread\n");
    }
    pthread_detach(download_thread);
}

void firmware_version_check() {
    const fs::path config_filename = VERSION_PATH;
    std::map lan_map = readConfigFile(config_filename);
    std::string ver = "0";
    if (lan_map.find("version") != lan_map.end()) {
        ver = lan_map["version"];
    }
    curVersion = ver;//std::stof(ver);

    std::map<std::string, std::string> json_kv;
    std::string ota_json_data = get_json_data(std::string(get_ota_info()));
    if(ota_json_data.empty()) {
        return;
    }
    json_kv = nlohmann_parse_json(ota_json_data);
    newVersion = json_kv["versionCode"];
    MKSLOG_YELLOW("newVersion:%s, curVersion:%s", newVersion.c_str(), curVersion.c_str());
    // sprintf(vbuf, "%.1f/%.1f", curVersion, newVersionFloat);
    // cur_remote_version = std::string(vbuf);
}