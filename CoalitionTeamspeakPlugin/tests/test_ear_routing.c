/* Ear-routing regression test for the Bifrost VON plugin fix: drives ts3plugin_onEditPostProcessVoiceDataEvent
   with different TeamSpeak speaker layouts and checks which output channels carry sound.
   Build (x64 Developer Command Prompt, from this folder):
     cl /nologo /O2 /TC /I..\include /I..\src test_ear_routing.c ..\include\cJSON.c /link Shell32.lib Ole32.lib User32.lib
   Run test_ear_routing.exe: exit code 0 and "ALL TESTS PASSED". The original 3.0.1 plugin fails 5 of the 10 checks. */
#include "../src/plugin.c"

#define FRAMES 480
#define MAXCH 8

static int g_fail = 0;

static void set_radio_talker(anyID id, const char* freq)
{
    VonSnapshot* v = &g_VonSnap[0];
    memset(v, 0, sizeof(*v));
    v->entries[0].id = id;
    v->entries[0].type = VON_RADIO;
    v->entries[0].leftGain = 1.0f;
    v->entries[0].rightGain = 1.0f;
    strcpy(v->entries[0].txFreq, freq);
    v->entries[0].txTimeDev = INT_MIN;
    v->entries[0].connQ = 1.0f;
    v->count = 1;
    v->loaded = 1;
}

static void set_direct_talker(anyID id, float l, float r)
{
    VonSnapshot* v = &g_VonSnap[0];
    memset(v, 0, sizeof(*v));
    v->entries[0].id = id;
    v->entries[0].type = VON_DIRECT;
    v->entries[0].leftGain = l;
    v->entries[0].rightGain = r;
    v->entries[0].txTimeDev = INT_MIN;
    v->entries[0].connQ = 1.0f;
    v->entries[0].sameLanguage = 1;
    v->count = 1;
    v->loaded = 1;
}

static void set_radios(int n, const char** freqs, const int* stereo)
{
    RadioSnapshot* r = &g_RadioSnap[0];
    memset(r, 0, sizeof(*r));
    for (int i = 0; i < n; ++i) {
        strcpy(r->list[i].freq, freqs[i]);
        r->list[i].timeDev = INT_MIN;
        r->list[i].volStep = 9;
        r->list[i].stereo = stereo[i];
    }
    r->count = n;
    r->loaded = 1;
}

/* Runs 30 buffers (gain ramps settle), returns per-channel RMS of the last one */
static unsigned int run(anyID id, int channels, const unsigned int* layout, unsigned int fillIn, double* rms)
{
    short buf[FRAMES * MAXCH];
    unsigned int mask = 0;
    for (int pass = 0; pass < 30; ++pass) {
        for (int i = 0; i < FRAMES; ++i) {
            for (int ch = 0; ch < channels; ++ch) {
                if ((fillIn >> ch) & 1u)
                    buf[i * channels + ch] = (short)(9000.0 * sin(2.0 * M_PI * 440.0 * (pass * FRAMES + i) / 48000.0));
                else
                    buf[i * channels + ch] = 12345; /* "undefined" data TeamSpeak may leave in unfilled channels */
            }
        }
        mask = fillIn;
        ts3plugin_onEditPostProcessVoiceDataEvent(1, id, buf, FRAMES, channels, layout, &mask);
    }
    for (int ch = 0; ch < channels; ++ch) {
        double s = 0;
        for (int i = 0; i < FRAMES; ++i) {
            double x = buf[i * channels + ch];
            s += x * x;
        }
        rms[ch] = sqrt(s / FRAMES);
    }
    return mask;
}

static void expect(const char* name, int cond)
{
    printf("%s %s\n", cond ? "PASS" : "FAIL", name);
    if (!cond)
        g_fail = 1;
}

int main(void)
{
    double rms[MAXCH];
    setvbuf(stdout, NULL, _IONBF, 0);
    InitializeCriticalSection(&g_directLock);
    InitializeCriticalSection(&g_radioLock);
    InitializeCriticalSection(&g_talkLock);
    InterlockedExchange(&g_inVonActiveFlag, 1);

    /* 1. Plain stereo, radio set to RIGHT ear */
    {
        const unsigned int layout[2] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT};
        const char* f[1] = {"31120"}; const int s[1] = {1};
        set_radio_talker(7, "31120"); set_radios(1, f, s);
        unsigned int m = run(7, 2, layout, 0x3, rms);
        printf("  stereo/right: L=%.0f R=%.0f mask=%x\n", rms[0], rms[1], m);
        expect("stereo layout, right-ear radio plays right only", rms[0] < 1 && rms[1] > 500);
    }
    /* 2. Headphones layout, radio LEFT */
    {
        const unsigned int layout[2] = {SPEAKER_HEADPHONES_LEFT, SPEAKER_HEADPHONES_RIGHT};
        const char* f[1] = {"31120"}; const int s[1] = {2};
        set_radio_talker(7, "31120"); set_radios(1, f, s);
        run(7, 2, layout, 0x3, rms);
        printf("  headphones/left: L=%.0f R=%.0f\n", rms[0], rms[1]);
        expect("headphone layout, left-ear radio plays left only", rms[0] > 500 && rms[1] < 1);
    }
    /* 3. Right speaker listed first: the old code sent "left" to channel 0, i.e. the right speaker */
    {
        const unsigned int layout[2] = {SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_LEFT};
        const char* f[1] = {"31120"}; const int s[1] = {2};
        set_radio_talker(7, "31120"); set_radios(1, f, s);
        run(7, 2, layout, 0x3, rms);
        printf("  swapped/left: ch0(R)=%.0f ch1(L)=%.0f\n", rms[0], rms[1]);
        expect("swapped layout, left-ear radio lands on the left speaker", rms[1] > 500 && rms[0] < 1);
    }
    /* 4. 6-channel, only the centre filled (the TeamSpeak SDK's own example) */
    {
        const unsigned int layout[6] = {SPEAKER_FRONT_CENTER, SPEAKER_LOW_FREQUENCY, SPEAKER_BACK_LEFT, SPEAKER_BACK_RIGHT, SPEAKER_SIDE_LEFT, SPEAKER_SIDE_RIGHT};
        const char* f[1] = {"31120"}; const int s[1] = {2};
        set_radio_talker(7, "31120"); set_radios(1, f, s);
        unsigned int m = run(7, 6, layout, 0x1, rms);
        printf("  6ch/left: C=%.0f LFE=%.0f BL=%.0f BR=%.0f SL=%.0f SR=%.0f mask=%x\n", rms[0], rms[1], rms[2], rms[3], rms[4], rms[5], m);
        expect("6ch layout, left radio on left speakers", rms[2] > 500 && rms[4] > 500);
        expect("6ch layout, nothing on right speakers or LFE", rms[3] < 1 && rms[5] < 1 && rms[1] < 1);
        expect("6ch layout, garbage in unfilled channels ignored and every channel marked filled", m == 0x3F);
    }
    /* 5. Same net on a left radio and a right radio: both ears */
    {
        const unsigned int layout[2] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT};
        const char* f[2] = {"31120", "31120"}; const int s[2] = {2, 1};
        set_radio_talker(7, "31120"); set_radios(2, f, s);
        run(7, 2, layout, 0x3, rms);
        printf("  merge: L=%.0f R=%.0f\n", rms[0], rms[1]);
        expect("same net on left and right radios plays in both ears", rms[0] > 500 && rms[1] > 500);
    }
    /* 6. Two nets, two radios: each talker in its own radio's ear */
    {
        const unsigned int layout[2] = {SPEAKER_FRONT_LEFT, SPEAKER_FRONT_RIGHT};
        const char* f[2] = {"31120", "31130"}; const int s[2] = {2, 1};
        set_radios(2, f, s);
        set_radio_talker(8, "31130");
        run(8, 2, layout, 0x3, rms);
        printf("  second net: L=%.0f R=%.0f\n", rms[0], rms[1]);
        expect("talker on the right-ear radio's net plays right only", rms[0] < 1 && rms[1] > 500);
    }
    /* 7. Direct voice panned hard left, swapped layout */
    {
        const unsigned int layout[2] = {SPEAKER_FRONT_RIGHT, SPEAKER_FRONT_LEFT};
        set_direct_talker(9, 1.0f, 0.0f);
        run(9, 2, layout, 0x3, rms);
        printf("  direct left on swapped: ch0(R)=%.0f ch1(L)=%.0f\n", rms[0], rms[1]);
        expect("direct voice from the left comes out of the left speaker", rms[1] > 500 && rms[1] > rms[0] * 4);
    }
    /* 8. Mono output device still works */
    {
        const char* f[1] = {"31120"}; const int s[1] = {2};
        set_radio_talker(7, "31120"); set_radios(1, f, s);
        run(7, 1, NULL, 0x1, rms);
        printf("  mono: %.0f\n", rms[0]);
        expect("mono output plays the radio", rms[0] > 300);
    }

    printf(g_fail ? "SOME TESTS FAILED\n" : "ALL TESTS PASSED\n");
    return g_fail;
}
