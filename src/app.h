#pragma once
#include "tcp_probe.h"

class App {
public:
    App();
    bool initialize();
    int run();
    void shutdown();
private:
    enum Screen { SCREEN_HOME, SCREEN_TCP_TEST };
    void render();
    bool running_;
    Screen screen_;
    TcpProbe probe_;
};
