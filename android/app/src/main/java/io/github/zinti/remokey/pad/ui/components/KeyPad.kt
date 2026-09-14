package io.github.zinti.remokey.pad.ui.components

import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.theme.*

/** 4 列 × 2 行功能按键，顺序与网页一致 */
private val keys = listOf(
    "TAB" to "tab", "↑" to "up", "MENU" to "menu", "BACKSPACE" to "backspace",
    "←" to "left", "↓" to "down", "→" to "right", "ENTER" to "enter"
)

@Composable
fun KeyPad(onKey: (String) -> Unit) {
    SectionTitle("功能按键")
    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
        keys.chunked(4).forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                row.forEach { (label, key) ->
                    KeyBtn(label, Modifier.weight(1f)) { onKey(key) }
                }
            }
        }
    }
}

@Composable
private fun KeyBtn(label: String, modifier: Modifier, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = modifier.height(40.dp),
        shape = RoundedCornerShape(6.dp),
        colors = ButtonDefaults.buttonColors(containerColor = Panel2, contentColor = Fg),
        contentPadding = PaddingValues(horizontal = 2.dp, vertical = 0.dp)
    ) { Text(label, fontSize = 11.5.sp) }
}
