#include "app.h"
#include "platform.h"
#include <xtl.h>
#include <stdio.h>

namespace {
const unsigned int WHITE = 0xFFF7F9FD;
const unsigned int MUTED = 0xFFB6C4D4;
const unsigned int GREEN = 0xFF107C10;
const unsigned int LIME = 0xFF6CDB70;
const unsigned int AMBER = 0xFFFFC06E;
const unsigned int PANEL = 0xFF172332;
const unsigned int TILE = 0xFF213346;

const char* status_name(int state) {
    if (state == 1) return "CONECTOU";
    if (state == 2) return "FALHOU";
    return "PENDENTE";
}
}

App::App() : running_(false), screen_(SCREEN_HOME) {}
bool App::initialize() {
    if (!platform::initialize()) return false;
    running_ = true;
    screen_ = SCREEN_HOME;
    platform::debug_log("[XCloud360] v0.9 RGH3 UDP DNS and TCP diagnostics.\n");
    return true;
}
int App::run() {
    while (running_ && platform::pump()) {
        const platform::InputState in = platform::read_input();
        probe_.update();
        if (in.back_pressed) running_ = false;
        if (in.b_pressed) screen_ = SCREEN_HOME;
        if (in.a_pressed) {
            if (screen_ == SCREEN_HOME) screen_ = SCREEN_TCP_TEST;
            probe_.start();
        }
        render();
        Sleep(16);
    }
    return 0;
}
void App::render() {
    platform::clear(10,15,23);
    // Branded top bar.
    platform::draw_rect(0,0,1280,104,0xFF101A28);
    platform::draw_rect(43,30,43,43,GREEN);
    platform::draw_text(107,24,2.35f,WHITE,"XCLOUD360");
    platform::draw_text(1001,37,1.25f,MUTED,"POWERPC / RGH3  V0.9");
    platform::draw_rect(43,133,1195,2,0xFF354958);

    if (screen_ == SCREEN_HOME) {
        platform::draw_text(60,167,2.35f,WHITE,"XBOX CLOUD GAMING");
        platform::draw_text(60,226,1.35f,MUTED,"Xbox 360 RGH3: diagnostico de rede para futura integracao.");
        platform::draw_rect(60,291,770,226,PANEL);
        platform::draw_rect(60,291,7,226,GREEN);
        platform::draw_text(94,312,1.55f,LIME,"CONECTIVIDADE");
        platform::draw_text(94,363,1.43f,WHITE,"www.xbox.com : 443");
        platform::draw_text(94,409,1.2f,MUTED,"DNS Xbox + DNS UDP Google + TCP");
        platform::draw_rect(92,453,696,51,GREEN);
        platform::draw_text(116,459,1.63f,WHITE,"A   TESTAR REDE TCP / UDP");
        platform::draw_rect(850,291,370,226,TILE);
        platform::draw_text(875,318,1.42f,WHITE,"FORTNITE / XCLOUD");
        platform::draw_text(875,366,1.12f,MUTED,"Objetivo: streaming nativo");
        platform::draw_text(875,399,1.12f,MUTED,"Jogo: nao disponivel");
        platform::draw_text(875,454,1.0f,LIME,"Proxima etapa: TLS");
        platform::draw_text(60,553,1.1f,MUTED,"Desenvolvido para Xbox 360 RGH3 - PowerPC Xenon.");
    } else {
        char line[140];
        const TcpProbe::State state = probe_.state();
        platform::draw_text(60,156,2.15f,WHITE,"DIAGNOSTICO TCP");
        if (state == TcpProbe::RUNNING)
            platform::draw_text(61,215,1.27f,AMBER,"Testando DNS Xbox, UDP Google e TCP...");
        else if (state == TcpProbe::CONNECTED)
            platform::draw_text(61,215,1.27f,LIME,"TCP conectado: nao significa acesso ao xCloud.");
        else if (state == TcpProbe::FAILED)
            platform::draw_text(61,215,1.27f,AMBER,"As conexoes nao foram estabelecidas.");
        else
            platform::draw_text(61,215,1.27f,MUTED,"Aperte A para iniciar o teste.");

        platform::draw_rect(60,270,572,241,PANEL);
        platform::draw_rect(649,270,571,241,TILE);
        platform::draw_rect(60,270,572,7,GREEN);
        platform::draw_rect(649,270,571,7,0xFF47688D);
        platform::draw_text(85,298,1.49f,WHITE,"XBOX.COM  /  DNS + TCP");
        platform::draw_text(675,298,1.49f,WHITE,"1.1.1.1  /  IP DIRETO");
        sprintf_s(line,sizeof(line),"DNS Xbox: %d IP(s)",probe_.dns_addresses());
        platform::draw_text(85,339,1.11f,MUTED,line);
        if (probe_.udp_dns_state() == 1) {
            sprintf_s(line,sizeof(line),"DNS UDP Google: %d IP(s)",probe_.udp_dns_addresses());
            platform::draw_text(85,369,1.05f,LIME,line);
        } else if (probe_.udp_dns_state() == 2) {
            sprintf_s(line,sizeof(line),"DNS UDP erro: %lu (fase %d)",probe_.udp_dns_error(),probe_.udp_dns_error_source());
            platform::draw_text(85,369,1.05f,AMBER,line);
        } else {
            platform::draw_text(85,369,1.05f,MUTED,"DNS UDP: nao necessario / aguardando");
        }
        platform::draw_text(85,406,1.44f,probe_.xbox_result()==1?LIME:AMBER,
                            status_name(probe_.xbox_result()));
        platform::draw_text(675,385,1.47f,probe_.control_result()==1?LIME:AMBER,
                            status_name(probe_.control_result()));
        if (probe_.xbox_result()==2) {
            sprintf_s(line,sizeof(line),"ERRO XBOX: %lu",probe_.xbox_error());
            platform::draw_text(85,449,1.10f,MUTED,line);
            if (probe_.xbox_error_source() >= TcpProbe::DNS_UDP_SOCKET)
                sprintf_s(line,sizeof(line),"Origem DNS UDP: %d",probe_.xbox_error_source());
            else
                sprintf_s(line,sizeof(line),"Origem DNS Xbox: %d",probe_.xbox_error_source());
            platform::draw_text(85,473,1.00f,MUTED,line);
        }
        if (probe_.control_result()==2) {
            sprintf_s(line,sizeof(line),"ERRO IP: %lu",probe_.control_error());
            platform::draw_text(675,438,1.14f,MUTED,line);
            sprintf_s(line,sizeof(line),"Origem: %d",probe_.control_error_source());
            platform::draw_text(675,466,1.03f,MUTED,line);
        }
        sprintf_s(line,sizeof(line),"ETAPA: %d  /  ORIGEM: %d",probe_.stage(),probe_.error_source());
        platform::draw_text(62,544,1.2f,MUTED,line);
        platform::draw_text(62,583,1.04f,MUTED,"UDP/TCP nao e TLS. Fortnite e login ainda pendentes.");
        platform::draw_text(62,620,1.10f,LIME,"A  REPETIR     B  VOLTAR     BACK  SAIR");
    }
    platform::present();
}
void App::shutdown() { running_ = false; platform::shutdown(); }
