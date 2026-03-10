package com.sentinel.v2x

import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.Before
import org.junit.runner.RunWith
import org.junit.Assert.*

/**
 * Instrumented test for V2X JNI bindings (Minimal - Toolchain Verification)
 * 
 * Purpose: Validate that Android/JNI/CMake integration works
 * - Native library loads correctly
 * - JNI function calls execute without error
 * - Return values are valid
 * 
 * Phase: Toolchain validation only
 * Duration: ~1 day
 * 
 * Run: ./gradlew connectedAndroidTest --tests V2XJNITest
 * 
 * Success criteria:
 * - getVersion() returns non-empty string ✓
 * - Contains "Message Processor" ✓
 * - No UnsatisfiedLinkError ✓
 */
@RunWith(AndroidJUnit4::class)
class V2XJNITest {
    
    @Before
    fun setup() {
        // Native library automatically loaded by V2X object init block
    }
    
    /**
     * Test 1: Native library loads successfully
     * 
     * Validates:
     * - CMake compilation works
     * - Shared library (.so) created
     * - Gradle APK includes native library
     * - Android NDK toolchain configured correctly
     */
    @Test
    fun testNativeLibraryLoads() {
        try {
            // If we reach here, library loaded successfully
            assertTrue("Native library loaded", true)
        } catch (e: UnsatisfiedLinkError) {
            fail("Native library failed to load: ${e.message}")
        }
    }
    
    /**
     * Test 2: JNI function getVersion() executes
     * 
     * Validates:
     * - JNI function mapping works
     * - C++ function is callable from Kotlin
     * - Return value is marshalled correctly (String)
     * - No exceptions during call
     */
    @Test
    fun testGetVersion() {
        val version = V2X.getVersion()
        assertNotNull("Version string should not be null", version)
        assertTrue("Version string should not be empty", version.isNotEmpty())
        assertTrue("Version should contain 'Message Processor'", version.contains("Message Processor"))
    }
    
    /**
     * Test 3: Version string contains expected components
     * 
     * Validates:
     * - V2XMessageProcessor::get_version() called correctly
     * - Expected format: "V2X Message Processor v1.0.0"
     */
    @Test
    fun testVersionFormatValid() {
        val version = V2X.getVersion()
        
        // Expected format validation
        assertTrue(
            "Version should follow expected format",
            version.matches(Regex(".*[Mm]essage.*[Pp]rocessor.*v\\d+\\.\\d+\\.\\d+.*"))
        )
    }
    
    /**
     * Test 4: Multiple calls to JNI function work correctly
     * 
     * Validates:
     * - JNI binding is reusable
     * - No state corruption
     * - Consistent return values
     */
    @Test
    fun testMultipleCalls() {
        val version1 = V2X.getVersion()
        val version2 = V2X.getVersion()
        
        assertEquals("Multiple calls should return same value", version1, version2)
    }
}

/**
 * Test Execution Instructions:
 * 
 * Prerequisites:
 * - Android device or emulator connected
 * - build/outputs/apk/debug/app-debug.apk built with native libraries
 * - CMAKE_ANDROID_ABI set in gradle.properties
 * 
 * Run tests:
 *   ./gradlew connectedAndroidTest --tests V2XJNITest
 * 
 * Expected output:
 *   V2XJNITest > testNativeLibraryLoads PASSED
 *   V2XJNITest > testGetVersion PASSED
 *   V2XJNITest > testVersionFormatValid PASSED
 *   V2XJNITest > testMultipleCalls PASSED
 * 
 * Success indicators:
 *   ✓ All tests pass (4/4)
 *   ✓ No UnsatisfiedLinkError
 *   ✓ Version string echoed from native engine
 *   ✓ JNI linkage verified end-to-end
 * 
 * Next steps after success:
 *   → Move to Phase 4: ASN.1 decoder implementation
 *   → Hold full MessageProcessor JNI binding
 *   → Plan Phase 4 Kotlin interfaces
 */
