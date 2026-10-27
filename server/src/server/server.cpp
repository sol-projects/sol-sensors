#include <asio.hpp>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "server/server.hpp"
#include <sensors/sensors.hpp>
#include <sensors/Device.hpp>
#include <sensors/error.hpp>

using asio::ip::tcp;
namespace fs = std::filesystem;

namespace {

struct DeviceInfo {
    std::string id;
    std::string type;
    std::string displayName;
    int warnThreshold     = 6000;   // ×100 space: 60.00
    int criticalThreshold = 8500;   // ×100 space: 85.00
    sensors::Device* device = nullptr;
    bool supportsLoad = false;
    bool supportsTemp = false;
};

static std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else out += c;
        }
    }
    return out;
}

static std::string formatScaled(int raw, int precision) {
    double v = static_cast<double>(raw) / std::pow(10.0, precision);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", precision, v);
    return buf;
}

static std::string makeDisplayName(const std::string& full) {
    std::string base = full;
    std::string suffix;

    auto tpos = base.rfind(" thread #");
    if (tpos != std::string::npos) {
        suffix = base.substr(tpos);
        base   = base.substr(0, tpos);
    }

    auto cut = [&](const std::string& needle) {
        auto p = base.find(needle);
        if (p != std::string::npos) base = base.substr(0, p);
    };
    cut(" with ");
    cut(" CPU @ ");
    cut(" @ ");
    cut(" Processor");

    for (const char* v : {"AMD ", "Intel ", "NVIDIA ", "Apple ", "Qualcomm ", "ARM ", "MediaTek "}) {
        if (base.rfind(v, 0) == 0) { base = base.substr(std::strlen(v)); break; }
    }

    std::string out;
    for (char c : base) {
        if (c == ' ' && !out.empty() && out.back() == ' ') continue;
        out += c;
    }
    while (!out.empty() && out.back() == ' ') out.pop_back();
    if (out.empty()) out = full;
    if (out.size() > 22) out = out.substr(0, 21) + "…";
    return out + suffix;
}

static double parseCapacityGb(const std::string& name) {
    std::regex re(R"((\d+(?:\.\d+)?)\s*G(?:i)?B)", std::regex::icase);
    std::smatch m;
    if (std::regex_search(name, m, re)) {
        try { return std::stod(m[1].str()); } catch (...) {}
    }
    return 0.0;
}

static std::string getLanIp() {
    try {
        asio::io_context ctx;
        asio::ip::udp::socket s(ctx);
        s.open(asio::ip::udp::v4());
        s.connect({asio::ip::make_address("8.8.8.8"), 80});
        return s.local_endpoint().address().to_string();
    } catch (...) { return ""; }
}

static std::string findClientDir() {
    if (const char* env = std::getenv("SOL_CLIENT_DIR")) return env;
    if (fs::exists("./client/index.html")) return "./client";
#ifdef __linux__
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = 0;
        fs::path exe(buf);
        auto dir = exe.parent_path();
        for (auto candidate : { dir / "client", dir.parent_path() / "client" }) {
            if (fs::exists(candidate / "index.html")) return candidate.string();
        }
    }
#endif
    return "./client";
}

static std::string mimeFor(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".css")  return "text/css; charset=utf-8";
    if (ext == ".js")   return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".png")  return "image/png";
    if (ext == ".ico")  return "image/x-icon";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    return "application/octet-stream";
}

class Server {
public:
    Server(const std::string& clientDir, int intervalMs)
        : io_(), acceptor_(io_), clientDir_(clientDir), intervalMs_(intervalMs) {}

    void run() {
        auto cpuThreads = sensors::getDevices(sensors::Device::Type::CPUThread);
        auto cpus       = sensors::getDevices(sensors::Device::Type::CPU);
        auto rams       = sensors::getDevices(sensors::Device::Type::RAM);
        auto vrams      = sensors::getDevices(sensors::Device::Type::VRAM);
        auto gpus       = sensors::getDevices(sensors::Device::Type::GPU);

        cpuThreadsStore_ = std::move(cpuThreads);
        cpusStore_       = std::move(cpus);
        ramsStore_       = std::move(rams);
        vramsStore_      = std::move(vrams);
        gpusStore_       = std::move(gpus);

        auto add = [&](std::vector<sensors::Device>& vec, const std::string& type) {
            for (size_t i = 0; i < vec.size(); ++i) {
                DeviceInfo d;
                d.id     = type + ":" + std::to_string(i);
                d.type   = type;
                d.device = &vec[i];
                devices_.push_back(d);
            }
        };
        add(cpuThreadsStore_, "cpu_thread");
        add(cpusStore_,       "cpu");
        add(ramsStore_,       "ram");
        add(vramsStore_,      "vram");
        add(gpusStore_,       "gpu");

        // Warm up + detect capabilities once
        for (int round = 0; round < 2; ++round) {
            for (auto& d : devices_) {
                try { sensors::getLoad(*d.device, 2); } catch (...) {}
                try { sensors::getTemp(*d.device, 2); } catch (...) {}
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        for (auto& d : devices_) {
            try { d.supportsLoad = sensors::getLoad(*d.device, 2) != sensors::error::code; }
            catch (...) { d.supportsLoad = false; }
            try { d.supportsTemp = sensors::getTemp(*d.device, 2) != sensors::error::code; }
            catch (...) { d.supportsTemp = false; }
        }

        // Display names + thresholds
        for (auto& d : devices_) {
            d.displayName = makeDisplayName(d.device->name);
            if (d.type == "cpu_thread") { d.warnThreshold = 7000; d.criticalThreshold = 9000; }
            else if (d.type == "cpu" || d.type == "gpu") { d.warnThreshold = 6000; d.criticalThreshold = 8500; }
            else if (d.type == "ram" || d.type == "vram") {
                double totalGb = parseCapacityGb(d.device->name);
                if (totalGb > 0) {
                    d.warnThreshold     = static_cast<int>(totalGb * 0.80 * 100);
                    d.criticalThreshold = static_cast<int>(totalGb * 0.95 * 100);
                } else {
                    d.warnThreshold = 8000; d.criticalThreshold = 9500;
                }
            }
        }

        std::string lan = getLanIp();
        std::cout << "Serving from: " << clientDir_ << "\n"
                  << "Sample interval: " << intervalMs_ << " ms\n"
                  << "Devices: " << devices_.size() << "\n\n"
                  << "Open in a browser:\n"
                  << "  http://127.0.0.1:8080/\n";
        if (!lan.empty() && lan != "127.0.0.1")
            std::cout << "  http://" << lan << ":8080/   (LAN)\n";
        std::cout << std::flush;

        tcp::endpoint ep(tcp::v4(), 8080);
        acceptor_.open(ep.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();

        samplerThread_ = std::thread([this]{ samplerLoop(); });
        startAccept();
        io_.run();

        stopSampler_ = true;
        if (samplerThread_.joinable()) samplerThread_.join();
    }

private:
    asio::io_context io_;
    tcp::acceptor acceptor_;
    std::string clientDir_;
    int intervalMs_;

    std::vector<sensors::Device> cpuThreadsStore_, cpusStore_, ramsStore_, vramsStore_, gpusStore_;
    std::vector<DeviceInfo> devices_;

    std::vector<std::shared_ptr<tcp::socket>> sseClients_;
    std::mutex sseMutex_;
    std::thread samplerThread_;
    std::atomic<bool> stopSampler_{false};

    void startAccept() {
        auto sock = std::make_shared<tcp::socket>(io_);
        acceptor_.async_accept(*sock, [this, sock](std::error_code ec) {
            if (!ec) std::thread([this, sock]{ handleConnection(sock); }).detach();
            if (!stopSampler_) startAccept();
        });
    }

    void handleConnection(std::shared_ptr<tcp::socket> sock) {
        try {
            asio::streambuf buf;
            std::error_code ec;
            asio::read_until(*sock, buf, "\r\n\r\n", ec);
            if (ec) return;

            std::istream is(&buf);
            std::string method, rawPath;
            is >> method >> rawPath;

            std::string path = rawPath, query;
            auto qpos = path.find('?');
            if (qpos != std::string::npos) {
                query = path.substr(qpos + 1);
                path = path.substr(0, qpos);
            }

            if (method != "GET") { sendError(sock, 405, "Method Not Allowed"); return; }

            if (path == "/api/config") { serveConfig(sock); return; }
            if (path == "/devices")    { serveDevices(sock); return; }
            if (path == "/stream")     { serveStream(sock); return; }
            if (path == "/" || path.empty()) path = "/index.html";

            if (path.find("..") != std::string::npos) { sendError(sock, 403, "Forbidden"); return; }
            serveStatic(sock, path);
        } catch (std::exception& e) {
            std::cerr << "Connection error: " << e.what() << "\n";
        }
    }

    void serveConfig(std::shared_ptr<tcp::socket> sock) {
        std::ostringstream json;
        json << "{"
             << "\"interval\":" << intervalMs_
             << ",\"precision\":2"
             << ",\"historyMax\":3000"
             << ",\"defaultWindowS\":10"
             << ",\"windowOptions\":["
             << "{\"label\":\"5s\",\"seconds\":5},"
             << "{\"label\":\"10s\",\"seconds\":10},"
             << "{\"label\":\"30s\",\"seconds\":30},"
             << "{\"label\":\"1m\",\"seconds\":60},"
             << "{\"label\":\"5m\",\"seconds\":300}"
             << "],\"groups\":["
             << "{\"key\":\"cpu_thread\",\"title\":\"CPU Threads\",\"yMax\":100,\"unit\":\"%\",\"axisLabel\":\"Percent\",\"fill\":false,\"showTemp\":false},"
             << "{\"key\":\"cpu\",\"title\":\"CPU\",\"yMax\":100,\"unit\":\"%\",\"axisLabel\":\"Percent\",\"fill\":true,\"showTemp\":true,\"tempUnit\":\"\\u00b0C\",\"tempAxisLabel\":\"Temp\"},"
             << "{\"key\":\"ram\",\"title\":\"Memory\",\"yMax\":null,\"unit\":\"GB\",\"axisLabel\":\"Usage\",\"fill\":true,\"showTemp\":false},"
             << "{\"key\":\"vram\",\"title\":\"VRAM\",\"yMax\":null,\"unit\":\"GB\",\"axisLabel\":\"Usage\",\"fill\":true,\"showTemp\":false},"
             << "{\"key\":\"gpu\",\"title\":\"GPU\",\"yMax\":100,\"unit\":\"%\",\"axisLabel\":\"Percent\",\"fill\":true,\"showTemp\":true,\"tempUnit\":\"\\u00b0C\",\"tempAxisLabel\":\"Temp\"}"
             << "]}";

        sendJsonResponse(sock, json.str());
    }

    void serveDevices(std::shared_ptr<tcp::socket> sock) {
        std::ostringstream json;
        json << "{\"devices\":[";
        bool first = true;
        for (auto& d : devices_) {
            if (!first) json << ",";
            first = false;
            double warn = d.warnThreshold / 100.0;
            double crit = d.criticalThreshold / 100.0;
            json << "{\"id\":\"" << d.id << "\""
                 << ",\"type\":\"" << d.type << "\""
                 << ",\"name\":\"" << escapeJson(d.device->name) << "\""
                 << ",\"displayName\":\"" << escapeJson(d.displayName) << "\""
                 << ",\"thresholds\":{\"warn\":" << warn << ",\"critical\":" << crit << "}"
                 << ",\"supportsLoad\":" << (d.supportsLoad ? "true" : "false")
                 << ",\"supportsTemp\":" << (d.supportsTemp ? "true" : "false")
                 << "}";
        }
        json << "]}";
        sendJsonResponse(sock, json.str());
    }

    void sendJsonResponse(std::shared_ptr<tcp::socket> sock, const std::string& body) {
        std::ostringstream head;
        head << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: application/json\r\n"
             << "Access-Control-Allow-Origin: *\r\n"
             << "Cache-Control: no-store\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n";
        writeAll(sock, head.str());
        writeAll(sock, body);
        closeSock(sock);
    }

    void serveStream(std::shared_ptr<tcp::socket> sock) {
        std::string head =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/event-stream\r\n"
            "Cache-Control: no-cache\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: keep-alive\r\n\r\n"
            "retry: 1000\n\n";
        writeAll(sock, head);
        std::lock_guard<std::mutex> lock(sseMutex_);
        sseClients_.push_back(sock);
    }

    void serveStatic(std::shared_ptr<tcp::socket> sock, const std::string& path) {
        fs::path full = fs::path(clientDir_) / path.substr(1);
        if (!fs::exists(full) || !fs::is_regular_file(full)) {
            sendError(sock, 404, "Not Found");
            return;
        }
        std::ifstream f(full, std::ios::binary);
        if (!f) { sendError(sock, 404, "Not Found"); return; }
        std::ostringstream buf; buf << f.rdbuf();
        std::string content = buf.str();

        std::ostringstream head;
        head << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << mimeFor(path) << "\r\n"
             << "Cache-Control: no-cache\r\n"
             << "Content-Length: " << content.size() << "\r\n"
             << "Connection: close\r\n\r\n";
        writeAll(sock, head.str());
        writeAll(sock, content);
        closeSock(sock);
    }

    void sendError(std::shared_ptr<tcp::socket> sock, int code, const char* msg) {
        std::string body = std::to_string(code) + " " + msg;
        std::ostringstream head;
        head << "HTTP/1.1 " << code << " " << msg << "\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n";
        writeAll(sock, head.str());
        writeAll(sock, body);
        closeSock(sock);
    }

    void writeAll(std::shared_ptr<tcp::socket> sock, const std::string& data) {
        std::error_code ec;
        asio::write(*sock, asio::buffer(data), ec);
    }

    void closeSock(std::shared_ptr<tcp::socket> sock) {
        std::error_code ec;
        sock->shutdown(tcp::socket::shutdown_both, ec);
        sock->close(ec);
    }

    void samplerLoop() {
        while (!stopSampler_) {
            auto start = std::chrono::steady_clock::now();
            std::string ev = "data: " + buildSampleJson() + "\n\n";

            {
                std::lock_guard<std::mutex> lock(sseMutex_);
                std::vector<std::shared_ptr<tcp::socket>> dead;
                for (auto& s : sseClients_) {
                    std::error_code ec;
                    asio::write(*s, asio::buffer(ev), ec);
                    if (ec) dead.push_back(s);
                }
                for (auto& d : dead) {
                    sseClients_.erase(std::remove(sseClients_.begin(), sseClients_.end(), d), sseClients_.end());
                    closeSock(d);
                }
            }

            auto elapsed = std::chrono::steady_clock::now() - start;
            auto sleepMs = std::chrono::milliseconds(intervalMs_) -
                           std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            if (sleepMs.count() > 0) std::this_thread::sleep_for(sleepMs);
        }
    }

    std::string buildSampleJson() {
        auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        std::ostringstream json;
        json << "{\"ts\":" << ts
             << ",\"precision\":2,\"interval\":" << intervalMs_
             << ",\"devices\":[";

        bool first = true;
        for (auto& d : devices_) {
            if (!first) json << ",";
            first = false;
            json << "{\"id\":\"" << d.id << "\",\"type\":\"" << d.type << "\"";

            int loadRaw = sensors::error::code;
            try { loadRaw = sensors::getLoad(*d.device, 2); } catch (...) {}
            std::string level = "ok";
            if (loadRaw == sensors::error::code) {
                json << ",\"load\":null";
            } else {
                d.device->load = loadRaw;
                json << ",\"load\":" << formatScaled(loadRaw, 2);
                if (loadRaw >= d.criticalThreshold) level = "crit";
                else if (loadRaw >= d.warnThreshold) level = "warn";
            }

            int tempRaw = sensors::error::code;
            try { tempRaw = sensors::getTemp(*d.device, 2); } catch (...) {}
            if (tempRaw == sensors::error::code) json << ",\"temp\":null";
            else { d.device->temperature = tempRaw; json << ",\"temp\":" << formatScaled(tempRaw, 2); }

            json << ",\"level\":\"" << level << "\"}";
        }
        json << "]}";
        return json.str();
    }
};

} // namespace

void server::serve() {
    std::string dir = findClientDir();
    if (!fs::exists(dir + "/index.html")) {
        std::cerr << "WARNING: client/index.html not found at '" << dir << "'\n";
    }
    int interval = 100;
    if (const char* env = std::getenv("SOL_INTERVAL")) interval = std::max(10, std::atoi(env));
    Server s(dir, interval);
    s.run();
}
