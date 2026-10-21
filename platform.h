#pragma once

namespace platform {

// Edge-triggered controller buttons, aggregated across Xbox 360 ports 0..3.
struct InputState {
    bool a_pressed;
    bool b_pressed;
    bool back_pressed;
    bool x_pressed;
    bool y_pressed;
    bool up_pressed;
    bool down_pressed;
    bool left_pressed;
    bool right_pressed;
    bool lb_pressed;
    bool rb_pressed;
    bool start_pressed;
};

bool initialize();
void shutdown();
bool pump();
InputState read_input();
void clear(unsigned char red, unsigned char green, unsigned char blue);
void draw_rect(float x, float y, float width, float height, unsigned int color);
void draw_text(float x, float y, float scale, unsigned int color, const char* text);
void present();
void debug_log(const char* message);

} // namespace platform
