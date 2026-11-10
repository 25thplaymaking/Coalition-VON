/* VONPluginInfo.json test: the TeamSpeak state the game's join notice reads (connected, in the VON channel,
   mic and sound mute), and the early rewrite when that state changes.
   Build (x64 Developer Command Prompt, from this folder):
     cl /nologo /O2 /TC /I..\include /I..\src test_plugin_info.c ..\include\cJSON.c /link Shell32.lib Ole32.lib User32.lib
   Run test_plugin_info.exe: exit code 0 and "ALL TESTS PASSED". */
#include "../src/plugin.c"

static int g_fail = 0;
static int g_status = STATUS_CONNECTION_ESTABLISHED;
static int g_micMuted = 0;
static int g_soundMuted = 0;

static unsigned int fake_getConnectionStatus(uint64 sch, int* result)
{
    (void)sch;
    *result = g_status;
    return ERROR_ok;
}

static unsigned int fake_getClientSelfVariableAsInt(uint64 sch, size_t flag, int* result)
{
    (void)sch;
    if (flag == CLIENT_INPUT_MUTED)
        *result = g_micMuted;
    else if (flag == CLIENT_OUTPUT_MUTED)
        *result = g_soundMuted;
    else
        *result = 0;
    return ERROR_ok;
}

static void check(const char* what, int ok)
{
    printf("%s  %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok)
        g_fail++;
}

static void read_file(const char* path, char* out, size_t size)
{
    out[0] = '\0';
    FILE* f = fopen(path, "rb");
    if (!f)
        return;
    size_t n = fread(out, 1, size - 1, f);
    out[n] = '\0';
    fclose(f);
}

int main(void)
{
    ts3Functions.getConnectionStatus = fake_getConnectionStatus;
    ts3Functions.getClientSelfVariableAsInt = fake_getClientSelfVariableAsInt;

    /* The exact line the game's self-test parses (BVON_SelfTest: "plugin info format") */
    PluginState st = {1, 1, 0, 1};
    char buf[384];
    format_plugin_info(buf, sizeof(buf), "3.1.0", "bin1|talking1", 1700000000LL, &st);
    check("format matches what the game reads",
          strcmp(buf, "{\"Version\":\"3.1.0\",\"Features\":\"bin1|talking1\",\"Time\":1700000000,\"Connected\":1,\"InChannel\":1,\"MicMuted\":0,\"SoundMuted\":1}") == 0);

    cJSON* root = cJSON_Parse(buf);
    check("valid JSON", root != NULL);
    if (root) {
        check("Connected is a number", cJSON_IsNumber(cJSON_GetObjectItem(root, "Connected")));
        check("SoundMuted = 1", cJSON_GetObjectItem(root, "SoundMuted")->valueint == 1);
        cJSON_Delete(root);
    }

    /* Not connected: every flag reads 0 */
    g_status = STATUS_DISCONNECTED;
    g_micMuted = 1;
    InterlockedExchange(&g_inVonActiveFlag, 1);
    read_plugin_state(1, &st);
    check("disconnected: all 0", st.connected == 0 && st.inChannel == 0 && st.micMuted == 0 && st.soundMuted == 0);
    read_plugin_state(0, &st);
    check("no server tab: all 0", st.connected == 0 && st.inChannel == 0);

    /* Connected, in the channel, mic muted */
    g_status = STATUS_CONNECTION_ESTABLISHED;
    read_plugin_state(1, &st);
    check("connected + in channel + mic muted", st.connected == 1 && st.inChannel == 1 && st.micMuted == 1 && st.soundMuted == 0);

    /* Rewritten straight away when the state changes, not on an unchanged state */
    char tmp[MAX_PATH];
    GetTempPathA(sizeof(tmp), tmp);
    _snprintf(g_InfoPath, sizeof(g_InfoPath), "%sbvon_test_plugin_info.json", tmp);
    DeleteFileA(g_InfoPath);
    write_plugin_info(1);
    char text[512];
    read_file(g_InfoPath, text, sizeof(text));
    check("file written with MicMuted 1", strstr(text, "\"MicMuted\":1") != NULL);

    DeleteFileA(g_InfoPath);
    write_plugin_info_if_state_changed(1);
    read_file(g_InfoPath, text, sizeof(text));
    check("unchanged state: no rewrite", text[0] == '\0');

    g_micMuted = 0;
    write_plugin_info_if_state_changed(1);
    read_file(g_InfoPath, text, sizeof(text));
    check("mic unmuted: rewritten with MicMuted 0", strstr(text, "\"MicMuted\":0") != NULL);

    InterlockedExchange(&g_inVonActiveFlag, 0);
    write_plugin_info_if_state_changed(1);
    read_file(g_InfoPath, text, sizeof(text));
    check("left the channel: rewritten with InChannel 0", strstr(text, "\"InChannel\":0") != NULL);
    DeleteFileA(g_InfoPath);

    printf(g_fail ? "%d TEST(S) FAILED\n" : "ALL TESTS PASSED\n", g_fail);
    return g_fail ? 1 : 0;
}
