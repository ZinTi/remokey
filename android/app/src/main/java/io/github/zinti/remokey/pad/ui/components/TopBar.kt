package io.github.zinti.remokey.pad.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.theme.*

@Composable
fun TopBar(onMenuClick: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Bg)
            .statusBarsPadding()
            .padding(horizontal = 16.dp, vertical = 8.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        Text(
            text = "RemokeyPad",
            fontSize = 13.sp, // 字号缩小
            fontWeight = FontWeight.SemiBold,
            color = Fg,
            modifier = Modifier
                .background(Panel2, RoundedCornerShape(6.dp))
                .border(1.dp, Border, RoundedCornerShape(6.dp))
                .clickable(onClick = onMenuClick)
                .padding(horizontal = 10.dp, vertical = 5.dp)
        )
    }
}
