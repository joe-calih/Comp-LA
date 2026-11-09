#include "PluginEditor.h"

using namespace juce;

namespace
{
    constexpr float kDeg = MathConstants<float>::pi / 180.f;

    struct VuPt { float db, f; };
    constexpr VuPt vuTable[] = { { -20, 0.00f }, { -10, 0.28f }, { -7, 0.43f }, { -5, 0.53f }, { -3, 0.63f },
                                 { -1, 0.73f }, { 0, 0.79f }, { 1, 0.85f }, { 2, 0.91f }, { 3, 1.00f } };

    float vuToPos (float db)
    {
        if (db <= vuTable[0].db) return 0.f;
        for (size_t i = 1; i < std::size (vuTable); ++i)
            if (db <= vuTable[i].db)
            {
                const float t = (db - vuTable[i - 1].db) / (vuTable[i].db - vuTable[i - 1].db);
                return vuTable[i - 1].f + t * (vuTable[i].f - vuTable[i - 1].f);
            }
        return 1.f;
    }

    void drawScrew (Graphics& g, float cx, float cy, float r)
    {
        ColourGradient gr (Colour (0xff5a5a5a), cx - r, cy - r, col::screwDark, cx + r, cy + r, false);
        g.setGradientFill (gr); g.fillEllipse (cx - r, cy - r, 2 * r, 2 * r);
        g.setColour (Colour (0xff070707)); g.drawEllipse (cx - r, cy - r, 2 * r, 2 * r, 1.2f);
        g.setColour (Colour (0xff9a9a9a));
        g.drawLine (cx - r * 0.55f, cy - r * 0.2f, cx + r * 0.55f, cy - r * 0.2f, 1.0f);
    }

    void drawJack (Graphics& g, float cx, float cy)
    {
        g.setColour (Colour (0xff0a0a0a)); g.fillEllipse (cx - 14, cy - 14, 28, 28);
        ColourGradient gr (Colour (0xff6e6e6e), cx, cy - 11, Colour (0xff1e1e1e), cx, cy + 11, false);
        g.setGradientFill (gr); g.fillEllipse (cx - 11, cy - 11, 22, 22);
        g.setColour (Colour (0xff000000)); g.fillEllipse (cx - 6, cy - 6, 12, 12);
    }
}

// ------------------------------------------------------------------ custom widgets
void ToggleSwitch::paintButton (Graphics& g, bool, bool)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    Path hex; hex.addPolygon (c, 6, 19.f, 0.f);
    ColourGradient nut (Colour (0xfff2f2f2), c.x - 18, c.y - 18, Colour (0xff8d9094), c.x + 18, c.y + 18, false);
    g.setColour (Colour (0x44000000)); g.fillPath (hex, AffineTransform::translation (1.5f, 2.f));
    g.setGradientFill (nut); g.fillPath (hex);
    g.setColour (Colour (0xff55585b)); g.strokePath (hex, PathStrokeType (1.2f));
    g.setColour (Colour (0xff2c2c2c)); g.fillEllipse (c.x - 11, c.y - 11, 22, 22);
    g.setColour (Colour (0xffa0a3a6)); g.drawEllipse (c.x - 11, c.y - 11, 22, 22, 1.2f);

    const bool up = getToggleState() != invert;
    const float len = up ? -17.f : 17.f;
    g.setColour (Colour (0xff1c1c1c)); g.drawLine (c.x, c.y, c.x, c.y + len, 9.f);
    ColourGradient lever (Colour (0xfff5f5f5), c.x - 3, c.y, Colour (0xff80848a), c.x + 3, c.y, false);
    g.setGradientFill (lever);
    g.fillRoundedRectangle (c.x - 3.f, jmin (c.y, c.y + len), 6.f, 17.f, 3.f);
    ColourGradient ball (Colour (0xffffffff), c.x - 3, c.y + len - 3, Colour (0xff6d7075), c.x + 4, c.y + len + 4, false);
    g.setGradientFill (ball); g.fillEllipse (c.x - 6.5f, c.y + len - 6.5f, 13.f, 13.f);
}

void PowerButton::paintButton (Graphics& g, bool, bool)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (getToggleState() ? col::accent : Colour (0xff3a3a3a));
    g.fillRoundedRectangle (b, 3.f);
    const auto c = b.getCentre();
    Path p;
    p.addCentredArc (c.x, c.y + 1, 8.5f, 8.5f, 0.f, 0.6f, MathConstants<float>::twoPi - 0.6f, true);
    p.startNewSubPath (c.x, c.y - 10.f); p.lineTo (c.x, c.y);
    g.setColour (getToggleState() ? Colour (0xff3a0f08) : Colour (0xff9a9a9a));
    g.strokePath (p, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

void VUMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (Colour (0xff070707)); g.fillRoundedRectangle (b, 9.f);
    g.setColour (Colour (0xff3b3b3b)); g.drawRoundedRectangle (b.reduced (1.f), 9.f, 1.5f);
    auto face = b.reduced (8.f);

    ColourGradient lamp (Colour (0xfffbe39a), face.getCentreX(), face.getBottom(),
                         Colour (0xffc98a28), face.getX(), face.getY(), true);
    g.setGradientFill (lamp); g.fillRoundedRectangle (face, 3.f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());

    const Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.45f);
    const float r1 = pivot.y - (face.getY() + 20.f);
    auto at = [&] (float f, float r) { const float a = (f - 0.5f) * 76.f * kDeg;
                                       return Point<float> (pivot.x + r * std::sin (a), pivot.y - r * std::cos (a)); };

    // red zone
    { Path red; const float a0 = (vuToPos (0.f) - 0.5f) * 76.f * kDeg, a1 = 38.f * kDeg;
      red.addCentredArc (pivot.x, pivot.y, r1 + 4.f, r1 + 4.f, 0.f, a0, a1, true);
      g.setColour (Colour (0xffb5281f)); g.strokePath (red, PathStrokeType (5.f)); }

    g.setColour (Colour (0xff2a1a05));
    { Path arc; arc.addCentredArc (pivot.x, pivot.y, r1 + 1.f, r1 + 1.f, 0.f, -38.f * kDeg, 38.f * kDeg, true);
      g.strokePath (arc, PathStrokeType (1.2f)); }

    g.setFont (uiFont (8.f, true));
    for (auto& t : vuTable)
    {
        const bool red = t.db > 0.5f;
        g.setColour (red ? Colour (0xffa01e17) : Colour (0xff2a1a05));
        g.drawLine ({ at (t.f, r1 - 5.f), at (t.f, r1 + 1.f) }, 1.3f);
        auto txt = (t.db > 0 ? "+" : "") + String ((int) t.db);
        if (t.db == -1.f) continue;
        g.drawText (txt, Rectangle<float> (26.f, 11.f).withCentre (at (t.f, r1 - 14.f)), Justification::centred, false);
    }
    const float r2 = r1 - 30.f;
    g.setColour (Colour (0xff2a1a05));
    for (int p = 0; p <= 100; p += 10)
    {
        const float f = (float) p / 100.f * 0.92f;
        g.drawLine ({ at (f, r2), at (f, r2 + (p % 20 == 0 ? 5.f : 3.f)) }, 1.0f);
        if (p % 20 == 0)
            g.drawText (String (p), Rectangle<float> (24.f, 10.f).withCentre (at (f, r2 - 8.f)), Justification::centred, false);
    }
    g.setFont (uiFont (13.f, true));
    g.drawText ("VU", Rectangle<float> (30.f, 16.f).withCentre ({ face.getRight() - 22.f, face.getBottom() - 11.f }),
                Justification::centred, false);

    // needle
    const auto tip = at (needle, r1 + 8.f);
    g.setColour (Colour (0x55000000)); g.drawLine ({ pivot.translated (2, 1), tip.translated (2, 1) }, 2.f);
    g.setColour (Colour (0xff150c04)); g.drawLine ({ pivot, tip }, 1.6f);

    // vignette + glass
    ColourGradient vig (Colour (0x00000000), face.getCentreX(), face.getCentreY(),
                        Colour (0x55301800), face.getX(), face.getY(), true);
    g.setGradientFill (vig); g.fillRect (face);
    ColourGradient glass (Colour (0x33ffffff), face.getX(), face.getY(), Colour (0x00ffffff), face.getX(), face.getCentreY(), false);
    g.setGradientFill (glass); g.fillRect (face.withHeight (face.getHeight() * 0.5f));
}

// ------------------------------------------------------------------ editor
void LALevelerAudioProcessorEditor::setupKnob (Slider& s, const char* id, const char* labels, int minor,
                                               Rectangle<int> b, const char* suffix, double def)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (-135.f * kDeg, 135.f * kDeg, true);
    s.getProperties().set ("labels", labels);
    s.getProperties().set ("minor", minor);
    s.setBounds (b);
    s.setTextValueSuffix (suffix);
    s.setDoubleClickReturnValue (true, def);
    s.setPopupDisplayEnabled (true, true, this);
    s.setLookAndFeel (&laf);
    addAndMakeVisible (s);
    juce::ignoreUnused (id);
}

LALevelerAudioProcessorEditor::LALevelerAudioProcessorEditor (LALevelerAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (978, 413);

    setupKnob (gain, "gain", "00|10|20|30|40|50|60|70|80|90|100", 2, { 236, 86, 128, 128 }, "", 35.0);
    setupKnob (peak, "peak", "OFF|10|20|30|40|50|60|70|80|90|100", 2, { 611, 88, 128, 128 }, "", 30.0);
    setupKnob (inLevel, "inlevel", "-15|~|~|0|~|~|+15", 2, { 245, 275, 104, 104 }, " dB", 0.0);
    setupKnob (output, "output", "-15|~|~|0|~|~|+15", 2, { 758, 275, 104, 104 }, " dB", 0.0);
    setupKnob (parallel, "parallel", "~|~|~|~|~|~|~|~|~|~|~", 1, { 628, 275, 104, 104 }, " %", 100.0);
    setupKnob (harm, "harm", "~", 1, { 528, 270, 46, 46 }, "", 0.25);
    setupKnob (lf, "lfsens", "~", 1, { 528, 337, 46, 46 }, " dB", 0.0);
    harm.getProperties().set ("arrowKnob", true); lf.getProperties().set ("arrowKnob", true);
    harm.setRotaryParameters (-90.f * kDeg, 90.f * kDeg, true);

    aGain.reset (new SA (proc.apvts, "gain", gain));       aPeak.reset (new SA (proc.apvts, "peak", peak));
    aIn.reset (new SA (proc.apvts, "inlevel", inLevel));   aOut.reset (new SA (proc.apvts, "output", output));
    aPar.reset (new SA (proc.apvts, "parallel", parallel)); aHarm.reset (new SA (proc.apvts, "harm", harm));
    aLf.reset (new SA (proc.apvts, "lfsens", lf));

    limitSw.setBounds (143, 163, 50, 64); tubeSw.setBounds (141, 296, 50, 64); msSw.setBounds (395, 296, 50, 64);
    for (auto* s : { &limitSw, &tubeSw, &msSw }) addAndMakeVisible (s);
    aLimit.reset (new BA (proc.apvts, "limit", limitSw)); aTube.reset (new BA (proc.apvts, "tube", tubeSw));
    aMs.reset (new BA (proc.apvts, "ms", msSw));

    power.setBounds (4, 5, 32, 32); addAndMakeVisible (power); aPower.reset (new BA (proc.apvts, "power", power));
    vu.setBounds (392, 96, 194, 96); addAndMakeVisible (vu);

    // top bar
    prevBtn.setBounds (42, 5, 30, 32);   nextBtn.setBounds (72, 5, 30, 32);
    presetBox.setBounds (106, 3, 312, 36);
    saveBtn.setBounds (424, 5, 52, 32);  saveAsBtn.setBounds (480, 5, 68, 32);
    aBtn.setBounds (562, 5, 32, 32);     abArrow.setBounds (596, 5, 24, 32);  bBtn.setBounds (622, 5, 32, 32);
    undoBtn.setBounds (662, 5, 38, 32);  redoBtn.setBounds (702, 5, 38, 32);
    menuBtn.setBounds (940, 3, 32, 36);
    menuBtn.setButtonText (String::fromUTF8 ("\xE2\x89\xA1"));
    for (auto* b : { &prevBtn, &nextBtn, &saveBtn, &saveAsBtn, &aBtn, &bBtn, &abArrow, &undoBtn, &redoBtn, &menuBtn })
        addAndMakeVisible (b);
    addAndMakeVisible (presetBox);
    for (auto* b : { &aBtn, &bBtn }) b->setClickingTogglesState (false);

    for (int i = 0; i < 4; ++i)
    {
        meterBtn[i].setBounds (404 + i * 40, 207, 38, 24);
        meterBtn[i].setClickingTogglesState (false);
        meterBtn[i].setColour (TextButton::buttonOnColourId, Colour (0xff2a2a2a));
        meterBtn[i].setColour (TextButton::textColourOnId, Colours::white);
        meterBtn[i].setColour (TextButton::textColourOffId, Colour (0xff777777));
        meterBtn[i].setColour (TextButton::buttonColourId, Colour (0xff111111));
        meterBtn[i].onClick = [this, i] { setMeterMode (i); };
        addAndMakeVisible (meterBtn[i]);
    }

    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id <= 0) return;
        if (id < 1000) proc.loadFactoryPreset (id - 1);
        else           proc.loadUserPreset (userNames[id - 1000]);
    };
    saveBtn.onClick = [this]
    {
        if (userNames.contains (proc.currentPresetName)) { proc.saveUserPreset (proc.currentPresetName); }
        else promptSaveAs();
    };
    saveAsBtn.onClick = [this] { promptSaveAs(); };
    aBtn.onClick = [this] { if (proc.getActiveSlot() != 0) proc.toggleAB(); };
    bBtn.onClick = [this] { if (proc.getActiveSlot() != 1) proc.toggleAB(); };
    abArrow.onClick = [this] { proc.toggleAB(); };
    undoBtn.onClick = [this] { proc.undoManager.undo(); };
    redoBtn.onClick = [this] { proc.undoManager.redo(); };
    menuBtn.onClick = [this] { showMainMenu(); };

    buildPresetMenu();
    startTimerHz (30);
}

LALevelerAudioProcessorEditor::~LALevelerAudioProcessorEditor()
{
    for (auto* s : { &gain, &peak, &inLevel, &output, &parallel, &harm, &lf }) s->setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void LALevelerAudioProcessorEditor::buildPresetMenu()
{
    presetBox.clear (dontSendNotification);
    auto& f = LALevelerAudioProcessor::factoryPresets();
    for (size_t i = 0; i < f.size(); ++i) presetBox.addItem (f[i].name, (int) i + 1);
    userNames = proc.getUserPresetNames();
    if (userNames.size() > 0)
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("User");
        for (int i = 0; i < userNames.size(); ++i) presetBox.addItem (userNames[i], 1000 + i);
    }
    bool found = false;
    for (int i = 0; i < presetBox.getNumItems() && ! found; ++i)
        if (presetBox.getItemText (i) == proc.currentPresetName) { presetBox.setSelectedItemIndex (i, dontSendNotification); found = true; }
    if (! found) { presetBox.setSelectedId (0, dontSendNotification);
                   presetBox.setTextWhenNothingSelected (proc.currentPresetName); }
}

void LALevelerAudioProcessorEditor::stepPreset (int dir)
{
    int idx = -1;
    auto& f = LALevelerAudioProcessor::factoryPresets();
    for (size_t i = 0; i < f.size(); ++i) if (proc.currentPresetName == f[i].name) idx = (int) i;
    idx = (idx + dir + (int) f.size()) % (int) f.size();
    proc.loadFactoryPreset (idx);
    buildPresetMenu();
}

void LALevelerAudioProcessorEditor::promptSaveAs()
{
    auto* w = new AlertWindow ("Save preset", "Preset name:", MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.currentPresetName == "Initial Preset" ? String ("My Preset") : proc.currentPresetName);
    w->addButton ("Save", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    Component::SafePointer<LALevelerAudioProcessorEditor> safe (this);
    w->enterModalState (true, ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (r == 1 && safe != nullptr)
        {
            safe->proc.saveUserPreset (w->getTextEditorContents ("name"));
            safe->buildPresetMenu();
        }
    }), true);
}

void LALevelerAudioProcessorEditor::setMeterMode (int m)
{
    if (auto* p = proc.apvts.getParameter ("meter")) p->setValueNotifyingHost (p->convertTo0to1 ((float) m));
}

void LALevelerAudioProcessorEditor::showMainMenu()
{
    PopupMenu m;
    m.addItem (1, "Reset all controls to default");
    m.addItem (2, "Open user preset folder");
    m.addSeparator();
    m.addItem (10, "Size 75%"); m.addItem (11, "Size 100%"); m.addItem (12, "Size 125%"); m.addItem (13, "Size 150%");
    m.addSeparator();
    m.addItem (20, "About Comp LA / Loops");
    m.addItem (21, "Visit loops.co.ke");
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (&menuBtn), [this] (int r)
    {
        if (r == 1)
        {
            proc.undoManager.beginNewTransaction();
            for (auto* prm : proc.getParameters())
                if (auto* rp = dynamic_cast<RangedAudioParameter*> (prm))
                    if (rp->paramID != "meter") rp->setValueNotifyingHost (rp->getDefaultValue());
            proc.currentPresetName = "Initial Preset"; buildPresetMenu();
        }
        else if (r == 2) { auto d = proc.getUserPresetDir(); d.createDirectory(); d.revealToUser(); }
        else if (r == 20) showAbout();
        else if (r == 21) URL ("https://loops.co.ke").launchInDefaultBrowser();
        else if (r >= 10 && r <= 13) setScaleFactor (r == 10 ? 0.75f : r == 11 ? 1.f : r == 12 ? 1.25f : 1.5f);
    });
}

void LALevelerAudioProcessorEditor::showAbout()
{
    auto* w = new AlertWindow ("About Comp LA",
        "Comp LA 1.0  -  opto leveling compressor (2A / 3A style)\n"
        "Made by LOOPS\n\n"
        "LOOPS is a music-production platform for royalty-free sound packs, loops and one-shots, "
        "plugins, mastering and distribution, built for producers.\n\n"
        "https://loops.co.ke",
        MessageBoxIconType::NoIcon, this);
    w->addButton ("Visit loops.co.ke", 1);
    w->addButton ("Close", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([] (int r)
    { if (r == 1) URL ("https://loops.co.ke").launchInDefaultBrowser(); }), true);
}

void LALevelerAudioProcessorEditor::timerCallback()
{
    const int mode = (int) proc.apvts.getRawParameterValue ("meter")->load();
    for (int i = 0; i < 4; ++i) meterBtn[i].setToggleState (i == mode, dontSendNotification);
    aBtn.setToggleState (proc.getActiveSlot() == 0, dontSendNotification);
    bBtn.setToggleState (proc.getActiveSlot() == 1, dontSendNotification);
    undoBtn.setEnabled (proc.undoManager.canUndo()); redoBtn.setEnabled (proc.undoManager.canRedo());

    float target = 0.f;
    switch (mode)
    {
        case 0:  target = vuToPos (proc.meterIn.load()  + 14.f); break;
        case 1:  target = jlimit (0.f, 1.f, proc.meterGR.load() / 20.f); break;
        case 2:  target = vuToPos (proc.meterOut.load() + 14.f); break;
        default: target = vuToPos (proc.meterHar.load() + 34.f); break;
    }
    needleSmooth += (target - needleSmooth) * 0.22f;
    vu.setGRMode (mode == 1); vu.setValue (needleSmooth);

    if (proc.currentPresetName != presetBox.getText() && presetBox.getSelectedId() != 0) buildPresetMenu();
}

void LALevelerAudioProcessorEditor::paint (Graphics& g)
{
    const int W = getWidth();
    g.fillAll (Colours::black);
    g.setColour (col::topBar); g.fillRect (0, 0, W, 43);

    // upper (light) and lower (silver) panels
    g.setGradientFill (ColourGradient (col::panelHi, 0, 44, col::panelLo, 0, 247, false)); g.fillRect (0, 44, W, 203);
    g.setGradientFill (ColourGradient (col::silverHi, 0, 249, col::silverLo, 0, 413, false)); g.fillRect (0, 249, W, 164);
    g.setColour (Colour (0xff6f7173)); g.fillRect (0, 246, W, 3);
    g.setColour (Colour (0x66ffffff)); g.fillRect (0, 249, W, 1);

    // jacks + screws
    for (float x : { 22.f, 955.f }) { drawJack (g, x, 96.f); drawJack (g, x, 190.f); drawJack (g, x, 287.f); drawJack (g, x, 378.f); }
    for (float x : { 72.f, 905.f }) { drawScrew (g, x, 104.f, 9.f); drawScrew (g, x, 183.f, 9.f); drawScrew (g, x, 287.f, 9.f); drawScrew (g, x, 378.f, 9.f); }

    // logo
    g.setColour (col::logoRed);
    g.setFont (Font (FontOptions (24.f, Font::bold | Font::italic)));
    g.drawText ("Comp LA", 100, 54, 200, 30, Justification::left, false);
    g.setFont (uiFont (8.f, true));
    g.drawText ("LEVELING AMPLIFIER  -  MODEL 2A / 3A", 102, 84, 220, 10, Justification::left, false);
    g.fillRect (102, 96, 130, 2);

    g.setColour (col::ink);
    auto label = [&g] (const String& t, float cx, float cy, float h = 11.5f, bool bold = true)
    { g.setFont (uiFont (h, bold)); g.drawText (t, Rectangle<float> (130.f, 16.f).withCentre ({ cx, cy }), Justification::centred, false); };

    label ("LIMIT", 168, 158);  label ("COMPRESS", 168, 232);
    label ("GAIN", 300, 211, 13.f);
    label ("PEAK REDUCTION", 677, 211, 13.f);

    label ("TUBE (2A)", 166, 292);  label ("SOLID STATE (3A)", 166, 365);
    label ("IN LEVEL", 297, 392, 12.f);
    label ("STEREO", 420, 292);     label ("MID-SIDE", 420, 365);
    label ("ORIGINAL", 551, 266, 10.f);
    label ("LIN", 517, 293, 10.f);  label ("DIST", 586, 293, 10.f);
    label ("HARMONICS", 551, 322, 10.f);
    label ("0", 521, 374, 10.f);    label ("-10dB", 585, 374, 10.f);
    label ("LF SENS", 551, 397, 10.f);
    label ("DRY", 638, 364, 10.f);  label ("COMP", 722, 364, 10.f);
    label ("PARALLEL", 680, 392, 12.f);
    label ("OUTPUT", 810, 392, 12.f);

    // meter-mode button tray
    g.setColour (Colour (0xff0b0b0b)); g.fillRoundedRectangle (400.f, 203.f, 168.f, 32.f, 4.f);

    // top bar chrome
    g.setColour (Colours::white); g.setFont (uiFont (10.f, true));
}
