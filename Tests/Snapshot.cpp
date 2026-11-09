#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>
using namespace juce;

int main()
{
    ScopedJuceInitialiser_GUI init;
    LALevelerAudioProcessor proc;
    proc.prepareToPlay (48000.0, 512);

    // ---- DSP test: 1 kHz sine, quiet -> loud, check gain reduction and output level
    auto run = [&] (float ampDb, float peak, bool limit)
    {
        proc.apvts.getParameter ("peak")->setValueNotifyingHost (proc.apvts.getParameter ("peak")->convertTo0to1 (peak));
        proc.apvts.getParameter ("limit")->setValueNotifyingHost (limit ? 1.f : 0.f);
        proc.prepareToPlay (48000.0, 512);
        AudioBuffer<float> b (2, 512); MidiBuffer m; double ph = 0; float grLast = 0, outDb = 0;
        for (int blk = 0; blk < 300; ++blk)
        {
            for (int i = 0; i < 512; ++i) { float s = Decibels::decibelsToGain (ampDb) * (float) std::sin (ph); ph += 2 * MathConstants<double>::pi * 1000 / 48000;
                                            b.setSample (0, i, s); b.setSample (1, i, s); }
            proc.processBlock (b, m); grLast = proc.meterGR.load(); outDb = proc.meterOut.load();
        }
        std::printf ("in %5.1f dB peak=%3.0f limit=%d -> GR %5.2f dB, out rms %6.2f dB, NaN=%d\n", ampDb, peak, (int) limit, grLast, outDb,
                     (int) std::isnan (b.getSample (0, 100)));
    };
    for (float a : { -40.f, -20.f, -10.f, -3.f }) run (a, 50.f, false);
    run (-10.f, 50.f, true); run (-10.f, 0.f, false);

    proc.loadFactoryPreset (0);
    auto* ed = proc.createEditor();
    ed->setBounds (0, 0, 978, 413);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    PNGImageFormat png; File out ("/home/claude/snapshot.png"); out.deleteFile();
    FileOutputStream os (out); png.writeImageToStream (img, os);
    delete ed;
    std::printf ("snapshot %dx%d\n", img.getWidth(), img.getHeight());
    return 0;
}
