
#include "../net/inetaddress.h"
#include "../base/logging.h"
#include "../base/singleton.h"
#include "FileServer.h"
#include "FileSession.h"

bool FileServer::Init(const char* ip, short port, EventLoop* loop)
{
    InetAddress addr(ip, port);
    m_server.reset(new TcpServer(loop, addr, "YFY-MYFileServer", TcpServer::kReusePort));
    m_server->setConnectionCallback(std::bind(&FileServer::OnConnection, this, std::placeholders::_1));
    //启动侦听
    m_server->start();

    return true;
}

void FileServer::OnConnection(std::shared_ptr<TcpConnection> conn)
{
    if (conn->connected())
    {
        LOG_INFO << "client connected:" << conn->peerAddress().toIpPort();
        ++ m_baseUserId;
        std::shared_ptr<FileSession> spSession(new FileSession(conn));
        conn->setMessageCallback(std::bind(&FileSession::OnRead, spSession.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        std::lock_guard<std::mutex> guard(m_sessionMutex);
        m_sessions.push_back(spSession);
    }
    else
    {
        OnClose(conn);
    }
}

void FileServer::OnClose(const std::shared_ptr<TcpConnection>& conn)
{
    //TODO: 这样的代码逻辑太混乱，需要优化
    std::lock_guard<std::mutex> guard(m_sessionMutex);
    for (auto iter = m_sessions.begin(); iter != m_sessions.end(); ++iter)
    {
        if ((*iter)->GetConnectionPtr() == NULL)
        {
            LOG_ERROR << "connection is NULL";
            break;
        }
                          
        //用户下线
        m_sessions.erase(iter);
        //bUserOffline = true;
        LOG_INFO << "client disconnected: " << conn->peerAddress().toIpPort();
        break;       
    }    
}