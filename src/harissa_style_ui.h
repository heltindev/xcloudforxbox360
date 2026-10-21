#pragma once
// Native Xbox 360 dashboard with a restrained emerald/charcoal palette.
// All shapes and art authored independently; no GPL assets/source copied.
namespace gameui {
static const unsigned int BACK_TOP=0xFF0C1712u;
static const unsigned int BACK_BOTTOM=0xFF060B08u;
static const unsigned int PANEL=0xFF111D17u;
static const unsigned int PANEL_DARK=0xFF0A120Eu;
static const unsigned int BORDER=0xFF263A2Fu;
static const unsigned int TEXT=0xFFF4FAF6u;
static const unsigned int MUTED=0xFFAFBEB5u;
static const unsigned int SUBTLE=0xFF789083u;
static const unsigned int ACCENT=0xFF21C86Au;
static const unsigned int ACCENT_HI=0xFF69E69Eu;
static const unsigned int SUCCESS=0xFF69E69Eu;
static const unsigned int WARNING=0xFFFFD286u;
static const unsigned int FAILURE=0xFFFF9292u;

void text(float x,float y,float scale,unsigned int color,const char* str);
void box(float x,float y,float w,float h,unsigned int fill,
         unsigned int edge);
void pill(float x,float y,float w,float h,unsigned int color);
void backdrop();
void header(const char* page,const char* indicator);
void footer(const char* controls);
void hero(float x,float y,float w,float h);
void badge(float x,float y,float w,const char* label,
           unsigned int label_color);
void progress(float x,float y,float width,int percent);
}
