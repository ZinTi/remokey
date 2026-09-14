package io.github.zinti.remokey.pad.net

import org.json.JSONObject

/** 从二维码文本里解析出的服务器信息 */
data class ServerInfo(val host: String, val port: Int)

/**
 * 解析二维码内容。支持两种格式：
 * 1) JSON：{"type":"server","ip":"192.168.122.239","port":8888,"url":"..."}
 * 2) 纯文本："192.168.122.239:8888" 或 "192.168.122.239"
 *
 * 解析失败返回 null。
 */
object QrParser {

    fun parse(raw: String): ServerInfo? {
        val text = raw.trim()
        if (text.isEmpty()) return null

        // 优先按 JSON 解析
        if (text.startsWith("{")) {
            parseJson(text)?.let { return it }
        }

        // 退化：按 "host:port" / "host" 解析
        return parsePlain(text)
    }

    private fun parseJson(text: String): ServerInfo? = try {
        val obj = JSONObject(text)
        val ip = obj.optString("ip").takeIf { it.isNotBlank() }
            ?: obj.optString("host").takeIf { it.isNotBlank() }
            ?: return null

        val host = RemokeyClient.sanitizeHost(ip)

        // port 可能是数字，也可能被写成字符串
        val port = when {
            obj.has("port") && !obj.isNull("port") -> {
                val p = obj.opt("port")
                when (p) {
                    is Number -> p.toInt()
                    is String -> p.trim().toIntOrNull()
                    else -> null
                }
            }
            else -> null
        } ?: extractPortFromUrl(obj.optString("url"))
        ?: 8888

        if (host.isBlank() || port !in 1..65535) null else ServerInfo(host, port)
    } catch (_: Exception) {
        null
    }

    private fun extractPortFromUrl(url: String): Int? {
        if (url.isBlank()) return null
        return try {
            // 简单解析：host:port 形式，或交给 URI
            val u = java.net.URI(url)
            if (u.port > 0) u.port else null
        } catch (_: Exception) {
            null
        }
    }

    private fun parsePlain(text: String): ServerInfo? {
        val clean = RemokeyClient.sanitizeHost(text)
        if (clean.isBlank()) return null

        // sanitizeHost 已经去掉了端口，这里再从原始串里尝试抠端口
        val port = text.substringAfterLast(':', "")
            .takeIf { it.isNotEmpty() && it.all(Char::isDigit) }
            ?.toIntOrNull()
            ?: 8888

        return ServerInfo(clean, port)
    }
}
