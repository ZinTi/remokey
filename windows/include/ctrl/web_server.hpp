#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <windows.h>
#include <functional>
#include <string>

// 消息回调函数类型
using MessageCallback = std::function<void(const std::string&)>;

// 端口占用回调函数类型（参数：被占用的端口号）
using PortBusyCallback = std::function<void(int)>;

/**
 * 启动 Web 服务线程
 * @param mainHwnd 主窗口句柄
 * @param onMessage 消息回调函数
 * @param onPortBusy 端口占用回调函数
 * @return true 表示启动成功，false 表示端口被占用或启动失败
 */
bool StartWebServer(HWND mainHwnd, MessageCallback onMessage, PortBusyCallback onPortBusy = nullptr);

// 停止 Web 服务
void StopWebServer();

#endif // WEBSERVER_HPP
