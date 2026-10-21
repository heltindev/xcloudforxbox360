#pragma once
// Native Xbox 360 dashboard, visually inspired by a two-pane game browser.
// All shapes and art authored independently; no GPL assets/source copied.
namespace gameui {
static const unsigned int BACK_TOP=0xFF0B1808u;
static const unsigned int BACK_BOTTOM=0xFF050C05u;
static const unsigned int PANEL=0xFF111F0Eu;
static const unsigned int PANEL_DARK=0xFF0D180Bu;
static const unsigned int BORDER=0xFF2B4620u;
static const unsigned int TEXT=0xFFF5FFF7u;
static const unsigned int MUTED=0xFFB8C8AEu;
static const unsigned int SUBTLE=0xFF8BA47Du;
static const unsigned int ACCENT=0xFF6FE04Bu;
static const unsigned int ACCENT_HI=0xFF9BF575u;
static const unsigned int SUCCESS=0xFF9BF575u;
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
