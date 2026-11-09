#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

// ------------------------------------------------------------------ parameters
AudioProcessorValueTreeState::ParameterLayout LALevelerAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto fl = [] (const char* id, const char* nm, float lo, float hi, float def, float step = 0.01f)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (lo, hi, step), def);
    };
    auto bl = [] (const char* id, const char* nm, bool def)
    {
        return std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, nm, def);
    };
    p.push_back (bl ("power", "Power", true));
    p.push_back (fl ("gain", "Gain", 0.f, 100.f, 35.f, 0.1f));
    p.push_back (fl ("peak", "Peak Reduction", 0.f, 100.f, 30.f, 0.1f));
    p.push_back (bl ("limit", "Limit", false));
    p.push_back (bl ("tube", "Tube (2A)", true));
    p.push_back (fl ("inlevel", "In Level", -15.f, 15.f, 0.f, 0.1f));
    p.push_back (fl ("output", "Output", -15.f, 15.f, 0.f, 0.1f));
    p.push_back (fl ("parallel", "Parallel", 0.f, 100.f, 100.f, 0.1f));
    p.push_back (bl ("ms", "Mid-Side", false));
    p.push_back (fl ("harm", "Harmonics", 0.f, 1.f, 0.5f, 0.001f));   // 0 = Lin, 0.5 = Original, 1 = Dist
    p.push_back (fl ("lfsens", "LF Sens", 0.f, 10.f, 0.f, 0.1f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "meter", 1 }, "Meter",
                                                         StringArray { "IN", "GR", "OUT", "HAR" }, 1));
    return { p.begin(), p.end() };
}

LALevelerAudioProcessor::LALevelerAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "STATE", createLayout())
{
}

bool LALevelerAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == AudioChannelSet::mono() || in == AudioChannelSet::stereo());
}

void LALevelerAudioProcessor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;
    if (! toneInit) { toneInit = true; if (wrapperType == wrapperType_Standalone) testTone = true; }  // standalone has no input: demo signal
    for (auto& f : scShelf) { f.reset(); }
    lastLf = -1.f;
    for (int i = 0; i < 2; ++i) { env[i] = gr[i] = mem[i] = dcX[i] = dcY[i] = 0.f; }
    smInGain.reset (sampleRate, 0.02);  smMakeup.reset (sampleRate, 0.03);
    smOutGain.reset (sampleRate, 0.02); smMix.reset (sampleRate, 0.03); smHarm.reset (sampleRate, 0.03);
    auto g = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    smInGain.setCurrentAndTargetValue (Decibels::decibelsToGain (g ("inlevel")));
    smMakeup.setCurrentAndTargetValue (-12.f + 0.36f * g ("gain"));
    smOutGain.setCurrentAndTargetValue (Decibels::decibelsToGain (g ("output")));
    smMix.setCurrentAndTargetValue (g ("parallel") * 0.01f);
    smHarm.setCurrentAndTargetValue (g ("harm"));
}

// ------------------------------------------------------------------ DSP
static inline float gainComputer (float levelDb, float thresh, float ratio, float knee)
{
    const float over = levelDb - thresh, slope = 1.f - 1.f / ratio;
    if (2.f * over < -knee) return 0.f;
    if (2.f * std::abs (over) <= knee) { const float t = over + knee * 0.5f; return slope * t * t / (2.f * knee); }
    return slope * over;
}

void LALevelerAudioProcessor::processBlock (AudioBuffer<float>& buf, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    const int n = buf.getNumSamples();
    const int nch = jmin (2, buf.getNumChannels());
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    if (get ("power") < 0.5f)
    {
        meterIn = meterOut = meterHar = -100.f; meterGR = 0.f;
        return;
    }

    if (testTone.load())      // built-in demo signal (standalone has no input; also handy to check the meters)
    {
        for (int i = 0; i < n; ++i)
        {
            toneLfo += 1.0 / (fs * 5.0); if (toneLfo > 1.0) toneLfo -= 1.0;
            const double lvlDb = -34.0 + 26.0 * (0.5 + 0.5 * std::sin (6.283185307 * toneLfo));
            tonePhase += 6.283185307 * 110.0 / fs; if (tonePhase > 6.283185307) tonePhase -= 6.283185307;
            const float v = (float) (Decibels::decibelsToGain (lvlDb) * (0.6 * std::sin (tonePhase) + 0.3 * std::sin (2.0 * tonePhase) + 0.1 * std::sin (11.0 * tonePhase)));
            for (int c = 0; c < nch; ++c) buf.getWritePointer (c)[i] = v;
        }
    }

    const float peakP  = get ("peak") * 0.01f;
    const bool  compOn = get ("peak") >= 1.0f;           // knob fully left = COMP OFF
    const bool  limit  = get ("limit") > 0.5f;
    const bool  tube   = get ("tube") > 0.5f;
    const bool  ms     = get ("ms") > 0.5f && nch == 2;
    const float lf     = get ("lfsens");

    smInGain.setTargetValue (Decibels::decibelsToGain (get ("inlevel")));
    smMakeup.setTargetValue (-12.f + 0.36f * get ("gain"));
    smOutGain.setTargetValue (Decibels::decibelsToGain (get ("output")));
    smMix.setTargetValue (get ("parallel") * 0.01f);
    smHarm.setTargetValue (get ("harm"));

    if (std::abs (lf - lastLf) > 1.0e-4f)
    {
        lastLf = lf;
        auto c = dsp::IIR::Coefficients<float>::makeLowShelf (fs, 150.0, 0.7071f, Decibels::decibelsToGain (-lf));
        for (auto& f : scShelf) f.coefficients = c;
    }

    const float thresh = limit ? (-2.f - 34.f * peakP) : (-4.f - 38.f * peakP);
    const float ratio  = limit ? 12.f : 3.f;
    const float knee   = limit ? 4.f : 14.f;
    const float aAtk   = 1.f / (float) (fs * (limit ? 0.004 : 0.010));
    const float aEnvR  = 1.f / (float) (fs * 0.006);
    const float aMem   = 1.f / (float) (fs * 3.0);
    const bool  linked = ! ms;

    float* d[2] = { buf.getWritePointer (0), nch > 1 ? buf.getWritePointer (1) : nullptr };

    double sIn = 0, sOut = 0, sHar = 0; float grMax = 0.f;

    for (int i = 0; i < n; ++i)
    {
        const float inG = smInGain.getNextValue(), mk = smMakeup.getNextValue();
        const float outG = smOutGain.getNextValue(), mix = smMix.getNextValue(), h = smHarm.getNextValue();

        float x[2] = { d[0][i] * inG, nch > 1 ? d[1][i] * inG : 0.f };
        const float dry[2] = { x[0], x[1] };
        sIn += 0.5 * (x[0] * x[0] + x[1] * x[1]);

        float a[2] = { x[0], x[1] };
        if (ms) { a[0] = 0.5f * (x[0] + x[1]); a[1] = 0.5f * (x[0] - x[1]); }

        float sc[2];
        for (int c = 0; c < nch; ++c) sc[c] = scShelf[(size_t) c].processSample (a[c]);

        // detector
        float det[2];
        if (linked) { const float p = nch > 1 ? 0.5f * (sc[0] * sc[0] + sc[1] * sc[1]) : sc[0] * sc[0];
                      det[0] = det[1] = std::sqrt (p) * 1.2f; }
        else        { det[0] = std::abs (sc[0]); det[1] = std::abs (sc[1]); }

        float y[2] = { 0, 0 };
        for (int c = 0; c < nch; ++c)
        {
            env[c] = det[c] > env[c] ? det[c] : env[c] - env[c] * aEnvR;      // short peak hold
            float target = 0.f;
            if (compOn) target = jmin (30.f, gainComputer (Decibels::gainToDecibels (env[c] + 1.0e-9f), thresh, ratio, knee));

            // opto-style behaviour: fast-ish attack, release that slows down with programme memory
            if (target > gr[c]) gr[c] += (target - gr[c]) * aAtk;
            else {
                const float tRel = 0.06f + mem[c] * 1.9f;
                gr[c] += (target - gr[c]) * (1.f / ((float) fs * tRel));
            }
            mem[c] += (jmin (gr[c] / 12.f, 1.f) - mem[c]) * aMem;
            if (gr[c] < 0.f) gr[c] = 0.f;

            y[c] = a[c] * Decibels::decibelsToGain (mk - gr[c]);
        }
        grMax = jmax (grMax, nch > 1 ? 0.5f * (gr[0] + gr[1]) : gr[0]);

        // tube (2A) / solid-state (3A) colour
        float har = 0.f;
        for (int c = 0; c < nch; ++c)
        {
            float s;
            const float hb = (h - 0.5f) * 2.f;                    // -1 (Lin) .. 0 (Original) .. +1 (Dist)
            const float up = jmax (0.f, hb);
            if (tube)
            {
                const float k = (1.f + up * 5.f) * 0.7f, bias = 0.12f;
                const float t = std::tanh (bias);
                s = (std::tanh (k * y[c] + bias) - t) / (k * (1.f - t * t));
                const float amt = hb < 0.f ? 0.25f * (1.f + hb) : 0.25f + 0.75f * hb;
                s = y[c] + (s - y[c]) * amt;
            }
            else
            {
                const float k = (1.f + up * 4.f) * 0.7f;
                s = std::tanh (k * y[c]) / k;
                const float amt = hb < 0.f ? 0.10f * (1.f + hb) : 0.10f + 0.90f * hb;
                s = y[c] + (s - y[c]) * amt;
            }
            har += (s - y[c]) * (s - y[c]);
            // DC blocker
            const float o = s - dcX[c] + 0.995f * dcY[c]; dcX[c] = s; dcY[c] = o;
            y[c] = o;
        }
        sHar += har;

        float o[2] = { y[0], y[1] };
        if (ms) { o[0] = y[0] + y[1]; o[1] = y[0] - y[1]; }

        for (int c = 0; c < nch; ++c)
        {
            const float out = (dry[c] * (1.f - mix) + o[c] * mix) * outG;
            d[c][i] = out;
            sOut += 0.5 * out * out * (nch > 1 ? 1.0 : 2.0);
        }
    }

    const double inv = 1.0 / jmax (1, n);
    meterIn  = (float) (10.0 * std::log10 (sIn  * inv + 1.0e-12));
    meterOut = (float) (10.0 * std::log10 (sOut * inv + 1.0e-12));
    meterHar = (float) (10.0 * std::log10 (sHar * inv * 4.0 + 1.0e-12));
    meterGR  = grMax;
}

// ------------------------------------------------------------------ presets
const std::vector<LALevelerAudioProcessor::Preset>& LALevelerAudioProcessor::factoryPresets()
{
    //            name                       gain peak limit tube  in   out  par   ms   harm  lf
    static const std::vector<Preset> p {
        { "Initial Preset",                  35,  30, false, true,  0.f, 0.f, 100,  false, 0.625f, 0.f },
        { "Vocal - Smooth Leveler",          42,  38, false, true,  0.f, 0.f, 100,  false, 0.650f, 2.f },
        { "Vocal - Intimate Close Mic",      45,  46, false, true,  1.f,-1.f,  85,  false, 0.675f, 3.f },
        { "Vocal - Hard Limit Presence",     48,  55, true,  false, 0.f,-1.f, 100,  false, 0.600f, 2.f },
        { "Vocal - Parallel Air",            50,  70, true,  true,  0.f, 0.f,  35,  false, 0.725f, 3.f },
        { "Bass - Tube Warmth",              44,  42, false, true,  0.f, 0.f, 100,  false, 0.775f, 0.f },
        { "Bass - Tight Solid State",        40,  50, true,  false, 0.f, 0.f, 100,  false, 0.575f, 4.f },
        { "Drum Bus - Glue",                 36,  28, false, false, 0.f, 0.f, 100,  false, 0.600f, 5.f },
        { "Drum Bus - Room Squash",          55,  75, true,  true,  1.f,-2.f,  45,  false, 0.800f, 3.f },
        { "Acoustic Guitar - Even Out",      40,  35, false, true,  0.f, 0.f, 100,  false, 0.600f, 3.f },
        { "Electric Guitar - Sustain",       50,  52, false, true,  1.f,-1.f, 100,  false, 0.750f, 2.f },
        { "Piano - Gentle Leveling",         36,  22, false, false, 0.f, 0.f, 100,  false, 0.550f, 2.f },
        { "Synth Pad - Hold Steady",         38,  30, false, true,  0.f, 0.f, 100,  false, 0.650f, 0.f },
        { "Mix Bus - Light 2A",              34,  16, false, true,  0.f, 0.f, 100,  false, 0.575f, 6.f },
        { "Mix Bus - Clean 3A",              33,  18, false, false, 0.f, 0.f, 100,  false, 0.525f, 6.f },
        { "M/S - Tame the Mids",             36,  30, false, false, 0.f, 0.f, 100,  true,  0.575f, 4.f },
        { "Parallel Smash",                  60,  85, true,  true,  2.f,-3.f,  30,  false, 0.850f, 3.f },
        { "Broadcast Voice",                 50,  60, true,  false, 0.f, 0.f, 100,  false, 0.625f, 4.f },
        { "Sub Taming (LF Sens)",            38,  40, false, false, 0.f, 0.f, 100,  false, 0.550f, 10.f },
    };
    return p;
}

void LALevelerAudioProcessor::setParam (const char* id, float v)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void LALevelerAudioProcessor::loadFactoryPreset (int idx)
{
    auto& all = factoryPresets();
    if (! isPositiveAndBelow (idx, (int) all.size())) return;
    const auto& q = all[(size_t) idx];
    undoManager.beginNewTransaction();
    setParam ("gain", q.gain);       setParam ("peak", q.peak);
    setParam ("limit", q.limit);     setParam ("tube", q.tube);
    setParam ("inlevel", q.inLevel); setParam ("output", q.output);
    setParam ("parallel", q.parallel); setParam ("ms", q.midSide);
    setParam ("harm", q.harm);       setParam ("lfsens", q.lf);
    currentPresetName = q.name;
}

File LALevelerAudioProcessor::getUserPresetDir() const
{
    return File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("Comp LA").getChildFile ("Presets");
}

StringArray LALevelerAudioProcessor::getUserPresetNames() const
{
    StringArray s;
    for (auto& f : getUserPresetDir().findChildFiles (File::findFiles, false, "*.lalpreset"))
        s.add (f.getFileNameWithoutExtension());
    s.sort (true);
    return s;
}

bool LALevelerAudioProcessor::saveUserPreset (const String& name)
{
    auto clean = File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = getUserPresetDir();
    if (! dir.createDirectory().wasOk()) return false;
    if (! exportPresetTo (dir.getChildFile (clean + ".lalpreset"))) return false;
    currentPresetName = clean;
    return true;
}

bool LALevelerAudioProcessor::loadUserPreset (const String& name)
{
    return importPresetFrom (getUserPresetDir().getChildFile (name + ".lalpreset"), true);
}

bool LALevelerAudioProcessor::importPresetFrom (const File& f, bool applyIt)
{
    if (auto xml = parseXML (f))
    {
        undoManager.beginNewTransaction();
        auto vt = ValueTree::fromXml (*xml);
        if (vt.isValid())
        {
            for (auto c : vt)      // only restore parameter values, keep host-side identity intact
                if (auto* p = apvts.getParameter (c.getProperty ("id").toString()))
                    p->setValueNotifyingHost (p->convertTo0to1 ((float) c.getProperty ("value")));
            if (! applyIt) return true;
            currentPresetName = f.getFileNameWithoutExtension();
            presetNote = vt.getProperty ("note", "").toString();
            return true;
        }
    }
    return false;
}

bool LALevelerAudioProcessor::exportPresetTo (const File& f)
{
    auto st = apvts.copyState(); st.setProperty ("note", presetNote, nullptr);
    std::unique_ptr<XmlElement> xml (st.createXml());
    return xml != nullptr && f.getParentDirectory().createDirectory().wasOk() && xml->writeTo (f);
}

String LALevelerAudioProcessor::stateToString()
{
    auto st = apvts.copyState(); st.setProperty ("note", presetNote, nullptr);
    if (auto xml = st.createXml()) return xml->toString();
    return {};
}

bool LALevelerAudioProcessor::applyStateString (const String& text)
{
    if (auto xml = parseXML (text))
    {
        auto vt = ValueTree::fromXml (*xml);
        if (! vt.isValid() || vt.getNumChildren() == 0) return false;
        undoManager.beginNewTransaction();
        bool any = false;
        for (auto c : vt)
            if (auto* p = apvts.getParameter (c.getProperty ("id").toString()))
            { p->setValueNotifyingHost (p->convertTo0to1 ((float) c.getProperty ("value"))); any = true; }
        presetNote = vt.getProperty ("note", "").toString();
        return any;
    }
    return false;
}

void LALevelerAudioProcessor::toggleAB()
{
    slots[(size_t) activeSlot] = apvts.copyState();
    const int other = 1 - activeSlot;
    if (! slots[(size_t) other].isValid()) slots[(size_t) other] = apvts.copyState();
    activeSlot = other;
    apvts.replaceState (slots[(size_t) activeSlot].createCopy());
}

// ------------------------------------------------------------------ state / editor
void LALevelerAudioProcessor::getStateInformation (MemoryBlock& dest)
{
    auto st = apvts.copyState();
    st.setProperty ("presetName", currentPresetName, nullptr);
    st.setProperty ("note", presetNote, nullptr);
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, dest);
}

void LALevelerAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto vt = ValueTree::fromXml (*xml);
        if (vt.isValid())
        {
            currentPresetName = vt.getProperty ("presetName", "Initial Preset").toString();
            presetNote = vt.getProperty ("note", "").toString();
            apvts.replaceState (vt);
        }
    }
}

AudioProcessorEditor* LALevelerAudioProcessor::createEditor() { return new LALevelerAudioProcessorEditor (*this); }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LALevelerAudioProcessor(); }
