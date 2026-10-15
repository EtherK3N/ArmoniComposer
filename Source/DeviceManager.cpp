#include "DeviceManager.h"

DeviceManager::DeviceManager()
{
    rawInput.onNewDeviceDetected = [this](const DeviceId& id)
    {
        if (devices.find(id.toStdString()) == devices.end())
        {
            KnownDevice d;
            d.id = id;
            d.friendlyName = "Keyboard " + juce::String(devices.size() + 1);
            d.role = KeyboardRole::Unassigned;

            // Smart Reconnect: check if a device with matching hardware VID/PID was previously configured
            const auto fp = RawInputHandler::extractHardwareFingerprint(id);
            if (fp.isNotEmpty())
            {
                for (const auto& [existingId, existingDev] : devices)
                {
                    if (RawInputHandler::extractHardwareFingerprint(existingDev.id) == fp)
                    {
                        d.friendlyName = existingDev.friendlyName;
                        d.role = existingDev.role;
                        break;
                    }
                }
            }

            devices[id.toStdString()] = d;
            rawInput.setFriendlyName(id, d.friendlyName);

            saveToDisk();

            if (onNewDeviceNeedsNaming != nullptr)
                onNewDeviceNeedsNaming(id);
        }
    };

    rawInput.onKeyEvent = [this](const RawKeyEvent& e)
    {
        auto it = devices.find(e.deviceId.toStdString());
        auto role = (it != devices.end()) ? it->second.role : KeyboardRole::Unassigned;

        if (onKeyEvent != nullptr)
            onKeyEvent(e, role);
    };

    loadFromDisk();
}

DeviceManager::~DeviceManager()
{
    saveToDisk();
}

void DeviceManager::initialise(void* nativeWindowHandle)
{
    rawInput.attachToWindow(nativeWindowHandle);
}

void DeviceManager::handleRawInputMessage(void* lParam)
{
    rawInput.processRawInputMessage(lParam);
}

juce::Array<KnownDevice> DeviceManager::getDevices() const
{
    juce::Array<KnownDevice> result;
    for (const auto& [key, value] : devices)
        result.add(value);
    return result;
}

void DeviceManager::renameDevice(const DeviceId& id, const juce::String& newName)
{
    auto it = devices.find(id.toStdString());
    if (it != devices.end())
    {
        it->second.friendlyName = newName;
        rawInput.setFriendlyName(id, newName);
        saveToDisk();
    }
}

void DeviceManager::setDeviceRole(const DeviceId& id, KeyboardRole role)
{
    auto it = devices.find(id.toStdString());
    if (it != devices.end())
    {
        it->second.role = role;
        saveToDisk();
    }
}

juce::File DeviceManager::getConfigFile() const
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("LoopStation");
    dir.createDirectory();
    return dir.getChildFile("devices.json");
}

void DeviceManager::loadFromDisk()
{
    auto file = getConfigFile();
    if (! file.existsAsFile())
        return;

    auto json = juce::JSON::parse(file);
    if (! json.isArray())
        return;

    if (auto* arr = json.getArray())
    {
        for (auto& item : *arr)
        {
            if (! item.isObject())
                continue;

            KnownDevice d;
            d.id = item.getProperty("id", "").toString();
            d.friendlyName = item.getProperty("name", "Keyboard").toString();
            d.role = static_cast<KeyboardRole>(static_cast<int>(item.getProperty("role", 0)));

            if (d.id.isNotEmpty())
            {
                devices[d.id.toStdString()] = d;
                rawInput.setFriendlyName(d.id, d.friendlyName);
            }
        }
    }
}

void DeviceManager::saveToDisk() const
{
    juce::Array<juce::var> arr;

    for (const auto& [key, d] : devices)
    {
        auto obj = std::make_unique<juce::DynamicObject>();
        obj->setProperty("id", d.id);
        obj->setProperty("name", d.friendlyName);
        obj->setProperty("role", static_cast<int>(d.role));
        arr.add(juce::var(obj.release()));
    }

    auto file = getConfigFile();
    file.replaceWithText(juce::JSON::toString(juce::var(arr)));
}
