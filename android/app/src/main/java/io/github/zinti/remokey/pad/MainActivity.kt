package io.github.zinti.remokey.pad

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import io.github.zinti.remokey.pad.ui.RemokeyScreen
import io.github.zinti.remokey.pad.ui.theme.RemokeyPadTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            RemokeyPadTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    RemokeyScreen() // 只挂顶层界面，具体在 RemokeyScreen 里组装
                }
            }
        }
    }
}
