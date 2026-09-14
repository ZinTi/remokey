package io.github.zinti.remokey.pad.net

import android.os.Handler
import android.os.Looper
import android.util.Log
import io.github.zinti.remokey.pad.model.WsMessage
import okhttp3.*
import org.json.JSONObject
import java.util.concurrent.TimeUnit

class RemokeyClient(
    private val onStatus: (String, Boolean) -> Unit
) {
    private var ws: WebSocket? = null
    private var host: String = ""
    private var port: Int = 8888
    private var manualClose = false

    // 用主线程 Handler 做重连延时，避免裸 Thread
    private val handler = Handler(Looper.getMainLooper())
    private val reconnectRunnable = Runnable {
        if (!manualClose) connectInternal()
    }

    private val client = OkHttpClient.Builder()
        .readTimeout(0, TimeUnit.MILLISECONDS)   // WebSocket 长连接
        .pingInterval(20, TimeUnit.SECONDS)      // 心跳，防止被中间设备掐断
        .build()

    /** 对外入口：切地址/端口或首次连接 */
    fun connect(host: String, port: Int) {
        this.host = sanitizeHost(host)
        this.port = port
        this.manualClose = false
        handler.removeCallbacks(reconnectRunnable)
        closeSocketOnly()          // 只关 socket，不改 manualClose
        connectInternal()
    }

    /** 内部连接，重连时也走这里 */
    private fun connectInternal() {
        closeSocketOnly()

        // 再次清洗，兜底防止 host 被外部直接修改
        val cleanHost = sanitizeHost(host)
        val url = "ws://$cleanHost:$port/ws"

        val request = try {
            Request.Builder().url(url).build()
        } catch (e: IllegalArgumentException) {
            Log.e("RemokeyClient", "非法 URL: $url", e)
            onStatus("地址格式错误: $url", false)
            if (!manualClose) scheduleReconnect()
            return
        }

        onStatus("正在连接 $url …", false)
        Log.i("RemokeyClient", "connecting to $url")

        ws = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                Log.i("RemokeyClient", "ws opened: $url")
                onStatus("已连接", true)
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                Log.i("RemokeyClient", "ws closed: $code $reason")
                if (!manualClose) scheduleReconnect()
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                Log.w("RemokeyClient", "ws failure", t)
                onStatus("连接错误: ${t.message}", false)
                if (!manualClose) scheduleReconnect()
            }
        })
    }

    private fun scheduleReconnect() {
        onStatus("连接断开，2 秒后重连…", false)
        handler.removeCallbacks(reconnectRunnable)
        handler.postDelayed(reconnectRunnable, 2000)
    }

    /** 只关闭底层 socket，不触碰 manualClose 标志 */
    private fun closeSocketOnly() {
        try { ws?.close(1000, null) } catch (_: Exception) {}
        try { ws?.cancel() } catch (_: Exception) {}
        ws = null
    }

    /** 主动关闭（用户切端口/退出） */
    fun close() {
        manualClose = true
        handler.removeCallbacks(reconnectRunnable)
        closeSocketOnly()
    }

    fun send(msg: WsMessage) {
        val json = JSONObject().apply {
            put("type", msg.type)
            put("content", msg.content)
        }.toString()
        val ok = ws?.send(json) ?: false
        if (!ok) Log.w("RemokeyClient", "send failed (ws null or closed): $json")
    }

    companion object {
        /**
         * 清洗 host：去掉用户可能误填的协议前缀、路径、首尾空白。
         * 允许输入形如：
         *   "192.168.1.100"
         *   "ws://192.168.1.100"
         *   "http://192.168.1.100/ws"
         *   "ws://192.168.1.100:8888/ws"
         * 统一归一化为纯主机名（或 IP）。
         */
        fun sanitizeHost(raw: String): String {
            var h = raw.trim()
            h = h.removePrefix("ws://")
            h = h.removePrefix("wss://")
            h = h.removePrefix("http://")
            h = h.removePrefix("https://")
            h = h.trimEnd('/')
            // 去掉路径部分（例如 /ws）
            val slash = h.indexOf('/')
            if (slash >= 0) h = h.substring(0, slash)
            // 去掉可能携带的端口部分（端口由调用方单独传入）
            val colon = h.indexOf(':')
            if (colon >= 0) h = h.substring(0, colon)
            return h.trim()
        }

        fun diffToMessages(oldValue: String, newValue: String): List<WsMessage> {
            if (oldValue == newValue) return emptyList()
            var i = 0
            val minLen = minOf(oldValue.length, newValue.length)
            while (i < minLen && oldValue[i] == newValue[i]) i++
            var oldEnd = oldValue.length
            var newEnd = newValue.length
            while (oldEnd > i && newEnd > i &&
                oldValue[oldEnd - 1] == newValue[newEnd - 1]
            ) { oldEnd--; newEnd-- }
            val delCount = oldEnd - i
            val inserted = newValue.substring(i, newEnd)
            val result = mutableListOf<WsMessage>()
            repeat(delCount) { result.add(WsMessage("key", "backspace")) }
            if (inserted.isNotEmpty()) result.add(WsMessage("text", inserted))
            return result
        }
    }
}
