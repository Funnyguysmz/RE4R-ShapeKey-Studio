#include <shlobj.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
static HWND directory_edit,port_edit,ip_edit;
static wchar_t app_dir[1024],config_file[1200],game_directory[1024],setup_message[1024],lan_addresses[512];
static int setup_page=1,directory_valid,receiver_page,setup_render,setup_test;
static HANDLE install_process;
static void *app_logo;
enum {SETUP_OPEN=410,DIR_BROWSE,DIR_VALIDATE,BRIDGE_INSTALL,WORKBENCH_ENTER,RECEIVER_APPLY,RECEIVER_START,RECEIVER_STOP};
static int valid_game_directory(const wchar_t *path){
    if(!path || !path[0])return 0;wchar_t exe[1200];swprintf(exe,1200,L"%ls\\re4.exe",path);
    DWORD root=GetFileAttributesW(path),attr=GetFileAttributesW(exe);
    return root!=INVALID_FILE_ATTRIBUTES && (root&FILE_ATTRIBUTE_DIRECTORY) && attr!=INVALID_FILE_ATTRIBUTES && !(attr&FILE_ATTRIBUTE_DIRECTORY);
}
static int ipv4_valid(const wchar_t *s){
    unsigned a,b,c,d;wchar_t tail;return swscanf(s,L"%u.%u.%u.%u%lc",&a,&b,&c,&d,&tail)==4 && a<=255 && b<=255 && c<=255 && d<=255 && !wcschr(s,L'-') && !wcschr(s,L'+') && !wcschr(s,L' ');
}
static int input_port(void){wchar_t s[32],*end;GetWindowTextW(port_edit,s,32);long p=wcstol(s,&end,10);return end!=s && !*end && p>=1 && p<=65535?(int)p:0;}
static void local_addresses(void){
    ULONG size=16384;IP_ADAPTER_ADDRESSES *a=malloc(size);if(!a)return;
    ULONG ret=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,NULL,a,&size);
    if(ret==ERROR_BUFFER_OVERFLOW){free(a);a=malloc(size);if(!a)return;ret=GetAdaptersAddresses(AF_INET,0,NULL,a,&size);}
    lan_addresses[0]=0;
    if(ret==NO_ERROR)for(IP_ADAPTER_ADDRESSES *p=a;p;p=p->Next)if(p->OperStatus==IfOperStatusUp && p->IfType!=IF_TYPE_SOFTWARE_LOOPBACK){
        for(IP_ADAPTER_UNICAST_ADDRESS *u=p->FirstUnicastAddress;u;u=u->Next){
            if(u->Address.lpSockaddr->sa_family!=AF_INET)continue;SOCKADDR_IN *v=(SOCKADDR_IN*)u->Address.lpSockaddr;unsigned char *b=(unsigned char*)&v->sin_addr;
            if(b[0]==127 || (b[0]==169 && b[1]==254) || (b[0]==198 && (b[1]==18 || b[1]==19)))continue;wchar_t address[64];swprintf(address,64,L"%u.%u.%u.%u",b[0],b[1],b[2],b[3]);
            if(wcslen(lan_addresses)+wcslen(address)+4<512){if(lan_addresses[0])wcscat(lan_addresses,L" / ");wcscat(lan_addresses,address);}
        }
    }free(a);
}
static int save_directory(void){
    if(demo || smoke || setup_test || render_path[0])return 1;
    if(GetFileAttributesW(config_file)==INVALID_FILE_ATTRIBUTES){
        HANDLE f=CreateFileW(config_file,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
        if(f!=INVALID_HANDLE_VALUE){WCHAR bom=0xfeff;DWORD n;WriteFile(f,&bom,sizeof(bom),&n,NULL);CloseHandle(f);}
    }
    return WritePrivateProfileStringW(L"Studio",L"GameDirectory",game_directory,config_file)!=0;
}
static void validate_directory(void){
    GetWindowTextW(directory_edit,game_directory,1024);size_t n=wcslen(game_directory);
    while(n>3 && (game_directory[n-1]==L'\\' || game_directory[n-1]==L'/'))game_directory[--n]=0;
    directory_valid=valid_game_directory(game_directory);
    if(directory_valid){int saved=save_directory();wcscpy(setup_message,saved?tr(L"已找到 re4.exe，可以进入工作台。",L"re4.exe found. You can enter the workbench."):tr(L"目录有效，但设置无法保存；请把程序解压到可写文件夹。",L"Directory valid, but settings cannot be saved. Use a writable folder."));}
    else wcscpy(setup_message,tr(L"请选择直接包含 re4.exe 的游戏文件夹。",L"Choose the game folder containing re4.exe."));
    InvalidateRect(win,NULL,FALSE);
}
static int CALLBACK browse_callback(HWND h,UINT m,LPARAM p,LPARAM data){(void)p;if(m==BFFM_INITIALIZED && data)SendMessageW(h,BFFM_SETSELECTIONW,TRUE,data);return 0;}
static void browse_directory(void){
    BROWSEINFOW b={0};b.hwndOwner=win;b.lpszTitle=tr(L"选择包含 re4.exe 的 RE4 游戏目录",L"Select the RE4 folder containing re4.exe");b.ulFlags=BIF_RETURNONLYFSDIRS|BIF_USENEWUI;b.lpfn=browse_callback;b.lParam=(LPARAM)game_directory;
    PIDLIST_ABSOLUTE id=SHBrowseForFolderW(&b);if(id){wchar_t path[MAX_PATH];if(SHGetPathFromIDListW(id,path)){SetWindowTextW(directory_edit,path);validate_directory();}CoTaskMemFree(id);}
}
static void run_install(void){
    validate_directory();if(!directory_valid || install_process || demo)return;
    wchar_t ps[1200],command_line[4096],log_path[1200];GetSystemDirectoryW(ps,1024);wcscat(ps,L"\\WindowsPowerShell\\v1.0\\powershell.exe");
    swprintf(command_line,4096,L"\"%ls\" -NoProfile -ExecutionPolicy Bypass -File \"%ls\\Install.ps1\" -GameDirectory \"%ls\"",ps,app_dir,game_directory);
    swprintf(log_path,1200,L"%ls\\installer.log",app_dir);SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
    HANDLE log_file=CreateFileW(log_path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(log_file==INVALID_HANDLE_VALUE){wcscpy(setup_message,tr(L"无法写入 installer.log，请使用可写文件夹。",L"Cannot write installer.log. Use a writable folder."));return;}
    HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,0,NULL);
    STARTUPINFOW si={sizeof(si)};si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;si.wShowWindow=SW_HIDE;si.hStdOutput=si.hStdError=log_file;si.hStdInput=input;
    PROCESS_INFORMATION pi={0};BOOL ok=CreateProcessW(ps,command_line,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,app_dir,&si,&pi);CloseHandle(log_file);if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);
    if(ok){install_process=pi.hProcess;CloseHandle(pi.hThread);wcscpy(setup_message,tr(L"正在安装桥接，原脚本会先备份…",L"Installing bridge; existing scripts are backed up first…"));}
    else wcscpy(setup_message,tr(L"无法启动安装器，请查看使用说明。",L"Unable to start installer. See the user guide."));
}
static void check_install(void){if(install_process){DWORD code;GetExitCodeProcess(install_process,&code);if(code!=STILL_ACTIVE){CloseHandle(install_process);install_process=NULL;wcscpy(setup_message,code==0?tr(L"桥接已安装。启动 / 重启 RE4 后连接。",L"Bridge installed. Start / restart RE4 to connect."):tr(L"安装失败：先退出游戏；具体原因见 installer.log。",L"Install failed. Close RE4; see installer.log for details."));}}}
static void receiver_send(const char *op){
    int port=input_port();wchar_t ip[64];GetWindowTextW(ip_edit,ip,64);
    if(strcmp(op,"receiver_stop") && (!port || !ipv4_valid(ip))){wcscpy(setup_message,tr(L"UDP 端口必须为 1–65535，绑定地址必须为 IPv4。",L"UDP port must be 1–65535 and bind address must be IPv4."));return;}
    char p[32],address[128];snprintf(p,sizeof(p),"%d",port);WideCharToMultiByte(CP_UTF8,0,ip,-1,address,sizeof(address),NULL,NULL);command(op,p,address);
}
static void draw_logo(HDC dc,int x,int y,int size){if(app_logo){void *g;GdipCreateFromHDC(dc,&g);GdipSetInterpolationMode(g,7);UINT w,h;GdipGetImageWidth(app_logo,&w);GdipGetImageHeight(app_logo,&h);GdipDrawImageRectRectI(g,app_logo,px(x),px(y),px(size),px(size),0,0,w,h,2,NULL,NULL,NULL);GdipDeleteGraphics(g);}}
static void receiver_card(HDC dc,int x,int y,int w,int h){
    round_box(dc,x,y,w,h,12,WHITE,BORDER);text(dc,tr(L"RhyLive 面捕",L"RhyLive capture"),x+24,y+20,w-48,36,1,INK,DT_SINGLELINE);
    text(dc,tr(L"通过游戏内接收器读取 ARKit 52 通道",L"Receive 52 ARKit channels through the game plugin"),x+24,y+66,w-48,35,3,MUTED,DT_WORDBREAK);
    text(dc,tr(L"UDP 端口",L"UDP port"),x+24,y+113,120,32,0,INK,DT_SINGLELINE|DT_VCENTER);round_box(dc,x+156,y+111,w-180,36,7,WHITE,BORDER);
    text(dc,L"Bind IPv4",x+24,y+168,120,32,0,INK,DT_SINGLELINE|DT_VCENTER);round_box(dc,x+156,y+166,w-180,36,7,WHITE,BORDER);
    if(render_path[0]){wchar_t entry[64];GetWindowTextW(port_edit,entry,64);text(dc,entry,x+166,y+111,w-200,36,0,INK,DT_SINGLELINE|DT_VCENTER);GetWindowTextW(ip_edit,entry,64);text(dc,entry,x+166,y+166,w-200,36,0,INK,DT_SINGLELINE|DT_VCENTER);}
    text(dc,tr(L"手机填写电脑局域网地址",L"Phone destination: this PC's LAN address"),x+24,y+228,w-48,26,0,INK,DT_SINGLELINE);
    wchar_t addresses[768];swprintf(addresses,768,L"%ls\n%ls",lan_addresses[0]?lan_addresses:tr(L"未找到局域网 IPv4",L"No LAN IPv4 found"),tr(L"手机端口与上方一致；0.0.0.0 仅用于电脑监听。",L"Use the same port on phone. 0.0.0.0 is for PC binding only."));text(dc,addresses,x+24,y+265,w-48,78,3,MUTED,DT_WORDBREAK);
    int enabled=!demo && connected && receiver_available;
    button(dc,tr(L"启动 / 重启接收",L"Start / restart"),x+24,y+338,(w-60)/2,38,RECEIVER_START,enabled,1,0);
    button(dc,tr(L"停止接收",L"Stop receiver"),x+36+(w-60)/2,y+338,(w-60)/2,38,RECEIVER_STOP,enabled,0,0);
    button(dc,tr(L"保存接收设置",L"Save receiver settings"),x+24,y+391,w-48,34,RECEIVER_APPLY,enabled,0,0);
    wchar_t stats[1200],age[64];if(connected && receiver_available && receiver_frames>0 && receiver_age<1000000)swprintf(age,64,L"%d ms",receiver_age);else wcscpy(age,L"—");
    swprintf(stats,1200,L"%ls\nPackets %d  ·  Face frames %d\nAge %ls  ·  Rejected %d  ·  Error %d",enabled?(receiver_running?tr(L"监听中",L"Listening"):tr(L"已停止",L"Stopped")):tr(L"等待游戏启动并加载桥接",L"Waiting for game and receiver bridge"),receiver_packets,receiver_frames,age,receiver_rejected,receiver_error);
    text(dc,stats,x+24,y+440,w-48,62,0,INK,DT_WORDBREAK);text(dc,receiver_status,x+24,y+h-28,w-48,23,3,MUTED,DT_WORDBREAK);
}
static void paint_setup(HDC dc){
    hit_count=0;RECT client={0,0,px(width),px(height)};HBRUSH bg=CreateSolidBrush(rgb(PAPER));FillRect(dc,&client,bg);DeleteObject(bg);
    round_box(dc,0,0,width,76,1,WHITE,0);line(dc,0,76,width,76,BORDER,1);draw_logo(dc,19,17,42);
    text(dc,tr(L"RE4R 形态键工作台",L"RE4R ShapeKey Studio"),72,18,355,39,1,INK,DT_SINGLELINE|DT_VCENTER);
    button(dc,L"中文 / EN",width-220,22,94,34,LANG,1,0,0);button(dc,L"−",width-102,22,29,34,MINIMIZE,1,0,0);button(dc,L"□",width-70,22,29,34,MAXIMIZE,1,0,0);button(dc,L"×",width-38,22,29,34,CLOSE,1,0,0);
    line(dc,214,76,214,height-35,BORDER,1);
    const wchar_t *steps[]={tr(L"游戏目录",L"Game folder"),tr(L"安装桥接",L"Install bridge"),tr(L"面捕连接",L"Face capture")};
    for(int i=0;i<3;i++){int y=111+i*128;round_box(dc,14,y,185,81,10,i==0?SOFT:PAPER,0);wchar_t n[4];swprintf(n,4,L"%d",i+1);round_box(dc,28,y+24,32,32,16,i==0?ACCENT:TRACK,0);text(dc,n,28,y+24,32,32,0,i==0?WHITE:MUTED,DT_SINGLELINE|DT_CENTER|DT_VCENTER);text(dc,steps[i],72,y+20,120,39,0,INK,DT_SINGLELINE|DT_VCENTER);}
    int x=240,w=(width-290)/2,right=x+w+24,top=211;
    text(dc,tr(L"先连接你的游戏",L"Connect your game"),x,106,width-x-24,42,2,INK,DT_SINGLELINE);
    text(dc,tr(L"设置 RE4 目录，准备形态键与面捕控制。",L"Set your RE4 folder to prepare shape-key and face-capture controls."),x,164,width-x-24,30,0,MUTED,DT_SINGLELINE);
    round_box(dc,x,top,w,310,12,WHITE,BORDER);text(dc,tr(L"游戏目录",L"Game directory"),x+24,top+20,w-48,36,1,INK,DT_SINGLELINE);
    text(dc,tr(L"选择直接包含 re4.exe 的文件夹",L"Choose the folder containing re4.exe"),x+24,top+64,w-48,28,3,MUTED,DT_SINGLELINE);
    round_box(dc,x+24,top+109,w-48,40,7,WHITE,BORDER);
    if(render_path[0]){wchar_t entry[1024];GetWindowTextW(directory_edit,entry,1024);text(dc,entry,x+35,top+109,w-70,40,0,MUTED,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);}
    button(dc,tr(L"浏览文件夹",L"Browse folder"),x+24,top+166,(w-60)/2,36,DIR_BROWSE,!install_process,1,0);button(dc,tr(L"验证目录",L"Validate"),x+36+(w-60)/2,top+166,(w-60)/2,36,DIR_VALIDATE,!install_process,0,0);
    round_box(dc,x+24,top+226,w-48,52,8,directory_valid?0xffedf5ef:SOFT,0);text(dc,directory_valid?tr(L"已找到 re4.exe",L"re4.exe found"):tr(L"目录未验证，工作台暂未开放",L"Validate a directory to unlock controls"),x+36,top+226,w-72,52,0,directory_valid?GREEN:ACCENT,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    round_box(dc,x,top+328,w,210,12,WHITE,BORDER);text(dc,tr(L"安装桥接",L"Install bridge"),x+24,top+348,w-48,36,1,INK,DT_SINGLELINE);
    text(dc,tr(L"安装前退出 RE4；自动备份当前脚本。\n已安装时可直接启动游戏并连接。",L"Close RE4 before installation. Existing scripts are backed up.\nAlready installed? Start the game to connect."),x+24,top+392,w-48,68,3,MUTED,DT_WORDBREAK);
    button(dc,install_process?tr(L"正在安装…",L"Installing…"):tr(L"安装 / 更新桥接与面捕",L"Install / update bridge + capture"),x+24,top+484,w-48,40,BRIDGE_INSTALL,directory_valid && !install_process && !demo,1,0);
    receiver_card(dc,right,top,w,height-top-95);
    text(dc,setup_message,x,height-108,w,32,3,MUTED,DT_SINGLELINE|DT_END_ELLIPSIS);
    button(dc,tr(L"进入工作台",L"Enter workbench"),x,height-69,width-x-25,38,WORKBENCH_ENTER,directory_valid && !install_process,1,0);
    text(dc,tr(L"首次设置 · 0.3.0",L"Setup · 0.3.0"),24,height-28,190,24,3,MUTED,DT_SINGLELINE);
}
static void setup_layout(void){
    int x=240,w=(width-290)/2,right=x+w+24,top=211;
    MoveWindow(directory_edit,px(x+35),px(top+118),px(w-70),px(24),TRUE);ShowWindow(directory_edit,setup_page?SW_SHOW:SW_HIDE);
    int rx=setup_page?right:240,ry=setup_page?top:180,rw=setup_page?w:width-294;
    MoveWindow(port_edit,px(rx+166),px(ry+118),px(rw-200),px(24),TRUE);MoveWindow(ip_edit,px(rx+166),px(ry+173),px(rw-200),px(24),TRUE);
    ShowWindow(port_edit,(setup_page || receiver_page)?SW_SHOW:SW_HIDE);ShowWindow(ip_edit,(setup_page || receiver_page)?SW_SHOW:SW_HIDE);
}
static int setup_action(int a){
    switch(a){
    case SETUP_OPEN:command("release","","");setup_page=1;receiver_page=0;break;
    case DIR_BROWSE:browse_directory();break;case DIR_VALIDATE:validate_directory();break;case BRIDGE_INSTALL:run_install();break;
    case WORKBENCH_ENTER:validate_directory();if(directory_valid && !install_process){setup_page=0;receiver_page=0;}break;
    case RECEIVER_START:receiver_send("receiver_start");break;case RECEIVER_STOP:receiver_send("receiver_stop");break;case RECEIVER_APPLY:receiver_send("receiver_config");break;
    default:return 0;
    }return 1;
}
