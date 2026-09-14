package io.github.zinti.remokey.pad.ui

import android.Manifest
import android.content.pm.PackageManager
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.OptIn
import androidx.camera.core.CameraSelector
import androidx.camera.core.ExperimentalGetImage
import androidx.camera.core.ImageAnalysis
import androidx.camera.core.ImageProxy
import androidx.camera.core.Preview
import androidx.camera.lifecycle.ProcessCameraProvider
import androidx.camera.view.PreviewView
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.ButtonDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalLifecycleOwner
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.core.content.ContextCompat
import com.google.mlkit.vision.barcode.BarcodeScannerOptions
import com.google.mlkit.vision.barcode.BarcodeScanning
import com.google.mlkit.vision.barcode.common.Barcode
import com.google.mlkit.vision.common.InputImage
import io.github.zinti.remokey.pad.net.RemokeyClient
import io.github.zinti.remokey.pad.ui.theme.*
import java.util.concurrent.Executors

/**
 * 连接服务器页面：正方形摄像头预览框（二维码扫描）+ 手动填写 IP / 端口 + 保存按钮。
 *
 * onScanned：扫描并解析成功后的回调，参数为 (host, port)。
 */
@Composable
fun ConnectScreen(
    host: String, onHostChange: (String) -> Unit,
    port: String, onPortChange: (String) -> Unit,
    onSave: () -> Unit,
    onScanned: (host: String, port: Int) -> Unit = { _, _ -> }
) {
    Surface(color = Bg, modifier = Modifier.fillMaxSize()) {
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(rememberScrollState())
                .statusBarsPadding()
                .padding(horizontal = 16.dp, vertical = 16.dp),
            verticalArrangement = Arrangement.spacedBy(14.dp)
        ) {
            Text("连接服务器", fontSize = 17.sp, fontWeight = FontWeight.SemiBold, color = Fg)

            // 正方形摄像头预览框，解码成功后回调原始文本
            CameraScanBox(
                onDecoded = { raw ->
                    val info = io.github.zinti.remokey.pad.net.QrParser.parse(raw)
                    if (info != null) {
                        onScanned(info.host, info.port)
                    }
                }
            )

            // 输入 + 保存
            Column(
                modifier = Modifier
                    .fillMaxWidth()
                    .background(Panel, RoundedCornerShape(8.dp))
                    .border(1.dp, Border, RoundedCornerShape(8.dp))
                    .padding(20.dp),
                verticalArrangement = Arrangement.spacedBy(14.dp)
            ) {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    verticalAlignment = Alignment.CenterVertically,
                    horizontalArrangement = Arrangement.spacedBy(10.dp)
                ) {
                    LabeledField("IP", host, onHostChange, 130.dp)
                    LabeledField("端口", port, onPortChange, 68.dp)
                }

                Button(
                    onClick = {
                        val cleanHost = RemokeyClient.sanitizeHost(host)
                        if (cleanHost != host) onHostChange(cleanHost)
                        onSave()
                    },
                    modifier = Modifier.fillMaxWidth().height(42.dp),
                    shape = RoundedCornerShape(6.dp),
                    colors = ButtonDefaults.buttonColors(
                        containerColor = Accent, contentColor = AccentFg
                    )
                ) { Text("保存", fontSize = 13.sp) }
            }
        }
    }
}

/** 正方形摄像头预览框，集成 ML Kit 二维码解析。 */
@Composable
private fun CameraScanBox(onDecoded: (String) -> Unit) {
    val context = LocalContext.current
    val lifecycleOwner = LocalLifecycleOwner.current

    var hasPermission by remember {
        mutableStateOf(
            ContextCompat.checkSelfPermission(context, Manifest.permission.CAMERA)
                    == PackageManager.PERMISSION_GRANTED
        )
    }
    val permissionLauncher = rememberLauncherForActivityResult(
        contract = ActivityResultContracts.RequestPermission()
    ) { granted -> hasPermission = granted }

    LaunchedEffect(Unit) {
        if (!hasPermission) permissionLauncher.launch(Manifest.permission.CAMERA)
    }

    val cameraExecutor = remember { Executors.newSingleThreadExecutor() }
    DisposableEffect(Unit) { onDispose { cameraExecutor.shutdown() } }

    // 用 rememberUpdatedState 保证 analyzer 里拿到最新的 onDecoded
    val currentOnDecoded by rememberUpdatedState(onDecoded)
    // 防止同一个码连续回调
    var lastDecoded by remember { mutableStateOf<String?>(null) }
    var lastDecodeTime by remember { mutableStateOf(0L) }

    Box(
        modifier = Modifier
            .fillMaxWidth()
            .aspectRatio(1f)
            .background(Panel2, RoundedCornerShape(10.dp))
            .border(1.dp, Border, RoundedCornerShape(10.dp))
    ) {
        if (hasPermission) {
            AndroidView(
                factory = { ctx ->
                    val previewView = PreviewView(ctx).apply {
                        scaleType = PreviewView.ScaleType.FILL_CENTER
                    }

                    // ML Kit 扫描器：只识别二维码
                    val options = BarcodeScannerOptions.Builder()
                        .setBarcodeFormats(Barcode.FORMAT_QR_CODE)
                        .build()
                    val scanner = BarcodeScanning.getClient(options)

                    val cameraProviderFuture = ProcessCameraProvider.getInstance(ctx)
                    cameraProviderFuture.addListener({
                        val cameraProvider = cameraProviderFuture.get()

                        val preview = Preview.Builder().build().also {
                            it.setSurfaceProvider(previewView.surfaceProvider)
                        }

                        val analysis = ImageAnalysis.Builder()
                            .setBackpressureStrategy(ImageAnalysis.STRATEGY_KEEP_ONLY_LATEST)
                            .build()
                            .also { ia ->
                                ia.setAnalyzer(cameraExecutor) { imageProxy ->
                                    processImage(
                                        imageProxy = imageProxy,
                                        scanner = scanner,
                                        onDecoded = { raw ->
                                            val now = System.currentTimeMillis()
                                            // 同一个码 2 秒内不重复回调
                                            if (raw == lastDecoded && now - lastDecodeTime < 2000) {
                                                return@processImage
                                            }
                                            lastDecoded = raw
                                            lastDecodeTime = now
                                            previewView.post { currentOnDecoded(raw) }
                                        }
                                    )
                                }
                            }

                        try {
                            cameraProvider.unbindAll()
                            cameraProvider.bindToLifecycle(
                                lifecycleOwner,
                                CameraSelector.DEFAULT_BACK_CAMERA,
                                preview,
                                analysis
                            )
                        } catch (_: Exception) {
                        }
                    }, ContextCompat.getMainExecutor(ctx))

                    previewView
                },
                modifier = Modifier.fillMaxSize()
            )
        } else {
            Text(
                "未授予摄像头权限",
                fontSize = 13.sp,
                color = FgDim,
                modifier = Modifier.align(Alignment.Center)
            )
        }

        // 取景边框（纯装饰）
        Box(
            modifier = Modifier
                .align(Alignment.Center)
                .fillMaxSize(0.62f)
                .border(2.dp, Accent.copy(alpha = 0.8f), RoundedCornerShape(8.dp))
        )

        Text(
            "将二维码放入框内即可自动连接",
            fontSize = 12.sp,
            color = FgDim,
            modifier = Modifier
                .align(Alignment.BottomCenter)
                .background(
                    Panel.copy(alpha = 0.85f),
                    RoundedCornerShape(bottomStart = 10.dp, bottomEnd = 10.dp)
                )
                .fillMaxWidth()
                .padding(horizontal = 12.dp, vertical = 8.dp)
        )
    }
}

/** 把 ImageProxy 交给 ML Kit 识别二维码。 */
@OptIn(ExperimentalGetImage::class)
private fun processImage(
    imageProxy: ImageProxy,
    scanner: com.google.mlkit.vision.barcode.BarcodeScanner,
    onDecoded: (String) -> Unit
) {
    val mediaImage = imageProxy.image
    if (mediaImage == null) {
        imageProxy.close()
        return
    }
    val image = InputImage.fromMediaImage(
        mediaImage, imageProxy.imageInfo.rotationDegrees
    )
    scanner.process(image)
        .addOnSuccessListener { barcodes ->
            barcodes.firstOrNull { !it.rawValue.isNullOrBlank() }?.rawValue?.let(onDecoded)
        }
        .addOnCompleteListener { imageProxy.close() }
}

@Composable
private fun LabeledField(
    label: String, value: String,
    onChange: (String) -> Unit,
    width: androidx.compose.ui.unit.Dp
) {
    Row(
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(4.dp)
    ) {
        Text(label, fontSize = 12.sp, color = FgDim)
        BasicTextField(
            value = value,
            onValueChange = onChange,
            singleLine = true,
            textStyle = TextStyle(color = Fg, fontSize = 13.sp),
            cursorBrush = SolidColor(Fg),
            modifier = Modifier
                .width(width)
                .background(Panel2, RoundedCornerShape(6.dp))
                .border(1.dp, Border, RoundedCornerShape(6.dp))
                .padding(horizontal = 8.dp, vertical = 5.dp)
        )
    }
}
