package io.github.zinti.remokey.pad.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.RadioButton
import androidx.compose.material3.RadioButtonDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.InputMode
import io.github.zinti.remokey.pad.ui.theme.*

@Composable
fun ModeSelector(mode: InputMode, onModeChange: (InputMode) -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .background(Panel2, RoundedCornerShape(6.dp))
            .border(1.dp, Border, RoundedCornerShape(6.dp))
            .padding(horizontal = 8.dp, vertical = 2.dp), // 高度显著减小
        horizontalArrangement = Arrangement.spacedBy(14.dp),
        verticalAlignment = Alignment.CenterVertically
    ) {
        ModeItem("实时模式", mode == InputMode.REALTIME) { onModeChange(InputMode.REALTIME) }
        ModeItem("草稿模式", mode == InputMode.DRAFT) { onModeChange(InputMode.DRAFT) }
    }
}

@Composable
private fun ModeItem(label: String, selected: Boolean, onClick: () -> Unit) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        modifier = Modifier.clickable(onClick = onClick)
    ) {
        RadioButton(
            selected = selected,
            onClick = onClick,
            modifier = Modifier.size(26.dp), // 缩小
            colors = RadioButtonDefaults.colors(selectedColor = Accent, unselectedColor = FgDim)
        )
        Text(label, fontSize = 12.sp, color = if (selected) Fg else FgDim)
    }
}
