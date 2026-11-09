#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>
using namespace juce;

static void collect (Component* c, Array<Component*>& out) { for (auto* ch : c->getChildren()) { out.add (ch); collect (ch, out); } }

int main()
{
    setvbuf (stdout, nullptr, _IONBF, 0);
    ScopedJuceInitialiser_GUI init;
    LALevelerAudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);
    std::unique_ptr<AudioProcessorEditor> ed (proc.createEditor());
    ed->setBounds (0, 0, 978, 413);
    Array<Component*> kids; collect (ed.get(), kids);

    auto param = [&] (const char* id) { return proc.apvts.getRawParameterValue (id)->load(); };
    AudioBuffer<float> b (2, 512); MidiBuffer m; double ph = 0;
    auto feed = [&] (float ampDb, int blocks)
    {
        for (int k = 0; k < blocks; ++k)
        {
            for (int i = 0; i < 512; ++i) { float s = Decibels::decibelsToGain (ampDb) * (float) std::sin (ph); ph += 2 * MathConstants<double>::pi * 440 / 48000;
                                            b.setSample (0, i, s); b.setSample (1, i, s); }
            proc.processBlock (b, m);
            MessageManager::getInstance()->runDispatchLoopUntil (5);
        }
    };

    // --- buttons
    for (auto* c : kids)
        if (auto* bt = dynamic_cast<Button*> (c))
        {
            const auto t = bt->getButtonText();
            if (bt->getX() == 932 || t == "SAVE" || t == "SAVE AS" || t.containsNonWhitespaceChars() && t.length() == 1 && (juce_wchar) t[0] > 127) continue;
            const auto before = std::make_tuple (param ("power"), param ("limit"), param ("tube"), param ("ms"), param ("meter"), proc.getActiveSlot());
            bt->triggerClick();
            MessageManager::getInstance()->runDispatchLoopUntil (50);
            const auto after = std::make_tuple (param ("power"), param ("limit"), param ("tube"), param ("ms"), param ("meter"), proc.getActiveSlot());
            std::printf ("button '%s' bounds(%d,%d) enabled=%d showing=%d visible=%d : %s\n", bt->getButtonText().toRawUTF8(), bt->getX(), bt->getY(),
                         (int) bt->isEnabled(), (int) bt->isShowing(), (int) bt->isVisible(), before != after ? "CHANGED STATE" : "no param change");
            if (before != after) bt->triggerClick();  // restore for toggles
        }

    // --- knobs: set value through slider, see parameter + audio effect
    for (auto* c : kids)
        if (auto* s = dynamic_cast<Slider*> (c))
        {
            const double v0 = s->getValue();
            s->setValue (s->getMaximum(), sendNotificationSync);
            MessageManager::getInstance()->runDispatchLoopUntil (20);
            std::printf ("slider range %.1f..%.1f: %.2f -> %.2f\n", s->getMinimum(), s->getMaximum(), v0, s->getValue());
            s->setValue (v0, sendNotificationSync);
        }

    // --- meters through the editor timer
    proc.loadFactoryPreset (0);
    feed (-18.f, 120);
    std::printf ("meters: in %.1f out %.1f gr %.2f har %.1f\n", proc.meterIn.load(), proc.meterOut.load(), proc.meterGR.load(), proc.meterHar.load());
    auto snap = [&] (const char* path)
    {
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        File out (path); out.deleteFile(); FileOutputStream os (out); PNGImageFormat().writeImageToStream (img, os);
    };
    auto setP = [&] (const char* id, float v) { auto* p = proc.apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    setP ("tube", 1.f); setP ("meter", 1.f); setP ("peak", 55.f); feed (-12.f, 100);
    MessageManager::getInstance()->runDispatchLoopUntil (400); snap ("/home/claude/snap_light.png");
    setP ("tube", 0.f); setP ("meter", 0.f); feed (-12.f, 100);
    MessageManager::getInstance()->runDispatchLoopUntil (400); snap ("/home/claude/snap_dark.png");
    return 0;
}
