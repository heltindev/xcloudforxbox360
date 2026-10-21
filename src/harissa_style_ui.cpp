#include "harissa_style_ui.h"
#include "platform.h"

namespace gameui {
void text(float x,float y,float scale,unsigned int color,const char* s) {
    platform::draw_text(x,y,scale,color,s);
}
static unsigned int mix(unsigned int a,unsigned int b,int t,int total) {
    const unsigned int ar=(a>>16)&255u,ag=(a>>8)&255u,ab=a&255u;
    const unsigned int br=(b>>16)&255u,bg=(b>>8)&255u,bb=b&255u;
    const unsigned int r=(ar*(total-t)+br*t)/total;
    const unsigned int g=(ag*(total-t)+bg*t)/total;
    const unsigned int bl=(ab*(total-t)+bb*t)/total;
    return 0xff000000u | (r<<16) | (g<<8) | bl;
}
void box(float x,float y,float w,float h,unsigned int fill,
         unsigned int edge) {
    if(w<9.f || h<9.f)return;
    // Minimal one-pixel edges remain crisp at both supported HD modes.
    platform::draw_rect(x+4,y,w-8,h,fill);
    platform::draw_rect(x,y+4,w,h-8,fill);
    platform::draw_rect(x+5,y,w-10,1,edge);
    platform::draw_rect(x+5,y+h-1,w-10,1,edge);
    platform::draw_rect(x,y+5,1,h-10,edge);
    platform::draw_rect(x+w-1,y+5,1,h-10,edge);
}
void pill(float x,float y,float w,float h,unsigned int color) {
    if(w<8.f || h<8.f)return;
    platform::draw_rect(x+3,y,w-6,h,color);
    platform::draw_rect(x,y+3,w,h-6,color);
}
void backdrop() {
    // Restrained 720p/1080p-friendly green-black vertical gradient.
    for(int i=0;i<36;++i) {
        const float y=(float)(i*20);
        platform::draw_rect(0,y,1280,21,mix(BACK_TOP,BACK_BOTTOM,i,35));
    }
    // A quiet emerald edge, without patterned noise behind the type.
    platform::draw_rect(0,0,1280,3,ACCENT);
}
void header(const char* page,const char* indicator) {
    pill(64,46,30,30,ACCENT);
    text(72,51,1.10f,0xFF062112u,"X");
    text(108,44,1.70f,TEXT,"XCLOUD360");
    text(336,54,1.00f,SUBTLE,page?page:"");
    if(indicator && *indicator)
        text(1030,54,1.00f,ACCENT_HI,indicator);
    platform::draw_rect(64,109,1152,1,BORDER);
}
void footer(const char* controls) {
    platform::draw_rect(64,648,1152,1,BORDER);
    text(66,665,1.22f,MUTED,controls?controls:"A  SIGN IN       B  EXIT");
}
void hero(float x,float y,float w,float h) {
    // Self-authored game art: a glowing emerald arcade horizon / abstract
    // skyline. No copied Fortnite or Harissa image assets.
    box(x,y,w,h,0xFF10351Fu,0xFF377C50u);
    for(int i=0;i<36;++i) {
        const float gy=y+2.f+(float)i*(h-4.f)/36.f;
        platform::draw_rect(x+3.f,gy,w-6.f,(h-4.f)/36.f+1.f,
            mix(0xFF23603Bu,0xFF09170Fu,i,35));
    }
    // Approximate glow using progressively smaller solid bands.
    for(int i=0;i<16;++i) {
        const int n=16-i;
        platform::draw_rect(x+w-188.f-(float)(n*10),
              y+57.f+(float)i*3.f,150.f+(float)(n*12),
              3.f,mix(0xFF2D7047u,0xFF69E69Eu,i,15));
    }
    // Original silhouetted buildings, not a game cover asset.
    for(int j=0;j<21;++j) {
        const unsigned int k=29u+(unsigned int)j*17u;
        const float bh=(float)(44u+(k*31u)%92u);
        platform::draw_rect(x+7.f+(float)j*(w-14.f)/21.f,
            y+h-bh,(w-14.f)/21.f+1.f,bh,0xFF102A1Bu);
    }
    for(int j=0;j<17;++j) {
        const unsigned int k=41u+(unsigned int)j*11u;
        const float bh=(float)(25u+(k*23u)%48u);
        platform::draw_rect(x+6.f+(float)j*(w-12.f)/17.f,
            y+h-bh,(w-12.f)/17.f+1.f,bh,0xFF07130Cu);
    }
    platform::draw_rect(x+32,y+34,4,20,ACCENT_HI);

    text(x+32,y+h-94,3.10f,TEXT,"FORTNITE");
}
void badge(float x,float y,float w,const char* label,
           unsigned int label_color) {
    pill(x,y,w,35,0xFF183524u);
    platform::draw_rect(x+2,y+8,2,15,ACCENT);
    text(x+12,y+8,1.03f,label_color,label);
}
void progress(float x,float y,float width,int percent) {
    if(percent<0)percent=0;
    if(percent>100)percent=100;
    pill(x,y,width,13,0xFF1D3527u);
    const float green=width*(float)percent/100.f;
    if(green>8.f)pill(x,y,green,13,ACCENT_HI);
    // Display completion point without hiding remaining-track context.
    if(green>16.f)platform::draw_rect(x+green-3,y,3,13,0xFFC6FFE0u);
}
}
