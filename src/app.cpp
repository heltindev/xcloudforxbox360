#include "app.h"
#include "platform.h"
#include "protocol_selftest.h"
#include "controller_input.h"
#include <xtl.h>
#include <stdio.h>

namespace {
const unsigned int WHITE = 0xFFF6F9FC;
const unsigned int MUTED = 0xFFB8C7D8;
const unsigned int GREEN = 0xFF107C10;
const unsigned int LIME = 0xFF6CDB78;
const unsigned int AMBER = 0xFFFFC06E;
const unsigned int RED = 0xFFFF7C7C;
const unsigned int PANEL = 0xFF172332;
const unsigned int TILE = 0xFF213346;

void text(float x, float y, float size, unsigned int color, const char* str) {
    platform::draw_text(x, y, size, color, str);
}
void tile(float x,float y,float w,float h,unsigned int color) {
    platform::draw_rect(x,y,w,h,color);
}
void bar(float x,float y,float w,unsigned int accent) {
    tile(x,y,w,6,accent);
}
void line(float y) {
    tile(54,y,1172,2,0xFF344653);
}
void footer(const char* message) {
    text(70,651,1.11f,MUTED,message);
}
const char* status_name(int state) {
    if (state == 1) return "CONECTOU";
    if (state == 2) return "FALHOU";
    return "PENDENTE";
}
}

App::App():running_(false),protocol_ok_(false),screen_(SCREEN_HOME) {}

bool App::initialize() {
    if(!platform::initialize()) return false;
    running_=true;
    screen_=SCREEN_HOME;
    protocol_ok_=protocol_selftest();
    if(!protocol_ok_)
        platform::debug_log("[XCloud360] Offline parsers FAIL.\n");
    else
        platform::debug_log("[XCloud360] Offline parsers OK.\n");
    platform::debug_log("[XCloud360] RGH native UI ready. No automatic network or login.\n");
    return true;
}

int App::run() {
    while(running_ && platform::pump()) {
        const platform::InputState in=platform::read_input();
        probe_.update();

        if(in.back_pressed) running_=false;
        if(in.b_pressed) screen_=SCREEN_HOME;
        // X is the only button that can start a network diagnostic.
        if(in.x_pressed) {
            screen_=SCREEN_NETWORK;
            probe_.start();
        }
        if(in.y_pressed) screen_=SCREEN_FORTNITE;
        if(in.a_pressed) {
            if(screen_==SCREEN_HOME) screen_=SCREEN_ROADMAP;
            else if(screen_==SCREEN_NETWORK) probe_.start();
            else if(screen_==SCREEN_FORTNITE) screen_=SCREEN_ROADMAP;
            else screen_=SCREEN_HOME;
        }

        render();
        Sleep(16);
    }
    return 0;
}
void App::render() {
    platform::clear(10,15,23);
    tile(0,0,1280,105,0xFF101A28);
    tile(44,30,43,43,GREEN);
    text(110,23,2.38f,WHITE,"XCLOUD360");
    text(890,38,1.13f,MUTED,"XBOX 360  POWERPC  RGH3");
    line(130);
    if(screen_==SCREEN_HOME) render_home();
    else if(screen_==SCREEN_ROADMAP) render_roadmap();
    else if(screen_==SCREEN_FORTNITE) render_fortnite();
    else render_network();
    platform::present();
}

void App::render_home() {
    text(64,159,2.37f,WHITE,"XBOX CLOUD GAMING");
    text(64,213,1.25f,MUTED,"Projeto experimental nativo para o Xbox 360.");
    tile(64,273,736,298,PANEL);
    bar(64,273,736,GREEN);
    text(100,311,1.70f,WHITE,"STATUS DO PROJETO");
    text(100,356,1.18f,LIME,"POWERPC / INTERFACE: IMPLEMENTADOS");
    text(100,391,1.18f,AMBER,"REDE EXTERNA: TIMEOUT 10060 NO RGH");
    text(100,426,1.18f,MUTED,"TLS, OAUTH E STREAMING: PENDENTES");
    tile(99,479,653,61,GREEN);
    text(131,489,1.52f,WHITE,"A  VER ETAPAS QUE FALTAM");

    tile(817,273,400,298,TILE);
    bar(817,273,400,0xFF5A7694);
    text(846,312,1.53f,WHITE,"FORTNITE");
    text(846,360,1.13f,AMBER,"NAO JOGAVEL NESTA VERSAO");
    text(846,412,1.13f,MUTED,"Y  DETALHES DO JOGO");
    text(846,461,1.13f,MUTED,"X  TESTE DE REDE OPCIONAL");
    text(846,510,1.10f,protocol_ok_?LIME:RED,
         protocol_ok_?"PARSERS OFFLINE: OK":"PARSERS OFFLINE: ERRO");
    footer("A ETAPAS     Y FORTNITE     X REDE     BACK SAIR");
}

void App::render_roadmap() {
    text(63,169,2.15f,WHITE,"O QUE FALTA PARA FUNCIONAR");
    text(63,218,1.15f,MUTED,"Ordem de dependencias - nenhum recurso e declarado pronto sem teste.");
    tile(61,274,1157,321,PANEL);
    bar(61,274,1157,GREEN);
    const char* rows[] = {
        "01  Rede TCP/UDP no RGH                    BLOQUEADA 10060",
        "02  HTTPS/TLS + certificados             NAO IMPLEMENTADO",
        "03  Conta Microsoft / OAuth autorizado   NAO IMPLEMENTADO",
        "04  Sessao Fortnite nos servidores       NAO IMPLEMENTADO",
        "05  WebRTC e recebimento de midia         NAO IMPLEMENTADO",
        "06  Decoder H264 / audio + controles     NAO IMPLEMENTADO"
    };
    for(int i=0;i<6;++i) {
        text(92,301+i*47,1.22f,i==0?AMBER:MUTED,rows[i]);
    }
    footer("A INICIO     Y FORTNITE     X REDE     B VOLTAR     BACK SAIR");
}

void App::render_fortnite() {
    text(63,169,2.36f,WHITE,"FORTNITE");
    text(63,223,1.23f,AMBER,"JOGO INDISPONIVEL: AINDA NAO EXISTE SESSAO XCLOUD");
    tile(63,291,1155,288,PANEL);
    bar(63,291,1155,0xFF456789);
    text(98,321,1.33f,WHITE,"POR QUE AINDA NAO ABRE?");
    text(98,371,1.14f,MUTED,"O servidor precisa autenticar a conta e aceitar uma sessao.");
    text(98,413,1.14f,MUTED,"O console precisa negociar streaming, receber video e audio.");
    text(98,455,1.14f,MUTED,"Os comandos do controle precisam chegar ao jogo remoto.");
    text(98,513,1.18f,AMBER,"NENHUM JOGO FOI INICIADO. NENHUMA SENHA FOI ENVIADA.");
    footer("A VER ETAPAS     X TESTE DE REDE     B VOLTAR     BACK SAIR");
}

void App::render_network() {
    char buf[180];
    const TcpProbe::State state=probe_.state();
    text(64,154,2.14f,WHITE,"DIAGNOSTICO DE REDE (OPCIONAL)");
    if(state==TcpProbe::RUNNING)
        text(64,214,1.22f,AMBER,"Consultando DNS e testando TCP...");
    else if(state==TcpProbe::CONNECTED)
        text(64,214,1.22f,LIME,"TCP respondeu. HTTPS e xCloud ainda nao foram testados.");
    else if(state==TcpProbe::FAILED)
        text(64,214,1.22f,AMBER,"Nenhuma conexao externa foi confirmada.");
    else
        text(64,214,1.22f,MUTED,"Pressione A para iniciar.");

    tile(62,269,560,318,PANEL);
    tile(644,269,573,318,TILE);
    bar(62,269,560,GREEN);
    bar(644,269,573,0xFF58789B);
    text(87,292,1.42f,WHITE,"XBOX.COM :443");
    text(668,292,1.42f,WHITE,"1.1.1.1  TCP 443 E TCP 80");
    sprintf_s(buf,sizeof(buf),"DNS Xbox: %d IP(s)",probe_.dns_addresses());
    text(86,335,1.16f,MUTED,buf);

    if(probe_.udp_dns_state()==1) {
        sprintf_s(buf,sizeof(buf),"DNS UDP: %d IP(s)",probe_.udp_dns_addresses());
        text(86,371,1.08f,LIME,buf);
    } else if(probe_.udp_dns_state()==2) {
        sprintf_s(buf,sizeof(buf),"DNS UDP erro %lu (fase %d)",
                  probe_.udp_dns_error(),probe_.udp_dns_error_source());
        text(86,371,1.08f,AMBER,buf);
    } else text(86,371,1.08f,MUTED,"DNS UDP: nao solicitado");

    text(86,412,1.34f,probe_.xbox_result()==1?LIME:AMBER,
         status_name(probe_.xbox_result()));
    if(probe_.xbox_result()==2) {
        sprintf_s(buf,sizeof(buf),"ERRO %lu / ORIGEM %d",
                  probe_.xbox_error(),probe_.xbox_error_source());
        text(86,461,1.10f,MUTED,buf);
    }

    sprintf_s(buf,sizeof(buf),"TCP 443: %s",status_name(probe_.control_result()));
    text(669,344,1.26f,probe_.control_result()==1?LIME:AMBER,buf);
    if(probe_.control_result()==2) {
        sprintf_s(buf,sizeof(buf),"Erro %lu / origem %d",
                  probe_.control_error(),probe_.control_error_source());
        text(669,384,1.07f,MUTED,buf);
    }
    sprintf_s(buf,sizeof(buf),"TCP 80: %s",status_name(probe_.control80_result()));
    text(669,445,1.26f,probe_.control80_result()==1?LIME:AMBER,buf);
    if(probe_.control80_result()==2) {
        sprintf_s(buf,sizeof(buf),"Erro %lu / origem %d",
                  probe_.control80_error(),probe_.control80_error_source());
        text(669,486,1.07f,MUTED,buf);
    }
    sprintf_s(buf,sizeof(buf),"ETAPA %d   /   ORIGEM %d",
              probe_.stage(),probe_.error_source());
    text(70,607,1.08f,MUTED,buf);
    footer("A REPETIR     Y FORTNITE     B INICIO     BACK SAIR");
}

void App::shutdown() {
    running_=false;
    platform::shutdown();
}
