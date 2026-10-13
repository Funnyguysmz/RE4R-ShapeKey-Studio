/* RE4R ShapeKey Studio 0.4 host (Claude, 2026-10-12).
   Native window + WebView2. The UI lives in ui/ (HTML/CSS/JS, mapped to https://studio.local/, no network, no port).
   Same named-pipe protocol as studio.c (UTF-8 TSV, S/R/K/M rows); install / directory logic follows setup_ui.h.
   JS -> host : "cmd\top\targ\tvalue"  or  "host\taction\targ"
   host -> JS : JSON {t:"snap", online, raw} | {t:"host", ...} | {t:"preset", text} */
#define UNICODE
#define _UNICODE
#undef _WIN32_WINNT
#undef WINVER
#define _WIN32_WINNT 0x0A00
#define WINVER 0x0A00
#define COBJMACROS
#define CINTERFACE
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <commdlg.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WebView2.h"

#define STUDIO_VERSION L"0.4.2-beta.1"
#define PIPE_NAME L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.v1"
#define CAP 262144

static HWND win;
static ICoreWebView2Controller *controller;
static ICoreWebView2 *webview;
static int demo, selftest, ui_ready, dark, topmost;
static wchar_t app_dir[1024], config_file[1200], game_directory[1024], setup_message[1024], lan_addresses[512];
static int directory_valid;
static HANDLE install_process;
static FILE *self_log;

/* ------------------------------------------------------------------ pipe client (protocol of studio_core.h) */
static SRWLOCK gate = SRWLOCK_INIT;
static char latest[CAP], pending[CAP], token[2048];
static size_t pending_len;
static int online;
static LONG stopping;
static int transfer(HANDLE p, void *buf, DWORD n, DWORD *done, int writing) {
    OVERLAPPED o = {0}; o.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL); if (!o.hEvent) return 0;
    BOOL ok = writing ? WriteFile(p, buf, n, done, &o) : ReadFile(p, buf, n, done, &o);
    if (!ok && GetLastError() == ERROR_IO_PENDING) {
        if (WaitForSingleObject(o.hEvent, 700) == WAIT_OBJECT_0) ok = GetOverlappedResult(p, &o, done, FALSE);
        else { CancelIoEx(p, &o); GetOverlappedResult(p, &o, done, TRUE); ok = FALSE; }
    }
    CloseHandle(o.hEvent); return ok;
}
static DWORD WINAPI network(void *unused) {
    (void)unused; char *req = malloc(CAP), *response = malloc(CAP); if (!req || !response) return 1;
    while (!InterlockedCompareExchange(&stopping, 0, 0)) {
        WaitNamedPipeW(PIPE_NAME, 80);
        HANDLE p = CreateFileW(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL); int success = 0;
        if (p != INVALID_HANDLE_VALUE) {
            DWORD mode = PIPE_READMODE_MESSAGE, n = 0, w = 0; SetNamedPipeHandleState(p, &mode, NULL, NULL);
            AcquireSRWLockExclusive(&gate); size_t len = pending_len;
            if (len) memcpy(req, pending, len); else { memcpy(req, "PING\n", 5); len = 5; }
            pending_len = 0; pending[0] = 0; ReleaseSRWLockExclusive(&gate);
            if (transfer(p, req, (DWORD)len, &w, 1) && w == len && transfer(p, response, CAP - 1, &n, 0)) {
                response[n] = 0; AcquireSRWLockExclusive(&gate); memcpy(latest, response, n + 1); ReleaseSRWLockExclusive(&gate); success = 1;
            }
            CloseHandle(p);
        }
        AcquireSRWLockExclusive(&gate); online = success; ReleaseSRWLockExclusive(&gate); Sleep(50);
    }
    free(req); free(response); return 0;
}
static void command(const char *op, const char *arg, const char *value) {
    if (!online) return;
    int receiver = !strncmp(op, "receiver_", 9);
    if (!receiver && (!token[0] || !strcmp(token, "WAIT"))) return;
    char b[4096]; int n = snprintf(b, sizeof(b), "C\t%s\t%s\t%s\t%s\n", token[0] ? token : "WAIT", op, arg ? arg : "", value ? value : "");
    if (n <= 0 || n >= (int)sizeof(b)) return;
    AcquireSRWLockExclusive(&gate);
    if (pending_len + (size_t)n < CAP) { memcpy(pending + pending_len, b, n); pending_len += n; pending[pending_len] = 0; }
    ReleaseSRWLockExclusive(&gate);
}

/* ------------------------------------------------------------------ JSON out */
typedef struct { char *p; size_t n, cap; } Buf;
static void put(Buf *b, const char *s, size_t n) {
    if (b->n + n + 1 > b->cap) { size_t c = (b->cap + n + 1) * 2; char *q = realloc(b->p, c); if (!q) return; b->p = q; b->cap = c; }
    memcpy(b->p + b->n, s, n); b->n += n; b->p[b->n] = 0;
}
static void puts_(Buf *b, const char *s) { put(b, s, strlen(s)); }
static void put_str(Buf *b, const char *s) {        /* UTF-8 in, JSON string out */
    puts_(b, "\"");
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s; char e[8];
        if (c == '"' || c == '\\') { e[0] = '\\'; e[1] = c; put(b, e, 2); }
        else if (c == '\n') puts_(b, "\\n"); else if (c == '\t') puts_(b, "\\t"); else if (c == '\r') puts_(b, "\\r");
        else if (c < 0x20) { snprintf(e, sizeof e, "\\u%04x", c); puts_(b, e); }
        else put(b, (const char *)&c, 1);
    }
    puts_(b, "\"");
}
static void put_wstr(Buf *b, const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL); char *s = malloc(n > 0 ? n : 1); if (!s) return;
    if (n <= 0 || !WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL)) s[0] = 0;
    put_str(b, s); free(s);
}
static void post_json(Buf *b) {
    if (!webview || !ui_ready || !b->p) return;
    int n = MultiByteToWideChar(CP_UTF8, 0, b->p, -1, NULL, 0); wchar_t *w = malloc(sizeof(wchar_t) * (n > 0 ? n : 1)); if (!w) return;
    if (n > 0 && MultiByteToWideChar(CP_UTF8, 0, b->p, -1, w, n)) ICoreWebView2_PostWebMessageAsJson(webview, w);
    free(w);
}

/* ------------------------------------------------------------------ host state (setup_ui.h logic) */
static int valid_game_directory(const wchar_t *path) {
    if (!path || !path[0]) return 0; wchar_t exe[1200]; swprintf(exe, 1200, L"%ls\\re4.exe", path);
    DWORD root = GetFileAttributesW(path), attr = GetFileAttributesW(exe);
    return root != INVALID_FILE_ATTRIBUTES && (root & FILE_ATTRIBUTE_DIRECTORY) && attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}
static void local_addresses(void) {
    ULONG size = 16384; IP_ADAPTER_ADDRESSES *a = malloc(size); if (!a) return;
    ULONG ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER, NULL, a, &size);
    if (ret == ERROR_BUFFER_OVERFLOW) { free(a); a = malloc(size); if (!a) return; ret = GetAdaptersAddresses(AF_INET, 0, NULL, a, &size); }
    lan_addresses[0] = 0;
    if (ret == NO_ERROR) for (IP_ADAPTER_ADDRESSES *p = a; p; p = p->Next) if (p->OperStatus == IfOperStatusUp && p->IfType != IF_TYPE_SOFTWARE_LOOPBACK) {
        for (IP_ADAPTER_UNICAST_ADDRESS *u = p->FirstUnicastAddress; u; u = u->Next) {
            if (u->Address.lpSockaddr->sa_family != AF_INET) continue;
            SOCKADDR_IN *v = (SOCKADDR_IN *)u->Address.lpSockaddr; unsigned char *b = (unsigned char *)&v->sin_addr;
            if (b[0] == 127 || (b[0] == 169 && b[1] == 254) || (b[0] == 198 && (b[1] == 18 || b[1] == 19))) continue;
            wchar_t address[64]; swprintf(address, 64, L"%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
            if (wcslen(lan_addresses) + wcslen(address) + 4 < 512) { if (lan_addresses[0]) wcscat(lan_addresses, L" / "); wcscat(lan_addresses, address); }
        }
    }
    free(a);
}
static void apply_topmost(void) {
    if (win) SetWindowPos(win, topmost ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
static void post_host(void) {
    Buf b = {0}; char num[64];
    puts_(&b, "{\"t\":\"host\",\"version\":"); put_wstr(&b, STUDIO_VERSION);
    puts_(&b, ",\"dir\":"); put_wstr(&b, game_directory);
    snprintf(num, sizeof num, ",\"valid\":%d,\"installing\":%d,\"demo\":%d", directory_valid, install_process != NULL, demo); puts_(&b, num);
    snprintf(num, sizeof num, ",\"topmost\":%d", topmost); puts_(&b, num);
    puts_(&b, ",\"msg\":"); put_wstr(&b, setup_message);
    puts_(&b, ",\"lan\":"); put_wstr(&b, lan_addresses);
    puts_(&b, "}"); post_json(&b); free(b.p);
}
static void set_msg(const wchar_t *m) { wcsncpy(setup_message, m, 1023); setup_message[1023] = 0; }
static void validate_directory(void) {
    size_t n = wcslen(game_directory);
    while (n > 3 && (game_directory[n - 1] == L'\\' || game_directory[n - 1] == L'/')) game_directory[--n] = 0;
    directory_valid = valid_game_directory(game_directory);
    if (directory_valid) {
        int saved = demo || WritePrivateProfileStringW(L"Studio", L"GameDirectory", game_directory, config_file);
        set_msg(saved ? L"dir_ok" : L"dir_unsaved");
    } else set_msg(L"dir_bad");
}
static int CALLBACK browse_callback(HWND h, UINT m, LPARAM p, LPARAM data) { (void)p; if (m == BFFM_INITIALIZED && data) SendMessageW(h, BFFM_SETSELECTIONW, TRUE, data); return 0; }
static void browse_directory(void) {
    BROWSEINFOW b = {0}; b.hwndOwner = win; b.lpszTitle = L"RE4 (re4.exe)"; b.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI; b.lpfn = browse_callback; b.lParam = (LPARAM)game_directory;
    PIDLIST_ABSOLUTE id = SHBrowseForFolderW(&b);
    if (id) { wchar_t path[MAX_PATH]; if (SHGetPathFromIDListW(id, path)) { wcscpy(game_directory, path); validate_directory(); } CoTaskMemFree(id); }
}
static void run_install(void) {
    validate_directory(); if (!directory_valid || install_process || demo) return;
    wchar_t ps[1200], command_line[4096], log_path[1200]; GetSystemDirectoryW(ps, 1024); wcscat(ps, L"\\WindowsPowerShell\\v1.0\\powershell.exe");
    swprintf(command_line, 4096, L"\"%ls\" -NoProfile -ExecutionPolicy Bypass -File \"%ls\\Install.ps1\" -GameDirectory \"%ls\"", ps, app_dir, game_directory);
    swprintf(log_path, 1200, L"%ls\\installer.log", app_dir); SECURITY_ATTRIBUTES sa = {sizeof(sa), NULL, TRUE};
    HANDLE log_file = CreateFileW(log_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log_file == INVALID_HANDLE_VALUE) { set_msg(L"log_unwritable"); return; }
    HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    STARTUPINFOW si = {sizeof(si)}; si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW; si.wShowWindow = SW_HIDE; si.hStdOutput = si.hStdError = log_file; si.hStdInput = input;
    PROCESS_INFORMATION pi = {0};
    BOOL ok = CreateProcessW(ps, command_line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, app_dir, &si, &pi);
    CloseHandle(log_file); if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
    if (ok) { install_process = pi.hProcess; CloseHandle(pi.hThread); set_msg(L"installing"); }
    else set_msg(L"install_nostart");
}
static void check_install(void) {
    if (!install_process) return; DWORD code; GetExitCodeProcess(install_process, &code);
    if (code != STILL_ACTIVE) { CloseHandle(install_process); install_process = NULL; set_msg(code == 0 ? L"installed" : L"install_failed"); post_host(); }
}
static void preset_dialog(int saving, const char *content) {
    wchar_t path[MAX_PATH] = L"ShapeKey.preset.tsv"; OPENFILENAMEW of = {0}; of.lStructSize = sizeof(of); of.hwndOwner = win; of.lpstrFile = path; of.nMaxFile = MAX_PATH;
    of.lpstrFilter = L"ShapeKey preset (*.tsv)\0*.tsv\0\0"; of.lpstrDefExt = L"tsv"; of.Flags = OFN_NOCHANGEDIR | (saving ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(saving ? GetSaveFileNameW(&of) : GetOpenFileNameW(&of))) return;
    FILE *f = _wfopen(path, saving ? L"wb" : L"rb"); if (!f) return;
    if (saving) fwrite(content, 1, strlen(content), f);
    else {
        char *t = malloc(CAP); if (t) { size_t n = fread(t, 1, CAP - 1, f); t[n] = 0;
            Buf b = {0}; puts_(&b, "{\"t\":\"preset\",\"text\":"); put_str(&b, t); puts_(&b, "}"); post_json(&b); free(b.p); free(t); }
    }
    fclose(f);
}
static void title_theme(void) { BOOL d = dark; DwmSetWindowAttribute(win, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &d, sizeof d); }

/* ------------------------------------------------------------------ messages from the UI */
static char *next_field(char **p) { char *s = *p; if (!s) return ""; char *e = strchr(s, '\t'); if (e) { *e = 0; *p = e + 1; } else *p = NULL; return s; }
static void on_message(const wchar_t *w) {
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL); if (n <= 0) return;
    char *s = malloc(n); if (!s) return; WideCharToMultiByte(CP_UTF8, 0, w, -1, s, n, NULL, NULL);
    char *p = s, *kind = next_field(&p);
    if (!strcmp(kind, "cmd")) { char *op = next_field(&p), *arg = next_field(&p), *val = next_field(&p); command(op, arg, val); }
    else if (!strcmp(kind, "host")) {
        char *action = next_field(&p);
        if (!strcmp(action, "ready")) { ui_ready = 1; post_host(); if (self_log) { fprintf(self_log, "UI_READY\n"); fflush(self_log); } }
        else if (!strcmp(action, "browse")) { browse_directory(); post_host(); }
        else if (!strcmp(action, "validate")) { MultiByteToWideChar(CP_UTF8, 0, next_field(&p), -1, game_directory, 1024); validate_directory(); post_host(); }
        else if (!strcmp(action, "install")) { run_install(); post_host(); }
        else if (!strcmp(action, "lan")) { local_addresses(); post_host(); }
        else if (!strcmp(action, "preset_save")) preset_dialog(1, p ? p : "");
        else if (!strcmp(action, "preset_load")) preset_dialog(0, NULL);
        else if (!strcmp(action, "theme")) { dark = !strcmp(next_field(&p), "dark"); title_theme(); }
        else if (!strcmp(action, "topmost")) {
            topmost = next_field(&p)[0] == '1'; apply_topmost();
            if (!demo) WritePrivateProfileStringW(L"Studio", L"Topmost", topmost ? L"1" : L"0", config_file);
            post_host();
        }
        else if (!strcmp(action, "log")) { if (self_log) { fprintf(self_log, "%s\n", p ? p : ""); fflush(self_log); } }
        else if (!strcmp(action, "quit")) PostMessageW(win, WM_CLOSE, 0, 0);
    }
    free(s);
}
static void pump_snapshot(void) {
    static char *previous; static int prev_online = -1;
    if (!previous) { previous = calloc(1, CAP); if (!previous) return; }
    char *data = malloc(CAP); if (!data) return;
    AcquireSRWLockShared(&gate); int on = online; strcpy(data, latest); ReleaseSRWLockShared(&gate);
    if (on != prev_online || strcmp(previous, data)) {
        prev_online = on; strcpy(previous, data);
        token[0] = 0;
        if (!strncmp(data, "S\t", 2)) { const char *t = data + 2; size_t k = strcspn(t, "\t\n"); if (k < sizeof token) { memcpy(token, t, k); token[k] = 0; } }
        Buf b = {0}; char num[32]; snprintf(num, sizeof num, "{\"t\":\"snap\",\"online\":%d,\"raw\":", on); puts_(&b, num); put_str(&b, data); puts_(&b, "}");
        post_json(&b); free(b.p);
    }
    free(data);
}

/* ------------------------------------------------------------------ WebView2 COM handlers (C vtables) */
typedef struct { void *vtbl; } Handler;
static HRESULT STDMETHODCALLTYPE h_qi(void *This, REFIID riid, void **out) { (void)riid; *out = This; return S_OK; }
static ULONG STDMETHODCALLTYPE h_ref(void *This) { (void)This; return 1; }

static HRESULT STDMETHODCALLTYPE msg_invoke(ICoreWebView2WebMessageReceivedEventHandler *This, ICoreWebView2 *sender, ICoreWebView2WebMessageReceivedEventArgs *args) {
    (void)This; (void)sender; LPWSTR m = NULL;
    if (SUCCEEDED(ICoreWebView2WebMessageReceivedEventArgs_TryGetWebMessageAsString(args, &m)) && m) { on_message(m); CoTaskMemFree(m); }
    return S_OK;
}
static ICoreWebView2WebMessageReceivedEventHandlerVtbl msg_vtbl = {(void *)h_qi, (void *)h_ref, (void *)h_ref, msg_invoke};
static ICoreWebView2WebMessageReceivedEventHandler msg_handler = {&msg_vtbl};

static void fit(void) { if (controller) { RECT r; GetClientRect(win, &r); ICoreWebView2Controller_put_Bounds(controller, r); } }
static const IID IID_WV2_3 = {0xA0D6DF20, 0x3B92, 0x416D, {0xAA, 0x0C, 0x43, 0x7A, 0x9C, 0x72, 0x78, 0x57}};
static const IID IID_WV2Controller2 = {0xc979903e, 0xd4ca, 0x4228, {0x92, 0xeb, 0x47, 0xee, 0x3f, 0xa9, 0x6e, 0xab}};
static HRESULT STDMETHODCALLTYPE ctl_invoke(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *This, HRESULT hr, ICoreWebView2Controller *c) {
    (void)This;
    if (FAILED(hr) || !c) { MessageBoxW(win, L"WebView2 初始化失败 / WebView2 failed to start.", L"ShapeKey Studio", MB_ICONERROR); return S_OK; }
    controller = c; ICoreWebView2Controller_AddRef(c); ICoreWebView2Controller_get_CoreWebView2(c, &webview);
    ICoreWebView2Controller2 *c2 = NULL;
    if (SUCCEEDED(ICoreWebView2Controller_QueryInterface(c, &IID_WV2Controller2, (void **)&c2)) && c2) {
        COREWEBVIEW2_COLOR bg = dark ? (COREWEBVIEW2_COLOR){255, 0x17, 0x15, 0x1A} : (COREWEBVIEW2_COLOR){255, 0xFB, 0xF8, 0xF6};
        ICoreWebView2Controller2_put_DefaultBackgroundColor(c2, bg); ICoreWebView2Controller2_Release(c2);
    }
    ICoreWebView2Settings *st = NULL;
    if (SUCCEEDED(ICoreWebView2_get_Settings(webview, &st)) && st) {
        ICoreWebView2Settings_put_AreDefaultContextMenusEnabled(st, FALSE);
        ICoreWebView2Settings_put_AreDevToolsEnabled(st, GetEnvironmentVariableW(L"STUDIO_DEVTOOLS", NULL, 0) > 0);
        ICoreWebView2Settings_put_IsStatusBarEnabled(st, FALSE); ICoreWebView2Settings_put_IsZoomControlEnabled(st, FALSE);
        ICoreWebView2Settings_Release(st);
    }
    ICoreWebView2_3 *w3 = NULL; wchar_t ui[1100]; swprintf(ui, 1100, L"%ls\\ui", app_dir);
    if (SUCCEEDED(ICoreWebView2_QueryInterface(webview, &IID_WV2_3, (void **)&w3)) && w3) {
        ICoreWebView2_3_SetVirtualHostNameToFolderMapping(w3, L"studio.local", ui, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
        ICoreWebView2_3_Release(w3);
    }
    EventRegistrationToken tok; ICoreWebView2_add_WebMessageReceived(webview, &msg_handler, &tok);
    wchar_t url[256]; swprintf(url, 256, L"https://studio.local/index.html?host=1%ls%ls", demo ? L"&demo=1" : L"", dark ? L"&theme=dark" : L"");
    ICoreWebView2_Navigate(webview, url); fit();
    return S_OK;
}
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl ctl_vtbl = {(void *)h_qi, (void *)h_ref, (void *)h_ref, ctl_invoke};
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandler ctl_handler = {&ctl_vtbl};
static HRESULT STDMETHODCALLTYPE env_invoke(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *This, HRESULT hr, ICoreWebView2Environment *env) {
    (void)This;
    if (FAILED(hr) || !env) {
        MessageBoxW(win, L"需要 Microsoft Edge WebView2 Runtime。\nWebView2 Runtime is required (built into Windows 11).", L"ShapeKey Studio", MB_ICONERROR);
        PostMessageW(win, WM_CLOSE, 0, 0); return S_OK;
    }
    ICoreWebView2Environment_CreateCoreWebView2Controller(env, win, &ctl_handler); return S_OK;
}
static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl env_vtbl = {(void *)h_qi, (void *)h_ref, (void *)h_ref, env_invoke};
static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler env_handler = {&env_vtbl};

/* ------------------------------------------------------------------ window */
static LRESULT CALLBACK proc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
    switch (m) {
    case WM_SIZE: fit(); return 0;
    case WM_TIMER:
        if (wp == 1) { pump_snapshot(); check_install(); }
        if (wp == 2 && selftest) { if (self_log) { fprintf(self_log, ui_ready ? "SELFTEST_PASS\n" : "SELFTEST_FAIL no ui\n"); fflush(self_log); } PostMessageW(h, WM_CLOSE, 0, 0); }
        return 0;
    case WM_GETMINMAXINFO: { MINMAXINFO *mm = (MINMAXINFO *)lp; UINT dpi = GetDpiForWindow(h); mm->ptMinTrackSize.x = MulDiv(1080, dpi, 96); mm->ptMinTrackSize.y = MulDiv(700, dpi, 96); return 0; }
    case WM_DESTROY: InterlockedExchange(&stopping, 1); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(h, m, wp, lp);
}
typedef HRESULT (STDAPICALLTYPE *CreateEnvFn)(PCWSTR, PCWSTR, ICoreWebView2EnvironmentOptions *, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);
int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, PWSTR cmd, int show) {
    (void)prev;
    demo = cmd && wcsstr(cmd, L"--demo") != NULL; selftest = cmd && wcsstr(cmd, L"--selftest") != NULL;
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    GetModuleFileNameW(NULL, app_dir, 1024); wchar_t *slash = wcsrchr(app_dir, L'\\'); if (slash) *slash = 0;
    swprintf(config_file, 1200, L"%ls\\Studio.settings.ini", app_dir);
    if (selftest) { wchar_t lp[1200]; swprintf(lp, 1200, L"%ls\\selftest.log", app_dir); self_log = _wfopen(lp, L"w"); }
    GetPrivateProfileStringW(L"Studio", L"GameDirectory", L"", game_directory, 1024, config_file);
    topmost = GetPrivateProfileIntW(L"Studio", L"Topmost", 0, config_file) != 0;
    if (game_directory[0]) validate_directory(); else set_msg(L"dir_none");
    DWORD v = 0, sz = sizeof v; dark = RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_DWORD, NULL, &v, &sz) == ERROR_SUCCESS && v == 0;
    local_addresses();
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    WNDCLASSEXW wc = {sizeof wc}; wc.lpfnWndProc = proc; wc.hInstance = inst; wc.lpszClassName = L"RE4RShapeKeyStudio04";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW); wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1)); wc.hbrBackground = CreateSolidBrush(dark ? RGB(0x17, 0x15, 0x1A) : RGB(0xFB, 0xF8, 0xF6));
    RegisterClassExW(&wc);
    UINT dpi = GetDpiForSystem();
    win = CreateWindowExW(0, wc.lpszClassName, L"RE4R 形态键工作台 · ShapeKey Studio", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                          MulDiv(1440, dpi, 96), MulDiv(900, dpi, 96), NULL, NULL, inst, NULL);
    title_theme(); apply_topmost(); ShowWindow(win, show); UpdateWindow(win);
    wchar_t loader[1100]; swprintf(loader, 1100, L"%ls\\WebView2Loader.dll", app_dir);
    HMODULE lib = LoadLibraryW(loader); CreateEnvFn create = lib ? (CreateEnvFn)(void *)GetProcAddress(lib, "CreateCoreWebView2EnvironmentWithOptions") : NULL;
    if (!create) { MessageBoxW(win, L"缺少 WebView2Loader.dll，请完整解压程序包。\nWebView2Loader.dll is missing.", L"ShapeKey Studio", MB_ICONERROR); return 2; }
    wchar_t data[1200]; if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, data))) wcscat(data, L"\\RE4R_ShapeKeyStudio\\WebView2"); else swprintf(data, 1200, L"%ls\\webview_data", app_dir);
    if (FAILED(create(NULL, data, NULL, &env_handler))) { MessageBoxW(win, L"WebView2 Runtime 不可用。", L"ShapeKey Studio", MB_ICONERROR); return 3; }
    if (!demo) CloseHandle(CreateThread(NULL, 0, network, NULL, 0, NULL));
    SetTimer(win, 1, 50, NULL); if (selftest) SetTimer(win, 2, 8000, NULL);
    MSG msg; while (GetMessageW(&msg, NULL, 0, 0)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    if (self_log) fclose(self_log);
    CoUninitialize(); return (int)msg.wParam;
}
