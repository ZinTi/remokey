package io.github.zinti.remokey.pad.data

import android.content.Context
import androidx.datastore.preferences.core.edit
import androidx.datastore.preferences.core.stringPreferencesKey
import androidx.datastore.preferences.preferencesDataStore
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.map

private val Context.dataStore by preferencesDataStore(name = "remokey_settings")

class SettingsRepository(private val context: Context) {

    private val keyHost = stringPreferencesKey("host")
    private val keyPort = stringPreferencesKey("port")

    val hostFlow: Flow<String> = context.dataStore.data.map { it[keyHost] ?: DEFAULT_HOST }
    val portFlow: Flow<String> = context.dataStore.data.map { it[keyPort] ?: DEFAULT_PORT }

    suspend fun save(host: String, port: String) {
        context.dataStore.edit {
            it[keyHost] = host
            it[keyPort] = port
        }
    }

    companion object {
        const val DEFAULT_HOST = "192.168.1.100"
        const val DEFAULT_PORT = "8888"
    }
}
