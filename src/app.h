#pragma once
#include "tcp_probe.h"
#include "xbox_account.h"

// Native Xbox 360 home and device-code screens. Detailed diagnostics stay
// in the debug log; the controller flow exposes only enter/retry and back.
class App {
public:
    App();
    bool initialize();
    int run();
    void shutdown();
private:
    enum { LOG_LINES=10, LINE_CAP=144 };
    enum Screen { HOME=0, LOGIN=1 };
    void log(const char* message);
    void log_step(int number, int percentage, const char* message);
    void run_offline_tests();
    void start_fortnite();
    void poll_network();
    void poll_account();
    void draw();
    void draw_home();
    void draw_login();
    void draw_network_diagnostics();

    Screen screen_;
    bool running_;
    bool diagnostics_visible_;
    bool tests_ok_;
    bool flow_started_;
    bool login_launched_;
    int progress_percent_;
    TcpProbe probe_;
    XboxAccount account_;
    unsigned int tests_run_;
    int network_previous_;
    int account_previous_;
    unsigned int line_count_;
    char history_[LOG_LINES][LINE_CAP];
};
