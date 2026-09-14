package io.github.zinti.remokey.pad.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.theme.*

/**
 * 关于页面。由 RemokeyScreen 在抽屉点击“关于”后切换过来。
 */
@Composable
fun AboutScreen() {
    Surface(color = Bg, modifier = Modifier.fillMaxSize()) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .statusBarsPadding()
                .padding(horizontal = 16.dp, vertical = 16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            Text("关于", fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = Fg)

            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Panel, RoundedCornerShape(8.dp))
                    .border(1.dp, Border, RoundedCornerShape(8.dp))
                    .padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(10.dp)
            ) {
                Text(
                    "关于 RemokeyPad",
                    fontSize = 14.sp, fontWeight = FontWeight.SemiBold, color = Fg
                )
                Text(
                    "RemokeyPad 是 remokey 项目中的安卓客户端程序，需配合 windows 桌面端（服务端）的 remokey.exe 使用。",
                    fontSize = 13.sp, color = FgDim
                )
                Text("问题反馈：vip201@126.com", fontSize = 13.sp, color = FgDim)
            }
        }
    }
}
