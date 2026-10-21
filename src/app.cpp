#include "app.h"
#include "platform.h"
#include "protocol_selftest.h"
#include "region_catalog.h"
#include "xbox_live_auth.h"
#include "harissa_style_ui.h"
#include "../auth_config.h"
#include "network_ready.h"
#include "login_cache.h"
#include <xtl.h>
#include <stdio.h>
#include <string.h>

namespace {
const char* login_error(const XboxAccount::Snapshot& auth) {
    if(auth.state==XboxAccount::NOT_CONFIGURED)
        return "Configure your Microsoft application.";
    if(auth.wsa_error==10060)
        return "Connection timed out. Please try again.";
    if(auth.wsa_error)
        return "Check your internet connection.";
    if(auth.http_status==401 || auth.http_status==403)
        return "The service denied access to this account.";
    if(auth.tls_error)
        return "Could not establish a secure connection.";
    return "Please try again shortly.";
}
}

App::App()
    : screen_(HOME),running_(false),diagnostics_visible_(false),tests_ok_(false),flow_started_(false),
      login_launched_(false),progress_percent_(0),tests_run_(0),
      network_previous_(-1),account_previous_(-1),line_count_(0) {
    memset(history_,0,sizeof(history_));
}
void App::log(const char* message){
    if(!message)return;
    if(line_count_>=LOG_LINES){
        for(int i=1;i<LOG_LINES;++i)
            memcpy(history_[i-1],history_[i],LINE_CAP);
        line_count_=LOG_LINES-1;
    }
    strncpy_s(history_[line_count_],LINE_CAP,message,_TRUNCATE);
    ++line_count_;
    // No device/user code, password, account token or client ID in logs.
    char message_debug[200];
    sprintf_s(message_debug,sizeof(message_debug),"[XCLOUD360] %s\n",message);
    platform::debug_log(message_debug);
}
void App::log_step(int index,int percentage,const char* what){
    if(percentage>progress_percent_)progress_percent_=percentage;
    char message[LINE_CAP];
    sprintf_s(message,sizeof(message),"%d. [%d%%] %s",index,percentage,what);
    log(message);
}
bool App::initialize(){
    if(!platform::initialize())return false;
    running_=true;
    screen_=HOME;
    run_offline_tests();
    return true;
}
void App::run_offline_tests(){
    ++tests_run_;
    const bool protocol=protocol_selftest();
    const bool json=region_catalog::offline_selftest();
    tests_ok_=protocol && json;
    char status[LINE_CAP];
    sprintf_s(status,sizeof(status),"SELFTEST %u: PROTOCOL=%s JSON=%s",
              tests_run_,protocol?"PASS":"FAIL",json?"PASS":"FAIL");
    log(status);
}
void App::start_fortnite(){
    screen_=LOGIN; // HOME -> dedicated sign-in page, never a web redirect
    XboxAccount::Snapshot account=account_.snapshot();
    if(account.state==XboxAccount::WAITING_FOR_USER ||
       account.state==XboxAccount::STARTING ||
       account.state==XboxAccount::XBOX_EXCHANGING ||
       (account.state==XboxAccount::ACCOUNT_AUTHORIZED &&
        account.cloud_login_verified && !account.session_test_complete)){
        log("5. LOGIN IN PROGRESS - CODE REMAINS ON SCREEN.");
        return;
    }
    if(flow_started_ && probe_.state()==TcpProbe::RUNNING){
        log("3. WAITING FOR NETWORK TEST.");
        return;
    }
    flow_started_=true;
    login_launched_=false;
    progress_percent_=0;
    account_previous_=-1;
    network_previous_=-1;
    line_count_=0;
    log_step(1,10,"FORTNITE SELECTED - PREPARING AUTHENTICATION");
    log_step(2,15,"CHECKING LOCAL PROTOCOLS");
    run_offline_tests();
    if(!tests_ok_){
        log("2. FAILED: PROTOCOL SELFTEST - CHECK NATIVE XEX");
        flow_started_=false;
        return;
    }
    log_step(2,20,"OFFLINE PROTOCOLS VERIFIED");
    log_step(3,25,"TESTING XNET / DNS / TCP...");
    if(probe_.state()!=TcpProbe::RUNNING && !probe_.start()){
        log("3. NETWORK DIAGNOSTIC COULD NOT START");
        flow_started_=false;
    }
}
void App::poll_network(){
    probe_.update();
    const int state=(int)probe_.state();
    if(state==network_previous_)return;
    network_previous_=state;
    if(state==TcpProbe::RUNNING){
        if(!flow_started_)log("NETWORK: MANUAL DIAGNOSTIC.");
        return;
    }
    if(state!=TcpProbe::CONNECTED && state!=TcpProbe::FAILED)return;
    char summary[LINE_CAP];
    sprintf_s(summary,sizeof(summary),
              "NET: XBOX443=%d(%lu) IP443=%d(%lu) HTTP80=%d(%lu)",
              probe_.xbox_result(),probe_.xbox_error(),
              probe_.control_result(),probe_.control_error(),
              probe_.control80_result(),probe_.control80_error());
    log(summary);
    sprintf_s(summary,sizeof(summary),
              "DNS: XNET=%d PATH=%d PUBLIC_IPS=%d",
              probe_.dns_addresses(),probe_.dns_path(),
              probe_.udp_dns_addresses());
    log(summary);
    if(!flow_started_ || login_launched_)return;
    const bool dns=probe_.dns_addresses()>0 ||
                   probe_.udp_dns_addresses()>0;
    log_step(3,dns?35:30,dns?
             "DNS RESOLVED - VERIFYING HTTPS SEPARATELY":
             "DNS FAILED - TLS WILL TRY ITS OWN RESOLVER");
    login_launched_=true;
    if(account_.start(XCLOUD360_CLIENT_ID))
        log_step(4,45,"REQUESTING MICROSOFT DEVICE CODE");
    else
        log("4. DEVICE CODE REQUEST UNAVAILABLE - CHECK APP ID");
}
void App::poll_account(){
    const XboxAccount::Snapshot auth=account_.snapshot();
    if((int)auth.state==account_previous_)return;
    account_previous_=(int)auth.state;
    switch(auth.state){
    case XboxAccount::IDLE:break;
    case XboxAccount::STARTING:
        log("4. MICROSOFT HTTPS HANDSHAKE / DEVICE CODE REQUEST");
        break;
    case XboxAccount::WAITING_FOR_USER:
        log_step(5,60,"REAL DEVICE CODE RECEIVED - APPROVE ON PHONE");
        log("CODE VALID UNTIL MICROSOFT SERVER EXPIRES IT");
        break;
    case XboxAccount::XBOX_EXCHANGING:
        log_step(7,71,"AUTHORIZED XBOX USER / XSTS HTTPS EXCHANGE");
        break;
    case XboxAccount::ACCOUNT_AUTHORIZED: {
        log_step(6,70,"MICROSOFT OAUTH APPROVED");
        xbox_live_auth::Status capabilities={0};
        capabilities.microsoft_oauth=true;
        capabilities.xbox_scope=auth.xbox_scope_verified;
        capabilities.xbox_user_token=auth.xbox_user_token_verified;
        capabilities.xsts_token=auth.xsts_token_verified;
        if(auth.xbox_user_token_verified)
            log_step(7,75,"VERIFIED XBOX USER TOKEN RECEIVED (CLEARED)");
        if(auth.xsts_token_verified)
            log_step(8,80,"VERIFIED XSTS TOKEN RECEIVED (CLEARED)");
        if(auth.cloud_login_verified)
            log("XCLOUD SERVICE LOGIN CONFIRMED; NO MEDIA RUNTIME");
        log("NEXT BLOCKER:");
        log(xbox_live_auth::gate_text(
            xbox_live_auth::first_blocker(capabilities)));
        log("NO XCLOUD STREAM / WEBRTC / DECODER IMPLEMENTED");
        break;
    }
    case XboxAccount::LOGIN_FAILED:
    case XboxAccount::NOT_CONFIGURED: {
        char status[LINE_CAP];
        sprintf_s(status,sizeof(status),"MSA FAILED: %.54s TLS=%d HTTP=%d",
                  auth.stage,auth.tls_error,auth.http_status);
        log(status);
        log("PRESS A TO RETRY. DO NOT ENTER PASSWORD IN THIS APP.");
        break;
    }
    case XboxAccount::USER_CANCELLED:
        log("MICROSOFT LOGIN CANCELLED");
        break;
    default:break;
    }
}

void App::draw_home(){
    gameui::hero(64,150,684,450);
    gameui::text(804,206,1.10f,gameui::ACCENT_HI,"XBOX CLOUD GAMING");
    gameui::text(802,257,2.25f,gameui::TEXT,"Connect account");
    gameui::text(804,324,1.22f,gameui::MUTED,"Sign in with your Microsoft account.");
    gameui::pill(804,405,356,60,gameui::ACCENT);
    gameui::text(826,423,1.36f,0xFF062112u,"A  SIGN IN");
    const XboxAccount::Snapshot auth=account_.snapshot();
    const bool cache_error=auth.cache_status!=login_cache::OK &&
        auth.cache_status!=login_cache::MISSING;
    const char* saved=cache_error ? "Could not save/restore sign-in." :
        (auth.cache_resumed ? "Sign-in restored." :
        (auth.cache_available ? "Sign-in saved on this console." : "Remember sign-in automatically."));
    gameui::text(804,485,0.92f,gameui::MUTED,saved);
    gameui::text(804,527,1.10f,gameui::SUBTLE,"Playback is not available yet.");
}
void App::draw_login(){
    const XboxAccount::Snapshot auth=account_.snapshot();
    gameui::box(200,150,880,456,gameui::PANEL,gameui::BORDER);
    gameui::text(244,181,1.10f,gameui::ACCENT_HI,"FORTNITE");
    if(auth.state==XboxAccount::WAITING_FOR_USER){
        gameui::text(244,228,2.08f,gameui::TEXT,"Sign in to your account");
        gameui::text(244,292,1.20f,gameui::MUTED,"On your phone or computer, visit:");
        gameui::text(244,330,1.35f,gameui::ACCENT_HI,auth.verification_url);
        gameui::pill(244,387,456,77,gameui::PANEL_DARK);
        gameui::text(264,400,3.05f,gameui::TEXT,auth.user_code);
        char timer[64];
        sprintf_s(timer,sizeof(timer),"Expires in %u:%02u",
                  auth.seconds_left/60,auth.seconds_left%60);
        gameui::text(244,510,1.16f,gameui::MUTED,timer);
        gameui::text(244,553,1.12f,gameui::ACCENT_HI,"Waiting for your approval...");
    } else if(auth.state==XboxAccount::ACCOUNT_AUTHORIZED){
        gameui::badge(244,234,212,"ACCOUNT CONNECTED",gameui::SUCCESS);
        if(auth.cloud_login_verified && !auth.session_test_complete) {
            gameui::text(244,305,2.08f,gameui::TEXT,"Connecting to Fortnite...");
            if(auth.session_wait_seconds) {
                char queue[64];
                sprintf_s(queue,sizeof(queue),"In queue. Estimate: %u min",
                    (auth.session_wait_seconds+59u)/60u);
                gameui::text(244,371,1.22f,gameui::MUTED,queue);
            } else {
                gameui::text(244,371,1.22f,gameui::MUTED,"Checking the game session.");
            }
            gameui::progress(244,491,792,progress_percent_);
        } else if(auth.cloud_login_verified && auth.session_test_complete) {
            const bool verified=auth.session_created && auth.session_closed &&
                !auth.session_error[0] && !auth.session_cleanup_failed;
            gameui::text(244,305,2.08f,gameui::TEXT,
                verified ? "Session verified" : "Unable to start");
            gameui::text(244,374,1.22f,gameui::MUTED,
                auth.session_cleanup_failed ? "Session termination was not confirmed." :
                (verified ? "Playback is not available yet." :
                 "The service did not complete the connection."));
            if(!verified && auth.session_error[0])
                gameui::text(244,431,1.10f,gameui::SUBTLE,auth.session_error);
        } else {
            gameui::text(244,305,2.08f,gameui::TEXT,"Xbox access pending");
            gameui::text(244,374,1.22f,gameui::MUTED,
                auth.xbox_scope_verified ? "The service did not authorize this connection." :
                "Account authorization for Xbox access is required.");
        }
    } else if(auth.state==XboxAccount::LOGIN_FAILED ||
              auth.state==XboxAccount::NOT_CONFIGURED){
        gameui::text(244,259,2.08f,gameui::TEXT,"Connection failed");
        gameui::text(244,329,1.22f,gameui::MUTED,login_error(auth));
        gameui::pill(244,433,340,60,gameui::ACCENT);
        gameui::text(266,451,1.30f,0xFF062112u,"A  RETRY");
    } else {
        gameui::text(244,259,2.08f,gameui::TEXT,
            auth.state==XboxAccount::XBOX_EXCHANGING ?
            "Connecting to Xbox..." :
            (auth.state==XboxAccount::USER_CANCELLED ?
             "Sign-in cancelled" : (auth.cache_available ? "Restoring your account..." : "Connecting to Microsoft...")));
        gameui::text(244,329,1.22f,gameui::MUTED,
            auth.state==XboxAccount::USER_CANCELLED ?
            "Press A to try again." : "Please wait a moment.");
        if(auth.state!=XboxAccount::USER_CANCELLED)
            gameui::progress(244,469,792,progress_percent_);
    }
}
void App::draw_network_diagnostics(){
    const XboxAccount::Snapshot auth=account_.snapshot();
    gameui::box(52,100,1176,504,gameui::PANEL,gameui::BORDER);
    gameui::text(80,122,1.25f,gameui::ACCENT_HI,"DIAGNOSTICS  AUTH-DIAG-A2 / SYSAPP");
    char row[180];
    const unsigned long ip_value=probe_.local_ip();
    const unsigned char* ip=(const unsigned char*)&ip_value;
    sprintf_s(row,sizeof(row),"IP %u.%u.%u.%u   LINK %08lX   XNADDR %08lX",
        ip[0],ip[1],ip[2],ip[3],probe_.link_flags(),probe_.address_flags());
    gameui::text(80,171,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"%s   wait %lu ms   test %d / stage %d",
        network_ready::text((network_ready::State)probe_.address_state()),
        probe_.address_wait_ms(),(int)probe_.state(),probe_.stage());
    gameui::text(80,213,0.95f,gameui::MUTED,row);
    sprintf_s(row,sizeof(row),"DNS XNet %d IPs   fallback %d   path %d   error %lu",
        probe_.dns_addresses(),probe_.udp_dns_addresses(),probe_.dns_path(),probe_.dns_error());
    gameui::text(80,255,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"Xbox:443 result %d   error %lu   source %d",
        probe_.xbox_result(),probe_.xbox_error(),probe_.xbox_error_source());
    gameui::text(80,297,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"1.1.1.1:443 result %d   error %lu   source %d",
        probe_.control_result(),probe_.control_error(),probe_.control_error_source());
    gameui::text(80,339,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"HTTP:80 result %d   error %lu   source %d",
        probe_.control80_result(),probe_.control80_error(),probe_.control80_error_source());
    gameui::text(80,381,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"HTTPS %.44s   WSA %d   TLS %d   HTTP %d",
        auth.stage,auth.wsa_error,auth.tls_error,auth.http_status);
    gameui::text(80,423,0.95f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"CACHE %d saved %d restored %d | Xbox scope %d",
        auth.cache_status,auth.cache_available?1:0,auth.cache_resumed?1:0,auth.xbox_scope_verified?1:0);
    gameui::text(80,460,0.88f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"XBOX HTTP %d OK %d | XSTS HTTP %d OK %d | XERR %u",
        auth.xbox_http_status,auth.xbox_user_token_verified?1:0,
        auth.xsts_http_status,auth.xsts_token_verified?1:0,auth.xbox_xerr);
    gameui::text(80,491,0.88f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"CLOUD HTTP %d OK %d | SESSION stage %u HTTP %d cleanup %d",
        auth.cloud_http_status,auth.cloud_login_verified?1:0,
        auth.session_stage,auth.session_http_status,auth.session_cleanup_http);
    gameui::text(80,522,0.88f,gameui::TEXT,row);
    sprintf_s(row,sizeof(row),"Session error: %.63s | Y: network test on Home",
        auth.session_error[0]?auth.session_error:"none");
    gameui::text(80,553,0.88f,gameui::MUTED,row);
}
void App::draw(){
    platform::clear(6,11,8);
    gameui::backdrop();
    gameui::header("","");
    if(diagnostics_visible_)draw_network_diagnostics();
    else if(screen_==HOME)draw_home();
    else draw_login();
    gameui::footer(diagnostics_visible_ ? "X/B  BACK       Y  TEST FROM HOME" :
        (screen_==HOME ? "A  SIGN IN       X  NETWORK       START  CLEAR SIGN-IN       B  EXIT" :
                        "A  RETRY        X  NETWORK       B  BACK"));
    platform::present();
}
int App::run(){
    while(running_ && platform::pump()){
        const platform::InputState input=platform::read_input();
        if(input.x_pressed)diagnostics_visible_=!diagnostics_visible_;
        const bool diagnostics_input=diagnostics_visible_ || input.x_pressed;
        if(diagnostics_visible_){
            if(input.b_pressed || input.back_pressed)diagnostics_visible_=false;
            const XboxAccount::Snapshot status=account_.snapshot();
            const bool busy=status.state==XboxAccount::STARTING ||
                status.state==XboxAccount::WAITING_FOR_USER ||
                status.state==XboxAccount::XBOX_EXCHANGING ||
                (status.cloud_login_verified && !status.session_test_complete);
            if(input.y_pressed && screen_==HOME && !busy && !flow_started_ &&
               probe_.state()!=TcpProbe::RUNNING) {
                network_previous_=-1;
                if(!probe_.start())log("NETWORK DIAGNOSTIC COULD NOT START");
            }
        } else if(!diagnostics_input && (input.back_pressed || input.b_pressed) && screen_==HOME){
            account_.cancel();
            running_=false;
            break;
        }
        if(!diagnostics_input && (input.b_pressed || input.back_pressed) && screen_==LOGIN){
            account_.cancel();
            flow_started_=false;
            login_launched_=false;
            screen_=HOME;
        }
        if(!diagnostics_input && screen_==HOME && input.start_pressed) {
            account_.forget_saved_login();
            flow_started_=false;login_launched_=false;
            log("SAVED LOGIN REMOVAL REQUESTED");
        } else if(!diagnostics_input && input.a_pressed)start_fortnite();
        poll_network();
        poll_account();
        draw();
        Sleep(16);
    }
    return 0;
}
void App::shutdown(){
    running_=false;
    account_.cancel();
    platform::shutdown();
}
