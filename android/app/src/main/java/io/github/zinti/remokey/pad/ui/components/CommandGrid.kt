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

private val commands = listOf(
    "全选" to "select_all", "复制" to "copy", "剪切" to "cut",
    "粘贴" to "paste", "撤销" to "undo", "重做" to "redo"
)

@Composable
fun CommandGrid(onCommand: (String) -> Unit) {
    SectionTitle("系统指令")
    Column(verticalArrangement = Arrangement.spacedBy(6.dp)) {
        commands.chunked(3).forEach { row ->
            Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                row.forEach { (label, cmd) ->
                    Button(
                        onClick = { onCommand(cmd) },
                        modifier = Modifier.weight(1f).height(34.dp),
                        shape = RoundedCornerShape(6.dp),
                        colors = ButtonDefaults.buttonColors(
                            containerColor = Panel2, contentColor = Fg
                        ),
                        contentPadding = PaddingValues(horizontal = 2.dp, vertical = 0.dp)
                    ) { Text(label, fontSize = 11.5.sp) }
                }
            }
        }
    }
}

@Composable
internal fun SectionTitle(title: String) {
    Text(
        text = title,
        fontSize = 12.sp,
        color = FgDim,
        modifier = Modifier.padding(top = 0.dp, bottom = 6.dp)
    )
}
