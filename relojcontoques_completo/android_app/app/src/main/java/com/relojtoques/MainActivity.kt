package com.relojtoques

import android.Manifest
import android.content.BroadcastReceiver
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.ServiceConnection
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import android.util.Log
import android.view.View
import android.view.animation.AnimationUtils
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

class MainActivity : AppCompatActivity() {

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    private lateinit var btnStartStop: Button
    private lateinit var tvCardDisplay: TextView
    private lateinit var tvStatus: TextView
    private lateinit var tvHistory: TextView

    private var scannerService: CardScannerService? = null
    private var isScanning = false
    private val cardHistory = mutableListOf<String>()

    private val cardReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            val suit = intent.getStringExtra(CardScannerService.EXTRA_CARD_SUIT) ?: return
            val number = intent.getIntExtra(CardScannerService.EXTRA_CARD_NUMBER, 0)
            val name = intent.getStringExtra(CardScannerService.EXTRA_CARD_NAME) ?: return

            val card = Card(suit, number)
            showCard(card)
        }
    }

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, service: IBinder) {
            val binder = service as CardScannerService.LocalBinder
            scannerService = binder.getService()
            Log.i(TAG, "Servicio conectado")
        }

        override fun onServiceDisconnected(name: ComponentName) {
            scannerService = null
            Log.i(TAG, "Servicio desconectado")
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initViews()
        checkPermissions()
        bindService()
    }

    private fun initViews() {
        btnStartStop = findViewById(R.id.btnStartStop)
        tvCardDisplay = findViewById(R.id.tvCardDisplay)
        tvStatus = findViewById(R.id.tvStatus)
        tvHistory = findViewById(R.id.tvHistory)

        btnStartStop.setOnClickListener {
            if (isScanning) {
                stopScanning()
            } else {
                startScanning()
            }
        }
    }

    private fun checkPermissions() {
        val permissions = mutableListOf(
            Manifest.permission.ACCESS_FINE_LOCATION,
            Manifest.permission.ACCESS_COARSE_LOCATION,
            Manifest.permission.ACCESS_WIFI_STATE,
            Manifest.permission.CHANGE_WIFI_STATE
        )

        val missingPermissions = permissions.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }

        if (missingPermissions.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this,
                missingPermissions.toTypedArray(),
                PERMISSION_REQUEST_CODE
            )
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == PERMISSION_REQUEST_CODE) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            if (!allGranted) {
                Toast.makeText(
                    this,
                    "Se necesitan permisos de ubicación y WiFi",
                    Toast.LENGTH_LONG
                ).show()
            }
        }
    }

    private fun bindService() {
        val intent = Intent(this, CardScannerService::class.java)
        bindService(intent, serviceConnection, Context.BIND_AUTO_CREATE)
    }

    private fun startScanning() {
        scannerService?.startScanning()
        isScanning = true

        btnStartStop.text = "Detener Escaneo"
        btnStartStop.setBackgroundColor(Color.parseColor("#D32F2F"))
        tvStatus.text = "🔍 Escaneando WiFi..."
        tvStatus.setTextColor(Color.parseColor("#4CAF50"))

        Toast.makeText(this, "Escaneo iniciado", Toast.LENGTH_SHORT).show()
    }

    private fun stopScanning() {
        scannerService?.stopScanning()
        isScanning = false

        btnStartStop.text = "Iniciar Escaneo"
        btnStartStop.setBackgroundColor(Color.parseColor("#4CAF50"))
        tvStatus.text = "⏸️ Escaneo detenido"
        tvStatus.setTextColor(Color.parseColor("#666666"))

        Toast.makeText(this, "Escaneo detenido", Toast.LENGTH_SHORT).show()
    }

    private fun showCard(card: Card) {
        runOnUiThread {
            // Actualizar display principal
            tvCardDisplay.text = card.getShortName()
            tvCardDisplay.setTextColor(Color.parseColor(card.getColor()))

            // Animación
            val animation = AnimationUtils.loadAnimation(this, android.R.anim.fade_in)
            tvCardDisplay.startAnimation(animation)

            // Actualizar historial
            cardHistory.add(0, card.getDisplayName())
            if (cardHistory.size > 10) {
                cardHistory.removeLast()
            }

            updateHistoryDisplay()

            // Notificación
            Toast.makeText(
                this,
                "Carta recibida: ${card.getDisplayName()}",
                Toast.LENGTH_SHORT
            ).show()
        }
    }

    private fun updateHistoryDisplay() {
        val historyText = if (cardHistory.isEmpty()) {
            "Ninguna carta recibida aún"
        } else {
            "Últimas cartas:\n" + cardHistory.joinToString("\n") { "• $it" }
        }
        tvHistory.text = historyText
    }

    override fun onStart() {
        super.onStart()
        val filter = IntentFilter(CardScannerService.ACTION_CARD_DETECTED)
        registerReceiver(cardReceiver, filter)
    }

    override fun onStop() {
        super.onStop()
        try {
            unregisterReceiver(cardReceiver)
        } catch (e: Exception) {
            Log.w(TAG, "Error al desregistrar receiver: ${e.message}")
        }
    }

    override fun onDestroy() {
        if (isScanning) {
            stopScanning()
        }
        unbindService(serviceConnection)
        super.onDestroy()
    }
}
