#include "app.h"
#include "platform.h"
#include "protocol_selftest.h"
#include "region_catalog.h"
#include "xbox_live_auth.h"
#include "harissa_style_ui.h"
#include "../auth_config.h"
#include <xtl.h>
#include <stdio.h>
#include <string.h>

namespace {
const unsigned int BG=0xFF060C0A;
const unsigned int PANEL=0xFF101B15;
const unsigned int BORDER=0xFF24422F;
const unsigned int BRIGHT=0xFF6AE99B;
const unsigned int WHITE=0xFFE2F4E6;
const unsigned int DIM=0xFF9AB2A1;
const unsigned int WARN=0xFFFFC679;
const unsigned int BAD=0xFFFF8888;
const char* const FORTNITE_URL=
    "https://play.xbox.com/products/BT5P2X999VH2/fortnite";
const char* const LEGACY_LINK=
    "https://login.live.com/oauth20_remoteconnect.srf";
void line(float x,float y,float scale,unsigned int color,const char* value){
    platform::draw_text(x,y,scale,color,value);
}
void rule(float x,float y,float width) {
    platform::draw_rect(x,y,width,2,BORDER);
}
}

App::App()
    : screen_(HOME),running_(false),tests_ok_(false),flow_started_(false),
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
       account.state==XboxAccount::XBOX_EXCHANGING){
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
const char* App::network_status() const {
    if(probe_.state()==TcpProbe::RUNNING)return "NETWORK DIAGNOSTIC RUNNING";
    if(probe_.state()==TcpProbe::CONNECTED)
        return probe_.control80_result()==1?
            "HTTP/80 RESPONSE VERIFIED; HTTPS SEPARATE":
            "TCP CHECK ONLY; HTTPS NOT VERIFIED";
    if(probe_.state()==TcpProbe::FAILED)return "NETWORK DIAGNOSTIC FAILED";
    return "NETWORK NOT CHECKED";
}
const char* App::activity_status(){
    const XboxAccount::Snapshot auth=account_.snapshot();
    switch(auth.state){
    case XboxAccount::WAITING_FOR_USER:return "ENTER THE CODE ON ANOTHER DEVICE";
    case XboxAccount::XBOX_EXCHANGING:return "VERIFYING XBOX USER / XSTS OVER HTTPS";
    case XboxAccount::STARTING:return "REQUESTING THE MICROSOFT CODE";
    case XboxAccount::ACCOUNT_AUTHORIZED:return "MICROSOFT LOGIN OK / XCLOUD PENDING";
    case XboxAccount::LOGIN_FAILED:return "LOGIN FAILED: SEE THE ERROR BELOW";
    case XboxAccount::NOT_CONFIGURED:return "REGISTER AN OWNED PUBLIC APP ID";
    case XboxAccount::USER_CANCELLED:return "LOGIN CANCELLED";
    default:return flow_started_?"FORTNITE SETUP IN PROGRESS":"READY";
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
    gameui::hero(64,150,700,450);
    gameui::text(818,218,1.85f,gameui::TEXT,"Conectar conta");
    gameui::text(820,260,1.04f,gameui::MUTED,"Xbox Cloud Gaming");
    gameui::badge(820,314,180,"EXPERIMENTAL",gameui::ACCENT_HI);
    gameui::text(820,380,0.98f,gameui::MUTED,"Streaming indisponivel.");
    gameui::pill(820,454,352,60,gameui::ACCENT);
    gameui::text(844,471,1.24f,0xFF04220Cu,"A  ENTRAR");
}
void App::draw_login(){
    const XboxAccount::Snapshot auth=account_.snapshot();
    gameui::box(225,152,830,452,gameui::PANEL,gameui::BORDER);
    if(auth.state==XboxAccount::WAITING_FOR_USER){
        gameui::text(270,185,1.65f,gameui::TEXT,"Conectar conta");
        gameui::text(270,239,1.00f,gameui::MUTED,"No celular, acesse:");
        gameui::text(270,281,1.16f,gameui::ACCENT_HI,auth.verification_url);
        gameui::text(270,354,3.30f,gameui::TEXT,auth.user_code);
        char timer[64];
        sprintf_s(timer,sizeof(timer),"Expira em %u:%02u",
                  auth.seconds_left/60,auth.seconds_left%60);
        gameui::text(270,462,1.00f,gameui::MUTED,timer);
        gameui::text(270,535,0.95f,gameui::ACCENT_HI,"Aguardando...");
    } else if(auth.state==XboxAccount::ACCOUNT_AUTHORIZED){
        gameui::text(270,203,1.85f,gameui::ACCENT_HI,"Conta conectada");
        gameui::text(270,288,1.12f,gameui::TEXT,"Streaming ainda indisponivel.");
            const char* status = !auth.xbox_scope_verified ? "Acesso Xbox pendente" :
            (!auth.xbox_user_token_verified ? "Conta Xbox pendente" :
            (!auth.xsts_token_verified ? "Autorizacao Xbox pendente" :
             (auth.cloud_login_verified ? "Login xCloud confirmado" : "Login xCloud pendente")));
        gameui::badge(270,422,300,status,gameui::MUTED);
        // Display only non-secret failure diagnostics. OAuth approval alone
        // must not hide a failed Xbox service exchange behind "connected".
        if(auth.xbox_scope_verified && !auth.cloud_login_verified) {
            gameui::text(270,476,0.92f,gameui::FAILURE,auth.stage);
            char result[112];
            sprintf_s(result,sizeof(result),"Xbox %d / XSTS %d / Cloud %d / XErr %u",
                auth.xbox_http_status,auth.xsts_http_status,auth.cloud_http_status,auth.xbox_xerr);
            gameui::text(270,514,0.90f,gameui::MUTED,result);
        }

    } else if(auth.state==XboxAccount::LOGIN_FAILED ||
              auth.state==XboxAccount::NOT_CONFIGURED){
        gameui::text(270,203,1.75f,gameui::TEXT,"Falha ao conectar");
        char error[152];
        sprintf_s(error,sizeof(error),"%.67s  TLS=%d HTTP=%d",
                  auth.stage,auth.tls_error,auth.http_status);
        gameui::text(270,296,0.92f,gameui::FAILURE,error);
        gameui::pill(270,426,350,58,gameui::ACCENT);
        gameui::text(294,443,1.18f,0xFF04220Cu,"A  TENTAR");
    } else {
        gameui::text(270,213,1.80f,gameui::TEXT,
            auth.state==XboxAccount::XBOX_EXCHANGING ?
            "Conectando ao Xbox..." :
            (auth.state==XboxAccount::USER_CANCELLED ?
             "Login cancelado" : "Conectando a Microsoft..."));
        gameui::text(270,304,1.04f,gameui::MUTED,
            auth.state==XboxAccount::USER_CANCELLED ?
            "A para tentar novamente." : "Aguarde um instante.");
        gameui::progress(270,438,740,progress_percent_);
    }
}
void App::draw(){
    platform::clear(5,16,7);
    gameui::backdrop();
    gameui::header("","");
    if(screen_==HOME)draw_home();
    else draw_login();
    const XboxAccount::Snapshot account=account_.snapshot();
    const bool retry=account.state==XboxAccount::LOGIN_FAILED ||
        account.state==XboxAccount::NOT_CONFIGURED ||
        account.state==XboxAccount::USER_CANCELLED ||
        account.state==XboxAccount::ACCOUNT_AUTHORIZED;
    gameui::footer(screen_==HOME ? "BACK  SAIR" :
        (retry ? "A  TENTAR       B  VOLTAR       BACK  SAIR" :
                 "B  VOLTAR       BACK  SAIR"));
    platform::present();
}
int App::run(){
    while(running_ && platform::pump()){
        const platform::InputState input=platform::read_input();
        if(input.back_pressed){
            account_.cancel();
            running_=false;
            break;
        }
        if(input.b_pressed && screen_==LOGIN){
            account_.cancel();
            flow_started_=false;
            login_launched_=false;
            screen_=HOME;
        }
        if(input.y_pressed)run_offline_tests();
        if(input.x_pressed && screen_==LOGIN &&
           probe_.state()!=TcpProbe::RUNNING){
            network_previous_=-1;
            if(!probe_.start())log("NETWORK WORKER START FAILED");
        }
        if(input.a_pressed)start_fortnite();
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
