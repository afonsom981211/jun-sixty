#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>

namespace sixty
{
inline constexpr int kNumChords = 11;
inline constexpr const char* kChordNames[kNumChords] = { "Octave", "Fifth", "Sus4", "Minor", "Minor7", "Minor9",
                                                         "Minor11", "Major", "Major7", "Major9", "Major11" };
inline constexpr int kChordIntervals[kNumChords][6] = {
    { 0, 12, -1, -1, -1, -1 }, { 0, 7, 12, -1, -1, -1 }, { 0, 5, 7, -1, -1, -1 }, { 0, 3, 7, -1, -1, -1 },
    { 0, 3, 7, 10, -1, -1 },   { 0, 3, 7, 10, 14, -1 },  { 0, 3, 7, 10, 14, 17 }, { 0, 4, 7, -1, -1, -1 },
    { 0, 4, 7, 11, -1, -1 },   { 0, 4, 7, 11, 14, -1 },  { 0, 4, 7, 11, 14, 17 }
};

struct EngineParams
{
    bool arpOn = false, hold = false, chordOn = false;
    int arpMode = 0;          // 0 up, 1 up & down, 2 down
    int arpRange = 1;         // 1..3 oitavas
    int chordType = 0;
    double stepSamples = 5000.0;
};

// Entre o MIDI recebido e as vozes: hold, modo de acordes e arpejador (sample-accurate).
class NoteEngine
{
public:
    NoteEngine() { reset(); }

    void reset()
    {
        nActive = 0; nPhys = 0;
        counts.fill (0);
        arpRunning = false; stepAt = 0.0; gateOffAt = -1.0; curArpNote = -1; arpIndex = 0;
        lastVel = 100;
        lastP = EngineParams {};
    }

    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const EngineParams& p)
    {
        cur = &p;
        outBuf = &out;

        if (p.arpOn != lastP.arpOn || p.chordOn != lastP.chordOn || p.chordType != lastP.chordType)
            applyModeChange (0);

        if (lastP.hold && ! p.hold)
            for (int i = nActive - 1; i >= 0; --i)
                if (! isPhysical (active[i].note))
                    removeActive (i, 0);

        lastP = p;

        for (const auto meta : in)
        {
            const auto msg = meta.getMessage();
            const int pos = meta.samplePosition;

            if (msg.isNoteOn())
            {
                advanceArp ((double) pos);
                noteOn (msg.getNoteNumber(), (int) msg.getVelocity(), pos);
            }
            else if (msg.isNoteOff())
            {
                advanceArp ((double) pos);
                noteOff (msg.getNoteNumber(), pos);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                advanceArp ((double) pos);
                allOff (pos);
                outBuf->addEvent (msg, pos);
            }
            else
                outBuf->addEvent (msg, pos);
        }

        advanceArp ((double) numSamples);
        stepAt -= (double) numSamples;
        if (gateOffAt >= 0.0) gateOffAt -= (double) numSamples;
    }

private:
    struct Key { int note = 0; int vel = 100; int notes[6] = {}; int n = 0; };

    void emitOn (int note, int vel, int pos)  { outBuf->addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) vel), pos); }
    void emitOff (int note, int pos)          { outBuf->addEvent (juce::MidiMessage::noteOff (1, note), pos); }

    void expand (Key& k) const
    {
        k.n = 0;
        if (cur->chordOn)
        {
            const int* iv = kChordIntervals[std::max (0, std::min (kNumChords - 1, cur->chordType))];
            for (int i = 0; i < 6 && iv[i] >= 0; ++i)
                if (k.note + iv[i] <= 127)
                    k.notes[k.n++] = k.note + iv[i];
        }
        else
            k.notes[k.n++] = k.note;
    }

    void soundKey (const Key& k, int pos)
    {
        for (int i = 0; i < k.n; ++i)
            if (counts[(size_t) k.notes[i]]++ == 0)
                emitOn (k.notes[i], k.vel, pos);
    }

    void silenceKey (const Key& k, int pos)
    {
        for (int i = 0; i < k.n; ++i)
            if (counts[(size_t) k.notes[i]] > 0 && --counts[(size_t) k.notes[i]] == 0)
                emitOff (k.notes[i], pos);
    }

    bool isPhysical (int note) const { for (int i = 0; i < nPhys; ++i) if (phys[i] == note) return true; return false; }
    int findActive (int note) const { for (int i = 0; i < nActive; ++i) if (active[i].note == note) return i; return -1; }

    void addPhys (int note) { if (! isPhysical (note) && nPhys < 32) phys[nPhys++] = note; }
    void removePhys (int note)
    {
        for (int i = 0; i < nPhys; ++i)
            if (phys[i] == note) { for (int j = i; j < nPhys - 1; ++j) phys[j] = phys[j + 1]; --nPhys; return; }
    }

    void removeActive (int i, int pos)
    {
        if (! lastP.arpOn) silenceKey (active[i], pos);
        for (int j = i; j < nActive - 1; ++j) active[j] = active[j + 1];
        --nActive;
    }

    void clearActive (int pos)
    {
        if (! lastP.arpOn)
            for (int i = 0; i < nActive; ++i) silenceKey (active[i], pos);
        nActive = 0;
    }

    void noteOn (int note, int vel, int pos)
    {
        lastVel = vel;
        if (lastP.hold && nPhys == 0)
            clearActive (pos);
        addPhys (note);

        if (findActive (note) >= 0 || nActive >= 32)
            return;

        Key k; k.note = note; k.vel = vel; expand (k);
        active[nActive++] = k;

        if (lastP.arpOn)
        {
            if (! arpRunning) { arpRunning = true; stepAt = (double) pos; arpIndex = 0; }
        }
        else
            soundKey (active[nActive - 1], pos);
    }

    void noteOff (int note, int pos)
    {
        removePhys (note);
        if (! lastP.hold)
        {
            const int i = findActive (note);
            if (i >= 0) removeActive (i, pos);
        }
    }

    void allOff (int pos)
    {
        if (lastP.arpOn)
        {
            if (curArpNote >= 0) emitOff (curArpNote, pos);
        }
        else
            for (int i = 0; i < nActive; ++i) silenceKey (active[i], pos);

        nActive = 0; nPhys = 0;
        counts.fill (0);
        arpRunning = false; curArpNote = -1; gateOffAt = -1.0;
    }

    // Muda de modo (arp / acordes): silencia o que soava e recomeca com as teclas activas.
    void applyModeChange (int pos)
    {
        if (lastP.arpOn)
        {
            if (curArpNote >= 0) { emitOff (curArpNote, pos); curArpNote = -1; }
            gateOffAt = -1.0;
            arpRunning = false;
        }
        else
            for (int i = 0; i < nActive; ++i) silenceKey (active[i], pos);

        for (int i = 0; i < nActive; ++i) expand (active[i]);

        if (cur->arpOn)
        {
            arpRunning = nActive > 0;
            stepAt = (double) pos;
            arpIndex = 0;
        }
        else
            for (int i = 0; i < nActive; ++i) soundKey (active[i], pos);

        lastP.arpOn = cur->arpOn;
        lastP.chordOn = cur->chordOn;
        lastP.chordType = cur->chordType;
    }

    int buildSequence (int* seq) const
    {
        int pool[256]; int np = 0;
        for (int i = 0; i < nActive; ++i)
            for (int j = 0; j < active[i].n; ++j)
            {
                const int v = active[i].notes[j];
                bool dup = false;
                for (int q = 0; q < np; ++q) if (pool[q] == v) { dup = true; break; }
                if (! dup && np < 256) pool[np++] = v;
            }
        if (np == 0) return 0;
        std::sort (pool, pool + np);

        const int range = std::max (1, std::min (3, cur->arpRange));
        int up[512]; int nu = 0;
        for (int o = 0; o < range; ++o)
            for (int i = 0; i < np; ++i)
            {
                const int v = pool[i] + 12 * o;
                if (v <= 127 && nu < 512) up[nu++] = v;
            }

        int ns = 0;
        if (cur->arpMode == 2)
            for (int i = nu - 1; i >= 0; --i) seq[ns++] = up[i];
        else
        {
            for (int i = 0; i < nu; ++i) seq[ns++] = up[i];
            if (cur->arpMode == 1)
                for (int i = nu - 2; i >= 1; --i) seq[ns++] = up[i];
        }
        return ns;
    }

    void advanceArp (double to)
    {
        if (! cur->arpOn) return;
        const double step = std::max (cur->stepSamples, 32.0);

        for (;;)
        {
            const bool stepDue = arpRunning && stepAt < to;
            const bool offDue = gateOffAt >= 0.0 && gateOffAt < to;
            if (! stepDue && ! offDue) break;

            if (offDue && (! stepDue || gateOffAt <= stepAt))
            {
                if (curArpNote >= 0) emitOff (curArpNote, (int) gateOffAt);
                curArpNote = -1;
                gateOffAt = -1.0;
            }
            else
            {
                const int pos = std::max (0, (int) stepAt);
                if (curArpNote >= 0) { emitOff (curArpNote, pos); curArpNote = -1; gateOffAt = -1.0; }

                int seq[512];
                const int ns = buildSequence (seq);
                if (ns == 0) { arpRunning = false; break; }

                const int idx = arpIndex % ns;
                arpIndex = idx + 1;
                curArpNote = seq[idx];
                emitOn (curArpNote, lastVel, pos);
                gateOffAt = stepAt + step * 0.5;
                stepAt += step;
            }
        }
    }

    const EngineParams* cur = nullptr;
    EngineParams lastP;
    juce::MidiBuffer* outBuf = nullptr;

    Key active[32]; int nActive = 0;
    int phys[32] = {}; int nPhys = 0;
    std::array<int, 128> counts {};

    bool arpRunning = false;
    double stepAt = 0.0, gateOffAt = -1.0;
    int curArpNote = -1, arpIndex = 0, lastVel = 100;
};
} // namespace sixty
