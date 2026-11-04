// Custom standalone application: a frameless window whose title bar is the plug-in's own header,
// guitar input live by default (no "input is muted" strip), and a settings menu inside the plug-in UI.
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

namespace
{
using namespace juce;

class NoirWindow final : public DocumentWindow
{
public:
    NoirWindow (const String& title, std::unique_ptr<StandalonePluginHolder> h)
        : DocumentWindow (title, Colour (0xff121212), 0), holder (std::move (h))
    {
        setUsingNativeTitleBar (false);
        setTitleBarHeight (0);
        setResizeLimits (1280, 620, 7680, 4320);
        setResizable (true, false);

        auto* proc = dynamic_cast<NoirProcessor*> (holder->processor.get());
        jassert (proc != nullptr);

        host.minimise       = [this] { setMinimised (true); };
        host.toggleMaximise = [this] { toggleMaximised(); };
        host.close          = [this] { closeButtonPressed(); };
        host.audioSettings  = [this] { holder->showAudioSettingsDialog(); };
        host.toggleMute     = [this] { setMuted (! isMuted()); };
        host.isMuted        = [this] { return isMuted(); };
        proc->standalone = &host;

        // The default holder mutes the input to avoid feedback. A guitar player wants it live, so only a user choice mutes it.
        const bool mute = holder->settings != nullptr && holder->settings->getBoolValue ("noirMuteInput", false);
        holder->getMuteInputValue().setValue (mute);

        editor.reset (proc->createEditorIfNeeded());
        setContentNonOwned (editor.get(), true);

        auto bounds = getBounds();
        if (auto* props = holder->settings.get())
        {
            const int w = props->getIntValue ("windowW", 0), hh = props->getIntValue ("windowH", 0);
            if (w >= 1280 && hh >= 620) bounds.setSize (w, hh);
            const int x = props->getIntValue ("windowX", -100000), y = props->getIntValue ("windowY", -100000);
            if (x != -100000 && y != -100000) bounds.setPosition (x, y);
            else bounds = bounds.withCentre (Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea.getCentre());
        }
        const auto& displays = Desktop::getInstance().getDisplays();
        if (auto* d = displays.getDisplayForRect (bounds))
            bounds = bounds.constrainedWithin (d->userArea);
        setBounds (bounds);
    }

    ~NoirWindow() override
    {
        if (auto* props = holder->settings.get())
        {
            props->setValue ("windowX", getX());
            props->setValue ("windowY", getY());
            props->setValue ("windowW", getWidth());
            props->setValue ("windowH", getHeight());
        }

        holder->stopPlaying();
        clearContentComponent();
        if (editor != nullptr)
        {
            holder->processor->editorBeingDeleted (editor.get());
            editor = nullptr;
        }
        if (auto* proc = dynamic_cast<NoirProcessor*> (holder->processor.get()))
            proc->standalone = nullptr;
        holder = nullptr;
    }

    void closeButtonPressed() override
    {
        holder->savePluginState();
        JUCEApplicationBase::quit();
    }

    std::unique_ptr<StandalonePluginHolder> holder;

private:
    bool isMuted() const { return (bool) holder->getMuteInputValue().getValue(); }

    void setMuted (bool m)
    {
        holder->getMuteInputValue().setValue (m);
        if (holder->settings != nullptr) holder->settings->setValue ("noirMuteInput", m);
    }

    void toggleMaximised()
    {
        auto* d = Desktop::getInstance().getDisplays().getDisplayForRect (getBounds());
        if (d == nullptr) return;

        if (getBounds() == d->userArea)
        {
            setBounds (restoreBounds.isEmpty() ? d->userArea.reduced (d->userArea.getWidth() / 10, d->userArea.getHeight() / 10) : restoreBounds);
        }
        else
        {
            restoreBounds = getBounds();
            setBounds (d->userArea);
        }
    }

    NoirProcessor::StandaloneHost host;
    std::unique_ptr<AudioProcessorEditor> editor;
    Rectangle<int> restoreBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirWindow)
};

class NoirStandaloneApp final : public JUCEApplication
{
public:
    NoirStandaloneApp()
    {
        PropertiesFile::Options options;
        options.applicationName     = CharPointer_UTF8 (JucePlugin_Name);
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
       #if JUCE_LINUX || JUCE_BSD
        options.folderName          = "~/.config";
       #else
        options.folderName          = "";
       #endif
        appProperties.setStorageParameters (options);
    }

    const String getApplicationName() override              { return CharPointer_UTF8 (JucePlugin_Name); }
    const String getApplicationVersion() override           { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override              { return true; }
    void anotherInstanceStarted (const String&) override    {}

    void initialise (const String&) override
    {
        auto holder = std::make_unique<StandalonePluginHolder> (appProperties.getUserSettings(), false, String{}, nullptr,
                                                                Array<StandalonePluginHolder::PluginInOuts>{}, false);
        window = std::make_unique<NoirWindow> (getApplicationName(), std::move (holder));
        window->setVisible (true);
    }

    void shutdown() override
    {
        window = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (window != nullptr)
            window->holder->savePluginState();
        quit();
    }

private:
    ApplicationProperties appProperties;
    std::unique_ptr<NoirWindow> window;
};
}

START_JUCE_APPLICATION (NoirStandaloneApp)
