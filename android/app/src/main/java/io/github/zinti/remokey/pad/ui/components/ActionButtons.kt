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

@Composable
fun ActionButtons(
    showSend: Boolean,
    onClearRemote: () -> Unit,
    onClearDraft: () -> Unit,
    onClearBoth: () -> Unit,
    onSend: () -> Unit
) {
    Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            SecondaryBtn("清空远端", Modifier.weight(1f), onClearRemote)
            SecondaryBtn("清空本地", Modifier.weight(1f), onClearDraft)
            SecondaryBtn("两端清空", Modifier.weight(1f), onClearBoth)
        }
        if (showSend) {
            Button(
                onClick = onSend,
                modifier = Modifier.fillMaxWidth().height(40.dp),
                shape = RoundedCornerShape(6.dp),
                colors = ButtonDefaults.buttonColors(
                    containerColor = Accent, contentColor = AccentFg
                ),
                contentPadding = PaddingValues(horizontal = 6.dp, vertical = 2.dp)
            ) { Text("发送", fontSize = 12.sp) }
        }
    }
}

@Composable
private fun SecondaryBtn(text: String, modifier: Modifier, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        modifier = modifier.height(38.dp),
        shape = RoundedCornerShape(6.dp),
        colors = ButtonDefaults.buttonColors(
            containerColor = Panel2, contentColor = Fg
        ),
        contentPadding = PaddingValues(horizontal = 4.dp, vertical = 2.dp)
    ) { Text(text, fontSize = 12.sp) }
}
