package com.sentinel.v2x

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

/**
 * MainActivity for V2X Message Processor
 * 
 * Purpose: Demonstrate V2X JNI bridge
 * - Load native library
 * - Call JNI functions
 * - Display message processing results
 * 
 * Features:
 * - Show engine version
 * - Process sample BSM message
 * - Display decoded fields (lat, lon, speed, heading)
 */
class MainActivity : AppCompatActivity() {
    
    private var messageLog: StringBuilder = StringBuilder()
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        val logText: TextView = findViewById(R.id.log_text)
        
        try {
            // Initialize V2X engine
            addLog("=== V2X Message Processor ===")
            addLog("Loading JNI bridge...")
            
            // Get version
            val version = V2X.getVersion()
            addLog("✓ Version: $version")
            
            // Test message processing
            addLog("")
            addLog("=== Testing Message Processing ===")
            testBSMMessage()
            testFrameTypeDetection()
            
            addLog("")
            addLog("✓ All tests passed!")
            
        } catch (e: Exception) {
            addLog("✗ Error: ${e.message}")
            addLog("Stack trace: ${e.stackTraceToString()}")
        }
        
        logText.text = messageLog.toString()
    }
    
    /**
     * Test: Process a BSM (Basic Safety Message)
     */
    private fun testBSMMessage() {
        try {
            addLog("Processing BSM message...")
            
            // Create minimal BSM COER message
            val bsmPayload = byteArrayOf(
                0x00, // Frame type: BSM
                0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, // Sender MAC
                0x00, 0x00, 0x00, 0x00, // Timestamp
                0x41, 0x82, 0x0D, 0x7C, // Latitude (37.773°)
                0x04, 0x25, 0xD3, 0x44, // Longitude (-122.415°)
                0x00, 0x32, // Speed (50 km/h)
                0x01, 0x2C  // Heading (300°)
            )
            
            val coerMessage = wrapInCOER(bsmPayload)
            addLog("  Message size: ${coerMessage.size} bytes")
            
            // Decode message
            val decoded = V2X.processMessage(coerMessage)
            addLog("  ✓ Message decoded")
            
            // Display decoded fields
            when (decoded) {
                is DecodedV2XMessage.BSM -> {
                    val msg = decoded.message
                    addLog("  Frame: BSM (Basic Safety Message)")
                    addLog("  Position: ${msg.geoPosition.latitude}°N, ${msg.geoPosition.longitude}°E")
                    addLog("  Speed: ${msg.speed} km/h")
                    addLog("  Heading: ${msg.heading}°")
                }
                is DecodedV2XMessage.SPaT -> {
                    addLog("  Frame: SPaT (Signal Phase and Timing)")
                }
                is DecodedV2XMessage.PSM -> {
                    addLog("  Frame: PSM (Personal Safety Message)")
                }
                is DecodedV2XMessage.Unknown -> {
                    addLog("  Frame: Unknown (${decoded.errorMessage})")
                }
            }
        } catch (e: Exception) {
            addLog("  ✗ Error: ${e.message}")
        }
    }
    
    /**
     * Test: Detect frame type from raw bytes
     */
    private fun testFrameTypeDetection() {
        try {
            addLog("")
            addLog("Detecting frame type...")
            
            val bsmPayload = byteArrayOf(
                0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                0x00, 0x00, 0x00, 0x00,
                0x41, 0x82, 0x0D, 0x7C,
                0x04, 0x25, 0xD3, 0x44,
                0x00, 0x32, 0x01, 0x2C
            )
            
            val coerMessage = wrapInCOER(bsmPayload)
            val frameType = V2X.detectFrameType(coerMessage)
            addLog("  Detected: $frameType")
            
        } catch (e: Exception) {
            addLog("  ✗ Error: ${e.message}")
        }
    }
    
    /**
     * Helper: Wrap payload in COER container
     */
    private fun wrapInCOER(payload: ByteArray): ByteArray {
        val coer = mutableListOf<Byte>()
        coer.add(0x00.toByte()) // Unsigned message header
        
        if (payload.size <= 127) {
            coer.add(payload.size.toByte())
        } else {
            coer.add(0x81.toByte())
            coer.add(payload.size.toByte())
        }
        
        coer.addAll(payload.toList())
        return coer.toByteArray()
    }
    
    /**
     * Helper: Add log message
     */
    private fun addLog(message: String) {
        if (messageLog.isNotEmpty()) {
            messageLog.append("\n")
        }
        messageLog.append(message)
    }
}
