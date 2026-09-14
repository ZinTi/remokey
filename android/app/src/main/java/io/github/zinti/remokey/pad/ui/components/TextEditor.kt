package io.github.zinti.remokey.pad.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import io.github.zinti.remokey.pad.ui.theme.*

@Composable
fun TextEditor(value: String, onValueChange: (String) -> Unit) {
    Box(
        modifier = Modifier
            .fillMaxWidth()
            .height(140.dp)
            .background(Panel2, RoundedCornerShape(6.dp))
            .border(1.dp, Border, RoundedCornerShape(6.dp))
            .padding(12.dp)
    ) {
        if (value.isEmpty()) {
            Text("在此输入…", color = FgDim.copy(alpha = 0.6f), fontSize = 15.sp)
        }
        BasicTextField(
            value = value,
            onValueChange = onValueChange,
            textStyle = TextStyle(color = Fg, fontSize = 15.sp),
            cursorBrush = SolidColor(Fg),
            modifier = Modifier.fillMaxSize()
        )
    }
}
