#include "PluginEditor.h"
#include "Presets.h"

namespace
{
juce::Colour cDyn, cFilt, cPitch, cGain, cAmp, cEq, cMod, cLofi, cTime, cVoice, cGrain, cGlit, cLoop, cOut;
juce::Colour slotColour[NumSlots];
const juce::Colour ink { 0xff0b0b0d };   // dark text on bright surfaces

void applyPalette (int theme)
{
    auto C = [] (juce::uint32 v) { return juce::Colour (v); };
    if (theme == 0)   // Pop Stompbox: bold flat colours
    {
        cDyn = C (0xfff8d21b); cFilt = C (0xffb79cff); cPitch = C (0xffff8fc8); cGain = C (0xffff9f6b); cAmp = C (0xff57e26b);
        cEq = C (0xffff8fc8);  cMod = C (0xff1cc8e8);  cLofi = C (0xffc6f135);  cTime = C (0xffb79cff); cVoice = C (0xffffd166);
        cGrain = C (0xff1cc8e8); cGlit = C (0xffff6b5a); cLoop = C (0xffff6b5a); cOut = C (0xfff4f1ea);
    }
    else              // Retro Amp: muted enamel colours
    {
        cDyn = C (0xff3f7f78); cFilt = C (0xff7a5a8f); cPitch = C (0xffa84a6e); cGain = C (0xffb8742a); cAmp = C (0xffa3382f);
        cEq = C (0xff5a5340);  cMod = C (0xff3d6f94);  cLofi = C (0xff6b8a3a);  cTime = C (0xff55608f); cVoice = C (0xff9a5f3a);
        cGrain = C (0xff2f7f8a); cGlit = C (0xffb3342b); cLoop = C (0xffc2982d); cOut = C (0xff6b624d);
    }
    const juce::Colour c[NumSlots] = { cDyn, cDyn, cFilt, cPitch, cGain, cGain, cAmp, cAmp, cEq, cVoice, cVoice,
                                       cMod, cMod, cLofi, cGlit, cGrain, cGrain, cTime, cTime, cLoop };
    for (int i = 0; i < NumSlots; ++i) slotColour[i] = c[i];
}

constexpr int kHeader = 42;   // pedal header height

using L = NoirLookAndFeel;
}

//==============================================================================
NoirLookAndFeel::NoirLookAndFeel() { applyTheme (0); }

void NoirLookAndFeel::applyTheme (int t)
{
    theme = t;
    if (t == 0)
    {
        bg = juce::Colour (0xff121212); panel = juce::Colour (0xff171717); line = juce::Colour (0xff2c2c2c);
        text = juce::Colour (0xfff4f1ea); dim = juce::Colour (0xff9a968d); accent = juce::Colour (0xffff9f6b);
        inset = juce::Colour (0xff141414); insetLine = juce::Colour (0xff2c2c2c);
        hdrText = text; hdrDim = dim; gold = juce::Colour (0xffff9f6b);
    }
    else
    {
        bg = juce::Colour (0xffd9cfb8); panel = juce::Colour (0xffefe6d2); line = juce::Colour (0xffbfb396);
        text = juce::Colour (0xff1d1a15); dim = juce::Colour (0xff6b624d); accent = juce::Colour (0xffc9923a);
        inset = juce::Colour (0xff1c1a17); insetLine = juce::Colour (0xff4a443a);
        hdrText = juce::Colour (0xffefe6d2); hdrDim = juce::Colour (0xffa89c83); gold = juce::Colour (0xffd9b26a);
    }
    applyPalette (t);

    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::ComboBox::textColourId, hdrText);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::headerTextColourId, dim);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, line);
    setColour (juce::PopupMenu::highlightedTextColourId, text);
    setColour (juce::TextButton::textColourOffId, hdrDim);
    setColour (juce::TextButton::textColourOnId, hdrText);
    setColour (juce::ListBox::textColourId, text);
    setColour (juce::ScrollBar::thumbColourId, dim.withAlpha (0.55f));
    setColour (juce::TextEditor::backgroundColourId, inset);
    setColour (juce::TextEditor::textColourId, hdrText);
    setColour (juce::TextEditor::outlineColourId, insetLine);
}

void NoirLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto fill = s.findColour (juce::Slider::rotarySliderFillColourId);
    const bool hot = s.isMouseOverOrDragging();
    const float amt = (float) (double) s.getProperties().getWithDefault ("amt", 1.0);   // 1 = pedal is on (filled surface)
    const float kr = radius * 0.72f;

    if (theme == 0)
    {
        const auto tickCol = fill.interpolatedWith (ink, amt);
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float r1 = radius * 0.90f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.95f);
            g.setColour (tickCol.withAlpha (a <= angle + 0.001f ? 0.9f : 0.35f));
            g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 1.4f);
        }
        g.setColour (juce::Colour (hot ? 0xff1d1d1d : 0xff121212));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (fill.withAlpha (1.0f - amt));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.8f);
        juce::Path tick;
        tick.startNewSubPath (c.x + std::sin (angle) * kr * 0.20f, c.y - std::cos (angle) * kr * 0.20f);
        tick.lineTo (c.x + std::sin (angle) * kr * 0.90f, c.y - std::cos (angle) * kr * 0.90f);
        g.setColour (fill.interpolatedWith (juce::Colour (0xfff4f1ea), amt));
        g.strokePath (tick, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else   // brass knob with a dark cap
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const float r1 = radius * 0.90f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.95f);
            g.setColour (text.withAlpha (a <= angle + 0.001f ? 0.8f : 0.3f));
            g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 1.3f);
        }
        g.setColour (juce::Colour (0x40000000));
        g.fillEllipse (c.x - kr, c.y - kr + 2.0f, kr * 2.0f, kr * 2.0f);
        juce::ColourGradient brass (juce::Colour (hot ? 0xfff3dfa8 : 0xffe9cf94), c.x - kr, c.y - kr,
                                    juce::Colour (0xff8a6a2e), c.x + kr, c.y + kr, false);
        g.setGradientFill (brass);
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (juce::Colour (0xff5a4620));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.2f);
        const float cr = kr * 0.62f;
        juce::ColourGradient cap (juce::Colour (0xff4a443a), c.x - cr * 0.5f, c.y - cr * 0.6f, juce::Colour (0xff14120f), c.x + cr, c.y + cr, false);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
        juce::Path tick;
        tick.startNewSubPath (c.x + std::sin (angle) * kr * 0.60f, c.y - std::cos (angle) * kr * 0.60f);
        tick.lineTo (c.x + std::sin (angle) * kr * 0.96f, c.y - std::cos (angle) * kr * 0.96f);
        g.setColour (juce::Colour (0xfff6ecd0));
        g.strokePath (tick, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void NoirLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle, juce::Slider& s)
{
    const auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cy = r.getCentreY();
    g.setColour (line);
    g.fillRoundedRectangle (r.getX(), cy - 2.0f, r.getWidth(), 4.0f, 2.0f);
    g.setColour (s.findColour (juce::Slider::trackColourId, true).isTransparent() ? accent : accent);
    g.fillRoundedRectangle (r.getX(), cy - 2.0f, pos - r.getX(), 4.0f, 2.0f);
    g.fillEllipse (pos - 6.0f, cy - 6.0f, 12.0f, 12.0f);
}

void NoirLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const bool on = b.getToggleState();

    if (b.getProperties().contains ("invisible"))
        return;

    if (b.getProperties().contains ("power"))
    {
        const auto c = r.reduced (1.0f);
        const auto pc = theme == 0 ? juce::Colour (0xffff6b5a) : juce::Colour (0xffb3342b);
        const float t = (float) (juce::Time::getMillisecondCounterHiRes() * 0.0025);
        if (on)
        {
            g.setColour (pc.withAlpha (0.16f + 0.08f * std::sin (t)));
            g.fillRoundedRectangle (c.expanded (3.0f), c.getHeight() * 0.5f + 3.0f);
        }
        g.setColour (on ? pc : inset);
        g.fillRoundedRectangle (c, c.getHeight() * 0.5f);
        g.setColour (on ? pc : insetLine);
        g.drawRoundedRectangle (c, c.getHeight() * 0.5f, 1.4f);
        g.setColour (on ? juce::Colour (0xfffff5e6) : (hl ? hdrText : hdrDim));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (on ? "POWER ON" : "BYPASSED", c.toNearestInt(), juce::Justification::centred);
        return;
    }

    if (b.getProperties().contains ("label"))   // small labelled switch
    {
        const auto col = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) cLoop.getARGB()));
        g.setColour (on ? col : inset);
        g.fillRoundedRectangle (r.reduced (0.5f), 5.0f);
        g.setColour (on ? col : insetLine);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (on ? onAccent (col) : (hl ? hdrText : hdrDim));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
        return;
    }

    // checkbox style (DAW tab etc.)
    const auto box = juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ 14.0f, r.getCentreY() });
    g.setColour (on ? accent : juce::Colour (0xff0f0f12));
    g.fillRoundedRectangle (box, 4.0f);
    g.setColour (on ? accent : line.brighter (0.2f));
    g.drawRoundedRectangle (box, 4.0f, 1.2f);
    if (on)
    {
        juce::Path tick;
        tick.startNewSubPath (box.getX() + 3.5f, box.getCentreY());
        tick.lineTo (box.getCentreX() - 0.5f, box.getBottom() - 4.0f);
        tick.lineTo (box.getRight() - 3.0f, box.getY() + 4.0f);
        g.setColour (ink);
        g.strokePath (tick, juce::PathStrokeType (2.0f));
    }
    g.setColour (hl ? text : text.withAlpha (0.85f));
    g.setFont (juce::FontOptions (12.5f));
    g.drawText (b.getButtonText(), 30, 0, b.getWidth() - 30, b.getHeight(), juce::Justification::centredLeft, true);
}

void NoirLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down)
{
    const auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool lit = b.getToggleState();
    const auto col = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) accent.getARGB()));
    g.setColour (lit ? col : (down ? insetLine : (hl ? inset.brighter (0.12f) : inset)));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (lit ? col : insetLine.brighter (hl ? 0.25f : 0.0f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
    if (lit) b.setColour (juce::TextButton::textColourOnId, onAccent (col));
}

void NoirLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (inset);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.isMouseOver() ? insetLine.brighter (0.3f) : insetLine);
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 3.5f, cy - 1.5f, cx + 3.5f, cy - 1.5f, cx, cy + 2.5f);
    g.setColour (hdrDim);
    g.fillPath (arrow);
}

//==============================================================================
void TunerView::setReading (float freqHz, float refA4)
{
    bool nowValid = freqHz > 20.0f;
    juce::String nn = "--";
    float c = 0.0f;

    if (nowValid)
    {
        static const char* names[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const float midi = 69.0f + 12.0f * std::log2 (freqHz / refA4);
        const int n = (int) std::lround (midi);
        c = (midi - (float) n) * 100.0f;
        nn = juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
    }

    if (nowValid != valid || nn != note || std::abs (c - cents) > 0.4f)
    {
        valid = nowValid; note = nn; cents = c; hz = freqHz;
        repaint();
    }
}

void TunerView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (L::inset);
    g.fillRoundedRectangle (r, 8.0f);
    const bool inTune = valid && std::abs (cents) < 5.0f;
    const auto col = ! valid ? L::hdrDim : (inTune ? juce::Colour (0xff6cf06c) : L::accent);

    g.setColour (col);
    g.setFont (juce::FontOptions (r.getHeight() * 0.50f, juce::Font::bold));
    g.drawText (note, r.removeFromTop (r.getHeight() * 0.58f).toNearestInt(), juce::Justification::centred);

    auto bar = r.reduced (8.0f, 0.0f).withTrimmedBottom (4.0f);
    const float y = bar.getCentreY();
    g.setColour (L::insetLine);
    g.drawLine (bar.getX(), y, bar.getRight(), y, 1.5f);
    for (int i = -5; i <= 5; ++i)
    {
        const float x = bar.getCentreX() + (float) i / 5.0f * bar.getWidth() * 0.5f;
        g.drawLine (x, y - (i == 0 ? 6.0f : 3.0f), x, y + (i == 0 ? 6.0f : 3.0f), 1.0f);
    }
    if (valid)
    {
        const float x = bar.getCentreX() + juce::jlimit (-50.0f, 50.0f, cents) / 50.0f * bar.getWidth() * 0.5f;
        g.setColour (col);
        g.fillEllipse (x - 5.0f, y - 5.0f, 10.0f, 10.0f);
    }
}

//==============================================================================
void LevelMeter::push (float peak, float dt)
{
    const float db = juce::Decibels::gainToDecibels (peak, -90.0f);
    level = juce::jmax (db, level - 55.0f * dt);
    if (db >= hold) { hold = db; holdTime = 0.9f; }
    else if ((holdTime -= dt) < 0.0f) hold = juce::jmax (-90.0f, hold - 25.0f * dt);

    sinceSignal = peak > 0.0003f ? 0.0f : sinceSignal + dt;     // about -70 dBFS
    clipTime = peak >= 0.97f ? 0.0f : clipTime + dt;
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (L::hdrDim);
    g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
    g.drawText (name, r.removeFromLeft (28.0f).toNearestInt(), juce::Justification::centredLeft);

    auto status = r.removeFromRight (74.0f);
    r.removeFromRight (6.0f);

    const bool clip = clipTime < 1.2f, sig = hasSignal();
    const auto lampCol = clip ? juce::Colour (0xffff4d4d) : (sig ? juce::Colour (0xff4be07a) : juce::Colour (0xff4a4a52));
    const auto lamp = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ status.getX() + 5.0f, status.getCentreY() });
    if (sig || clip)
    {
        g.setColour (lampCol.withAlpha (0.25f));
        g.fillEllipse (lamp.expanded (3.0f));
    }
    g.setColour (lampCol);
    g.fillEllipse (lamp);
    g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
    g.setColour (clip ? lampCol : (sig ? juce::Colour (0xffa9e8bd) : L::hdrDim));
    g.drawText (clip ? "CLIP" : (sig ? "SIGNAL" : "NO SIGNAL"), status.withTrimmedLeft (14.0f).toNearestInt(), juce::Justification::centredLeft);

    const int segs = 30;
    const float segW = r.getWidth() / (float) segs;
    const auto bar = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (9.0f, r.getHeight()));
    for (int i = 0; i < segs; ++i)
    {
        const float db = -60.0f + 60.0f * (float) i / (float) (segs - 1);
        const bool lit = level >= db;
        juce::Colour c = db > -3.0f ? juce::Colour (0xffff4d4d) : (db > -12.0f ? juce::Colour (0xfff7d21e) : juce::Colour (0xff4be07a));
        g.setColour (lit ? c : L::insetLine);
        g.fillRoundedRectangle (bar.getX() + (float) i * segW + 0.5f, bar.getY(), segW - 1.5f, bar.getHeight(), 1.2f);
    }
    if (hold > -60.0f)
    {
        const float f = juce::jlimit (0.0f, 1.0f, (hold + 60.0f) / 60.0f);
        g.setColour (L::hdrText.withAlpha (0.8f));
        g.fillRect (bar.getX() + f * (bar.getWidth() - segW), bar.getY() - 1.0f, 2.0f, bar.getHeight() + 2.0f);
    }
}

//==============================================================================
LooperView::LooperView (NoirProcessor& p) : proc (p)
{
    for (auto* b : { &rec, &play, &dub, &stop, &undo, &clear, &stopAll })
    {
        addAndMakeVisible (b);
        b->getProperties().set ("accent", (int) cLoop.getARGB());
    }
    rec.getProperties().set ("accent", (int) cGlit.getARGB());
    play.getProperties().set ("accent", (int) juce::Colour (0xff4be07a).getARGB());
    dub.getProperties().set ("accent", (int) cAmp.getARGB());

    rev.setButtonText ("REVERSE");
    rev.getProperties().set ("label", true);
    addAndMakeVisible (rev);
    revAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[LoopRev].id, rev);

    rec.onClick   = [this] { proc.loopRequest (selected, NoirProcessor::LcRecord); };
    play.onClick  = [this] { proc.loopRequest (selected, NoirProcessor::LcPlay); };
    dub.onClick   = [this] { proc.loopRequest (selected, NoirProcessor::LcOverdub); };
    stop.onClick  = [this] { proc.loopRequest (selected, NoirProcessor::LcStop); };
    undo.onClick  = [this] { proc.loopRequest (selected, NoirProcessor::LcUndo); };
    clear.onClick = [this] { proc.loopRequest (selected, NoirProcessor::LcClear); };
    stopAll.onClick = [this] { proc.loopRequest (selected, NoirProcessor::LcStopAll); };
}

juce::Rectangle<int> LooperView::rowBounds (int t) const
{
    const int h = rows.getHeight() / 4;
    return { rows.getX(), rows.getY() + t * h, rows.getWidth(), h };
}

void LooperView::resized()
{
    auto a = getLocalBounds();
    auto right = a.removeFromRight (juce::jmin (176, a.getWidth() * 2 / 5));
    a.removeFromRight (8);
    rows = a;

    const int rh = right.getHeight() / 4;
    auto r1 = right.removeFromTop (rh).reduced (0, 2), r2 = right.removeFromTop (rh).reduced (0, 2);
    auto r3 = right.removeFromTop (rh).reduced (0, 2), r4 = right.reduced (0, 2);
    auto three = [] (juce::Rectangle<int> row, juce::Component& x, juce::Component& y, juce::Component& z)
    {
        const int w = row.getWidth() / 3;
        x.setBounds (row.removeFromLeft (w).reduced (2, 0));
        y.setBounds (row.removeFromLeft (w).reduced (2, 0));
        z.setBounds (row.reduced (2, 0));
    };
    three (r1, rec, play, dub);
    three (r2, stop, undo, clear);
    stopAll.setBounds (r3.removeFromLeft (r3.getWidth() / 2).reduced (2, 0));
    rev.setBounds (r3.reduced (2, 0));
    (void) r4;
}

void LooperView::refresh()
{
    for (auto* b : { &rec, &play, &dub, &stop, &undo, &clear, &stopAll }) b->getProperties().set ("accent", (int) cLoop.getARGB());
    rec.getProperties().set ("accent", (int) cGlit.getARGB());
    play.getProperties().set ("accent", (int) juce::Colour (0xff4be07a).getARGB());
    dub.getProperties().set ("accent", (int) cAmp.getARGB());
    selected = proc.selectedLoopTrack();
    const int st = proc.loopState[selected].load();
    rec.setToggleState (st == 4, juce::dontSendNotification);
    play.setToggleState (st == 2, juce::dontSendNotification);
    dub.setToggleState (st == 3, juce::dontSendNotification);
    stop.setToggleState (st == 1, juce::dontSendNotification);
    undo.setEnabled (proc.loopUndoOk[selected].load());
    spin = std::fmod ((float) juce::Time::getMillisecondCounterHiRes() * 0.0012f, 1.0f);
    repaint();
}

void LooperView::mouseDown (const juce::MouseEvent& e)
{
    for (int t = 0; t < 4; ++t)
    {
        const auto rb = rowBounds (t);
        if (! rb.contains (e.getPosition())) continue;
        if (e.x > rb.getRight() - 30)
        {
            const Id id = (Id) (LoopMute1 + t);
            if (auto* p = proc.param (id)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f); p->endChangeGesture(); }
        }
        else
            proc.setParamValue (LoopTrack, (float) t);
        return;
    }
}

void LooperView::paint (juce::Graphics& g)
{
    static const char* names[5] = { "EMPTY", "STOPPED", "PLAYING", "OVERDUB", "RECORDING" };
    const juce::Colour cols[5] = { L::hdrDim, L::hdrDim.brighter (0.2f), juce::Colour (0xff4be07a), cAmp, cGlit };
    const float prog = proc.loopProgress.load();
    g.setColour (L::inset);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);

    for (int t = 0; t < 4; ++t)
    {
        const auto rb = rowBounds (t).reduced (0, 2).toFloat();
        const int st = proc.loopState[t].load();
        const bool sel = t == selected;
        const bool muted = proc.apvts.getRawParameterValue (kParams[LoopMute1 + t].id)->load() > 0.5f;
        const auto col = cols[juce::jlimit (0, 4, st)];

        g.setColour (sel ? cLoop.withAlpha (0.14f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (rb, 6.0f);
        g.setColour (sel ? cLoop : L::insetLine);
        g.drawRoundedRectangle (rb.reduced (0.5f), 6.0f, sel ? 1.4f : 1.0f);

        const auto badge = juce::Rectangle<float> (18.0f, 18.0f).withCentre ({ rb.getX() + 16.0f, rb.getCentreY() });
        g.setColour (sel ? cLoop : L::insetLine.brighter (0.2f));
        g.fillEllipse (badge);
        g.setColour (sel ? ink : L::hdrText);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (t + 1), badge.toNearestInt(), juce::Justification::centred);

        // state lamp + text
        const auto lamp = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ rb.getX() + 40.0f, rb.getCentreY() });
        float pulse = 1.0f;
        if (st == 4) pulse = 0.55f + 0.45f * std::sin (spin * 6.2832f * 3.0f);
        g.setColour (col.withAlpha (pulse));
        g.fillEllipse (lamp);
        g.setColour (st == 0 ? L::hdrDim : col);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (names[juce::jlimit (0, 4, st)], (int) rb.getX() + 50, (int) rb.getY(), 76, (int) rb.getHeight(), juce::Justification::centredLeft);

        // progress
        auto bar = rb.withTrimmedLeft (128.0f).withTrimmedRight (34.0f);
        bar = bar.withSizeKeepingCentre (bar.getWidth(), 5.0f);
        g.setColour (L::insetLine);
        g.fillRoundedRectangle (bar, 2.5f);
        if (st >= 1 && st != 4)
        {
            g.setColour ((muted ? L::hdrDim : col).withAlpha (0.9f));
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * prog), 2.5f);
            g.fillEllipse (bar.getX() + bar.getWidth() * prog - 4.0f, bar.getCentreY() - 4.0f, 8.0f, 8.0f);
        }

        // mute
        const auto m = juce::Rectangle<float> (22.0f, 18.0f).withCentre ({ rb.getRight() - 18.0f, rb.getCentreY() });
        g.setColour (muted ? cGlit : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (m, 4.0f);
        g.setColour (muted ? cGlit : L::insetLine);
        g.drawRoundedRectangle (m, 4.0f, 1.0f);
        g.setColour (muted ? ink : L::hdrDim);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("M", m.toNearestInt(), juce::Justification::centred);
    }
}

//==============================================================================
namespace
{
constexpr float kDbRange = 18.0f;
constexpr int kCtlH = 92;

juce::Colour eqBandColour (int b)
{
    static const juce::uint32 pop[8]   = { 0xffff6b5a, 0xffff9f6b, 0xfff8d21b, 0xffc6f135, 0xff57e26b, 0xff1cc8e8, 0xffb79cff, 0xffff8fc8 };
    static const juce::uint32 retro[8] = { 0xffe07a4f, 0xffe9a23b, 0xffd9c34a, 0xff9fc15a, 0xff5fb59a, 0xff5aa3d0, 0xff9a86d6, 0xffd67aa8 };
    return juce::Colour ((L::theme == 0 ? pop : retro)[juce::jlimit (0, 7, b)]);
}
}

juce::Colour EqPanel::bandColour (int b) const { return eqBandColour (b); }

juce::String EqPanel::fmtFreq (float f)
{
    return f >= 1000.0f ? juce::String (f / 1000.0f, f >= 10000.0f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (f)) + " Hz";
}

EqPanel::EqPanel (NoirProcessor& p) : proc (p), fftBuf ((size_t) N * 2, 0.0f), spec ((size_t) Cols, -120.0f)
{
    typeBox.addItemList (juce::StringArray::fromTokens (kParams[eqParam (0, EqfType)].choices, "|", ""), 1);
    addAndMakeVisible (typeBox);

    for (auto* s : { &freqS, &gainS, &qS })
    {
        s->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 15);
        s->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        s->setMouseDragSensitivity (180);
        s->setScrollWheelEnabled (false);
        s->getProperties().set ("amt", 0.0);
        addAndMakeVisible (s);
    }

    freqLabel.setText ("FREQ", juce::dontSendNotification);
    gainLabel.setText ("GAIN", juce::dontSendNotification);
    qLabel.setText ("Q", juce::dontSendNotification);
    for (auto* l : { &freqLabel, &gainLabel, &qLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setFont (juce::FontOptions (9.5f, juce::Font::bold));
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (l);
    }

    bandLabel.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    bandLabel.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (bandLabel);

    onBtn.setButtonText ("BAND ON");
    onBtn.getProperties().set ("label", true);
    addAndMakeVisible (onBtn);

    analyzerBtn.setButtonText ("ANALYZER");
    analyzerBtn.getProperties().set ("label", true);
    analyzerBtn.getProperties().set ("accent", (int) juce::Colour (0xff9aa3ad).getARGB());
    analyzerBtn.setToggleState (true, juce::dontSendNotification);
    analyzerBtn.onClick = [this] { analyzerOn = analyzerBtn.getToggleState(); repaint(); };
    addAndMakeVisible (analyzerBtn);

    selectBand (0);
    startTimerHz (30);
}

EqPanel::~EqPanel() { stopTimer(); }

void EqPanel::visibilityChanged() {}

void EqPanel::refreshTheme() { selectBand (selected); }

void EqPanel::selectBand (int b)
{
    selected = juce::jlimit (0, noir::ProEq::N - 1, b);
    typeAtt.reset(); freqAtt.reset(); gainAtt.reset(); qAtt.reset(); onAtt.reset();

    auto& st = proc.apvts;
    typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, kParams[eqParam (selected, EqfType)].id, typeBox);
    freqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, kParams[eqParam (selected, EqfFreq)].id, freqS);
    gainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, kParams[eqParam (selected, EqfGain)].id, gainS);
    qAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, kParams[eqParam (selected, EqfQ)].id, qS);
    onAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, kParams[eqParam (selected, EqfOn)].id, onBtn);

    const auto col = bandColour (selected);
    for (auto* s : { &freqS, &gainS, &qS })
    {
        s->setColour (juce::Slider::rotarySliderFillColourId, col);
        s->setColour (juce::Slider::textBoxTextColourId, L::hdrText);
        s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    }
    for (auto* l : { &freqLabel, &gainLabel, &qLabel }) l->setColour (juce::Label::textColourId, L::hdrDim);
    bandLabel.setColour (juce::Label::textColourId, col);
    bandLabel.setText ("BAND " + juce::String (selected + 1), juce::dontSendNotification);
    onBtn.getProperties().set ("accent", (int) col.getARGB());
    lastType = -1;
    repaint();
}

float EqPanel::xForFreq (float f) const
{
    return graph.getX() + graph.getWidth() * std::log (juce::jlimit (20.0f, 20000.0f, f) / 20.0f) / std::log (1000.0f);
}
float EqPanel::freqForX (float x) const
{
    return 20.0f * std::pow (1000.0f, juce::jlimit (0.0f, 1.0f, (x - graph.getX()) / graph.getWidth()));
}
float EqPanel::yForGain (float g) const
{
    return graph.getCentreY() - juce::jlimit (-kDbRange, kDbRange, g) / kDbRange * graph.getHeight() * 0.5f;
}
float EqPanel::gainForY (float y) const
{
    return juce::jlimit (-kDbRange, kDbRange, (graph.getCentreY() - y) / (graph.getHeight() * 0.5f) * kDbRange);
}

float EqPanel::getP (int band, int field) const
{
    return proc.apvts.getRawParameterValue (kParams[eqParam (band, field)].id)->load();
}

void EqPanel::setP (int band, int field, float plain)
{
    if (auto* p = proc.apvts.getParameter (kParams[eqParam (band, field)].id))
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
}

void EqPanel::gesture (int band, bool begin)
{
    for (int f : { (int) EqfFreq, (int) EqfGain, (int) EqfQ })
        if (auto* p = proc.apvts.getParameter (kParams[eqParam (band, f)].id))
        {
            if (begin) p->beginChangeGesture(); else p->endChangeGesture();
        }
}

juce::Rectangle<float> EqPanel::chipRect (int i) const
{
    return { (float) ctl.getX() + 12.0f + (float) (i % 4) * 29.0f, (float) ctl.getY() + 20.0f + (float) (i / 4) * 33.0f, 25.0f, 25.0f };
}

int EqPanel::bandAt (juce::Point<float> pt) const
{
    int best = -1;
    float bd = 13.0f;
    for (int b = 0; b < noir::ProEq::N; ++b)
    {
        if (getP (b, EqfOn) < 0.5f) continue;
        const int t = (int) getP (b, EqfType);
        const juce::Point<float> c (xForFreq (getP (b, EqfFreq)), yForGain (usesGain (t) ? getP (b, EqfGain) : 0.0f));
        float d = c.getDistanceFrom (pt);
        if (b == selected) d -= 2.0f;
        if (d < bd) { bd = d; best = b; }
    }
    return best;
}

void EqPanel::resized()
{
    auto a = getLocalBounds();
    ctl = a.removeFromBottom (kCtlH);
    graph = a.toFloat().withTrimmedLeft (34.0f).withTrimmedRight (12.0f).withTrimmedTop (12.0f).withTrimmedBottom (18.0f);

    auto c = ctl.reduced (0, 6);
    c.removeFromLeft (132);
    auto col = c.removeFromLeft (118);
    bandLabel.setBounds (col.removeFromTop (20));
    typeBox.setBounds (col.removeFromTop (26));
    col.removeFromTop (6);
    onBtn.setBounds (col.removeFromTop (24));

    c.removeFromLeft (10);
    analyzerBtn.setBounds (c.removeFromRight (92).removeFromTop (24).translated (-6, 8));
    const int w = juce::jmin (92, c.getWidth() / 3);
    auto place = [&c, w] (juce::Slider& sl, juce::Label& lb)
    {
        auto k = c.removeFromLeft (w);
        lb.setBounds (k.removeFromTop (14));
        sl.setBounds (k);
    };
    place (freqS, freqLabel);
    place (gainS, gainLabel);
    place (qS, qLabel);
}

void EqPanel::timerCallback()
{
    const int t = (int) getP (selected, EqfType);
    if (t != lastType)
    {
        lastType = t;
        const bool g = usesGain (t);
        gainS.setEnabled (g);
        gainS.setAlpha (g ? 1.0f : 0.3f);
        gainLabel.setAlpha (g ? 1.0f : 0.3f);
    }

    if (! isShowing()) return;
    phase += 0.033f;

    const double sr = proc.getSampleRateHz() > 1000.0 ? proc.getSampleRateHz() : 48000.0;
    const float binHz = (float) sr / (float) N;

    if (analyzerOn)
    {
        proc.getEqTap().copyLatest (fftBuf.data(), N);
        std::fill (fftBuf.begin() + N, fftBuf.end(), 0.0f);
        window.multiplyWithWindowingTable (fftBuf.data(), (size_t) N);
        fft.performFrequencyOnlyForwardTransform (fftBuf.data());

        auto mag = [this] (int k) { return fftBuf[(size_t) juce::jlimit (1, N / 2 - 1, k)]; };
        for (int c = 0; c < Cols; ++c)
        {
            const float f0 = 20.0f * std::pow (1000.0f, ((float) c - 0.5f) / (float) (Cols - 1));
            const float f1 = 20.0f * std::pow (1000.0f, ((float) c + 0.5f) / (float) (Cols - 1));
            const float fc = std::sqrt (f0 * f1);
            const float lo = f0 / binHz, hi = f1 / binHz;
            float m;
            if (hi - lo < 1.0f)
            {
                const float kf = fc / binHz;
                const int k = (int) std::floor (kf);
                m = mag (k) + (mag (k + 1) - mag (k)) * (kf - (float) k);
            }
            else
            {
                m = 0.0f;
                for (int k = (int) std::ceil (lo); k <= (int) std::floor (hi); ++k) m = juce::jmax (m, mag (k));
            }
            const float db = 20.0f * std::log10 (m * 4.0f / (float) N + 1.0e-7f) + 4.5f * std::log2 (fc / 1000.0f);
            float& s = spec[(size_t) c];
            s += (db - s) * (db > s ? 0.55f : 0.10f);
        }
    }
    else
        for (auto& s : spec) s = juce::jmax (-120.0f, s - 3.0f);

    repaint();
}

void EqPanel::paint (juce::Graphics& g)
{
    const auto all = getLocalBounds().toFloat();
    g.setColour (L::inset);
    g.fillRoundedRectangle (all, 8.0f);

    const double sr = proc.getSampleRateHz() > 1000.0 ? proc.getSampleRateHz() : 48000.0;
    const bool retro = L::theme == 1;

    // ---- grid
    g.setFont (juce::FontOptions (9.0f));
    static const float fl[] = { 20, 30, 40, 50, 60, 80, 100, 200, 300, 400, 500, 600, 800, 1000, 2000, 3000, 4000, 5000, 6000, 8000, 10000, 20000 };
    for (float f : fl)
    {
        const float x = xForFreq (f);
        const bool major = f == 100 || f == 1000 || f == 10000;
        g.setColour (L::insetLine.withAlpha (major ? 0.9f : 0.45f));
        g.drawVerticalLine (juce::roundToInt (x), graph.getY(), graph.getBottom());
        if (major || f == 20 || f == 20000 || f == 500 || f == 5000 || f == 50 || f == 200 || f == 2000)
        {
            g.setColour (L::hdrDim);
            g.drawText (f >= 1000 ? juce::String ((int) (f / 1000)) + "k" : juce::String ((int) f),
                        juce::Rectangle<float> (x - 18.0f, graph.getBottom() + 3.0f, 36.0f, 12.0f).toNearestInt(), juce::Justification::centred);
        }
    }
    for (int d = -18; d <= 18; d += 6)
    {
        const float y = yForGain ((float) d);
        g.setColour (d == 0 ? L::hdrDim.withAlpha (0.55f) : L::insetLine.withAlpha (0.7f));
        g.drawHorizontalLine (juce::roundToInt (y), graph.getX(), graph.getRight());
        g.setColour (L::hdrDim);
        g.drawText ((d > 0 ? "+" : "") + juce::String (d), juce::Rectangle<float> (0.0f, y - 6.0f, graph.getX() - 4.0f, 12.0f).toNearestInt(), juce::Justification::centredRight);
    }

    // ---- analyser
    if (analyzerOn)
    {
        juce::Path fill, line;
        for (int c = 0; c < Cols; ++c)
        {
            const float x = graph.getX() + graph.getWidth() * (float) c / (float) (Cols - 1);
            const float y = graph.getBottom() - juce::jlimit (0.0f, 1.0f, (spec[(size_t) c] + 100.0f) / 80.0f) * graph.getHeight();
            if (c == 0) { line.startNewSubPath (x, y); fill.startNewSubPath (x, graph.getBottom()); fill.lineTo (x, y); }
            else { line.lineTo (x, y); fill.lineTo (x, y); }
        }
        fill.lineTo (graph.getRight(), graph.getBottom());
        fill.closeSubPath();
        const auto sc = retro ? juce::Colour (0xffe9a23b) : juce::Colour (0xff8fa6c9);
        juce::ColourGradient grad (sc.withAlpha (0.38f), 0.0f, graph.getY(), sc.withAlpha (0.04f), 0.0f, graph.getBottom(), false);
        g.setGradientFill (grad);
        g.fillPath (fill);
        g.setColour (sc.withAlpha (0.65f));
        g.strokePath (line, juce::PathStrokeType (1.2f));
    }

    // ---- response
    noir::Biquad bq[noir::ProEq::N];
    bool on[noir::ProEq::N];
    int type[noir::ProEq::N];
    for (int b = 0; b < noir::ProEq::N; ++b)
    {
        on[b] = getP (b, EqfOn) > 0.5f;
        type[b] = (int) getP (b, EqfType);
        if (on[b]) noir::eqCoeffs (bq[b], type[b], sr, getP (b, EqfFreq), getP (b, EqfGain), getP (b, EqfQ));
    }

    juce::Path total, sel;
    const float zeroY = yForGain (0.0f);
    for (int c = 0; c < Cols; ++c)
    {
        const float x = graph.getX() + graph.getWidth() * (float) c / (float) (Cols - 1);
        const double f = 20.0 * std::pow (1000.0, (double) c / (double) (Cols - 1));
        double sum = 0.0;
        for (int b = 0; b < noir::ProEq::N; ++b)
            if (on[b]) sum += noir::biquadMagDb (bq[b], sr, f);
        const float y = yForGain ((float) sum);
        if (c == 0) total.startNewSubPath (x, y); else total.lineTo (x, y);

        if (on[selected])
        {
            const float ys = yForGain ((float) noir::biquadMagDb (bq[selected], sr, f));
            if (c == 0) { sel.startNewSubPath (x, zeroY); sel.lineTo (x, ys); } else sel.lineTo (x, ys);
        }
    }
    if (on[selected])
    {
        sel.lineTo (graph.getRight(), zeroY);
        sel.closeSubPath();
        g.setColour (bandColour (selected).withAlpha (0.26f));
        g.fillPath (sel);
    }
    g.setColour (L::hdrText);
    g.strokePath (total, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // ---- nodes
    for (int b = 0; b < noir::ProEq::N; ++b)
    {
        if (! on[b]) continue;
        const juce::Point<float> c (xForFreq (getP (b, EqfFreq)), yForGain (usesGain (type[b]) ? getP (b, EqfGain) : 0.0f));
        const auto col = bandColour (b);
        const float r = (b == hover || b == dragBand) ? 9.0f : 7.5f;
        g.setColour (col.withAlpha (0.22f));
        g.fillEllipse (c.x - r - 4.0f, c.y - r - 4.0f, (r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f);
        g.setColour (col);
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);
        if (b == selected)
        {
            g.setColour (L::hdrText);
            g.drawEllipse (c.x - r - 2.5f, c.y - r - 2.5f, (r + 2.5f) * 2.0f, (r + 2.5f) * 2.0f, 1.6f);
        }
        g.setColour (L::onAccent (col));
        g.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        g.drawText (juce::String (b + 1), juce::Rectangle<float> (c.x - 8.0f, c.y - 6.0f, 16.0f, 12.0f).toNearestInt(), juce::Justification::centred);
    }

    // ---- read-out of the hovered / dragged band
    const int info = dragBand >= 0 ? dragBand : hover;
    if (info >= 0 && on[info])
    {
        const float f = getP (info, EqfFreq), q = getP (info, EqfQ);
        juce::String txt = fmtFreq (f);
        if (usesGain (type[info])) txt << "   " << (getP (info, EqfGain) >= 0 ? "+" : "") << juce::String (getP (info, EqfGain), 1) << " dB";
        txt << "   Q " << juce::String (q, 2);
        const juce::Point<float> c (xForFreq (f), yForGain (usesGain (type[info]) ? getP (info, EqfGain) : 0.0f));
        auto box = juce::Rectangle<float> (txt.length() * 6.6f + 14.0f, 18.0f);
        box.setCentre (c.x, c.y - 24.0f);
        box = box.constrainedWithin (graph.expanded (6.0f));
        g.setColour (juce::Colour (0xe0000000));
        g.fillRoundedRectangle (box, 5.0f);
        g.setColour (bandColour (info));
        g.drawRoundedRectangle (box, 5.0f, 1.0f);
        g.setColour (juce::Colour (0xfff4f1ea));
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (txt, box.toNearestInt(), juce::Justification::centred);
    }

    // ---- control strip
    g.setColour (L::insetLine);
    g.drawHorizontalLine (ctl.getY(), 10.0f, (float) getWidth() - 10.0f);
    g.setColour (L::hdrDim);
    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("BANDS", (int) chipRect (0).getX(), ctl.getY() + 5, 80, 12, juce::Justification::centredLeft);
    for (int b = 0; b < noir::ProEq::N; ++b)
    {
        const auto r = chipRect (b);
        const auto col = bandColour (b);
        g.setColour (on[b] ? col : juce::Colours::transparentBlack);
        g.fillEllipse (r);
        g.setColour (b == selected ? L::hdrText : (on[b] ? col : L::insetLine.brighter (0.25f)));
        g.drawEllipse (r.reduced (b == selected ? 0.0f : 0.5f), b == selected ? 2.0f : 1.2f);
        g.setColour (on[b] ? L::onAccent (col) : L::hdrDim);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (b + 1), r.toNearestInt(), juce::Justification::centred);
    }
}

void EqPanel::mouseMove (const juce::MouseEvent& e)
{
    const int h = bandAt (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void EqPanel::showMenu (int band)
{
    juce::PopupMenu m;
    m.addSectionHeader ("Band " + juce::String (band + 1));
    const int cur = (int) getP (band, EqfType);
    const auto names = juce::StringArray::fromTokens (kParams[eqParam (0, EqfType)].choices, "|", "");
    for (int i = 0; i < names.size(); ++i) m.addItem (1 + i, names[i], true, i == cur);
    m.addSeparator();
    const bool isOn = getP (band, EqfOn) > 0.5f;
    m.addItem (20, isOn ? "Disable band" : "Enable band");
    m.addItem (21, "Reset band");
    m.showMenuAsync (juce::PopupMenu::Options(), [this, band] (int r)
    {
        if (r >= 1 && r <= 7) setP (band, EqfType, (float) (r - 1));
        else if (r == 20) setP (band, EqfOn, getP (band, EqfOn) > 0.5f ? 0.0f : 1.0f);
        else if (r == 21)
            for (int f : { (int) EqfType, (int) EqfFreq, (int) EqfGain, (int) EqfQ })
                setP (band, f, kParams[eqParam (band, f)].def);
        selectBand (band);
    });
}

void EqPanel::mouseDown (const juce::MouseEvent& e)
{
    for (int b = 0; b < noir::ProEq::N; ++b)
        if (chipRect (b).expanded (3.0f).contains (e.position))
        {
            if (e.mods.isPopupMenu()) { showMenu (b); return; }
            if (getP (b, EqfOn) < 0.5f) setP (b, EqfOn, 1.0f);
            selectBand (b);
            return;
        }

    const int b = bandAt (e.position);
    if (b < 0) return;
    if (e.mods.isPopupMenu()) { showMenu (b); return; }

    selectBand (b);
    dragBand = b;
    gesture (b, true);
    downPt = e.position;
    startF = getP (b, EqfFreq);
    startG = getP (b, EqfGain);
    repaint();
}

void EqPanel::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0) return;
    const float k = e.mods.isShiftDown() ? 0.15f : 1.0f;     // Shift = fine adjust
    const float nx = xForFreq (startF) + (e.position.x - downPt.x) * k;
    setP (dragBand, EqfFreq, freqForX (nx));
    if (usesGain ((int) getP (dragBand, EqfType)))
        setP (dragBand, EqfGain, gainForY (yForGain (startG) + (e.position.y - downPt.y) * k));
}

void EqPanel::mouseUp (const juce::MouseEvent&)
{
    if (dragBand >= 0) gesture (dragBand, false);
    dragBand = -1;
    repaint();
}

void EqPanel::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int b = bandAt (e.position);
    if (b >= 0) { setP (b, EqfOn, 0.0f); return; }                        // double-click a node removes it
    if (! graph.contains (e.position)) return;

    for (int i = 0; i < noir::ProEq::N; ++i)                               // double-click empty space adds a band there
        if (getP (i, EqfOn) < 0.5f)
        {
            setP (i, EqfType, (float) noir::EqBell);
            setP (i, EqfFreq, freqForX (e.position.x));
            setP (i, EqfGain, gainForY (e.position.y));
            setP (i, EqfQ, 1.0f);
            setP (i, EqfOn, 1.0f);
            selectBand (i);
            return;
        }
}

void EqPanel::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int b = bandAt (e.position);
    if (b < 0) return;
    gesture (b, true);
    setP (b, EqfQ, juce::jlimit (0.1f, 18.0f, getP (b, EqfQ) * std::exp (w.deltaY * 0.8f)));
    gesture (b, false);
}

//==============================================================================
Pedal::Pedal (juce::AudioProcessorValueTreeState& s, const juce::String& t, juce::Colour acc,
              int onParam, int typeParam, std::initializer_list<Id> knobIds, const juce::String& sub)
    : state (s), title (t), subtitle (sub), accent (acc), onId (onParam), typeId (typeParam)
{
    if (onId >= 0)
    {
        led.getProperties().set ("invisible", true);
        led.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible (led);
        ledAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (state, kParams[onId].id, led);
        controls.push_back ({ &led, (Id) onId });
    }

    if (typeId >= 0)
    {
        type.addItemList (juce::StringArray::fromTokens (kParams[typeId].choices, "|", ""), 1);
        addAndMakeVisible (type);
        typeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (state, kParams[typeId].id, type);
        controls.push_back ({ &type, (Id) typeId });
    }

    for (auto id : knobIds)
    {
        const auto& info = kParams[id];
        auto k = std::make_unique<Knob>();
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 15);
        k->slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        k->slider.setMouseDragSensitivity (180);
        k->slider.setScrollWheelEnabled (false);
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        k->slider.setDoubleClickReturnValue (true, info.def);
        k->label.setText (juce::String (info.name).toUpperCase(), juce::dontSendNotification);
        k->label.setJustificationType (juce::Justification::centred);
        k->label.setFont (juce::FontOptions (9.5f, juce::Font::bold));
        k->label.setColour (juce::Label::textColourId, accent.withAlpha (0.85f));
        k->label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (k->slider);
        addAndMakeVisible (k->label);
        k->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, info.id, k->slider);
        controls.push_back ({ &k->slider, id });
        knobs.push_back (std::move (k));
    }

    for (auto& c : controls)
        c.first->addMouseListener (this, false);

    applyColours();
}

void Pedal::applyColours()
{
    const bool pop = L::theme == 0;
    const bool filled = pop && isOn;
    const auto txt = filled ? L::onAccent (accent) : L::text;
    const auto lab = pop ? (filled ? L::onAccent (accent).withAlpha (0.85f) : accent) : L::dim;
    for (auto& k : knobs)
    {
        k->slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        k->slider.setColour (juce::Slider::textBoxTextColourId, txt);
        k->slider.getProperties().set ("amt", (double) onAmt);
        k->label.setColour (juce::Label::textColourId, lab);
    }
}

int Pedal::desiredWidth() const
{
    if (fixedWidth > 0) return fixedWidth;
    int w = (int) knobs.size() * 72 + 24;
    if (typeId >= 0) w = juce::jmax (w, 164);
    return juce::jmax (w, 120);
}

void Pedal::toggle()
{
    if (onId < 0) return;
    if (auto* p = state.getParameter (kParams[onId].id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (isOn ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
}

void Pedal::refresh()
{
    bool nowOn = true;
    if (onId >= 0)
        if (auto* v = state.getRawParameterValue (kParams[onId].id))
            nowOn = v->load() > 0.5f;

    if (nowOn == isOn) return;
    isOn = nowOn;
    flash = 1.0f;
    applyColours();
    repaint();
}

bool Pedal::tick (float dt, float signal)
{
    const float target = isOn ? 1.0f : 0.0f;
    const bool moving = std::abs (onAmt - target) > 0.002f;
    if (moving)
    {
        onAmt += (target - onAmt) * juce::jmin (1.0f, dt * 14.0f);
        if (std::abs (onAmt - target) <= 0.002f) onAmt = target;
        const float a = 0.50f + 0.50f * onAmt;
        type.setAlpha (a);
        for (auto& k : knobs) { k->slider.setAlpha (a); k->label.setAlpha (a); k->slider.getProperties().set ("amt", (double) onAmt); }
    }
    phase += dt;
    sig += (signal - sig) * juce::jmin (1.0f, dt * 10.0f);

    if (moving || flash > 0.0f)
    {
        flash = juce::jmax (0.0f, flash - dt * 2.4f);
        repaint();
        return true;
    }

    if (isOn)   // light-weight: only the LED and the activity line
    {
        repaint (4, 4, 36, kHeader - 8);
        repaint (10, getHeight() - 6, getWidth() - 20, 4);
        return true;
    }
    return false;
}

void Pedal::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.0f);
    const float a = onAmt;
    const bool pop = L::theme == 0;
    const float corner = pop ? 16.0f : 9.0f;
    juce::Colour textCol;

    if (pop)   // filled with the pedal colour when on, outline only when off
    {
        g.setColour (L::panel.interpolatedWith (accent, a));
        g.fillRoundedRectangle (r, corner);
        g.setColour (accent);
        g.drawRoundedRectangle (r, corner, hover ? 3.0f : 2.0f);
        textCol = accent.interpolatedWith (L::onAccent (accent), a);
    }
    else       // cream faceplate with an enamel band on top
    {
        g.setColour (juce::Colour (0x30000000));
        g.fillRoundedRectangle (r.translated (0.0f, 2.5f), corner);
        g.setColour (L::panel);
        g.fillRoundedRectangle (r, corner);
        juce::Path top;
        top.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), 9.0f, corner, corner, true, true, false, false);
        g.setColour (accent.interpolatedWith (juce::Colour (0xff9a937f), 1.0f - a));
        g.fillPath (top);
        g.setColour (L::line);
        g.drawRoundedRectangle (r, corner, hover ? 1.8f : 1.0f);
        textCol = L::text.interpolatedWith (L::dim, 1.0f - a);
    }

    if (flash > 0.0f)
    {
        g.setColour (accent.withAlpha (flash * 0.7f));
        g.drawRoundedRectangle (r.expanded ((1.0f - flash) * 7.0f), corner + 2.0f, 2.0f);
    }

    const int ty = pop ? 6 : 12;
    const int textX = onId >= 0 ? 38 : 16;
    g.setColour (textCol);
    g.setFont (juce::FontOptions (pop ? 14.0f : 15.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), textX, ty, getWidth() - textX - 28, 19, juce::Justification::centredLeft);
    if (subtitle.isNotEmpty())
    {
        g.setColour (textCol.withAlpha (0.75f));
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (subtitle, textX, ty + 18, getWidth() - textX - 28, 12, juce::Justification::centredLeft);
    }

    if (onId >= 0)   // LED
    {
        const float pulse = 0.78f + 0.22f * std::sin (phase * 3.0f);
        if (pop)
        {
            const auto c = juce::Point<float> (21.0f, (float) kHeader * 0.5f);
            g.setColour (ink.withAlpha (0.8f * a));
            g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
            g.setColour (accent.withAlpha (0.8f));
            g.drawEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f, 1.3f);
            if (a > 0.05f)
            {
                g.setColour (juce::Colours::white.withAlpha (a * pulse));
                g.fillEllipse (c.x - 3.6f, c.y - 3.6f, 7.2f, 7.2f);
                g.setColour (accent.withAlpha (a * 0.35f * pulse));
                g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
            }
        }
        else   // jewel lamp
        {
            const auto c = juce::Point<float> (21.0f, (float) kHeader * 0.5f + 4.0f);
            if (a > 0.05f)
            {
                g.setColour (juce::Colour (0xffff8a4a).withAlpha (0.35f * a * pulse));
                g.fillEllipse (c.x - 15.0f, c.y - 15.0f, 30.0f, 30.0f);
            }
            g.setColour (juce::Colour (0xff1c1a17));
            g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
            g.setColour (juce::Colour (0xff8b8068).interpolatedWith (juce::Colour (0xffff9b4a), a));
            g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
            g.setColour (juce::Colour (0xffd8d2c0).interpolatedWith (juce::Colour (0xfffff3d6), a).withAlpha (0.9f));
            g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 5.0f, 5.0f);
        }
    }

    if (slot >= 0 && ! noDrag)   // grip
    {
        g.setColour (textCol.withAlpha (0.55f));
        for (int ix = 0; ix < 2; ++ix)
            for (int iy = 0; iy < 3; ++iy)
                g.fillEllipse ((float) getWidth() - 22.0f + (float) ix * 5.0f, 13.0f + (float) iy * 5.0f, 2.4f, 2.4f);
    }

    // activity line along the bottom
    if (a > 0.05f)
    {
        g.setColour ((pop ? L::onAccent (accent) : accent).withAlpha (a * (0.10f + 0.65f * sig)));
        g.fillRoundedRectangle (r.getX() + 14.0f, r.getBottom() - 5.0f, r.getWidth() - 28.0f, 2.0f, 1.0f);
    }
}

void Pedal::resized()
{
    auto a = getLocalBounds();
    led.setBounds (6, 6, 30, kHeader - 12);
    a.removeFromTop (kHeader + 6);

    if (typeId >= 0)
    {
        type.setBounds (a.removeFromTop (24).reduced (12, 0));
        a.removeFromTop (4);
    }

    auto body = a.reduced (6, 2).withTrimmedBottom (8);

    if (onLayoutBody)
        body = onLayoutBody (body);

    if (knobs.empty() || body.isEmpty()) return;

    const int w = body.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto c = body.removeFromLeft (w);
        k->label.setBounds (c.removeFromTop (13));
        k->slider.setBounds (c);
    }
}

void Pedal::mouseMove (const juce::MouseEvent& e)
{
    if (e.eventComponent != this) return;
    setMouseCursor (onId >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void Pedal::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        for (auto& c : controls)
            if (e.eventComponent == c.first || e.originalComponent == c.first)
            {
                if (onContextMenu) onContextMenu (c.second);
                return;
            }
        if (e.eventComponent == this && onId >= 0 && onContextMenu) onContextMenu ((Id) onId);
        return;
    }

    if (e.eventComponent != this) return;   // left-clicks on child controls are handled by the controls themselves
    downPos = e.getPosition();
    dragArmed = slot >= 0 && ! noDrag && e.y < kHeader;
    dragging = false;
}

void Pedal::mouseDrag (const juce::MouseEvent& e)
{
    if (e.eventComponent != this || ! dragArmed) return;
    if (! dragging)
    {
        if (e.getPosition().getDistanceFrom (downPos) < 6) return;
        dragging = true;
        if (onDragStart) onDragStart (*this, e);
    }
    if (onDragMove) onDragMove (*this, e);
}

void Pedal::mouseUp (const juce::MouseEvent& e)
{
    if (e.eventComponent != this || e.mods.isPopupMenu()) return;

    if (dragging)
    {
        dragging = dragArmed = false;
        if (onDragEnd) onDragEnd (*this, e);
        return;
    }
    dragArmed = false;
    if (e.getPosition().getDistanceFrom (downPos) < 8)
        toggle();      // a click anywhere on the surface switches the pedal
}

//==============================================================================
ChainStrip::ChainStrip (NoirProcessor& p, std::vector<juce::Colour> c) : proc (p), colours (std::move (c)) {}

juce::Rectangle<float> ChainStrip::boxFor (int i) const
{
    const float gap = 9.0f;
    const float n = (float) (NumSlots + 2);
    const float w = ((float) getWidth() - gap * (n - 1.0f)) / n;
    return { (w + gap) * (float) i, 3.0f, w, (float) getHeight() - 6.0f };
}

int ChainStrip::indexAt (juce::Point<int> pt) const
{
    for (int i = 1; i <= NumSlots; ++i)
        if (boxFor (i).contains (pt.toFloat()))
            return i;
    return -1;
}

void ChainStrip::paint (juce::Graphics& g)
{
    auto isOn = [this] (int idx)
    {
        if (idx == 0 || idx == NumSlots + 1) return true;
        return proc.apvts.getRawParameterValue (kParams[kSlotOn[order[(size_t) idx - 1]]].id)->load() > 0.5f;
    };

    for (int i = 0; i < NumSlots + 2; ++i)
    {
        const bool endpoint = i == 0 || i == NumSlots + 1;
        const int slot = endpoint ? -1 : order[(size_t) i - 1];
        const auto b = boxFor (i);
        const bool on = isOn (i);

        if (i + 1 < NumSlots + 2)
        {
            const auto nb = boxFor (i + 1);
            const float y = b.getCentreY();
            const bool live = on && isOn (i + 1);
            g.setColour (live ? L::dim : L::line);
            g.drawLine (b.getRight() + 0.5f, y, nb.getX() - 0.5f, y, 1.2f);
            if (live)
            {
                const float x0 = b.getRight(), x1 = nb.getX();
                for (int d = 0; d < 2; ++d)
                {
                    const float f = std::fmod (phase + (float) d * 0.5f + (float) i * 0.13f, 1.0f);
                    g.setColour (L::text.withAlpha (0.9f * std::sin (f * 3.1416f)));
                    g.fillEllipse (x0 + (x1 - x0) * f - 1.8f, y - 1.8f, 3.6f, 3.6f);
                }
            }
        }

        const auto col = endpoint ? L::dim : colours[(size_t) slot];
        g.setColour (on && ! endpoint ? col.withAlpha (hover == i ? 1.0f : 0.92f) : L::panel);
        g.fillRoundedRectangle (b, 5.0f);
        g.setColour (on ? col.withAlpha (endpoint ? 0.5f : 1.0f) : L::line);
        g.drawRoundedRectangle (b.reduced (0.5f), 5.0f, 1.0f);

        g.setColour (on ? (endpoint ? L::text : L::onAccent (col)) : L::dim.withAlpha (0.7f));
        g.setFont (juce::FontOptions (9.5f, endpoint ? juce::Font::plain : juce::Font::bold));
        g.drawText (endpoint ? (i == 0 ? "IN" : "OUT") : juce::String (kSlotShort[slot]).toUpperCase(),
                    b.toNearestInt(), juce::Justification::centred);
    }
}

void ChainStrip::mouseMove (const juce::MouseEvent& e)
{
    const int h = indexAt (e.getPosition());
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void ChainStrip::mouseDown (const juce::MouseEvent& e)
{
    const int i = indexAt (e.getPosition());
    if (i < 0) return;
    if (auto* p = proc.param (kSlotOn[order[(size_t) i - 1]]))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getValue() > 0.5f ? 0.0f : 1.0f);
        p->endChangeGesture();
    }
    repaint();
}

//==============================================================================
Pedal* NoirEditor::makePedal (int slot)
{
    auto make = [this] (const char* title, juce::Colour c, int on, int type, std::initializer_list<Id> k, const char* sub) -> Pedal*
    {
        auto ped = std::make_unique<Pedal> (proc.apvts, title, c, on, type, k, sub);
        auto* p = ped.get();
        board.addAndMakeVisible (p);
        pedals.push_back (std::move (ped));
        return p;
    };

    switch (slot)
    {
        case SGate:   return make ("Noise Gate", cDyn,   GateOn,  -1,        { GateThr },                          "Dynamics");
        case SComp:   return make ("Compressor", cDyn,   CompOn,  -1,        { CompAmt, CompLvl, CompAtk },        "Dynamics / Sustain");
        case SWah:    return make ("Wah",        cFilt,  WahOn,   WahMode,   { WahPos, WahQ },                     "Filter");
        case SPitch:  return make ("Pitch",      cPitch, PitchOn, PitchType, { PitchShift, PitchMix },             "Pitch");
        case SBoost:  return make ("Boost",      cGain,  BoostOn, BoostType, { BoostGain },                        "Gain / EQ boost");
        case SDrive:  return make ("Drive",      cGain,  DrvOn,   DrvType,   { DrvDrive, DrvTone, DrvLevel },      "Overdrive / Dist / Fuzz / Muff");
        case SAmp:    return make ("Amp",        cAmp,   AmpOn,   -1,        { AmpGain, AmpBass, AmpMid, AmpTreble, AmpPres }, "Preamp + power stage");
        case SCab:    return make ("Cabinet",    cAmp,   CabOn,   CabType,   {},                                   "Speaker / IR");
        case SEq:     return make ("Equalizer",  cEq,    EqOn,    -1,        {},                                   "8-band parametric / analyzer");
        case STalk:   return make ("Talk Box",   cVoice, TbOn,    -1,        { TbVowel, TbAuto, TbRes, TbMix },    "Vowel formants");
        case SVoc:    return make ("Vocoder",    cVoice, VocOn,   -1,        { VocSens, VocBright, VocMix },       "Mic on input 2");
        case SMod1:   return make ("Mod 1",      cMod,   M1On,    M1Type,    { M1Rate, M1Depth, M1Mix },           "Modulation");
        case SMod2:   return make ("Mod 2",      cMod,   M2On,    M2Type,    { M2Rate, M2Depth, M2Mix },           "Modulation");
        case SLofi:   return make ("Lo-Fi",      cLofi,  LoOn,    -1,        { LoBits, LoRate, LoMix },            "Bitcrusher / Sample rate");
        case SGlitch: return make ("Glitch",     cGlit,  GltOn,   GltMode,   { GltLen, GltAmt, GltMix },           "Stutter / Glitch");
        case SGran:   return make ("Granular",   cGrain, GrOn,    -1,        { GrSize, GrDens, GrPitch, GrSpread, GrMix }, "Grain cloud");
        case SFreeze: return make ("Freeze",     cGrain, FrzOn,   -1,        { FrzSize, FrzMix },                  "Hold / infinite sustain");
        case SDelay:  return make ("Delay",      cTime,  DlyOn,   DlyType,   { DlyTime, DlyFb, DlyMix },           "Echo");
        case SReverb: return make ("Reverb",     cTime,  RevOn,   RevType,   { RevSize, RevMix },                  "Space");
        case SLooper: return make ("Looper",     cLoop,  LoopOn,  LoopSpeed, { LoopLevel },                        "4 tracks  /  record, overdub");
        default:      return nullptr;
    }
}

NoirEditor::NoirEditor (NoirProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    laf.applyTheme (proc.uiTheme.load());
    setLookAndFeel (&laf);

    viewport.setViewedComponent (&board, false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (8);
    viewport.setScrollOnDragMode (juce::Viewport::ScrollOnDragMode::never);
    addAndMakeVisible (viewport);

    for (int s = 0; s < NumSlots; ++s)
    {
        auto* ped = makePedal (s);
        ped->slot = s;
        bySlot[s] = ped;
        ped->onDragStart = [this] (Pedal& pd, const juce::MouseEvent& e)
        {
            dragging = &pd;
            grabOffset = e.getEventRelativeTo (&board).getPosition() - pd.getPosition();
            pd.toFront (false);
            pd.setAlpha (0.92f);
            board.repaint();
        };
        ped->onDragMove = [this] (Pedal& pd, const juce::MouseEvent& e)
        {
            if (dragging != &pd) return;
            const auto pos = e.getEventRelativeTo (&board).getPosition();
            juce::Desktop::getInstance().getAnimator().cancelAnimation (&pd, false);
            pd.setTopLeftPosition (pos - grabOffset);

            // keep the card under the cursor reachable when the board is scrolled
            const auto inView = e.getEventRelativeTo (&viewport).getPosition();
            if (inView.y < 30) viewport.setViewPosition (viewport.getViewPositionX(), viewport.getViewPositionY() - 14);
            else if (inView.y > viewport.getHeight() - 30) viewport.setViewPosition (viewport.getViewPositionX(), viewport.getViewPositionY() + 14);

            for (auto* c : cards)
            {
                if (c == &pd || ! c->targetBounds.contains (pos)) continue;
                const auto from = std::find (cards.begin(), cards.end(), &pd);
                const auto to = std::find (cards.begin(), cards.end(), c);
                const auto idx = (size_t) std::distance (cards.begin(), to);
                cards.erase (from);
                cards.insert (cards.begin() + (long) idx, &pd);
                layoutCards (true);
                break;
            }
        };
        ped->onDragEnd = [this] (Pedal& pd, const juce::MouseEvent&)
        {
            pd.setAlpha (1.0f);
            dragging = nullptr;
            layoutCards (true);
            commitOrder();
            board.repaint();
        };
    }

    board.onPaint = [this] (juce::Graphics& g)
    {
        if (dragging == nullptr) return;
        const auto r = dragging->targetBounds.toFloat().reduced (1.0f);
        g.setColour (L::accent.withAlpha (0.07f));
        g.fillRoundedRectangle (r, 11.0f);
        juce::Path outline, dashed;
        outline.addRoundedRectangle (r, 11.0f);
        const float dashes[] = { 5.0f, 4.0f };
        juce::PathStrokeType (1.4f).createDashedStroke (dashed, outline, dashes, 2);
        g.setColour (L::accent.withAlpha (0.8f));
        g.fillPath (dashed);
    };

    // Utility pedals (fixed position, not part of the chain)
    pTuner = new Pedal (proc.apvts, "Tuner", cOut, TunerOn, -1, { TunerRef }, "Mutes the output while on");
    pOut   = new Pedal (proc.apvts, "Levels", cOut, -1, -1, { InGain, Master, OutGain }, "Input / Master / Output");
    for (auto* u : { pTuner, pOut })
    {
        board.addAndMakeVisible (u);
        pedals.emplace_back (u);
    }
    pTuner->setDesiredWidth (250);

    pTuner->addAndMakeVisible (tunerView);
    pTuner->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        tunerView.setBounds (body.removeFromTop ((int) ((float) body.getHeight() * 0.56f)));
        return body;
    };

    // Cabinet: IR loader lives inside the cab pedal
    auto* cabPedal = bySlot[SCab];
    cabPedal->addAndMakeVisible (irButton);
    cabPedal->addChildComponent (irClear);
    irButton.onClick = [this] { chooseIR(); };
    irClear.onClick = [this] { proc.clearIR(); timerCallback(); };
    cabPedal->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        auto row = body.withTrimmedTop (4).removeFromTop (28).reduced (6, 0);
        irClear.setBounds (row.removeFromRight (28));
        row.removeFromRight (4);
        irButton.setBounds (row);
        return juce::Rectangle<int>();
    };

    // Looper: four tracks
    auto* lp = bySlot[SLooper];
    lp->setDesiredWidth (520);
    looperView = std::make_unique<LooperView> (proc);
    lp->addAndMakeVisible (*looperView);
    lp->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        auto knob = body.removeFromRight (76);
        looperView->setBounds (body);
        return knob;
    };

    // Equalizer: Pro-Q style analyser + draggable bands fill the pedal
    pEq = bySlot[SEq];
    eqPanel = std::make_unique<EqPanel> (proc);
    pEq->addAndMakeVisible (*eqPanel);
    pEq->setDesiredWidth (780);
    pEq->minRowHeight = 350;
    pEq->onLayoutBody = [this] (juce::Rectangle<int> body)
    {
        eqPanel->setBounds (body);
        return juce::Rectangle<int>();
    };

    for (int s = 0; s < NumSlots; ++s) cards.push_back (bySlot[s]);

    strip = std::make_unique<ChainStrip> (proc, std::vector<juce::Colour> (std::begin (slotColour), std::end (slotColour)));
    addAndMakeVisible (*strip);

    // Header
    for (const auto& pr : factoryPresets())
        presetBox.addItem (pr.name, presetBox.getNumItems() + 1);
    presetBox.setTextWhenNothingSelected ("Chains / Presets");
    presetBox.onChange = [this] { proc.loadPreset (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    auto step = [this] (int d)
    {
        const int n = presetBox.getNumItems();
        if (n == 0) return;
        const int cur = presetBox.getSelectedItemIndex();
        presetBox.setSelectedItemIndex (((cur < 0 ? (d > 0 ? -1 : 0) : cur) + d + n) % n, juce::sendNotificationSync);
    };
    prevPreset.onClick = [step] { step (-1); };
    nextPreset.onClick = [step] { step (1); };
    allOn.onClick  = [this] { setAllEffects (true); };
    allOff.onClick = [this] { setAllEffects (false); };
    resetOrder.setButtonText ("ORDER");
    resetOrder.setTooltip ("Reset the pedal order");
    resetOrder.onClick = [this] { proc.setChainOrder (NoirProcessor::defaultOrder()); syncOrderFromProcessor(); };
    for (auto* b : { &prevPreset, &nextPreset, &allOn, &allOff, &resetOrder })
        addAndMakeVisible (b);

    power.getProperties().set ("power", true);
    power.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    addAndMakeVisible (power);
    powerAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, kParams[Power].id, power);

    for (auto& pd : pedals)
        pd->onContextMenu = [this] (Id id) { showParamMenu (id); };

    // Tabs
    buildPanels();
    for (auto* t : { &tabBoard, &tabMidi, &subCtl, &subMacro }) { t->setClickingTogglesState (false); addAndMakeVisible (t); }
    addAndMakeVisible (themeBtn);
    tabBoard.onClick = [this] { setTab (0); };
    tabMidi.onClick  = [this] { setTab (1); };
    subCtl.onClick   = [this] { currentSub = 0; setTab (1); };
    subMacro.onClick = [this] { currentSub = 1; setTab (1); };
    themeBtn.setTooltip ("Interface theme");
    themeBtn.onClick = [this]
    {
        juce::PopupMenu m;
        m.addSectionHeader ("Theme");
        m.addItem (1, "Pop Stompbox  (default)", true, L::theme == 0);
        m.addItem (2, "Retro Amp", true, L::theme == 1);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&themeBtn), [this] (int r)
        {
            if (r > 0) setTheme (r - 1);
        });
    };
    cancelLearn.onClick = [this] { proc.cancelMidiLearn(); };
    addChildComponent (cancelLearn);
    addAndMakeVisible (meterIn);
    addAndMakeVisible (meterOut);
    tabBoard.setToggleState (true, juce::dontSendNotification);
    refreshAccents();
    setTab (0);

    {
        const auto o = proc.getChainOrder();
        cards.clear();
        for (int s : o) cards.push_back (bySlot[s]);
        strip->setOrder (o);
    }

    if (const int cp = proc.currentPreset.load(); cp >= 0) presetBox.setSelectedItemIndex (cp, juce::dontSendNotification);

    setResizable (true, true);
    setResizeLimits (1280, 620, 7680, 4320);
    setSize (1500, 900);
    startTimerHz (30);
    timerCallback();
    lastFrame = juce::Time::getMillisecondCounterHiRes();
    vblank = std::make_unique<juce::VBlankAttachment> (this, [this] { animTick(); });
}

NoirEditor::~NoirEditor()
{
    vblank.reset();
    stopTimer();
    juce::Desktop::getInstance().getAnimator().cancelAllAnimations (false);
    setLookAndFeel (nullptr);
}

void NoirEditor::setAllEffects (bool on)
{
    for (int s = 0; s < NumSlots; ++s)
        if (auto* p = proc.param (kSlotOn[s]))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (on ? 1.0f : 0.0f);
            p->endChangeGesture();
        }
}

void NoirEditor::chooseIR()
{
    chooser = std::make_unique<juce::FileChooser> ("Choose a cabinet impulse response (WAV)",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.wav;*.aif;*.aiff;*.flac");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              const auto f = fc.getResult();
                              if (f.existsAsFile())
                                  proc.loadIR (f);
                              timerCallback();
                          });
}

void NoirEditor::buildPanels()
{
    midi.reset();
    daw.reset();
    midi = std::make_unique<MidiPanel> (proc);
    daw = std::make_unique<DawPanel> (proc);
    addChildComponent (*midi);
    addChildComponent (*daw);
}

void NoirEditor::refreshAccents()
{
    for (int s = 0; s < NumSlots; ++s)
        if (bySlot[s] != nullptr) bySlot[s]->setAccent (slotColour[s]);
    pTuner->setAccent (cOut);
    pOut->setAccent (cOut);
    strip->setColours (std::vector<juce::Colour> (std::begin (slotColour), std::end (slotColour)));
    tabBoard.getProperties().set ("accent", (int) cGain.getARGB());
    tabMidi.getProperties().set ("accent", (int) cGain.getARGB());
    subCtl.getProperties().set ("accent", (int) cGain.getARGB());
    subMacro.getProperties().set ("accent", (int) cGain.getARGB());
    cancelLearn.getProperties().set ("accent", (int) cGlit.getARGB());
    if (eqPanel) eqPanel->refreshTheme();
}

void NoirEditor::setTheme (int t)
{
    t = juce::jlimit (0, 1, t);
    if (t == L::theme) return;
    proc.uiTheme.store (t);
    laf.applyTheme (t);
    refreshAccents();
    buildPanels();
    sendLookAndFeelChange();
    resized();
    setTab (currentTab);
    repaint();
}

void NoirEditor::setTab (int tab)
{
    currentTab = tab;
    tabBoard.setToggleState (tab == 0, juce::dontSendNotification);
    tabMidi.setToggleState (tab == 1, juce::dontSendNotification);
    subCtl.setToggleState (currentSub == 0, juce::dontSendNotification);
    subMacro.setToggleState (currentSub == 1, juce::dontSendNotification);

    strip->setVisible (tab == 0);
    viewport.setVisible (tab == 0);
    for (auto* b : { &allOn, &allOff, &resetOrder }) b->setVisible (tab == 0);
    subCtl.setVisible (tab == 1);
    subMacro.setVisible (tab == 1);
    midi->setVisible (tab == 1 && currentSub == 0);
    daw->setVisible (tab == 1 && currentSub == 1);
    repaint();
}

void NoirEditor::showParamMenu (Id id)
{
    juce::PopupMenu menu, macroMenu;
    menu.addSectionHeader (paramLabel (id));

    const int cc = proc.getCcFor (id);
    menu.addItem (1, "MIDI Learn  (then press a footswitch / move a knob)");
    if (cc >= 0) menu.addItem (2, "Clear MIDI mapping  (CC " + juce::String (cc) + ")");

    for (int m = 0; m < NoirProcessor::NumMacros; ++m)
        macroMenu.addItem (100 + m, "Macro " + juce::String (m + 1));
    menu.addSubMenu ("Assign to Macro", macroMenu, id < Macro1);

    menu.addSeparator();
    menu.addItem (3, "Reset to default");

    menu.showMenuAsync (juce::PopupMenu::Options(), [this, id] (int result)
    {
        if (result == 1) proc.startMidiLearn (id);
        else if (result == 2) proc.clearMidiFor (id);
        else if (result == 3) proc.setParamValue (id, kParams[id].def);
        else if (result >= 100) proc.assignMacro (result - 100, id);
    });
}

void NoirEditor::commitOrder()
{
    NoirProcessor::Order o {};
    for (size_t i = 0; i < cards.size(); ++i) o[i] = cards[i]->slot;
    proc.setChainOrder (o);
    strip->setOrder (o);
}

void NoirEditor::syncOrderFromProcessor()
{
    if (dragging != nullptr) return;

    const auto o = proc.getChainOrder();
    bool same = true;
    for (size_t i = 0; i < cards.size(); ++i)
        if (cards[i]->slot != o[i]) { same = false; break; }

    strip->setOrder (o);
    if (same) return;

    cards.clear();
    for (int s : o) cards.push_back (bySlot[s]);
    layoutCards (true);
}

void NoirEditor::timerCallback()
{
    const bool ir = proc.hasIR();
    irButton.setButtonText (ir ? proc.getIRName() : "Load IR");
    irClear.setVisible (ir);

    looperView->refresh();

    if (proc.apvts.getRawParameterValue (kParams[TunerOn].id)->load() > 0.5f)
    {
        const auto r = proc.getTuner().analyse();
        tunerView.setReading (r.freq, proc.apvts.getRawParameterValue (kParams[TunerRef].id)->load());
    }
    else
        tunerView.setReading (0.0f, 440.0f);

    const bool learning = proc.isLearning();
    if (cancelLearn.isVisible() != learning)
    {
        cancelLearn.setVisible (learning);
        presetBox.setVisible (! learning); prevPreset.setVisible (! learning); nextPreset.setVisible (! learning);
        repaint (headerArea);
    }

    if (currentTab == 0)
    {
        syncOrderFromProcessor();
        for (auto& p : pedals) p->refresh();
    }

    if (const int cp = proc.currentPreset.load(); cp >= 0 && cp != presetBox.getSelectedItemIndex())
        presetBox.setSelectedItemIndex (cp, juce::dontSendNotification);

    const int cpu = juce::roundToInt (proc.cpuLoad.load() * 100.0f);
    const auto txt = "CPU " + juce::String (cpu) + "%    glitches " + juce::String (proc.xruns.load())
                     + "    plugin latency " + juce::String (proc.getLatencySamples()) + " smp";
    if (txt != perfText) { perfText = txt; repaint (perfArea); }
}

void NoirEditor::animTick()
{
    const double now = juce::Time::getMillisecondCounterHiRes();
    const float dt = (float) juce::jlimit (0.001, 0.05, (now - lastFrame) * 0.001);
    lastFrame = now;

    const float ip = proc.inPeak.exchange (0.0f);
    const float op = proc.outPeak.exchange (0.0f);
    meterIn.push (ip, dt);
    meterOut.push (op, dt);

    outSmooth += (juce::jmin (1.0f, op * 5.0f) - outSmooth) * juce::jmin (1.0f, dt * 12.0f);
    wavePhase = std::fmod (wavePhase + dt * (1.5f + outSmooth * 6.0f), 6.2832f);
    repaint (waveArea);

    if (currentTab == 0)
    {
        const float s = juce::jmin (1.0f, outSmooth);
        for (auto& p : pedals) p->tick (dt, s);
        strip->tick (dt, s);
    }
    if (power.getToggleState()) power.repaint();
}

//==============================================================================
void NoirEditor::paint (juce::Graphics& g)
{
    g.fillAll (L::bg);
    const bool retro = L::theme == 1;

    if (retro)   // dark control-panel bar with a brass trim line
    {
        const int h = headerArea.getBottom() + 8;
        g.setColour (juce::Colour (0xff1f1c17));
        g.fillRect (0, 0, getWidth(), h);
        g.setColour (L::gold.withAlpha (0.6f));
        g.fillRect (0, h - 2, getWidth(), 2);
        g.setColour (juce::Colour (0x28000000));
        g.fillRect (0, h, getWidth(), 3);
    }
    else
    {
        juce::ColourGradient vignette (juce::Colour (0xff1d1d1d), (float) getWidth() * 0.5f, 0.0f,
                                       L::bg, (float) getWidth() * 0.5f, 200.0f, false);
        g.setGradientFill (vignette);
        g.fillRect (0, 0, getWidth(), 200);
    }

    g.setColour (retro ? L::gold : L::text);
    g.setFont (juce::FontOptions (24.0f, juce::Font::bold));
    g.drawText ("NOIR", 22, 8, 80, 30, juce::Justification::centredLeft);

    // living waveform next to the logo
    {
        juce::Path w;
        const auto r = waveArea.toFloat();
        for (int i = 0; i <= 48; ++i)
        {
            const float x = r.getX() + r.getWidth() * (float) i / 48.0f;
            const float env = std::sin (3.1416f * (float) i / 48.0f);
            const float y = r.getCentreY() + std::sin (wavePhase + (float) i * 0.45f) * r.getHeight() * (0.12f + 0.38f * outSmooth) * env;
            if (i == 0) w.startNewSubPath (x, y); else w.lineTo (x, y);
        }
        g.setColour (L::gold.withAlpha (0.9f));
        g.strokePath (w, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (L::hdrDim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("GUITAR  /  PEDALBOARD", 22, 38, 160, 12, juce::Justification::centredLeft);
    g.setColour (L::hdrDim.withAlpha (0.9f));
    g.drawText (perfText, perfArea, juce::Justification::centredLeft);

    if (const int learning = proc.getLearnTarget(); learning >= 0)
    {
        g.setColour (L::gold);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        const auto what = learning >= 1000 ? juce::String (NoirProcessor::actionName (learning - 1000)) : paramLabel (learning);
        g.drawText ("MIDI LEARN  >  " + what + "   -   press a footswitch / move a knob",
                    presetBox.getBounds().withX (prevPreset.getX()).withRight (nextPreset.getRight()).withTrimmedRight (-80),
                    juce::Justification::centredLeft);
    }
}

void NoirEditor::layoutCards (bool animate)
{
    const int gap = 12;
    std::vector<Pedal*> flow (cards.begin(), cards.end());
    flow.push_back (pTuner);
    flow.push_back (pOut);

    int contentH = 0;
    for (int pass = 0; pass < 2; ++pass)
    {
        boardWidth = juce::jmax (400, viewport.getMaximumVisibleWidth());
        const int W = boardWidth;

        std::vector<std::vector<Pedal*>> rows;
        int sum = 0;
        for (auto* p : flow)
        {
            const int w = p->desiredWidth();
            if (! rows.empty() && sum + gap + w <= W) { rows.back().push_back (p); sum += gap + w; }
            else { rows.push_back ({ p }); sum = w; }
        }

        const int nRows = (int) rows.size();
        const int avail = viewport.getHeight();
        const int fit = (avail - gap * (nRows - 1) - 4) / juce::jmax (1, nRows);
        const int baseH = juce::jlimit (206, 262, fit);
        std::vector<int> rowHs;
        contentH = gap * (nRows - 1) + 6;
        for (auto& row : rows)
        {
            int h = baseH;
            for (auto* p : row) h = juce::jmax (h, p->minRowHeight);
            rowHs.push_back (h);
            contentH += h;
        }

        int y = 2;
        for (int r = 0; r < nRows; ++r)
        {
            auto& row = rows[(size_t) r];
            int total = 0;
            for (auto* p : row) total += p->desiredWidth();
            const int availW = W - gap * ((int) row.size() - 1);
            const bool last = r == nRows - 1;
            const float scale = last ? juce::jmin (1.15f, (float) availW / (float) total) : (float) availW / (float) total;

            int x = 0;
            for (size_t i = 0; i < row.size(); ++i)
            {
                const bool lastInRow = i + 1 == row.size();
                const int w = (! last && lastInRow) ? W - x : (int) std::floor ((float) row[i]->desiredWidth() * scale);
                row[i]->targetBounds = { x, y, w, rowHs[(size_t) r] };
                x += w + gap;
            }
            y += rowHs[(size_t) r] + gap;
        }

        board.setSize (W, juce::jmax (contentH, viewport.getHeight()));
        if (viewport.getMaximumVisibleWidth() == W) break;   // the scroll bar appeared / went away: lay out once more
    }

    auto& animator = juce::Desktop::getInstance().getAnimator();
    for (auto* p : flow)
    {
        if (p == dragging) continue;
        if (p->getBounds() == p->targetBounds) continue;

        if (animate && p->getWidth() > 0)
            animator.animateComponent (p, p->targetBounds, 1.0f, 170, false, 1.0, 1.0);
        else
        {
            animator.cancelAnimation (p, false);
            p->setBounds (p->targetBounds);
        }
    }
}

void NoirEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 12);

    headerArea = area.removeFromTop (56);
    {
        auto h = headerArea;
        waveArea = { h.getX() + 76, h.getY() - 2, 86, 28 };
        perfArea = { h.getX() + 2, h.getY() + 41, 300, 14 };
        h.removeFromLeft (186);

        tabBoard.setBounds (h.removeFromLeft (66).withSizeKeepingCentre (64, 28));
        h.removeFromLeft (4);
        tabMidi.setBounds (h.removeFromLeft (66).withSizeKeepingCentre (64, 28));
        h.removeFromLeft (10);
        themeBtn.setBounds (h.removeFromLeft (68).withSizeKeepingCentre (66, 28));
        h.removeFromLeft (10);

        power.setBounds (h.removeFromRight (108).withSizeKeepingCentre (108, 28));
        h.removeFromRight (6);
        resetOrder.setBounds (h.removeFromRight (58).withSizeKeepingCentre (58, 28));
        h.removeFromRight (4);
        allOff.setBounds (h.removeFromRight (64).withSizeKeepingCentre (64, 28));
        h.removeFromRight (4);
        allOn.setBounds (h.removeFromRight (56).withSizeKeepingCentre (56, 28));
        h.removeFromRight (10);

        auto meters = h.removeFromRight (230);
        meterIn.setBounds (meters.removeFromTop (headerArea.getHeight() / 2 - 2).withSizeKeepingCentre (meters.getWidth(), 22));
        meters.removeFromTop (4);
        meterOut.setBounds (meters.withSizeKeepingCentre (meters.getWidth(), 22));
        h.removeFromRight (10);

        auto mid = h.withSizeKeepingCentre (juce::jmin (h.getWidth(), 360), 28);
        cancelLearn.setBounds (mid.removeFromRight (80));
        prevPreset.setBounds (mid.removeFromLeft (30));
        mid.removeFromLeft (6);
        nextPreset.setBounds (mid.removeFromRight (30));
        mid.removeFromRight (6);
        presetBox.setBounds (mid);
    }

    area.removeFromTop (8);
    {
        auto panelArea = area;
        auto sub = panelArea.removeFromTop (28);
        subCtl.setBounds (sub.removeFromLeft (120));
        sub.removeFromLeft (6);
        subMacro.setBounds (sub.removeFromLeft (150));
        panelArea.removeFromTop (8);
        midi->setBounds (panelArea);
        daw->setBounds (panelArea);
    }
    strip->setBounds (area.removeFromTop (30));
    area.removeFromTop (10);
    viewport.setBounds (area);

    layoutCards (false);
}
