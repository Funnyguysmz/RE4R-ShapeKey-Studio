#include "studio_core.h"
/* GDI+ flat API: antialiased vector controls, no external UI runtime. */
typedef struct {UINT version;void *callback;BOOL background,codec;} GPStartup;
__declspec(dllimport) int WINAPI GdiplusStartup(ULONG_PTR*,const GPStartup*,void*);
__declspec(dllimport) void WINAPI GdiplusShutdown(ULONG_PTR);
__declspec(dllimport) int WINAPI GdipCreateFromHDC(HDC,void**);
__declspec(dllimport) int WINAPI GdipDeleteGraphics(void*);
__declspec(dllimport) int WINAPI GdipSetSmoothingMode(void*,int);
__declspec(dllimport) int WINAPI GdipSetInterpolationMode(void*,int);
__declspec(dllimport) int WINAPI GdipCreatePath(int,void**);
__declspec(dllimport) int WINAPI GdipAddPathArc(void*,float,float,float,float,float,float);
__declspec(dllimport) int WINAPI GdipClosePathFigure(void*);
__declspec(dllimport) int WINAPI GdipDeletePath(void*);
__declspec(dllimport) int WINAPI GdipCreateSolidFill(UINT,void**);
__declspec(dllimport) int WINAPI GdipDeleteBrush(void*);
__declspec(dllimport) int WINAPI GdipFillPath(void*,void*,void*);
__declspec(dllimport) int WINAPI GdipCreatePen1(UINT,float,int,void**);
__declspec(dllimport) int WINAPI GdipDeletePen(void*);
__declspec(dllimport) int WINAPI GdipDrawPath(void*,void*,void*);
__declspec(dllimport) int WINAPI GdipDrawLine(void*,void*,float,float,float,float);
__declspec(dllimport) int WINAPI GdipLoadImageFromFile(const WCHAR*,void**);
__declspec(dllimport) int WINAPI GdipDisposeImage(void*);
__declspec(dllimport) int WINAPI GdipGetImageWidth(void*,UINT*);
__declspec(dllimport) int WINAPI GdipGetImageHeight(void*,UINT*);
__declspec(dllimport) int WINAPI GdipDrawImageRectRectI(void*,void*,int,int,int,int,int,int,int,int,int,void*,void*,void*);
__declspec(dllimport) int WINAPI GdipCreateBitmapFromHBITMAP(HBITMAP,HPALETTE,void**);
__declspec(dllimport) int WINAPI GdipSaveImageToFile(void*,const WCHAR*,const CLSID*,const void*);
static const UINT PAPER=0xfffcfbfa,WHITE=0xffffffff,INK=0xff242938,MUTED=0xff858894,BORDER=0xffe8e8ed,ACCENT=0xffee8b73,SOFT=0xfffdf0eb,TRACK=0xffebecef,GREEN=0xff6b986a;
static HFONT fonts[6];static HBRUSH white_brush;static ULONG_PTR gdiplus;static void *portrait;
static int width=1360,height=860,hover_action,hover_key=-1,drag_key=-1,drag_gain,edit_key=-1,dropdown,smoke_exit;
static wchar_t render_path[MAX_PATH];static RECT content_clip;static int content_total;
typedef struct {RECT r;int action,key,enabled;} Hit;
static Hit hits[1024];static int hit_count;
enum {NAV=100,START=200,RELEASE,ZERO,SAVE,LOAD,COMPARE,SWEEP,LANG,MINIMIZE,MAXIMIZE,CLOSE,MODE,KEY_ROW,KEY_SLIDER,KEY_VALUE,KEY_ZERO,GAIN_SLIDER,MODE_ITEM=300};
static int px(double x){return (int)lround(x*scale);}
static UINT argb(COLORREF c){return 0xff000000|((c&255)<<16)|(c&0xff00)|((c>>16)&255);}
static COLORREF rgb(UINT a){return RGB((a>>16)&255,(a>>8)&255,a&255);}
static RECT rect(int x,int y,int w,int h){RECT r={x,y,x+w,y+h};return r;}
static void hit(int x,int y,int w,int h,int action,int key,int enabled){if(hit_count<1024)hits[hit_count++]=(Hit){rect(x,y,w,h),action,key,enabled};}
static Hit *at(int x,int y){POINT p={x,y};for(int i=hit_count-1;i>=0;i--)if(PtInRect(&hits[i].r,p))return &hits[i];return NULL;}
static void round_box(HDC dc,int x,int y,int w,int h,int radius,UINT fill,UINT stroke){
    void *g=NULL,*path=NULL,*brush=NULL,*pen=NULL;float X=x*scale,Y=y*scale,W=w*scale,H=h*scale,R=radius*2*scale;
    if(W<=0 || H<=0)return;GdipCreateFromHDC(dc,&g);GdipSetSmoothingMode(g,4);GdipCreatePath(0,&path);
    GdipAddPathArc(path,X,Y,R,R,180,90);GdipAddPathArc(path,X+W-R,Y,R,R,270,90);GdipAddPathArc(path,X+W-R,Y+H-R,R,R,0,90);GdipAddPathArc(path,X,Y+H-R,R,R,90,90);GdipClosePathFigure(path);
    GdipCreateSolidFill(fill,&brush);GdipFillPath(g,brush,path);GdipDeleteBrush(brush);
    if(stroke){GdipCreatePen1(stroke,(float)scale,2,&pen);GdipDrawPath(g,pen,path);GdipDeletePen(pen);}GdipDeletePath(path);GdipDeleteGraphics(g);
}
static void line(HDC dc,int x,int y,int x2,int y2,UINT color,double weight){void *g=NULL,*p=NULL;GdipCreateFromHDC(dc,&g);GdipSetSmoothingMode(g,4);GdipCreatePen1(color,(float)(weight*scale),2,&p);GdipDrawLine(g,p,x*scale,y*scale,x2*scale,y2*scale);GdipDeletePen(p);GdipDeleteGraphics(g);}
static void text(HDC dc,const wchar_t *s,int x,int y,int w,int h,int font,UINT color,UINT flags){
    RECT r={px(x),px(y),px(x+w),px(y+h)};HFONT old=SelectObject(dc,fonts[font]);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,rgb(color));DrawTextW(dc,s,-1,&r,flags|DT_NOPREFIX);SelectObject(dc,old);
}
static void icon(HDC dc,int n,int x,int y,UINT c){
    if(n==0 || n==1){round_box(dc,x,y,22,22,11,WHITE,c);line(dc,x+6,y+8,x+7,y+8,c,2);line(dc,x+15,y+8,x+16,y+8,c,2);line(dc,x+7,y+15,x+11,y+17,c,1.5);line(dc,x+11,y+17,x+16,y+14,c,1.5);}
    else if(n==2){line(dc,x+6,y+1,x+1,y+7,c,1.5);line(dc,x+1,y+7,x+6,y+10,c,1.5);line(dc,x+6,y+10,x+4,y+22,c,1.5);line(dc,x+4,y+22,x+19,y+22,c,1.5);line(dc,x+19,y+22,x+17,y+10,c,1.5);line(dc,x+17,y+10,x+22,y+7,c,1.5);line(dc,x+22,y+7,x+17,y+1,c,1.5);line(dc,x+6,y+1,x+11,y+4,c,1.5);line(dc,x+11,y+4,x+17,y+1,c,1.5);}
    else if(n==3){round_box(dc,x,y,22,22,10,WHITE,c);line(dc,x+2,y+11,x+8,y+5,c,1.5);line(dc,x+8,y+5,x+13,y+11,c,1.5);line(dc,x+13,y+11,x+18,y+5,c,1.5);}
    else if(n==4){round_box(dc,x,y+6,23,17,3,WHITE,c);line(dc,x+2,y+6,x+2,y+2,c,1.5);line(dc,x+2,y+2,x+10,y+2,c,1.5);line(dc,x+10,y+2,x+13,y+6,c,1.5);}
    else {line(dc,x,y+15,x+6,y+15,c,1.5);line(dc,x+6,y+15,x+10,y+2,c,1.5);line(dc,x+10,y+2,x+14,y+23,c,1.5);line(dc,x+14,y+23,x+18,y+10,c,1.5);line(dc,x+18,y+10,x+24,y+10,c,1.5);}
}
static void button(HDC dc,const wchar_t *label,int x,int y,int w,int h,int action,int enabled,int primary,int toggle){
    UINT bg=primary?ACCENT:(toggle?SOFT:WHITE);if(hover_action==action && enabled)bg=primary?0xffe97f66:SOFT;
    round_box(dc,x,y,w,h,8,bg,primary?0:BORDER);text(dc,label,x+8,y,w-16,h,0,!enabled?MUTED:(primary?WHITE:(toggle?ACCENT:INK)),DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    hit(x,y,w,h,action,-1,enabled);
}
static const wchar_t *nav_name(int n){const wchar_t *zh[]={L"表情",L"捏脸",L"身体与服装",L"头发与兽耳",L"预设",L"诊断",L"RhyLive 面捕"};const wchar_t *en[]={L"Expressions",L"Face shape",L"Body & outfit",L"Hair & ears",L"Presets",L"Diagnostics",L"RhyLive capture"};return english?en[n]:zh[n];}
static const wchar_t *mode_name(int n){const wchar_t *zh[]={L"手动",L"原生动画",L"原生 + 手动微调",L"RhyLive + 微调",L"面捕 / 原生回落"};const wchar_t *en[]={L"Manual",L"Native animation",L"Native + offsets",L"RhyLive + offsets",L"Capture / fallback"};return n>=1 && n<=5?(english?en[n-1]:zh[n-1]):L"—";}
static const wchar_t *group_name(Key *k){
    if(!english && !wcscmp(k->group,k->region)){const wchar_t *ids[]={L"brow",L"eye",L"gaze",L"mouth",L"effect",L"shape",L"body",L"outfit",L"hair"};const wchar_t *names[]={L"眉部",L"眼部",L"视线",L"嘴部",L"特效",L"捏脸",L"身体",L"服装",L"头发与兽耳"};for(int i=0;i<9;i++)if(!wcscmp(k->region,ids[i]))return names[i];}return k->group;
}
static void pick(int k){selected=k;if(k>=0 && k<count)strcpy(selected_id,keys[k].id);InvalidateRect(win,NULL,FALSE);}
static void set_weight(int k,double v){if(k<0 || k>=count || !owned || !ready() || !keys[k].writable)return;char b[32];v=fmax(0,fmin(1,v));snprintf(b,sizeof(b),"%.4f",v);command("set",keys[k].id,b);keys[k].manual=v;InvalidateRect(win,NULL,FALSE);}
static void draw_portrait(HDC dc,int x,int y,int w,int h){
    if(portrait && (demo || strstr(profile_name,"Shinano"))){UINT iw=0,ih=0;GdipGetImageWidth(portrait,&iw);GdipGetImageHeight(portrait,&ih);void *g=NULL;GdipCreateFromHDC(dc,&g);GdipSetInterpolationMode(g,7);
        /* Display the real project render, centered on the face and shoulders. */
        GdipDrawImageRectRectI(g,portrait,px(x),px(y),px(w),px(h),(int)(iw*.255),0,(int)(iw*.49),(int)(ih*.47),2,NULL,NULL,NULL);GdipDeleteGraphics(g);
    }else{round_box(dc,x,y,w,h,10,0xfff2f1ef,0);icon(dc,0,x+w/2-11,y+h/2-28,MUTED);text(dc,tr(L"模型缩略图",L"Model thumbnail"),x,y+h/2+10,w,26,0,MUTED,DT_CENTER|DT_SINGLELINE);}
}
static void slider(HDC dc,int x,int y,int w,double value,int action,int key,int enabled){
    int progress=(int)(w*fmax(0,fmin(1,value)));round_box(dc,x,y+8,w,6,3,TRACK,0);
    if(progress>0)round_box(dc,x,y+8,progress,6,3,enabled?0xffffab96:0xffd1d2d7,0);
    round_box(dc,x+progress-8,y+3,16,16,8,WHITE,enabled?ACCENT:0xffc8cbd1);hit(x-8,y,w+16,25,action,key,enabled);
}
#include "setup_ui.h"
static void paint_ui(HDC dc){
    if(setup_page){paint_setup(dc);return;}
    hit_count=0;RECT client={0,0,px(width),px(height)};HBRUSH bg=CreateSolidBrush(rgb(PAPER));FillRect(dc,&client,bg);DeleteObject(bg);
    round_box(dc,0,0,width,76,1,WHITE,0);line(dc,0,76,width,76,BORDER,1);line(dc,190,76,190,height-35,BORDER,1);
    draw_logo(dc,19,17,42);
    text(dc,tr(L"RE4R 形态键工作台",L"RE4R ShapeKey Studio"),62,19,232,38,1,INK,DT_SINGLELINE|DT_VCENTER);
    wchar_t model[1024];wide(profile_name,model,1024);if(!model[0])wcscpy(model,tr(L"等待游戏模型",L"Waiting for model"));
    round_box(dc,308,21,231,35,7,WHITE,BORDER);text(dc,model,320,21,205,35,0,INK,DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    round_box(dc,560,34,8,8,4,demo?ACCENT:(ready()?GREEN:MUTED),0);
    text(dc,demo?tr(L"演示模式",L"Offline demo"):(ready()?(owned?tr(L"控制中",L"Controlling"):tr(L"已连接",L"Connected")):tr(L"未连接",L"Disconnected")),576,21,100,35,0,MUTED,DT_SINGLELINE|DT_VCENTER);
    button(dc,english?L"中文 / EN":L"中文 / EN",width-438,22,94,34,LANG,1,0,0);
    button(dc,tr(L"开始控制",L"Take control"),width-329,21,119,36,START,ready(),1,0);
    button(dc,tr(L"释放",L"Release"),width-198,21,79,36,RELEASE,ready() && owned,0,0);
    button(dc,L"−",width-102,22,29,34,MINIMIZE,1,0,0);button(dc,L"□",width-70,22,29,34,MAXIMIZE,1,0,0);button(dc,L"×",width-38,22,29,34,CLOSE,1,0,0);
    for(int i=0;i<7;i++){int y=104+i*61;if(category==i){round_box(dc,12,y,166,49,8,SOFT,0);line(dc,13,y+8,13,y+41,ACCENT,3);}
        icon(dc,i,27,y+13,category==i?ACCENT:INK);text(dc,nav_name(i),66,y,107,49,0,category==i?ACCENT:INK,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);hit(12,y,166,49,NAV+i,-1,1);
    }
    button(dc,tr(L"目录与初始化",L"Setup / folder"),12,height-151,166,36,SETUP_OPEN,1,0,0);
    if(receiver_page){receiver_card(dc,240,180,width-294,height-235);text(dc,setup_message,240,123,width-280,35,3,MUTED,DT_SINGLELINE|DT_END_ELLIPSIS);return;}
    wchar_t stats[128];swprintf(stats,128,tr(L"%d 个形态键",L"%d shape keys"),count);text(dc,stats,24,height-94,151,28,0,MUTED,DT_SINGLELINE);
    text(dc,L"ShapeKey Studio 0.3.0",24,height-65,150,20,3,MUTED,DT_SINGLELINE);
    int main_x=214,inspector_x=width-294,main_w=inspector_x-main_x-24,top=210,bottom=height-114;
    text(dc,nav_name(category),main_x,104,main_w-245,35,2,INK,DT_SINGLELINE|DT_VCENTER);
    button(dc,mode_name(source_mode),main_x+main_w-214,103,214,36,MODE,ready() && owned,0,0);
    round_box(dc,main_x,155,main_w,38,8,WHITE,BORDER);
    if(render_path[0])text(dc,tr(L"搜索形态键…",L"Search shape keys…"),main_x+14,155,main_w-28,38,0,MUTED,DT_SINGLELINE|DT_VCENTER);
    if(category<4){
        round_box(dc,main_x,top,main_w,bottom-top,12,WHITE,BORDER);
        int save_dc=SaveDC(dc);IntersectClipRect(dc,px(main_x+1),px(top+1),px(main_x+main_w-1),px(bottom-1));
        content_clip=rect(main_x,top,main_w,bottom-top);int y=top+12-scroll_y;wchar_t previous_group[128]=L"";
        for(int v=0;v<shown;v++){int i=visible[v];Key *k=&keys[i];const wchar_t *group=group_name(k);
            if(wcscmp(previous_group,group)){text(dc,group,main_x+22,y,main_w-44,38,1,INK,DT_SINGLELINE|DT_VCENTER);line(dc,main_x+20,y+42,main_x+main_w-20,y+42,BORDER,1);wcscpy(previous_group,group);y+=51;}
            if(y+76>top && y<bottom){int enabled=ready() && owned && k->writable;
                if(selected==i)round_box(dc,main_x+10,y+2,main_w-20,71,8,SOFT,0);
                text(dc,k->label,main_x+24,y+8,175,35,0,INK,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
                text(dc,k->writable?tr(L"已绑定",L"Bound"):tr(L"等待模块 / 网格",L"Waiting for mesh"),main_x+24,y+45,178,20,3,k->writable?MUTED:ACCENT,DT_SINGLELINE|DT_END_ELLIPSIS);
                if(y>=top && y+76<=bottom)hit(main_x+12,y,main_w-24,76,KEY_ROW,i,1);
                int sx=main_x+206,sw=main_w-386;
                if(y>=top && y+76<=bottom)slider(dc,sx,y+20,sw,k->manual,KEY_SLIDER,i,enabled);
                else {round_box(dc,sx,y+28,sw,6,3,TRACK,0);}
                int vx=main_x+main_w-157;round_box(dc,vx,y+16,73,35,7,WHITE,BORDER);
                wchar_t num[64];swprintf(num,64,L"%.2f",k->manual);text(dc,num,vx,y+16,73,35,0,INK,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
                if(y>=top && y+76<=bottom)hit(vx,y+16,73,35,KEY_VALUE,i,enabled);
                swprintf(num,64,k->writable?L"%.2f":L"—",k->actual);text(dc,num,main_x+main_w-74,y+13,51,24,3,MUTED,DT_SINGLELINE|DT_RIGHT|DT_VCENTER);
                round_box(dc,main_x+main_w-75,y+44,51,3,1,TRACK,0);if(k->actual>0)round_box(dc,main_x+main_w-75,y+44,(int)(51*fmin(1,k->actual)),3,1,GREEN,0);
                line(dc,main_x+20,y+76,main_x+main_w-20,y+76,0xfff3f3f5,1);
            }y+=77;
        }
        content_total=y+scroll_y-top+12;
        if(!shown)text(dc,count?tr(L"这个分类没有匹配的形态键",L"No matching keys in this category"):tr(L"等待游戏连接与模型配置",L"Waiting for game and model profile"),main_x+28,top+70,main_w-56,60,0,MUTED,DT_CENTER|DT_WORDBREAK);
        RestoreDC(dc,save_dc);
        if(content_total>bottom-top){int available=bottom-top-24,thumb=(int)((double)available*(bottom-top)/content_total);if(thumb<32)thumb=32;int travel=available-thumb;int ty=top+12+(int)((double)scroll_y/(content_total-(bottom-top))*travel);round_box(dc,main_x+main_w-9,ty,3,thumb,1,0xffd6d7dc,0);}
    }else {
        round_box(dc,main_x,top,main_w,bottom-top,12,WHITE,BORDER);
        if(category==4){text(dc,tr(L"保存你调好的表情与形态",L"Keep your expression and shape settings"),main_x+28,top+30,main_w-56,40,1,INK,DT_SINGLELINE);text(dc,tr(L"预设保存手动权重，可在同一模型上重新载入。\n原生动画和面捕的实时数值不写入预设。",L"Presets store manual offsets for the same model.\nLive animation and capture values are not saved."),main_x+28,top+88,main_w-56,90,0,MUTED,DT_WORDBREAK);button(dc,tr(L"保存预设…",L"Save preset…"),main_x+28,top+200,160,40,SAVE,ready(),1,0);button(dc,tr(L"载入预设…",L"Load preset…"),main_x+205,top+200,160,40,LOAD,ready() && owned,0,0);}
        else {wchar_t info[6000];int bound=0;for(int i=0;i<count;i++)bound+=keys[i].writable;swprintf(info,6000,L"%ls\r\n\r\n%ls  %d / %d\r\n\r\n%ls\r\n\r\n%ls\r\n\r\n%ls",tr(L"连接与模型状态",L"Connection and model status"),tr(L"可写入形态键",L"Writable keys"),bound,count,connection_status,capture_status,runtime_error);text(dc,info,main_x+28,top+25,main_w-56,bottom-top-45,0,INK,DT_WORDBREAK);}
    }
    button(dc,tr(L"全部归零",L"Zero all"),main_x,height-92,115,36,ZERO,ready() && owned,0,0);
    button(dc,tr(L"中性对比",L"Neutral"),main_x+127,height-92,115,36,COMPARE,ready() && owned,0,compare_on);
    button(dc,tr(L"保存预设",L"Save preset"),main_x+main_w-242,height-92,115,36,SAVE,ready(),0,0);
    button(dc,tr(L"载入预设",L"Load preset"),main_x+main_w-115,height-92,115,36,LOAD,ready() && owned,0,0);
    round_box(dc,inspector_x,102,270,height-151,12,WHITE,BORDER);
    draw_portrait(dc,inspector_x+12,114,246,238);
    round_box(dc,inspector_x+22,324,124,22,5,WHITE,0);text(dc,tr(L"静态工程预览",L"Static preview"),inspector_x+28,324,112,22,3,MUTED,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    text(dc,model,inspector_x+18,368,234,32,1,INK,DT_SINGLELINE|DT_END_ELLIPSIS);line(dc,inspector_x+18,410,inspector_x+252,410,BORDER,1);
    text(dc,tr(L"当前选中形态键",L"Selected shape key"),inspector_x+18,430,234,23,3,MUTED,DT_SINGLELINE);
    text(dc,selected>=0?keys[selected].label:tr(L"在左侧选择一个键",L"Select a key"),inspector_x+18,458,234,36,1,INK,DT_SINGLELINE|DT_END_ELLIPSIS);
    wchar_t num[64];text(dc,tr(L"手动权重",L"Manual weight"),inspector_x+18,515,128,31,0,MUTED,DT_SINGLELINE|DT_VCENTER);
    round_box(dc,inspector_x+158,514,92,34,7,WHITE,BORDER);swprintf(num,64,L"%.2f",selected>=0?keys[selected].manual:0);text(dc,num,inspector_x+158,514,92,34,0,INK,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    hit(inspector_x+158,514,92,34,KEY_VALUE,selected,ready() && owned && selected>=0 && keys[selected].writable);
    text(dc,tr(L"游戏回读",L"Game readback"),inspector_x+18,568,128,31,0,MUTED,DT_SINGLELINE|DT_VCENTER);round_box(dc,inspector_x+158,567,92,34,7,0xfff5f5f6,0);swprintf(num,64,selected>=0 && keys[selected].writable?L"%.2f":L"—",selected>=0?keys[selected].actual:0);text(dc,num,inspector_x+158,567,92,34,0,MUTED,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    button(dc,tr(L"当前键归零",L"Reset this key"),inspector_x+18,626,234,38,KEY_ZERO,ready() && owned && selected>=0 && keys[selected].writable,0,0);
    text(dc,tr(L"整体强度",L"Overall strength"),inspector_x+18,670,150,26,3,MUTED,DT_SINGLELINE);swprintf(num,64,L"%.2f",strength);text(dc,num,inspector_x+178,670,72,26,3,INK,DT_SINGLELINE|DT_RIGHT);slider(dc,inspector_x+26,700,218,strength/2,GAIN_SLIDER,-1,ready() && owned);
    button(dc,tr(L"逐键巡检",L"Sweep keys"),inspector_x+18,height-91,234,34,SWEEP,ready() && owned,0,sweep_on);
    line(dc,0,height-35,width,height-35,BORDER,1);
    const wchar_t *footer=demo?tr(L"离线演示 · 所有数值仅用于界面预览",L"Offline demo · values are illustrative"):(ready()?(owned?tr(L"桌面控制中 · 关闭窗口将释放覆盖",L"Desktop control active · close to release"):tr(L"观察模式 · 点击开始控制后才能修改",L"Observing · take control to edit")):tr(L"等待游戏连接 · 安装桥接后自动识别模型",L"Waiting for game · model connects automatically"));
    text(dc,footer,24,height-30,width-210,24,3,MUTED,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);text(dc,L"0.3.0  ·  CORAL",width-181,height-30,155,24,3,MUTED,DT_SINGLELINE|DT_RIGHT|DT_VCENTER);
    if(dropdown){int x=main_x+main_w-234,y=144;round_box(dc,x,y,234,215,10,WHITE,BORDER);for(int i=1;i<=5;i++){button(dc,mode_name(i),x+8,y+8+(i-1)*40,218,36,MODE_ITEM+i,ready() && owned,0,source_mode==i);}}
}
static void commit_number(void){if(edit_key>=0){wchar_t b[64];GetWindowTextW(numeric,b,64);wchar_t *end=NULL;double value=wcstod(b,&end);if(end!=b && !*end && isfinite(value))set_weight(edit_key,value);edit_key=-1;ShowWindow(numeric,SW_HIDE);}}
static void action(Hit *h){
    if(!h || !h->enabled)return;int a=h->action;
    if(a!=KEY_VALUE)SetFocus(win);
    if(setup_action(a)){setup_layout();ShowWindow(search,(!setup_page && !receiver_page && category<4)?SW_SHOW:SW_HIDE);InvalidateRect(win,NULL,FALSE);return;}
    if(a>=NAV && a<NAV+7){commit_number();category=a-NAV;receiver_page=category==6;scroll_y=0;dropdown=0;filter_keys();pick(shown?visible[0]:-1);}
    else if(a>MODE_ITEM && a<=MODE_ITEM+5){char b[16];snprintf(b,sizeof(b),"%d",a-MODE_ITEM);command("source",b,"");dropdown=0;}
    else switch(a){
        case START:command("start","","");break;case RELEASE:command("release","","");break;case ZERO:command("zero","","");break;
        case SAVE:preset(1);break;case LOAD:preset(0);break;case COMPARE:command("compare",compare_on?"0":"1","");break;case SWEEP:command("sweep",sweep_on?"0":"1","");break;
        case LANG:english=!english;SendMessageW(search,EM_SETCUEBANNER,0,(LPARAM)tr(L"搜索形态键…",L"Search shape keys…"));break;
        case MINIMIZE:ShowWindow(win,SW_MINIMIZE);break;case MAXIMIZE:ShowWindow(win,IsZoomed(win)?SW_RESTORE:SW_MAXIMIZE);break;case CLOSE:PostMessageW(win,WM_CLOSE,0,0);break;
        case MODE:dropdown=!dropdown;break;case KEY_ROW:pick(h->key);break;case KEY_ZERO:set_weight(selected,0);break;
        case KEY_VALUE:pick(h->key);edit_key=h->key;if(edit_key>=0){wchar_t b[64];swprintf(b,64,L"%.3f",keys[edit_key].manual);SetWindowTextW(numeric,b);MoveWindow(numeric,px(h->r.left+8),px(h->r.top+5),px(h->r.right-h->r.left-16),px(h->r.bottom-h->r.top-10),TRUE);ShowWindow(numeric,SW_SHOW);SetFocus(numeric);SendMessageW(numeric,EM_SETSEL,0,-1);}break;
    }InvalidateRect(win,NULL,FALSE);
}
static void export_preview(const wchar_t *path){
    HDC screen=GetDC(win),dc=CreateCompatibleDC(screen);HBITMAP bitmap=CreateCompatibleBitmap(screen,px(width),px(height));HGDIOBJ old=SelectObject(dc,bitmap);paint_ui(dc);void *image=NULL;GdipCreateBitmapFromHBITMAP(bitmap,NULL,&image);
    const CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
    if(GdipSaveImageToFile(image,path,&png,NULL))smoke_exit=20;GdipDisposeImage(image);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(win,screen);
}
static void load_demo(void){
    wchar_t path[MAX_PATH];GetModuleFileNameW(NULL,path,MAX_PATH);wchar_t *p=wcsrchr(path,L'\\');if(p)wcscpy(p+1,L"demo_profile.tsv");FILE *f=_wfopen(path,L"rb");
    strcpy(latest,"S\tDEMO\t0\t1\t1\t离线演示，不连接游戏\tShinano · 公式服\t演示数值\t\t0\t0\n");
    if(f){size_t len=strlen(latest),n=fread(latest+len,1,CAP-len-1,f);latest[len+n]=0;fclose(f);}else strcat(latest,"K\tjaw\t张嘴\tmouth\t0.0\t0.0\t1\t1\t嘴部\n");
}
static LRESULT CALLBACK edit_proc(HWND h,UINT m,WPARAM wp,LPARAM lp,UINT_PTR id,DWORD_PTR data){(void)id;(void)data;if(m==WM_KEYDOWN && wp==VK_RETURN){commit_number();SetFocus(win);return 0;}if(m==WM_KEYDOWN && wp==VK_ESCAPE){edit_key=-1;ShowWindow(numeric,SW_HIDE);SetFocus(win);return 0;}return DefSubclassProc(h,m,wp,lp);}
static void layout(void){RECT r;GetClientRect(win,&r);width=(int)(r.right/scale);height=(int)(r.bottom/scale);MoveWindow(search,px(228),px(162),px(width-214-318-28),px(24),TRUE);ShowWindow(search,(!setup_page && !receiver_page && category<4)?SW_SHOW:SW_HIDE);setup_layout();InvalidateRect(win,NULL,FALSE);}
static LRESULT CALLBACK window_proc(HWND h,UINT m,WPARAM wp,LPARAM lp){
    if(m==WM_CREATE){win=h;HDC dc=GetDC(h);scale=GetDeviceCaps(dc,LOGPIXELSY)/96.0;ReleaseDC(h,dc);
        RECT work;SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);double fit=fmin((work.right-work.left-32)/1360.0,(work.bottom-work.top-32)/870.0);if(scale>fit && fit>=1)scale=fit;
        int sizes[]={14,18,25,12,15,14};for(int i=0;i<6;i++)fonts[i]=CreateFontW(-px(sizes[i]),0,0,0,(i==1 || i==2)?FW_SEMIBOLD:FW_NORMAL,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei UI");white_brush=CreateSolidBrush(RGB(255,255,255));
        search=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,h,(HMENU)12,GetModuleHandleW(NULL),NULL);SendMessageW(search,WM_SETFONT,(WPARAM)fonts[0],TRUE);SendMessageW(search,EM_SETCUEBANNER,0,(LPARAM)L"搜索形态键…");
        numeric=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|ES_CENTER|ES_AUTOHSCROLL,0,0,1,1,h,(HMENU)13,GetModuleHandleW(NULL),NULL);SendMessageW(numeric,WM_SETFONT,(WPARAM)fonts[0],TRUE);SetWindowSubclass(numeric,edit_proc,1,0);
        wchar_t path[MAX_PATH];GetModuleFileNameW(NULL,path,MAX_PATH);wchar_t *p=wcsrchr(path,L'\\');if(p)wcscpy(p+1,L"Shinano_preview.png");GdipLoadImageFromFile(path,&portrait);
        HINSTANCE inst=GetModuleHandleW(NULL);
        directory_edit=CreateWindowExW(0,L"EDIT",game_directory,WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,h,(HMENU)21,inst,NULL);
        port_edit=CreateWindowExW(0,L"EDIT",L"9000",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL|ES_NUMBER,0,0,1,1,h,(HMENU)22,inst,NULL);
        ip_edit=CreateWindowExW(0,L"EDIT",L"0.0.0.0",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,0,0,1,1,h,(HMENU)23,inst,NULL);
        SendMessageW(directory_edit,EM_SETLIMITTEXT,1000,0);SendMessageW(port_edit,EM_SETLIMITTEXT,5,0);SendMessageW(ip_edit,EM_SETLIMITTEXT,15,0);
        SendMessageW(directory_edit,WM_SETFONT,(WPARAM)fonts[0],TRUE);SendMessageW(port_edit,WM_SETFONT,(WPARAM)fonts[0],TRUE);SendMessageW(ip_edit,WM_SETFONT,(WPARAM)fonts[0],TRUE);
        swprintf(path,MAX_PATH,L"%ls\\Studio_icon.png",app_dir);GdipLoadImageFromFile(path,&app_logo);local_addresses();
        directory_valid=valid_game_directory(game_directory);
        SetTimer(h,1,100,NULL);if(smoke || render_path[0])SetTimer(h,2,500,NULL);return 0;
    }
    if(m==WM_SIZE){commit_number();layout();return 0;}
    if(m==WM_GETMINMAXINFO){((MINMAXINFO*)lp)->ptMinTrackSize=(POINT){px(1120),px(840)};return 0;}
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){PAINTSTRUCT ps;HDC target=BeginPaint(h,&ps);HDC dc=CreateCompatibleDC(target);HBITMAP b=CreateCompatibleBitmap(target,px(width),px(height));HGDIOBJ old=SelectObject(dc,b);paint_ui(dc);BitBlt(target,0,0,px(width),px(height),dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(b);DeleteDC(dc);EndPaint(h,&ps);return 0;}
    if(m==WM_CTLCOLOREDIT){SetBkColor((HDC)wp,RGB(255,255,255));SetTextColor((HDC)wp,rgb(INK));return (LRESULT)white_brush;}
    if(m==WM_COMMAND && LOWORD(wp)==21 && HIWORD(wp)==EN_CHANGE){directory_valid=0;InvalidateRect(h,NULL,FALSE);return 0;}
    if(m==WM_COMMAND && LOWORD(wp)==12 && HIWORD(wp)==EN_CHANGE){scroll_y=0;filter_keys();InvalidateRect(h,NULL,FALSE);return 0;}
    if(m==WM_COMMAND && LOWORD(wp)==13 && HIWORD(wp)==EN_KILLFOCUS){commit_number();return 0;}
    if(m==WM_MOUSEWHEEL){int max_scroll=content_total-(height-114-210);if(max_scroll<0)max_scroll=0;scroll_y-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*77;scroll_y=max(0,min(max_scroll,scroll_y));commit_number();InvalidateRect(h,NULL,FALSE);return 0;}
    if(m==WM_LBUTTONDOWN){int x=(int)(GET_X_LPARAM(lp)/scale),y=(int)(GET_Y_LPARAM(lp)/scale);Hit *hh=at(x,y);commit_number();
        if(hh && hh->enabled && hh->action==KEY_SLIDER){SetFocus(h);pick(hh->key);drag_key=hh->key;interacting=1;SetCapture(h);int sx=214+206,sw=width-294-214-24-386;set_weight(drag_key,(x-sx)/(double)sw);}
        else if(hh && hh->enabled && hh->action==GAIN_SLIDER){drag_gain=1;interacting_gain=1;SetCapture(h);char b[32];double v=fmax(0,fmin(2,2*(x-(width-294+26))/218.0));strength=v;snprintf(b,sizeof(b),"%.3f",v);command("gain",b,"");}
        else action(hh);return 0;
    }
    if(m==WM_MOUSEMOVE){int x=(int)(GET_X_LPARAM(lp)/scale),y=(int)(GET_Y_LPARAM(lp)/scale);Hit *hh=at(x,y);hover_action=hh?hh->action:0;hover_key=hh?hh->key:-1;
        if(drag_key>=0){int sx=214+206,sw=width-294-214-24-386;set_weight(drag_key,(x-sx)/(double)sw);}if(drag_gain){char b[32];double v=fmax(0,fmin(2,2*(x-(width-294+26))/218.0));strength=v;snprintf(b,sizeof(b),"%.3f",v);command("gain",b,"");}InvalidateRect(h,NULL,FALSE);return 0;}
    if(m==WM_LBUTTONUP || m==WM_CAPTURECHANGED){drag_key=-1;drag_gain=0;interacting=0;interacting_gain=0;if(GetCapture()==h)ReleaseCapture();return 0;}
    if(m==WM_KEYDOWN){
        if(wp==VK_ESCAPE){dropdown=0;edit_key=-1;ShowWindow(numeric,SW_HIDE);InvalidateRect(h,NULL,FALSE);return 0;}
        if((wp==VK_LEFT || wp==VK_RIGHT) && selected>=0){set_weight(selected,keys[selected].manual+(wp==VK_RIGHT?.01:-.01));return 0;}
        if((wp==VK_UP || wp==VK_DOWN) && shown){int row=0;for(int i=0;i<shown;i++)if(visible[i]==selected)row=i;row=max(0,min(shown-1,row+(wp==VK_DOWN?1:-1)));pick(visible[row]);return 0;}
    }
    if(m==WM_SETCURSOR && LOWORD(lp)==HTCLIENT){POINT p;GetCursorPos(&p);ScreenToClient(h,&p);Hit *hh=at((int)(p.x/scale),(int)(p.y/scale));SetCursor(LoadCursorW(NULL,hh && hh->enabled?IDC_HAND:IDC_ARROW));return TRUE;}
    if(m==WM_NCHITTEST){LRESULT def=DefWindowProcW(h,m,wp,lp);if(def!=HTCLIENT)return def;POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};ScreenToClient(h,&p);int x=(int)(p.x/scale),y=(int)(p.y/scale);if(y<76 && !at(x,y))return HTCAPTION;return HTCLIENT;}
    if(m==WM_TIMER){if(wp==1){snapshot_update();filter_keys();check_install();
            static int synced;
            if(!connected)synced=0;
            if(receiver_available && !synced){wchar_t port[32];swprintf(port,32,L"%d",receiver_port);SetWindowTextW(port_edit,port);SetWindowTextW(ip_edit,receiver_ip);synced=1;}
            layout();return 0;}
        if(wp==2){snapshot_update();filter_keys();if(smoke){
                if(setup_test){
                    setup_page=1;SetWindowTextW(directory_edit,L"Z:\\missing-re4-folder");validate_directory();
                    setup_action(WORKBENCH_ENTER);if(!setup_page || directory_valid)smoke_exit=30;
                    wchar_t test_dir[1200],exe[1200];swprintf(test_dir,1200,L"%ls\\setup-test-fixture",app_dir);CreateDirectoryW(test_dir,NULL);swprintf(exe,1200,L"%ls\\re4.exe",test_dir);
                    HANDLE f=CreateFileW(exe,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);
                    SetWindowTextW(directory_edit,test_dir);setup_action(WORKBENCH_ENTER);if(setup_page || !directory_valid)smoke_exit=31;
                    SetWindowTextW(port_edit,L"65536");if(input_port())smoke_exit=32;
                    if(ipv4_valid(L"256.1.2.3") || ipv4_valid(L"1.2.3.4x") || !ipv4_valid(L"0.0.0.0"))smoke_exit=33;
                    DeleteFileW(exe);RemoveDirectoryW(test_dir);SetWindowTextW(directory_edit,L"");directory_valid=0;setup_page=0;
                }
                category=2;filter_keys();if(count!=77 || shown!=5 || owned)smoke_exit=5;
                command("start","","");pick(visible[0]);set_weight(selected,.42);if(fabs(keys[selected].manual-.42)>1e-6)smoke_exit=6;
                Hit entry={rect(250,300,73,35),KEY_VALUE,selected,1};action(&entry);SetWindowTextW(numeric,L"nan");commit_number();if(fabs(keys[selected].manual-.42)>1e-6)smoke_exit=10;
                action(&entry);SetWindowTextW(numeric,L"0.33");commit_number();if(fabs(keys[selected].manual-.33)>1e-6)smoke_exit=11;
                Hit mode={rect(0,0,1,1),MODE_ITEM+3,-1,1};action(&mode);if(source_mode!=3)smoke_exit=12;
                category=3;filter_keys();if(shown!=4)smoke_exit=7;SetWindowTextW(search,L"兽耳");filter_keys();if(shown!=2)smoke_exit=8;
                english=1;SetWindowTextW(search,L"");category=2;filter_keys();command("release","","");if(owned)smoke_exit=9;
            }
            if(render_path[0]){if(setup_render){setup_page=1;wcscpy(lan_addresses,L"192.168.1.100（示例）");SetWindowTextW(directory_edit,L"请选择你自己的 RE4 游戏目录");directory_valid=0;export_preview(render_path);}else {category=2;english=0;filter_keys();pick(shown>3?visible[3]:-1);command("start","","");export_preview(render_path);}}
            PostMessageW(h,WM_CLOSE,0,0);return 0;
        }
    }
    if(m==WM_CLOSE){commit_number();command("release","","");if(!demo)Sleep(150);InterlockedExchange(&stopping,1);DestroyWindow(h);return 0;}
    if(m==WM_DESTROY){if(portrait)GdipDisposeImage(portrait);if(app_logo)GdipDisposeImage(app_logo);if(install_process)CloseHandle(install_process);for(int i=0;i<6;i++)DeleteObject(fonts[i]);DeleteObject(white_brush);PostQuitMessage(smoke_exit);return 0;}
    return DefWindowProcW(h,m,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE prev,PWSTR args,int show){
    (void)prev;setup_test=wcsstr(args,L"--setup-smoke-test")!=NULL;setup_render=wcsstr(args,L"--render-setup")!=NULL;
    demo=wcsstr(args,L"--demo")!=NULL;smoke=setup_test || wcsstr(args,L"--smoke-test")!=NULL;
    int argc=0;LPWSTR *argv=CommandLineToArgvW(GetCommandLineW(),&argc);for(int i=1;i<argc-1;i++)if(!wcscmp(argv[i],L"--render-preview") || !wcscmp(argv[i],L"--render-setup")){wcsncpy(render_path,argv[i+1],MAX_PATH-1);demo=1;}LocalFree(argv);if(smoke)demo=1;
    if(wcsstr(args,L"--probe") || wcsstr(args,L"--test-probe")){const wchar_t *probe_pipe=wcsstr(args,L"--test-probe")?L"\\\\.\\pipe\\RE4R.ShapeKey.Desktop.Test.v1":PIPE_NAME;WaitNamedPipeW(probe_pipe,1000);HANDLE p=CreateFileW(probe_pipe,GENERIC_READ|GENERIC_WRITE,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,NULL);if(p==INVALID_HANDLE_VALUE)return 2;DWORD mode=PIPE_READMODE_MESSAGE,n=0,w=0;SetNamedPipeHandleState(p,&mode,NULL,NULL);char *buf=malloc(CAP);int ok=transfer(p,"PING\n",5,&w,1) && transfer(p,buf,CAP-1,&n,0);if(ok){buf[n]=0;printf("%s",buf);}CloseHandle(p);free(buf);return ok?0:3;}
    HANDLE single=CreateMutexW(NULL,FALSE,(smoke || render_path[0])?L"Local\\RE4R.ShapeKey.Studio.Test":L"Local\\RE4R.ShapeKey.Studio.v1");if(single && GetLastError()==ERROR_ALREADY_EXISTS){HWND old=FindWindowW(L"RE4RShapeKeyStudio",NULL);if(old){ShowWindow(old,SW_RESTORE);SetForegroundWindow(old);}CloseHandle(single);return 0;}
    GetModuleFileNameW(NULL,app_dir,1024);wchar_t *sep=wcsrchr(app_dir,L'\\');if(sep)*sep=0;
    swprintf(config_file,1200,L"%ls\\Studio.settings.ini",app_dir);
    if(!demo && !smoke)GetPrivateProfileStringW(L"Studio",L"GameDirectory",L"",game_directory,1024,config_file);
    setup_page=!demo || setup_render;CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);
    SetProcessDPIAware();GPStartup startup={1,NULL,FALSE,FALSE};if(GdiplusStartup(&gdiplus,&startup,NULL))return 4;
    INITCOMMONCONTROLSEX ic={sizeof(ic),ICC_STANDARD_CLASSES};InitCommonControlsEx(&ic);if(demo){load_demo();category=2;}else CreateThread(NULL,0,network,NULL,0,NULL);
    WNDCLASSW cls={0};cls.lpfnWndProc=window_proc;cls.hInstance=instance;cls.lpszClassName=L"RE4RShapeKeyStudio";cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));RegisterClassW(&cls);
    HWND h=CreateWindowExW(WS_EX_APPWINDOW,cls.lpszClassName,L"RE4R 形态键工作台 · 0.3.0",WS_POPUP|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_SYSMENU|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1360,870,NULL,NULL,instance,NULL);if(!h)return 4;
    SetWindowPos(h,NULL,0,0,px(1360),px(870),SWP_NOMOVE|SWP_NOZORDER);layout();if(!smoke && !render_path[0]){ShowWindow(h,show);UpdateWindow(h);}MSG msg;
    while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(h,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(single)CloseHandle(single);GdiplusShutdown(gdiplus);CoUninitialize();return (int)msg.wParam;
}
