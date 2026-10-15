/*
    RawInputHandler.h
    -----------------
    Low-level keyboard input interception using Windows Raw Input API (WM_INPUT).
    Allows distinguishing physical hardware sources (USB port / HID Device ID)
    across multiple concurrently connected keyboards.
*/

#pragma once

#include <juce_core/juce_core.h>
#include <functional>
#include <unordered_map>
#include <string>

/** Stable physical HID device identifier (Win32 Device Path). */
using DeviceId = juce::String;

/** Encapsulates a keyboard key event tagged with its physical origin device ID. */
struct RawKeyEvent
{
    DeviceId deviceId;
    int virtualKeyCode = 0; // Win32 VK_*
    bool isKeyDown = false;
    juce::int64 timestampMs = 0;
};

class RawInputHandler
{
public:
    RawInputHandler() = default;
    ~RawInputHandler() = default;

    /** Registers the host window to receive WM_INPUT messages from all connected keyboards. */
    bool attachToWindow(void* hwnd);

    /** Processes the WM_INPUT message received by the host window procedure. Returns true if handled. */
    bool processRawInputMessage(void* lParam);

    /** Returns the list of all detected Device IDs. */
    /** Extracts a persistent hardware fingerprint (VID:PID:MI) invariant to USB port re-plugging. */
    static juce::String extractHardwareFingerprint(const DeviceId& devicePath);

    juce::Array<DeviceId> getKnownDevices() const;

    /** Friendly display names assigned by the user to each keyboard. */
    juce::String getFriendlyName(const DeviceId& id) const;
    void setFriendlyName(const DeviceId& id, const juce::String& name);

    /** Callback invoked on every captured key event. */
    std::function<void(const RawKeyEvent&)> onKeyEvent;

    /** Callback invoked when a previously unseen keyboard is detected. */
    std::function<void(const DeviceId&)> onNewDeviceDetected;

private:
    juce::String resolveDevicePath(void* hRawInputDeviceHandle) const;

    std::unordered_map<std::string, juce::String> friendlyNames;
    juce::Array<DeviceId> knownDevices;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RawInputHandler)
};
