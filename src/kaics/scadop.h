#pragma once
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>

enum KaiRoles {
    NONE = 0,
    PRODUCER,
    CONSUMER,
    SERVER,
    BROKER,
    CLIENT,
    PUBLISH,
    SUBSCRIBE
};

#ifdef _WIN32
#define WINDOWS_IGNORE_PACKING_MISMATCH
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#define packed
#define __attribute__(a)
typedef int ssize_t;
#pragma comment(lib, "WS2_32.lib")
#include <WinSock2.h>
#else
using SOCKET = int;
#endif

class KaiSocket {
public:
#pragma pack(1)
    struct Header {
        char rsv;
        int etag;
        volatile unsigned long long ssid; //ssid = port | socket | ip
        char topic[32];
        unsigned int size;
    } __attribute__((packed));
#pragma pack()
#pragma pack(1)
    struct Message {
        Header head{};
        struct Payload {
            char stat[8];
#ifdef _WIN32
            char body[256];
#else
            char body[0];
#endif
        } __attribute__((packed)) data {};
        void* operator new(size_t, const Message& msg) {
            static void* mss = (void*)(&msg);
            return mss;
        }
    } __attribute__((packed));
#pragma pack()
    typedef int(*KAISOCKHOOK)(KaiSocket*);
    typedef void(*RECVCALLBACK)(const Message&);
    static char G_KaiRole[][0xa];
    KaiSocket() = default;
    virtual ~KaiSocket() = default;
public:
    int Initialize(unsigned short lstnprt);
    int Initialize(const char* srvip, unsigned short srvport);
    static KaiSocket& GetInstance();
    // workflow
    int start();
    int connect();
    //
    int Broker();
    ssize_t Publisher(const std::string& topic, const std::string& payload, ...);
    ssize_t Subscriber(const std::string& message, RECVCALLBACK callback = nullptr);
    // packaged
    static void wait(unsigned int tms);
    ssize_t send(const uint8_t* data, size_t len);
    ssize_t recv(uint8_t* buff, size_t size);
    ssize_t broadcast(const uint8_t* data, size_t len);
    // callback
    void registerCallback(KAISOCKHOOK func);
    void appendCallback(KAISOCKHOOK func);
    // private members should be deleted in release version head-file
private:
    struct Network {
        SOCKET socket;
        std::string IP;
        unsigned short PORT;
        volatile bool run_ = false;
        bool client = false;
        Header flag;
    } m_network;
    std::mutex m_lock = {};
    std::vector<Network> m_networks{};
    std::vector<int(*)(KaiSocket*)> m_callbacks{};
    static std::deque<const Message*>* m_msgQue;
private:
    uint64_t setSsid(const Network& network, SOCKET socket = 0);
    ssize_t writes(Network, const uint8_t*, size_t);
    bool checkSsid(SOCKET key, uint64_t ssid);
    bool running();
    void finish();
    void handleNotify(Network& network);
    void runCallback(KaiSocket* sock, KAISOCKHOOK func);
    void setTopic(const std::string& topic, Header& header);
    int produce(const Message& msg);
    int consume(Message& msg);
};
