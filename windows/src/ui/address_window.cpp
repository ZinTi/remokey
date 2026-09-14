#include "address_window.hpp"
#include <optional>

// ---------------------------------------------------------------------------
// GDI+ 依赖 COM/OLE 基础类型（PROPID、PropertyItem 等）。
// 由于工程全局定义了 WIN32_LEAN_AND_MEAN，windows.h 不会带出 objidl.h，
// 因此必须在 gdiplus.h 之前显式包含 objidl.h，否则 MinGW-w64 下会报
//   error: 'PROPID' 不是一个类型名
// ---------------------------------------------------------------------------
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <string>
#include <vector>

// QR Code 生成库
#include "qrcodegen.hpp"

#if defined(_MSC_VER)
    #pragma comment(lib, "gdiplus.lib")
#endif

namespace remokey {

namespace {

// ---------------------------------------------------------------------------
// GDI+ 生命周期：引用计数，保证多次打开窗口时只 Startup 一次
// ---------------------------------------------------------------------------
ULONG_PTR g_gdiplusToken = 0;
int       g_gdiplusRefs  = 0;

void AddGdiPlusRef() {
    if (g_gdiplusRefs++ == 0) {
        Gdiplus::GdiplusStartupInput input;
        if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &input, nullptr) != Gdiplus::Ok) {
            g_gdiplusToken = 0;
        }
    }
}

void ReleaseGdiPlusRef() {
    if (--g_gdiplusRefs <= 0) {
        g_gdiplusRefs = 0;
        if (g_gdiplusToken) {
            Gdiplus::GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
    }
}

// RAII 守卫：确保 Show() 任意路径返回都能正确释放引用
struct GdiPlusGuard {
    GdiPlusGuard()  { AddGdiPlusRef(); }
    ~GdiPlusGuard() { ReleaseGdiPlusRef(); }
    GdiPlusGuard(const GdiPlusGuard&) = delete;
    GdiPlusGuard& operator=(const GdiPlusGuard&) = delete;
};

HFONT MakeFont(int pt, bool bold = false) {
    return CreateFontW(
        pt, 0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Microsoft YaHei UI");
}

// ---------------------------------------------------------------------------
// JSON 字符串转义
// ---------------------------------------------------------------------------
std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x",
                                  static_cast<unsigned int>(static_cast<unsigned char>(c)));
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}

} // namespace

bool AddressWindow::EnsureClassRegistered(HINSTANCE hInstance) {
    static bool registered = false;
    if (registered) return true;

    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;

    registered = RegisterClassExW(&wc) != 0;
    return registered;
}

// ---------------------------------------------------------------------------
// 构建 JSON payload
// ---------------------------------------------------------------------------
std::string AddressWindow::BuildJsonPayload(const std::string& ip, int port) {
    // 构造形如：
    // {"type":"server","ip":"192.168.1.100","port":8888,"url":"http://192.168.1.100:8888/"}
    std::string url = "http://" + ip + ":" + std::to_string(port) + "/";

    std::string json;
    json.reserve(128);
    json += "{";
    json += "\"type\":\"server\",";
    json += "\"ip\":\"" + JsonEscape(ip) + "\",";
    json += "\"port\":" + std::to_string(port) + ",";
    json += "\"url\":\"" + JsonEscape(url) + "\"";
    json += "}";
    return json;
}

std::wstring AddressWindow::Utf8ToWide(const std::string& utf8) {
    if (utf8.empty()) return L"";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                   static_cast<int>(utf8.size()), nullptr, 0);
    if (wlen <= 0) return L"";
    std::wstring wtext(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                        static_cast<int>(utf8.size()), wtext.data(), wlen);
    return wtext;
}

// ---------------------------------------------------------------------------
// 用 GDI+ 绘制真实二维码
// ---------------------------------------------------------------------------
void AddressWindow::DrawQrCode(HDC hdc, const RECT& rc, const std::wstring& payload) {
    using namespace Gdiplus;

    // 将宽字符 payload 转为 UTF-8，QR 库只接受 char*
    std::string utf8;
    {
        int len = WideCharToMultiByte(CP_UTF8, 0, payload.c_str(),
                                      static_cast<int>(payload.size()),
                                      nullptr, 0, nullptr, nullptr);
        if (len <= 0) return;
        utf8.resize(len);
        WideCharToMultiByte(CP_UTF8, 0, payload.c_str(),
                            static_cast<int>(payload.size()),
                            utf8.data(), len, nullptr, nullptr);
    }

    // 生成 QR Code（Medium 纠错级别，平衡容量和鲁棒性）
    std::optional<qrcodegen::QrCode> qrOpt;
    try {
        qrOpt = qrcodegen::QrCode::encodeText(utf8.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    } catch (const std::exception&) {
        return;
    }
    qrcodegen::QrCode& qr = *qrOpt;

    const int qrSize = qr.getSize();          // 模块数（例如 25）
    const int quietZone = 2;                   // 静默区（模块数）
    const int totalModules = qrSize + quietZone * 2;

    int availW = rc.right - rc.left;
    int availH = rc.bottom - rc.top;
    int side = std::min(availW, availH);
    if (side <= 0) return;

    // 每个模块的像素大小（向下取整，保证整数倍，避免模糊）
    int modulePx = std::max(1, side / totalModules);
    int drawSize = modulePx * totalModules;

    int x0 = rc.left + (availW - drawSize) / 2;
    int y0 = rc.top  + (availH - drawSize) / 2;

    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeNone);
    g.SetPixelOffsetMode(PixelOffsetModeHalf);

    // 白色背景（含静默区）
    SolidBrush bg(Color(255, 255, 255, 255));
    g.FillRectangle(&bg, x0, y0, drawSize, drawSize);

    // 绘制黑色模块
    SolidBrush fg(Color(255, 0, 0, 0));
    for (int y = 0; y < qrSize; ++y) {
        for (int x = 0; x < qrSize; ++x) {
            if (qr.getModule(x, y)) {
                int px = x0 + (x + quietZone) * modulePx;
                int py = y0 + (y + quietZone) * modulePx;
                g.FillRectangle(&fg, px, py, modulePx, modulePx);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 更新 JSON 文本框内容 + 触发二维码重绘
// ---------------------------------------------------------------------------
void AddressWindow::RefreshPayload(HWND hwnd, State* st) {
    if (!st) return;

    // 计算当前选中的 IP
    std::string ipStr = "127.0.0.1";
    if (!st->addresses.empty()
        && st->selectedIndex >= 0
        && st->selectedIndex < static_cast<int>(st->addresses.size())) {
        ipStr = st->addresses[st->selectedIndex].ip;
    } else if (!st->addresses.empty()) {
        ipStr = st->addresses.front().ip;
    }

    // 构建 JSON 并写入只读文本框
    std::string json = BuildJsonPayload(ipStr, st->port);
    std::wstring wjson = Utf8ToWide(json);
    if (st->hJsonEdit) {
        SetWindowTextW(st->hJsonEdit, wjson.c_str());
    }

    // 触发二维码区域重绘
    InvalidateRect(hwnd, nullptr, TRUE);
}

void AddressWindow::Show(HWND owner, int listenPort) {
    HINSTANCE hInstance = reinterpret_cast<HINSTANCE>(
        GetWindowLongPtrW(owner, GWLP_HINSTANCE));
    if (!EnsureClassRegistered(hInstance)) return;

    // RAII：无论后面从哪条路径返回，都会正确 Shutdown（引用计数）
    GdiPlusGuard gdiplusGuard;

    auto* st = new State();
    st->port      = listenPort;
    st->addresses = CollectLocalIPv4();

    RECT ownerRc{};
    GetWindowRect(owner, &ownerRc);

    // 默认居中于 owner
    int x = ownerRc.left + ((ownerRc.right - ownerRc.left) - kWidth) / 2;
    int y = ownerRc.top  + ((ownerRc.bottom - ownerRc.top) - kHeight) / 2;

    // -----------------------------------------------------------------------
    // 钳制到工作区（排除任务栏），确保标题栏始终可见、可拖动。
    // 优先取 owner 所在显示器的工作区；失败时回退到主显示器工作区。
    // -----------------------------------------------------------------------
    HMONITOR hMon = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    RECT work{};
    if (hMon && GetMonitorInfoW(hMon, &mi)) {
        work = mi.rcWork;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    }

    int maxX = work.right  - kWidth;
    int maxY = work.bottom - kHeight;
    if (maxX < work.left) maxX = work.left;
    if (maxY < work.top)  maxY = work.top;

    if (x < work.left) x = work.left;
    if (y < work.top)  y = work.top;
    if (x > maxX)      x = maxX;
    if (y > maxY)      y = maxY;

    HWND hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClassName,
        kWindowTitle,
        WS_CAPTION | WS_SYSMENU | WS_POPUP,
        x, y, kWidth, kHeight,
        owner, nullptr, hInstance, st);

    if (!hwnd) {
        delete st;
        return;   // GdiPlusGuard 析构，自动释放
    }

    // WM_CREATE 里已经创建了子控件、写入了初始 JSON
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    EnableWindow(owner, FALSE);
    MSG msg{};
    while (IsWindow(hwnd) && GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(owner, TRUE);
    SetActiveWindow(owner);
}

LRESULT CALLBACK AddressWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    State* st = reinterpret_cast<State*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_CREATE: {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            st = static_cast<State*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(st));

            // 统一使用 Microsoft YaHei UI，避免 STATIC 控件回退到点阵 System 字体
            HFONT hFont = MakeFont(-13);

            // 顶部提示
            HWND hTip = CreateWindowExW(0, L"STATIC",
                L"请选择用于生成二维码的本机地址：",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                16, 12, kWidth - 32, 20,
                hwnd, nullptr, nullptr, nullptr);
            SendMessageW(hTip, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);

            // 下拉框
            HWND hCombo = CreateWindowExW(
                0, L"COMBOBOX", nullptr,
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                16, 38, kWidth - 32, 200,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdCombo)),
                nullptr, nullptr);

            SendMessageW(hCombo, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
            st->hCombo = hCombo;

            // 只读多行文本框，承载 JSON 原文
            // 样式说明：
            //   ES_MULTILINE     允许多行
            //   ES_AUTOVSCROLL   内容超出时自动纵向滚动
            //   ES_READONLY      只读（仍可选中/复制）
            //   WS_VSCROLL       显示纵向滚动条
            //   WS_EX_CLIENTEDGE 边框，视觉上与其他控件一致
            HWND hJson = CreateWindowExW(
                WS_EX_CLIENTEDGE, L"EDIT",
                L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | ES_LEFT,
                16, 72, kWidth - 32, 66,
                hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdJsonEdit)),
                nullptr, nullptr);

            // EDIT 控件默认字体是 System，需要显式设置
            HFONT hEditFont = MakeFont(-12);
            SendMessageW(hJson, WM_SETFONT, reinterpret_cast<WPARAM>(hEditFont), TRUE);
            st->hJsonEdit = hJson;

            // 初始化下拉框内容
            if (st->addresses.empty()) {
                SendMessageW(hCombo, CB_ADDSTRING, 0,
                             reinterpret_cast<LPARAM>(L"(未检测到可用 IPv4)"));
                SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
                st->selectedIndex = 0;
            } else {
                for (auto& a : st->addresses) {
                    std::wstring item = std::wstring(a.ip.begin(), a.ip.end())
                                      + L"  —  "
                                      + std::wstring(a.adapter.begin(), a.adapter.end());
                    SendMessageW(hCombo, CB_ADDSTRING, 0,
                                 reinterpret_cast<LPARAM>(item.c_str()));
                }
                SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
                st->selectedIndex = 0;
            }

            // 写入初始 JSON
            RefreshPayload(hwnd, st);
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == kIdCombo && HIWORD(wParam) == CBN_SELCHANGE) {
                if (st) {
                    st->selectedIndex = static_cast<int>(
                        SendMessageW(st->hCombo, CB_GETCURSEL, 0, 0));
                    RefreshPayload(hwnd, st);
                }
                return 0;
            }
            // 只读 EDIT 也可能发 EN_SETFOCUS 等通知，不需要处理
            return 0;
        }

        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT: {
            // 让 EDIT 控件背景为白色、文字为黑色
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkColor(hdc, RGB(255, 255, 255));
            return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            if (st) {
                // 只绘制二维码；JSON 文本已由 EDIT 控件承载
                constexpr int kQrSize = 200;
                int qrLeft = (kWidth - kQrSize) / 2;
                RECT qrRc = { qrLeft, 150, qrLeft + kQrSize, 150 + kQrSize };

                // 从只读文本框取回当前 JSON 文本作为二维码载荷，
                // 保证两者内容始终一致
                std::wstring wjson;
                if (st->hJsonEdit) {
                    int len = GetWindowTextLengthW(st->hJsonEdit);
                    if (len > 0) {
                        wjson.resize(static_cast<size_t>(len));
                        GetWindowTextW(st->hJsonEdit, wjson.data(), len + 1);
                    }
                }

                if (!wjson.empty()) {
                    DrawQrCode(hdc, qrRc, wjson);
                }
            }

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY: {
            if (st) {
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
                delete st;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace remokey
