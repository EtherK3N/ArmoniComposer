/*
    Main.cpp
    --------
    JUCE Application Entry Point:
      - MainWindow creation
      - Win32 WndProc Subclassing to capture WM_INPUT and forward to RawInputHandler
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "MainComponent.h"

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX 1
 #endif
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN 1
 #endif
 #include <windows.h>
#endif

class ArmoniComposerApplication : public juce::JUCEApplication
{
public:
    ArmoniComposerApplication() = default;

    const juce::String getApplicationName() override       { return "Armoni Composer"; }
    const juce::String getApplicationVersion() override    { return "0.1.0"; }

    bool moreThanOneInstanceAllowed() override             { return true; }

    void initialise(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted(const juce::String& commandLine) override
    {
        juce::ignoreUnused(commandLine);
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(name,
                             juce::Desktop::getInstance().getDefaultLookAndFeel()
                                 .findColour(juce::ResizableWindow::backgroundColourId),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            mainComponent = std::make_unique<MainComponent>();
            setContentNonOwned(mainComponent.get(), true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
           #else
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());
           #endif

            setVisible(true);

           #if JUCE_WINDOWS
            setupNativeHook();
           #endif
        }

        ~MainWindow() override
        {
           #if JUCE_WINDOWS
            removeNativeHook();
           #endif
            clearContentComponent();
        }

        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
       #if JUCE_WINDOWS
        HWND hwnd = nullptr;
        WNDPROC originalWndProc = nullptr;

        static LRESULT CALLBACK SubclassedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            auto* window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            if (msg == WM_INPUT && window != nullptr && window->mainComponent != nullptr)
            {
                window->mainComponent->handleRawInput(reinterpret_cast<void*>(lParam));
            }

            if (window != nullptr && window->originalWndProc != nullptr)
                return CallWindowProc(window->originalWndProc, hWnd, msg, wParam, lParam);

            return DefWindowProc(hWnd, msg, wParam, lParam);
        }

        void setupNativeHook()
        {
            if (auto* peer = getPeer())
            {
                hwnd = static_cast<HWND>(peer->getNativeHandle());
                if (hwnd != nullptr)
                {
                    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
                    originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)SubclassedWndProc);

                    mainComponent->setNativeWindowHandle(hwnd);
                }
            }
        }

        void removeNativeHook()
        {
            if (hwnd != nullptr && originalWndProc != nullptr)
            {
                SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)originalWndProc);
                SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
                hwnd = nullptr;
                originalWndProc = nullptr;
            }
        }

       #endif

        std::unique_ptr<MainComponent> mainComponent;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(ArmoniComposerApplication)
