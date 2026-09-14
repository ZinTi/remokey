package io.github.zinti.remokey.pad.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.theme.*

/**
 * 左侧抽屉导航。
 * 只负责展示两个入口按钮，点击后由外部切换到独立页面。
 */
@Composable
fun NavigationDrawer(
    onOpenConnect: () -> Unit,
    onOpenAbout: () -> Unit,
    modifier: Modifier = Modifier
) {
    Column(
        modifier = modifier
            .fillMaxHeight()
            .background(Panel)
            .border(1.dp, Border, RoundedCornerShape(topEnd = 10.dp, bottomEnd = 10.dp))
            .statusBarsPadding()
            .padding(16.dp)
            .verticalScroll(rememberScrollState()),
        verticalArrangement = Arrangement.spacedBy(10.dp)
    ) {
        DrawerButton("连接服务器", onOpenConnect)
        DrawerButton("关于", onOpenAbout)
    }
}

@Composable
private fun DrawerButton(text: String, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = Modifier.fillMaxWidth().height(42.dp),
        shape = RoundedCornerShape(6.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = Panel2,
            contentColor = Fg
        ),
        contentPadding = PaddingValues(horizontal = 10.dp, vertical = 0.dp)
    ) { Text(text, fontSize = 13.sp) }
}
