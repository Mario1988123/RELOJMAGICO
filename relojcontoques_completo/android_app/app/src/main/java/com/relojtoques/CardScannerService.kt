package com.relojtoques

import android.app.Service
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.net.wifi.WifiManager
import android.os.Binder
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Log

class CardScannerService : Service() {

    companion object {
        const val TAG = "CardScannerService"
        const val ACTION_CARD_DETECTED = "com.relojtoques.CARD_DETECTED"
        const val EXTRA_CARD_SUIT = "card_suit"
        const val EXTRA_CARD_NUMBER = "card_number"
        const val EXTRA_CARD_NAME = "card_name"
        const val SCAN_INTERVAL_MS = 500L
    }

    private val binder = LocalBinder()
    private lateinit var wifiManager: WifiManager
    private val handler = Handler(Looper.getMainLooper())
    private var isScanning = false

    // Caché para evitar procesar el mismo SSID múltiples veces
    private val seenSSIDs = mutableSetOf<String>()
    private val cacheTimeout = 10000L  // 10 segundos

    inner class LocalBinder : Binder() {
        fun getService(): CardScannerService = this@CardScannerService
    }

    private val wifiScanReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val success = intent.getBooleanExtra(WifiManager.EXTRA_RESULTS_UPDATED, false)
            if (success) {
                scanSuccess()
            } else {
                Log.w(TAG, "Escaneo WiFi falló")
            }
        }
    }

    override fun onCreate() {
        super.onCreate()
        wifiManager = applicationContext.getSystemService(Context.WIFI_SERVICE) as WifiManager
        Log.i(TAG, "Servicio creado")
    }

    override fun onBind(intent: Intent): IBinder {
        return binder
    }

    fun startScanning() {
        if (isScanning) return

        isScanning = true
        Log.i(TAG, "Iniciando escaneo WiFi")

        // Registrar receiver
        val intentFilter = IntentFilter(WifiManager.SCAN_RESULTS_AVAILABLE_ACTION)
        registerReceiver(wifiScanReceiver, intentFilter)

        // Limpiar caché periódicamente
        handler.postDelayed(cacheCleaner, cacheTimeout)

        // Iniciar escaneo periódico
        scanPeriodically()
    }

    fun stopScanning() {
        if (!isScanning) return

        isScanning = false
        Log.i(TAG, "Deteniendo escaneo WiFi")

        try {
            unregisterReceiver(wifiScanReceiver)
        } catch (e: Exception) {
            Log.w(TAG, "Error al desregistrar receiver: ${e.message}")
        }

        handler.removeCallbacks(scanRunnable)
        handler.removeCallbacks(cacheCleaner)
    }

    private val scanRunnable = object : Runnable {
        override fun run() {
            if (isScanning) {
                wifiManager.startScan()
                handler.postDelayed(this, SCAN_INTERVAL_MS)
            }
        }
    }

    private val cacheCleaner = object : Runnable {
        override fun run() {
            seenSSIDs.clear()
            Log.d(TAG, "Caché de SSIDs limpiada")
            if (isScanning) {
                handler.postDelayed(this, cacheTimeout)
            }
        }
    }

    private fun scanPeriodically() {
        handler.post(scanRunnable)
    }

    private fun scanSuccess() {
        val results = wifiManager.scanResults

        results.forEach { scanResult ->
            val ssid = scanResult.SSID

            // Solo procesar SSIDs que empiecen con "CARD_"
            if (!ssid.startsWith("CARD_")) return@forEach

            // Evitar procesar el mismo SSID múltiples veces
            if (seenSSIDs.contains(ssid)) return@forEach
            seenSSIDs.add(ssid)

            // Intentar decodificar
            CardDecoder.decodeCard(ssid)?.let { card ->
                Log.i(TAG, "Carta detectada: ${card.getDisplayName()}")
                notifyCardDetected(card)
            }
        }
    }

    private fun notifyCardDetected(card: Card) {
        // Enviar broadcast
        val intent = Intent(ACTION_CARD_DETECTED).apply {
            putExtra(EXTRA_CARD_SUIT, card.suit)
            putExtra(EXTRA_CARD_NUMBER, card.number)
            putExtra(EXTRA_CARD_NAME, card.getDisplayName())
        }
        sendBroadcast(intent)
    }

    override fun onDestroy() {
        stopScanning()
        super.onDestroy()
        Log.i(TAG, "Servicio destruido")
    }
}
