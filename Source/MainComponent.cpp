#include "MainComponent.h"

MainComponent::MainComponent()
    : editorComponent(mappingEngine, deviceManager, audioEngine),
      performanceComponent(audioEngine, mappingEngine, deviceManager)
{
    // 1. Audio Subsystem Initialization
    audioDeviceManager.initialiseWithDefaultDevices(0, 2);
    audioSourcePlayer.setSource(&audioEngine);
    audioDeviceManager.addAudioCallback(&audioSourcePlayer);

    // 2. Header & Top-Bar Controls
    addAndMakeVisible(titleLabel);
    titleLabel.setText("K3N ARMONI COMPOSER / LOOPSTATION", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);

    addAndMakeVisible(audioSettingsButton);
    audioSettingsButton.addListener(this);

    // Update Notification Button (hidden by default until a new release is detected)
    addChildComponent(updateButton);
    updateButton.addListener(this);
    updateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff10b981));
    updateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    // 3. Tabbed Interface Setup
    addAndMakeVisible(tabs);
    tabs.setTabBarDepth(32);
    tabs.addTab("1. Mapping Editor", juce::Colours::darkgrey, &editorComponent, false);
    tabs.addTab("2. Live Performance", juce::Colours::darkgrey.darker(0.3f), &performanceComponent, false);

    // 4. Raw Input Event Routing
    deviceManager.onKeyEvent = [this](const RawKeyEvent& event, KeyboardRole role)
    {
        // If Mapping Editor tab is active and waiting for key assignment, capture it
        if (tabs.getCurrentTabIndex() == 0)
        {
            if (editorComponent.captureKeyForAssignment(event))
                return;
        }

        // Otherwise route event to live performance looper & sampler
        performanceComponent.handleKeyEvent(event, role);
    };

    deviceManager.onNewDeviceNeedsNaming = [this](const DeviceId&)
    {
        juce::MessageManager::callAsync([this]()
        {
            editorComponent.refreshDeviceList();
        });
    };

    setSize(920, 680);

    // Check for updates asynchronously in the background
    checkForUpdatesAsync();
}

MainComponent::~MainComponent()
{
    audioDeviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff181a1f));
}

void MainComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    auto headerArea = area.removeFromTop(38);
    titleLabel.setBounds(headerArea.removeFromLeft(headerArea.getWidth() - 360));

    if (updateButton.isVisible())
    {
        updateButton.setBounds(headerArea.removeFromRight(170).reduced(2));
        headerArea.removeFromRight(6);
    }

    audioSettingsButton.setBounds(headerArea.removeFromRight(170).reduced(2));

    area.removeFromTop(6);
    tabs.setBounds(area);
}

void MainComponent::setNativeWindowHandle(void* hwnd)
{
    deviceManager.initialise(hwnd);
}

void MainComponent::handleRawInput(void* lParam)
{
    deviceManager.handleRawInputMessage(lParam);
}

void MainComponent::checkForUpdatesAsync()
{
    juce::Thread::launch([this]()
    {
        juce::URL url("https://api.github.com/repos/EtherK3N/ArmoniComposer/releases/latest");
        auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                               .withConnectionTimeoutMs(3000));

        if (stream != nullptr)
        {
            const auto jsonString = stream->readEntireStreamAsString();
            const auto json = juce::JSON::parse(jsonString);

            if (json.isObject())
            {
                const auto latestTag = json.getProperty("tag_name", "").toString();
                const auto htmlUrl = json.getProperty("html_url", "https://github.com/EtherK3N/ArmoniComposer/releases").toString();

                juce::String currentVersion;
                if (auto* app = juce::JUCEApplication::getInstance())
                    currentVersion = app->getApplicationVersion();

                if (latestTag.isNotEmpty() && currentVersion.isNotEmpty() && latestTag != ("v" + currentVersion))
                {
                    juce::MessageManager::callAsync([this, latestTag, htmlUrl]()
                    {
                        latestReleaseUrl = htmlUrl;
                        updateButton.setButtonText("🎉 New Update (" + latestTag + ") Available!");
                        updateButton.setVisible(true);
                        resized();
                    });
                }
            }
        }
    });
}

void MainComponent::buttonClicked(juce::Button* b)
{
    if (b == &audioSettingsButton)
    {
        showAudioSettingsModal();
    }
    else if (b == &updateButton)
    {
        if (latestReleaseUrl.isNotEmpty())
            juce::URL(latestReleaseUrl).launchInDefaultBrowser();
    }
}

void MainComponent::showAudioSettingsModal()
{
    auto* selector = new juce::AudioDeviceSelectorComponent(
        audioDeviceManager,
        0, 2, // Min/max audio inputs
        0, 2, // Min/max audio outputs
        false, false, false, false);

    selector->setSize(500, 420);

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned(selector);
    opts.dialogTitle = "Audio Device & Driver Setup";
    opts.dialogBackgroundColour = juce::Colour(0xff22252a);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = false;

    opts.launchAsync();
}
