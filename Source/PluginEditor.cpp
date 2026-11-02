#include "PluginEditor.h"
#include "Presets.h"

namespace
{
const juce::Colour cDyn   { 0xff3fe0a0 };
const juce::Colour cFilt  { 0xffb98cff };
const juce::Colour cPitch { 0xffff7ab8 };
const juce::Colour cGain  { 0xfff7d21e };
const juce::Colour cAmp   { 0xffff8a4c };
const juce::Colour cEq    { 0xff6cf06c };
const juce::Colour cMod   { 0xff29c9f2 };
const juce::Colour cLofi  { 0xffc9e34a };
const juce::Colour cTime  { 0xff8f8cff };
const juce::Colour cVoice { 0xffffa87a };
const juce::Colour cGrain { 0xff3fe3d4 };
const juce::Colour cGlit  { 0xffff5a5a };
const juce::Colour cLoop  { 0xffffc233 };
const juce::Colour cOut   { 0xffdedbd4 };
const juce::Colour ink    { 0xff0b0b0d };   // text on coloured headers

const juce::Colour slotColour[NumSlots] = {
    cDyn, cDyn, cFilt, cPitch, cGain, cGain, cAmp, cAmp, cEq, cVoice, cVoice,
    cMod, cMod, cLofi, cGlit, cGrain, cGrain, cTime, cTime, cLoop };

constexpr int kHeader = 42;   // pedal header height

using L = NoirLookAndFeel;
}

//==============================================================================
NoirLookAndFeel::NoirLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, accent);
    setColour (juce::Label::textColourId, dim);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, line);
    setColour (juce::PopupMenu::highlightedTextColourId, accent);
    setColour (juce::TextButton::textColourOffId, dim);
    setColour (juce::TextButton::textColourOnId, text);
    setColour (juce::ListBox::textColourId, text);
    setColour (juce::ScrollBar::thumbColourId, line.brighter (0.3f));
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f0f12));
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, line);
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

    // scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
        const float r1 = radius * 0.90f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.95f);
        g.setColour (fill.withAlpha (a <= angle + 0.001f ? 0.85f : 0.28f));
        g.drawLine (c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, c.x + std::sin (a) * r2, c.y - std::cos (a) * r2, 1.3f);
    }

    // solid knob
    const float kr = radius * 0.72f;
    g.setColour (fill.withMultipliedBrightness (hot ? 1.12f : 1.0f));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (ink.withAlpha (0.55f));
    g.drawEllipse (c.x - kr * 0.80f, c.y - kr * 0.80f, kr * 1.6f, kr * 1.6f, 1.0f);

    juce::Path tick;
    tick.startNewSubPath (c.x + std::sin (angle) * kr * 0.18f, c.y - std::cos (angle) * kr * 0.18f);
    tick.lineTo (c.x + std::sin (angle) * kr * 0.88f, c.y - std::cos (angle) * kr * 0.88f);
    g.setColour (ink);
    g.strokePath (tick, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        const float t = (float) (juce::Time::getMillisecondCounterHiRes() * 0.0025);
        if (on)
        {
            g.setColour (accent.withAlpha (0.10f + 0.05f * std::sin (t)));
            g.fillRoundedRectangle (c.expanded (3.0f), c.getHeight() * 0.5f + 3.0f);
        }
        g.setColour (on ? accent : panel);
        g.fillRoundedRectangle (c, c.getHeight() * 0.5f);
        g.setColour (on ? accent : line);
        g.drawRoundedRectangle (c, c.getHeight() * 0.5f, 1.4f);
        g.setColour (on ? ink : (hl ? text : dim));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (on ? "POWER ON" : "BYPASSED", c.toNearestInt(), juce::Justification::centred);
        return;
    }

    if (b.getProperties().contains ("label"))   // small labelled switch
    {
        const auto col = juce::Colour ((juce::uint32) (int) b.getProperties().getWithDefault ("accent", (int) cLoop.getARGB()));
        g.setColour (on ? col : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (r.reduced (0.5f), 5.0f);
        g.setColour (on ? col : line);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (on ? ink : (hl ? text : dim));
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
    g.setColour (lit ? col.withAlpha (0.92f) : (down ? line : (hl ? line.brighter (0.1f) : juce::Colours::transparentBlack)));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (lit ? col : line.brighter (hl ? 0.25f : 0.0f));
    g.drawRoundedRectangle (r, 5.0f, 1.0f);
    if (lit) b.setColour (juce::TextButton::textColourOnId, ink);
}

void NoirLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    const auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f);
    g.setColour (juce::Colour (0xff0f0f12));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (box.isMouseOver() ? line.brighter (0.3f) : line);
    g.drawRoundedRectangle (r, 5.0f, 1.0f);

    juce::Path arrow;
    const float cx = (float) w - 12.0f, cy = (float) h * 0.5f;
    arrow.addTriangle (cx - 3.5f, cy - 1.5f, cx + 3.5f, cy - 1.5f, cx, cy + 2.5f);
    g.setColour (dim);
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
    const bool inTune = valid && std::abs (cents) < 5.0f;
    const auto col = ! valid ? L::dim : (inTune ? juce::Colour (0xff6cf06c) : L::accent);

    g.setColour (col);
    g.setFont (juce::FontOptions (r.getHeight() * 0.50f, juce::Font::bold));
    g.drawText (note, r.removeFromTop (r.getHeight() * 0.58f).toNearestInt(), juce::Justification::centred);

    auto bar = r.reduced (8.0f, 0.0f).withTrimmedBottom (4.0f);
    const float y = bar.getCentreY();
    g.setColour (L::line);
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
    g.setColour (L::dim);
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
    g.setColour (clip ? lampCol : (sig ? juce::Colour (0xffa9e8bd) : L::dim));
    g.drawText (clip ? "CLIP" : (sig ? "SIGNAL" : "NO SIGNAL"), status.withTrimmedLeft (14.0f).toNearestInt(), juce::Justification::centredLeft);

    const int segs = 30;
    const float segW = r.getWidth() / (float) segs;
    const auto bar = r.withSizeKeepingCentre (r.getWidth(), juce::jmin (9.0f, r.getHeight()));
    for (int i = 0; i < segs; ++i)
    {
        const float db = -60.0f + 60.0f * (float) i / (float) (segs - 1);
        const bool lit = level >= db;
        juce::Colour c = db > -3.0f ? juce::Colour (0xffff4d4d) : (db > -12.0f ? juce::Colour (0xfff7d21e) : juce::Colour (0xff4be07a));
        g.setColour (lit ? c : L::line);
        g.fillRoundedRectangle (bar.getX() + (float) i * segW + 0.5f, bar.getY(), segW - 1.5f, bar.getHeight(), 1.2f);
    }
    if (hold > -60.0f)
    {
        const float f = juce::jlimit (0.0f, 1.0f, (hold + 60.0f) / 60.0f);
        g.setColour (L::text.withAlpha (0.8f));
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
    const juce::Colour cols[5] = { L::dim, L::dim.brighter (0.2f), juce::Colour (0xff4be07a), cAmp, cGlit };
    const float prog = proc.loopProgress.load();

    for (int t = 0; t < 4; ++t)
    {
        const auto rb = rowBounds (t).reduced (0, 2).toFloat();
        const int st = proc.loopState[t].load();
        const bool sel = t == selected;
        const bool muted = proc.apvts.getRawParameterValue (kParams[LoopMute1 + t].id)->load() > 0.5f;
        const auto col = cols[juce::jlimit (0, 4, st)];

        g.setColour (sel ? cLoop.withAlpha (0.14f) : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (rb, 6.0f);
        g.setColour (sel ? cLoop : L::line);
        g.drawRoundedRectangle (rb.reduced (0.5f), 6.0f, sel ? 1.4f : 1.0f);

        const auto badge = juce::Rectangle<float> (18.0f, 18.0f).withCentre ({ rb.getX() + 16.0f, rb.getCentreY() });
        g.setColour (sel ? cLoop : L::line.brighter (0.2f));
        g.fillEllipse (badge);
        g.setColour (sel ? ink : L::text);
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText (juce::String (t + 1), badge.toNearestInt(), juce::Justification::centred);

        // state lamp + text
        const auto lamp = juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ rb.getX() + 40.0f, rb.getCentreY() });
        float pulse = 1.0f;
        if (st == 4) pulse = 0.55f + 0.45f * std::sin (spin * 6.2832f * 3.0f);
        g.setColour (col.withAlpha (pulse));
        g.fillEllipse (lamp);
        g.setColour (st == 0 ? L::dim : col);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (names[juce::jlimit (0, 4, st)], (int) rb.getX() + 50, (int) rb.getY(), 76, (int) rb.getHeight(), juce::Justification::centredLeft);

        // progress
        auto bar = rb.withTrimmedLeft (128.0f).withTrimmedRight (34.0f);
        bar = bar.withSizeKeepingCentre (bar.getWidth(), 5.0f);
        g.setColour (L::line);
        g.fillRoundedRectangle (bar, 2.5f);
        if (st >= 1 && st != 4)
        {
            g.setColour ((muted ? L::dim : col).withAlpha (0.9f));
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * prog), 2.5f);
            g.fillEllipse (bar.getX() + bar.getWidth() * prog - 4.0f, bar.getCentreY() - 4.0f, 8.0f, 8.0f);
        }

        // mute
        const auto m = juce::Rectangle<float> (22.0f, 18.0f).withCentre ({ rb.getRight() - 18.0f, rb.getCentreY() });
        g.setColour (muted ? cGlit : juce::Colours::transparentBlack);
        g.fillRoundedRectangle (m, 4.0f);
        g.setColour (muted ? cGlit : L::line);
        g.drawRoundedRectangle (m, 4.0f, 1.0f);
        g.setColour (muted ? ink : L::dim);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText ("M", m.toNearestInt(), juce::Justification::centred);
    }
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
        const float a = 0.42f + 0.58f * onAmt;
        type.setAlpha (a);
        for (auto& k : knobs) { k->slider.setAlpha (a); k->label.setAlpha (a); }
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
    const float corner = 11.0f;

    g.setColour (L::panel.interpolatedWith (accent, 0.05f * a));
    g.fillRoundedRectangle (r, corner);

    // header band
    juce::Path head;
    head.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), (float) kHeader, corner, corner, true, true, false, false);
    g.setColour (accent.withAlpha (0.13f + 0.87f * a));
    g.fillPath (head);

    // outline
    g.setColour (accent.withAlpha ((0.32f + 0.68f * a) * (hover ? 1.0f : 0.92f)));
    g.drawRoundedRectangle (r, corner, hover ? 2.0f : 1.5f);

    if (flash > 0.0f)
    {
        g.setColour (accent.withAlpha (flash * 0.7f));
        g.drawRoundedRectangle (r.expanded ((1.0f - flash) * 7.0f), corner + 2.0f, 2.0f);
    }

    const auto textCol = accent.interpolatedWith (ink, a);
    const int textX = onId >= 0 ? 38 : 16;
    g.setColour (textCol);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (title.toUpperCase(), textX, 6, getWidth() - textX - 28, 19, juce::Justification::centredLeft);
    if (subtitle.isNotEmpty())
    {
        g.setColour (textCol.withAlpha (0.75f));
        g.setFont (juce::FontOptions (9.5f));
        g.drawText (subtitle, textX, 24, getWidth() - textX - 28, 12, juce::Justification::centredLeft);
    }

    if (onId >= 0)   // LED
    {
        const auto c = juce::Point<float> (21.0f, (float) kHeader * 0.5f);
        g.setColour (ink.withAlpha (0.8f * a));
        g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (accent.withAlpha (0.8f));
        g.drawEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f, 1.3f);
        if (a > 0.05f)
        {
            const float pulse = 0.78f + 0.22f * std::sin (phase * 3.0f);
            g.setColour (juce::Colours::white.withAlpha (a * pulse));
            g.fillEllipse (c.x - 3.6f, c.y - 3.6f, 7.2f, 7.2f);
            g.setColour (accent.withAlpha (a * 0.35f * pulse));
            g.fillEllipse (c.x - 11.0f, c.y - 11.0f, 22.0f, 22.0f);
        }
    }

    if (slot >= 0)   // grip
    {
        g.setColour (textCol.withAlpha (0.55f));
        for (int ix = 0; ix < 2; ++ix)
            for (int iy = 0; iy < 3; ++iy)
                g.fillEllipse ((float) getWidth() - 22.0f + (float) ix * 5.0f, 13.0f + (float) iy * 5.0f, 2.4f, 2.4f);
    }

    // activity line along the bottom
    if (a > 0.05f)
    {
        g.setColour (accent.withAlpha (a * (0.12f + 0.7f * sig)));
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
    dragArmed = slot >= 0 && e.y < kHeader;
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

        g.setColour (on ? (endpoint ? L::text : ink) : L::dim.withAlpha (0.7f));
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
        case SEq:     return make ("EQ",         cEq,    EqOn,    -1,        { EqLow, EqLowMid, EqHiMid, EqHigh }, "4-band parametric");
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
    daw = std::make_unique<DawPanel> (proc);
    midi = std::make_unique<MidiPanel> (proc);
    addChildComponent (*daw);
    addChildComponent (*midi);
    for (auto* t : { &tabBoard, &tabMidi, &tabDaw }) { t->setClickingTogglesState (false); addAndMakeVisible (t); }
    tabBoard.getProperties().set ("accent", (int) cGain.getARGB());
    tabMidi.getProperties().set ("accent", (int) cMod.getARGB());
    tabDaw.getProperties().set ("accent", (int) cTime.getARGB());
    tabBoard.onClick = [this] { setTab (0); };
    tabMidi.onClick  = [this] { setTab (1); };
    tabDaw.onClick   = [this] { setTab (2); };
    cancelLearn.onClick = [this] { proc.cancelMidiLearn(); };
    addChildComponent (cancelLearn);
    addAndMakeVisible (meterIn);
    addAndMakeVisible (meterOut);
    tabBoard.setToggleState (true, juce::dontSendNotification);

    {
        const auto o = proc.getChainOrder();
        cards.clear();
        for (int s : o) cards.push_back (bySlot[s]);
        strip->setOrder (o);
    }

    if (const int cp = proc.currentPreset.load(); cp >= 0) presetBox.setSelectedItemIndex (cp, juce::dontSendNotification);

    setResizable (true, true);
    setResizeLimits (1180, 620, 7680, 4320);
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

void NoirEditor::setTab (int tab)
{
    currentTab = tab;
    tabBoard.setToggleState (tab == 0, juce::dontSendNotification);
    tabMidi.setToggleState (tab == 1, juce::dontSendNotification);
    tabDaw.setToggleState (tab == 2, juce::dontSendNotification);

    strip->setVisible (tab == 0);
    viewport.setVisible (tab == 0);
    for (auto* b : { &allOn, &allOff, &resetOrder }) b->setVisible (tab == 0);
    midi->setVisible (tab == 1);
    daw->setVisible (tab == 2);
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

    juce::ColourGradient vignette (juce::Colour (0xff1a1a1f), (float) getWidth() * 0.5f, 0.0f,
                                   L::bg, (float) getWidth() * 0.5f, 200.0f, false);
    g.setGradientFill (vignette);
    g.fillRect (0, 0, getWidth(), 200);

    g.setColour (L::text);
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
        g.setColour (L::accent.withAlpha (0.9f));
        g.strokePath (w, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour (L::dim);
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("GUITAR  /  PEDALBOARD", 22, 38, 160, 12, juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (10.0f));
    g.setColour (L::dim.withAlpha (0.85f));
    g.drawText (perfText, perfArea, juce::Justification::centredLeft);

    if (const int learning = proc.getLearnTarget(); learning >= 0)
    {
        g.setColour (L::accent);
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
        const int rowH = juce::jlimit (206, 262, fit);
        contentH = nRows * rowH + gap * (nRows - 1) + 6;

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
                row[i]->targetBounds = { x, y, w, rowH };
                x += w + gap;
            }
            y += rowH + gap;
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
        h.removeFromLeft (230);

        tabBoard.setBounds (h.removeFromLeft (72).withSizeKeepingCentre (70, 28));
        h.removeFromLeft (4);
        tabMidi.setBounds (h.removeFromLeft (72).withSizeKeepingCentre (70, 28));
        h.removeFromLeft (4);
        tabDaw.setBounds (h.removeFromLeft (72).withSizeKeepingCentre (70, 28));
        h.removeFromLeft (14);

        power.setBounds (h.removeFromRight (108).withSizeKeepingCentre (108, 28));
        h.removeFromRight (8);
        resetOrder.setBounds (h.removeFromRight (62).withSizeKeepingCentre (62, 28));
        h.removeFromRight (6);
        allOff.setBounds (h.removeFromRight (68).withSizeKeepingCentre (68, 28));
        h.removeFromRight (6);
        allOn.setBounds (h.removeFromRight (60).withSizeKeepingCentre (60, 28));
        h.removeFromRight (14);

        auto meters = h.removeFromRight (290);
        meterIn.setBounds (meters.removeFromTop (headerArea.getHeight() / 2 - 2).withSizeKeepingCentre (meters.getWidth(), 22));
        meters.removeFromTop (4);
        meterOut.setBounds (meters.withSizeKeepingCentre (meters.getWidth(), 22));
        h.removeFromRight (14);

        auto mid = h.withSizeKeepingCentre (juce::jmin (h.getWidth(), 360), 28);
        cancelLearn.setBounds (mid.removeFromRight (80));
        prevPreset.setBounds (mid.removeFromLeft (30));
        mid.removeFromLeft (6);
        nextPreset.setBounds (mid.removeFromRight (30));
        mid.removeFromRight (6);
        presetBox.setBounds (mid);
    }

    area.removeFromTop (6);
    midi->setBounds (area);
    daw->setBounds (area);
    strip->setBounds (area.removeFromTop (30));
    area.removeFromTop (10);
    viewport.setBounds (area);

    layoutCards (false);
}
