package io.github.zinti.remokey.pad.ui

import androidx.activity.compose.BackHandler
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Surface
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import io.github.zinti.remokey.pad.data.SettingsRepository
import io.github.zinti.remokey.pad.model.WsMessage
import io.github.zinti.remokey.pad.net.RemokeyClient
import io.github.zinti.remokey.pad.ui.components.*
import io.github.zinti.remokey.pad.ui.theme.*
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch

/** 模式枚举 */
enum class InputMode { REALTIME, DRAFT }

/** 顶层页面 */
private enum class AppScreen { MAIN, CONNECT, ABOUT }

@Composable
fun RemokeyScreen() {
    val context = LocalContext.current
    val settings = remember { SettingsRepository(context) }
    val scope = rememberCoroutineScope()

    // ---------- 状态 ----------
    var host by remember { mutableStateOf(SettingsRepository.DEFAULT_HOST) }
    var port by remember { mutableStateOf(SettingsRepository.DEFAULT_PORT) }
    var mode by remember { mutableStateOf(InputMode.DRAFT) }
    var text by remember { mutableStateOf("") }
    var lastSent by remember { mutableStateOf("") }
    var status by remember { mutableStateOf("正在连接…") }
    var connected by remember { mutableStateOf(false) }

    var screen by remember { mutableStateOf(AppScreen.MAIN) }
    var drawerOpen by remember { mutableStateOf(false) }

    val snackbarHostState = remember { SnackbarHostState() }

    val client = remember {
        RemokeyClient { msg, ok ->
            status = msg
            connected = ok
        }
    }

    // 释放资源
    DisposableEffect(Unit) {
        onDispose { client.close() }
    }

    // 读取已保存的 host/port，并只触发一次连接
    LaunchedEffect(Unit) {
        val (savedHost, savedPort) = settings.hostFlow
            .combine(settings.portFlow) { h, p -> h to p }
            .first()
        host = savedHost
        port = savedPort
        if (savedHost.isNotBlank() && savedPort.isNotBlank()) {
            client.connect(
                RemokeyClient.sanitizeHost(savedHost),
                savedPort.toIntOrNull() ?: 8888
            )
        }
    }

    BackHandler(enabled = drawerOpen || screen != AppScreen.MAIN) {
        if (drawerOpen) drawerOpen = false else screen = AppScreen.MAIN
    }

    fun onRealtimeInput(newValue: String) {
        RemokeyClient.diffToMessages(lastSent, newValue).forEach { client.send(it) }
        lastSent = newValue
    }

    Surface(color = Bg, modifier = Modifier.fillMaxSize()) {
        Box(modifier = Modifier.fillMaxSize()) {
            when (screen) {
                AppScreen.MAIN -> MainContent(
                    mode = mode, onModeChange = {
                        mode = it
                        lastSent = text
                        status = if (it == InputMode.REALTIME) "Realtime mode" else "Draft mode"
                    },
                    text = text, onTextChange = {
                        text = it
                        if (mode == InputMode.REALTIME) onRealtimeInput(it)
                    },
                    status = status, connected = connected,
                    onMenuClick = { drawerOpen = true },
                    onClearRemote = {
                        client.send(WsMessage("cmd", "clear"))
                        status = "Cleared remote"
                    },
                    onClearDraft = {
                        text = ""; lastSent = ""; status = "Cleared local"
                    },
                    onClearBoth = {
                        text = ""; lastSent = ""
                        client.send(WsMessage("cmd", "clear"))
                        status = "Cleared both"
                    },
                    onSend = {
                        if (text.isNotEmpty()) {
                            client.send(WsMessage("text", text))
                            lastSent = text
                            status = "Sent"
                        }
                    },
                    onKey = {
                        client.send(WsMessage("key", it))
                        status = "Key: $it"
                    },
                    onCommand = {
                        client.send(WsMessage("cmd", it))
                        status = "Command: $it"
                    }
                )

                AppScreen.CONNECT -> ConnectScreen(
                    host = host, onHostChange = { host = it },
                    port = port, onPortChange = { port = it },
                    onSave = {
                        val cleanHost = RemokeyClient.sanitizeHost(host)
                        val cleanPort = port.trim()
                        scope.launch {
                            settings.save(cleanHost, cleanPort)
                            snackbarHostState.showSnackbar("已保存：$cleanHost:$cleanPort")
                            client.connect(
                                cleanHost,
                                cleanPort.toIntOrNull() ?: 8888
                            )
                        }
                    },
                    onScanned = { scannedHost, scannedPort ->
                        // 回填输入框
                        host = scannedHost
                        port = scannedPort.toString()
                        // 保存 + 连接 + 提示 + 回到主界面
                        scope.launch {
                            settings.save(scannedHost, scannedPort.toString())
                            client.connect(scannedHost, scannedPort)
                            snackbarHostState.showSnackbar("已扫码连接：$scannedHost:$scannedPort")
                        }
                        screen = AppScreen.MAIN
                    }
                )

                AppScreen.ABOUT -> AboutScreen()
            }

            if (drawerOpen) {
                Box(
                    modifier = Modifier
                        .fillMaxSize()
                        .background(Color.Black.copy(alpha = 0.5f))
                        .clickable { drawerOpen = false }
                )
            }

            if (drawerOpen) {
                NavigationDrawer(
                    onOpenConnect = {
                        drawerOpen = false
                        screen = AppScreen.CONNECT
                    },
                    onOpenAbout = {
                        drawerOpen = false
                        screen = AppScreen.ABOUT
                    },
                    modifier = Modifier
                        .fillMaxHeight()
                        .fillMaxWidth(0.7f)
                        .align(Alignment.CenterStart)
                )
            }

            // 悬浮提示
            SnackbarHost(
                hostState = snackbarHostState,
                modifier = Modifier
                    .align(Alignment.BottomCenter)
                    .padding(16.dp)
            )
        }
    }
}

@Composable
private fun MainContent(
    mode: InputMode, onModeChange: (InputMode) -> Unit,
    text: String, onTextChange: (String) -> Unit,
    status: String, connected: Boolean,
    onMenuClick: () -> Unit,
    onClearRemote: () -> Unit,
    onClearDraft: () -> Unit,
    onClearBoth: () -> Unit,
    onSend: () -> Unit,
    onKey: (String) -> Unit,
    onCommand: (String) -> Unit
) {
    Column(modifier = Modifier.fillMaxSize()) {
        TopBar(onMenuClick = onMenuClick)

        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(start = 16.dp, end = 16.dp, top = 8.dp, bottom = 16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // 区块一：状态栏
            StatusBar(text = status, connected = connected)

            // 区块二：编辑器 + 操作按钮
            SectionCard {
                ModeSelector(mode = mode, onModeChange = onModeChange)
                Spacer(Modifier.height(8.dp))
                TextEditor(value = text, onValueChange = onTextChange)
                Spacer(Modifier.height(10.dp))
                ActionButtons(
                    showSend = mode == InputMode.DRAFT,
                    onClearRemote = onClearRemote,
                    onClearDraft = onClearDraft,
                    onClearBoth = onClearBoth,
                    onSend = onSend
                )
            }

            // 区块三：功能按键
            SectionCard {
                KeyPad(onKey = onKey)
            }

            // 区块四：系统指令
            SectionCard {
                CommandGrid(onCommand = onCommand)
            }
        }
    }
}

/** 每个功能区块统一的圆角矩形框。 */
@Composable
private fun SectionCard(content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier = Modifier
            .fillMaxWidth()
            .background(Panel, RoundedCornerShape(8.dp))
            .border(1.dp, Border, RoundedCornerShape(8.dp))
            .padding(14.dp),
        content = content
    )
}
