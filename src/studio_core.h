#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PIPE_NAME L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.v1"
#define CAP 262144
#define MAX_KEYS 2048
typedef struct {char id[256];wchar_t label[256],region[128],group[128];double manual,actual;int writable,mesh;} Key;
static HWND win,search,numeric;
static int english,connected,owned,count,selected=-1,category,scroll_y,demo,smoke,compare_on,sweep_on,source_mode=1;
static double strength=1,scale=1;
static char token[2048],profile_name[1024],selected_id[256];static Key keys[MAX_KEYS];
static int visible[MAX_KEYS],shown;
static int interacting,interacting_gain;
static SRWLOCK gate=SRWLOCK_INIT;
static char latest[CAP],pending[CAP];static size_t pending_len;static int online;static LONG stopping;
static wchar_t connection_status[2048],capture_status[1024],runtime_error[1024];
static int receiver_available,receiver_running,receiver_port=9000,receiver_packets,receiver_frames,receiver_age=2147483647,receiver_rejected,receiver_error;
static wchar_t receiver_ip[64]=L"0.0.0.0",receiver_status[1024];
static const wchar_t *tr(const wchar_t *zh,const wchar_t *en){return english?en:zh;}
static void wide(const char *s,wchar_t *out,int n){if(!MultiByteToWideChar(CP_UTF8,0,s,-1,out,n))out[0]=0;}
static char *field(char **p){char *s=*p;if(!s)return "";char *e=strchr(s,'\t');if(e){*e=0;*p=e+1;}else *p=NULL;return s;}
static int transfer(HANDLE p,void *buf,DWORD n,DWORD *done,int writing){
    OVERLAPPED o={0};o.hEvent=CreateEventW(NULL,TRUE,FALSE,NULL);if(!o.hEvent)return 0;
    BOOL ok=writing?WriteFile(p,buf,n,done,&o):ReadFile(p,buf,n,done,&o);
    if(!ok && GetLastError()==ERROR_IO_PENDING){
        if(WaitForSingleObject(o.hEvent,700)==WAIT_OBJECT_0)ok=GetOverlappedResult(p,&o,done,FALSE);
        else {CancelIoEx(p,&o);GetOverlappedResult(p,&o,done,TRUE);ok=FALSE;}
    }CloseHandle(o.hEvent);return ok;
}
static DWORD WINAPI network(void *unused){
    (void)unused;char *req=malloc(CAP),*response=malloc(CAP);if(!req || !response)return 1;
    while(!InterlockedCompareExchange(&stopping,0,0)){
        WaitNamedPipeW(PIPE_NAME,80);HANDLE p=CreateFileW(PIPE_NAME,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);int success=0;
        if(p!=INVALID_HANDLE_VALUE){DWORD mode=PIPE_READMODE_MESSAGE,n=0,w=0;SetNamedPipeHandleState(p,&mode,NULL,NULL);
            AcquireSRWLockExclusive(&gate);size_t len=pending_len;
            if(len)memcpy(req,pending,len);else {memcpy(req,"PING\n",5);len=5;}pending_len=0;pending[0]=0;ReleaseSRWLockExclusive(&gate);
            if(transfer(p,req,(DWORD)len,&w,1) && w==len && transfer(p,response,CAP-1,&n,0)){
                response[n]=0;AcquireSRWLockExclusive(&gate);memcpy(latest,response,n+1);ReleaseSRWLockExclusive(&gate);success=1;
            }CloseHandle(p);
        }AcquireSRWLockExclusive(&gate);online=success;ReleaseSRWLockExclusive(&gate);Sleep(50);
    }free(req);free(response);return 0;
}
static int ready(void){return connected && token[0] && strcmp(token,"WAIT");}
static void command(const char *op,const char *arg,const char *value){
    if(demo){
        if(!strcmp(op,"start"))owned=1;else if(!strcmp(op,"release"))owned=0;
        else if(!owned)return;
        else if(!strcmp(op,"set")){double v=strtod(value,NULL);if(isfinite(v))for(int i=0;i<count;i++)if(!strcmp(keys[i].id,arg))keys[i].manual=keys[i].actual=fmax(0,fmin(1,v));}
        else if(!strcmp(op,"zero")){for(int i=0;i<count;i++)keys[i].manual=keys[i].actual=0;}
        else if(!strcmp(op,"source"))source_mode=atoi(arg);
        else if(!strcmp(op,"gain"))strength=atof(arg);
        else if(!strcmp(op,"compare"))compare_on=atoi(arg);
        else if(!strcmp(op,"sweep"))sweep_on=atoi(arg);
        return;
    }
    if(!ready() && !(connected && !strncmp(op,"receiver_",9)))return;char b[4096];int n=snprintf(b,sizeof(b),"C\t%s\t%s\t%s\t%s\n",token[0]?token:"WAIT",op,arg?arg:"",value?value:"");
    if(n<=0 || n>=(int)sizeof(b))return;AcquireSRWLockExclusive(&gate);
    if(pending_len+(size_t)n<CAP){memcpy(pending+pending_len,b,n);pending_len+=n;pending[pending_len]=0;}ReleaseSRWLockExclusive(&gate);
}
static int belongs(Key *k,int c){
    int body=!wcscmp(k->region,L"body"),outfit=!wcscmp(k->region,L"outfit"),hair=!wcscmp(k->region,L"hair"),shape=!wcscmp(k->region,L"shape");
    if(c==0)return !body && !outfit && !hair && !shape;
    if(c==1)return shape;if(c==2)return body || outfit;if(c==3)return hair;return 0;
}
static void filter_keys(void){
    wchar_t filter[256];GetWindowTextW(search,filter,256);shown=0;
    for(int i=0;i<count;i++)if(belongs(&keys[i],category) && (!filter[0] || wcsstr(keys[i].label,filter) || wcsstr(keys[i].group,filter)))visible[shown++]=i;
}
static void snapshot_update(void){
    static char previous[CAP];char *data=malloc(CAP);if(!data)return;
    AcquireSRWLockShared(&gate);int conn=online;strcpy(data,latest);ReleaseSRWLockShared(&gate);connected=demo?1:conn;
    if(!connected){receiver_available=0;receiver_running=0;receiver_age=2147483647;}
    if(strcmp(previous,data)){
        double held=(selected>=0 && selected<count)?keys[selected].manual:0,held_strength=strength;
        strcpy(previous,data);count=0;selected=-1;receiver_available=0;receiver_running=0;char *line=data;
        while(line && *line){char *next=strchr(line,'\n');if(next){*next=0;next++;}char *p=line;char *kind=field(&p);
            if(!strcmp(kind,"S")){
                char old_token[2048];strcpy(old_token,token);snprintf(token,sizeof(token),"%s",field(&p));
                if(strcmp(old_token,token)){scroll_y=0;SetWindowTextW(numeric,L"");ShowWindow(numeric,SW_HIDE);}
                owned=atoi(field(&p));source_mode=atoi(field(&p));strength=atof(field(&p));if(interacting_gain)strength=held_strength;
                wide(field(&p),connection_status,2048);snprintf(profile_name,sizeof(profile_name),"%s",field(&p));
                wide(field(&p),capture_status,1024);wide(field(&p),runtime_error,1024);compare_on=atoi(field(&p));sweep_on=atoi(field(&p));
            }else if(!strcmp(kind,"R")){
                receiver_available=atoi(field(&p));receiver_running=atoi(field(&p));receiver_port=atoi(field(&p));wide(field(&p),receiver_ip,64);
                receiver_packets=atoi(field(&p));receiver_frames=atoi(field(&p));receiver_age=atoi(field(&p));receiver_rejected=atoi(field(&p));receiver_error=atoi(field(&p));wide(field(&p),receiver_status,1024);
            }else if(!strcmp(kind,"K") && count<MAX_KEYS){Key *k=&keys[count];
                snprintf(k->id,sizeof(k->id),"%s",field(&p));wide(field(&p),k->label,256);wide(field(&p),k->region,128);
                k->manual=atof(field(&p));k->actual=atof(field(&p));k->writable=atoi(field(&p));k->mesh=atoi(field(&p));wide(field(&p),k->group,128);
                if(!k->group[0])wcscpy(k->group,k->region);if(!strcmp(k->id,selected_id)){selected=count;if(interacting)k->manual=held;}count++;
            }line=next;
        }filter_keys();
    }free(data);
}
static void preset(int saving){
    wchar_t path[MAX_PATH]=L"ShapeKey.preset.tsv";OPENFILENAMEW of={0};of.lStructSize=sizeof(of);of.hwndOwner=win;of.lpstrFile=path;of.nMaxFile=MAX_PATH;
    of.lpstrFilter=L"ShapeKey preset (*.tsv)\0*.tsv\0\0";of.lpstrDefExt=L"tsv";of.Flags=OFN_NOCHANGEDIR|(saving?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    if(!(saving?GetSaveFileNameW(&of):GetOpenFileNameW(&of)))return;FILE *f=_wfopen(path,saving?L"wb":L"rb");if(!f)return;
    if(saving){fprintf(f,"RE4R_SHAPEKEY_PRESET_V1\t%s\n",profile_name);for(int i=0;i<count;i++)fprintf(f,"%s\t%.6f\n",keys[i].id,keys[i].manual);}
    else {char b[2048];int accepted=0;
        if(fgets(b,sizeof(b),f)){b[strcspn(b,"\r\n")]=0;char *p=strchr(b,'\t');if(p && !strncmp(b,"RE4R_SHAPEKEY_PRESET_V1\t",strlen("RE4R_SHAPEKEY_PRESET_V1\t")) && !strcmp(p+1,profile_name))accepted=1;}
        if(!accepted)MessageBoxW(win,tr(L"预设不属于当前模型，未载入。",L"Preset model does not match. Nothing loaded."),L"ShapeKey",MB_ICONWARNING);
        else {command("zero","","");while(fgets(b,sizeof(b),f)){b[strcspn(b,"\r\n")]=0;char *p=strchr(b,'\t');if(!p)continue;*p++=0;double v=strtod(p,NULL);if(!isfinite(v) || v<0 || v>1)continue;for(int i=0;i<count;i++)if(!strcmp(b,keys[i].id)){command("set",b,p);break;}}}
    }fclose(f);
}
