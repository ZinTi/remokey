package io.github.zinti.remokey.pad.model

/** 与网页端一致的 WebSocket 消息结构 */
data class WsMessage(val type: String, val content: String)
