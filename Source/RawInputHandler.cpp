juce::String RawInputHandler::extractHardwareFingerprint(const DeviceId& devicePath)
{
    if (devicePath.isEmpty())
        return {};

    const auto upper = devicePath.toUpperCase();
    const int vidIdx = upper.indexOf("VID_");
    const int pidIdx = upper.indexOf("PID_");

    if (vidIdx >= 0 && pidIdx >= 0)
    {
        const juce::String vid = upper.substring(vidIdx, vidIdx + 8);
        const juce::String pid = upper.substring(pidIdx, pidIdx + 8);

        const int miIdx = upper.indexOf("MI_");
        const juce::String mi = (miIdx >= 0) ? upper.substring(miIdx, miIdx + 5) : "";

        return vid + ":" + pid + (mi.isNotEmpty() ? (":" + mi) : "");
    }

    return devicePath;
}

#include "RawInputHandler.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX 1
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN 1
 #endif
 #include <windows.h>

bool RawInputHandler::attachToWindow(void* hwndVoid)
{
    auto hwnd = static_cast<HWND>(hwndVoid);
    if (hwnd == nullptr)
        return false;

    // RIDEV_INPUTSINK allows receiving keyboard events even when the window is in the background
    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01; // Generic Desktop Controls
    rid[0].usUsage     = 0x06; // Keyboard
    rid[0].dwFlags     = RIDEV_INPUTSINK;
    rid[0].hwndTarget  = hwnd;

    return RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == TRUE;
}

juce::String RawInputHandler::resolveDevicePath(void* hRawInputDeviceHandle) const
{
    auto hDevice = static_cast<HANDLE>(hRawInputDeviceHandle);
    if (hDevice == nullptr)
        return {};

    UINT size = 0;
    GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, nullptr, &size);
    if (size == 0)
        return {};

    std::vector<wchar_t> buffer(size);
    if (GetRawInputDeviceInfoW(hDevice, RIDI_DEVICENAME, buffer.data(), &size) <= 0)
        return {};

    return juce::String(buffer.data());
}

bool RawInputHandler::processRawInputMessage(void* lParamVoid)
{
    auto lParam = reinterpret_cast<LPARAM>(lParamVoid);

    UINT dwSize = 0;
    GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                     nullptr, &dwSize, sizeof(RAWINPUTHEADER));

    if (dwSize == 0)
        return false;

    std::vector<BYTE> buffer(dwSize);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT,
                         buffer.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
        return false;

    auto* raw = reinterpret_cast<RAWINPUT*>(buffer.data());
    if (raw->header.dwType != RIM_TYPEKEYBOARD)
        return false;

    const auto devicePath = resolveDevicePath(raw->header.hDevice);
    if (devicePath.isEmpty())
        return false;

    const DeviceId deviceId = devicePath;

    if (! knownDevices.contains(deviceId))
    {
        knownDevices.add(deviceId);
        if (onNewDeviceDetected != nullptr)
            onNewDeviceDetected(deviceId);
    }

    RawKeyEvent event;
    event.deviceId       = deviceId;
    event.virtualKeyCode = raw->data.keyboard.VKey;
    event.isKeyDown      = (raw->data.keyboard.Flags & RI_KEY_BREAK) == 0;
    event.timestampMs    = juce::Time::currentTimeMillis();

    if (onKeyEvent != nullptr)
        onKeyEvent(event);

    return true;
}

juce::Array<DeviceId> RawInputHandler::getKnownDevices() const
{
    return knownDevices;
}

juce::String RawInputHandler::getFriendlyName(const DeviceId& id) const
{
    auto it = friendlyNames.find(id.toStdString());
    if (it != friendlyNames.end())
        return it->second;

    return "Keyboard (" + id.substring(0, 14) + "...)";
}

void RawInputHandler::setFriendlyName(const DeviceId& id, const juce::String& name)
{
    friendlyNames[id.toStdString()] = name;
}

#else // Non-Windows Stub

bool RawInputHandler::attachToWindow(void*) { return false; }
bool RawInputHandler::processRawInputMessage(void*) { return false; }
juce::Array<DeviceId> RawInputHandler::getKnownDevices() const { return {}; }
juce::String RawInputHandler::getFriendlyName(const DeviceId& id) const { return id; }
void RawInputHandler::setFriendlyName(const DeviceId&, const juce::String&) {}
juce::String RawInputHandler::resolveDevicePath(void*) const { return {}; }

#endif
