#pragma once
#include "tcp_probe.h"
#include "xbox_account.h"

// Xbox 360 Xenon/PowerPC native UI. HOME is a game selector; LOGIN is
// a separate screen for Microsoft Device Authorization and diagnostics.
// This program is not a browser and does not include a xCloud media client.
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
    const char* network_status() const;
    const char* activity_status();

    Screen screen_;
    bool running_;
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
