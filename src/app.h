#pragma once
#include "tcp_probe.h"

// Xbox 360 RGH screens. Diagnostics are always optional; the home screen
// never starts network traffic or asks for credentials.
class App {
public:
    App();
    bool initialize();
    int run();
    void shutdown();
private:
    enum Screen {
        SCREEN_HOME,
        SCREEN_ROADMAP,
        SCREEN_FORTNITE,
        SCREEN_NETWORK
    };
    void render();
    void render_home();
    void render_roadmap();
    void render_fortnite();
    void render_network();
    bool running_;
    bool protocol_ok_;
    Screen screen_;
    TcpProbe probe_;
};
