#include "xboxtls_backend.h"
#include "tls_trust.h"
#include "http_wire.h"
#include "xcloud_session.h"
#include "xboxtls/xbox_client.h"
#include "xboxtls/native_verifier.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

namespace {
const size_t WIRE_CAPACITY=secure_http::RESPONSE_CAP + http_wire::MAX_HEADERS;
void clear_bytes(void* p,size_t n) {secure_http::secure_zero(p,n);}

}
XboxTlsBackend::XboxTlsBackend() {
    memset(&diagnostic_,0,sizeof(diagnostic_));
    strcpy_s(diagnostic_.stage,sizeof(diagnostic_.stage),"NOT STARTED");
}
void XboxTlsBackend::record(const char* stage,int error) {
    strcpy_s(diagnostic_.stage,sizeof(diagnostic_.stage),stage);
    diagnostic_.tls_error=error;
    xboxtls::XboxNetworkDiagnostic network;
    xboxtls::xbox360_network_diagnostic(&network);
    diagnostic_.wsa_error=network.error;
    diagnostic_.dns_addresses=network.addresses;
    diagnostic_.link_flags=network.link_flags;
    diagnostic_.address_flags=network.xnaddr_flags;
    if(error!=0 && network.stage && network.error!=0)
        strcpy_s(diagnostic_.stage,sizeof(diagnostic_.stage),
                 network.stage);
}
void XboxTlsBackend::get_diagnostic(Diagnostic* out) const {
    if(out)*out=diagnostic_;
}
bool XboxTlsBackend::post_form(void* data,const char* host,
                               const char* path,const char* form,
                               size_t count,secure_http::Response* response) {
    if(!data)return false;
    return static_cast<XboxTlsBackend*>(data)->execute(
        host,path,form,count,response,false);
}
bool XboxTlsBackend::post_xbox_json(void* data,const char* host,
                                  const char* path,const char* json,
                                  size_t bytes,secure_http::Response* out) {
    if(!data)return false;
    return static_cast<XboxTlsBackend*>(data)->execute(
        host,path,json,bytes,out,true);
}
bool XboxTlsBackend::execute(const char* host,const char* path,
                             const char* form,size_t form_bytes,
                             secure_http::Response* response,bool xbox_json,
                             const char* method,const char* bearer) {
    if(!host||!path||!method||!response||(!form && form_bytes)||
       form_bytes>32768 || (!bearer && (!form || !form_bytes)))return false;
    // No arbitrary DNS, hostname, paths or HTTP downgrade can be injected.
    // OAuth and Xbox calls are independently restricted.
    if(bearer) {
        if(!xcloud_session::validate_request(method,host,path))return false;
    } else if(xbox_json) {
        const bool user=strcmp(host,"user.auth.xboxlive.com")==0 &&
                        strcmp(path,"/user/authenticate")==0;
        const bool xsts=strcmp(host,"xsts.auth.xboxlive.com")==0 &&
                        strcmp(path,"/xsts/authorize")==0;
        const bool cloud=strcmp(host,"xgpuweb.gssv-play-prod.xboxlive.com")==0 &&
                         strcmp(path,"/v2/login/user")==0;
        if(!user && !xsts && !cloud)return false;
    } else if(!(strcmp(host,"login.live.com")==0 &&
                strcmp(path,"/oauth20_token.srf")==0) &&
             (strcmp(host,"login.microsoftonline.com")!=0 ||
             (strcmp(path,"/consumers/oauth2/v2.0/devicecode")!=0 &&
              strcmp(path,"/consumers/oauth2/v2.0/token")!=0)))return false;
    memset(response,0,sizeof(*response));
    record("TLS INIT",0);
    xboxtls::XboxTlsClient client;
    // Verification uses embedded trust anchors + the SNI/hostname sent to
    // the library. A failed TLS handshake never produces a verified response.
    xboxtls::Error error=client.initialize(
        g_microsoft_roots,g_microsoft_roots_size);
    if(error!=xboxtls::XT_OK) {
        record("TLS INIT",(int)error);
        return false;
    }
    xboxtls::XboxTlsOptions options;
    options.alpn="http/1.1";
    options.require_alpn=false;
    options.send_timeout_ms=9000;
    options.receive_timeout_ms=9000;
    record("TLS HANDSHAKE",0);
    error=client.connect(host,options);
    if(error!=xboxtls::XT_OK) {
        if(client.last_alert_description()!=0) {
            char stage[48];
            sprintf_s(stage,sizeof(stage),"TLS ALERT L%u D%u",
                      (unsigned)client.last_alert_level(),
                      (unsigned)client.last_alert_description());
            record(stage,(int)error);
        } else if(error==xboxtls::XT_ERR_VERIFY) {
            char stage[48];
            sprintf_s(stage,sizeof(stage),"TLS CERT CHECK %d",
                      xboxtls::native_auth_verify_step());
            record(stage,(int)error);
        } else {
            record("TLS HANDSHAKE",(int)error);
        }
        return false;
    }
    // The client library is responsible for verifying the certificate
    // chain and hostname during connect(); it must not silently skip them.
    const size_t request_capacity=form_bytes+(bearer?10240:2048);
    char* request=(char*)malloc(request_capacity);
    if(!request) {record("ALLOC REQUEST",-2);return false;}
    size_t request_bytes=0;
    const bool cloud_login=xbox_json &&
        strcmp(host,"xgpuweb.gssv-play-prod.xboxlive.com")==0;
    const bool format=bearer ?
        http_wire::make_cloud_session_request(method,host,path,bearer,
            form,form_bytes,request,request_capacity,&request_bytes) :
        cloud_login ?
        http_wire::make_cloud_login_request(form,form_bytes,
            request,form_bytes+2048,&request_bytes) :
        xbox_json ?
        http_wire::make_xbox_json_request(
            host,path,form,form_bytes,request,form_bytes+2048,
            &request_bytes) :
        http_wire::make_request(
            "POST",host,path,"application/x-www-form-urlencoded",
            form,form_bytes,request,form_bytes+2048,&request_bytes);
    if(!format) {
        clear_bytes(request,request_capacity);free(request);
        record("FORMAT REQUEST",-1);return false;
    }
    record("HTTPS SEND",0);
    error=client.send(request,request_bytes);
    clear_bytes(request,request_capacity);free(request);
    if(error!=xboxtls::XT_OK) {
        record("HTTPS SEND",(int)error);
        return false;
    }
    char* bytes=(char*)malloc(WIRE_CAPACITY+1);
    if(!bytes) {record("ALLOC RESPONSE",-2);return false;}
    size_t received_total=0;
    bool complete=false;
    record("HTTPS RECEIVE",0);
    while(received_total<WIRE_CAPACITY) {
        size_t received=0;
        error=client.receive(bytes+received_total,
                             WIRE_CAPACITY-received_total,&received);
        if(error==xboxtls::XT_ERR_CLOSED) {complete=true;break;}
        if(error!=xboxtls::XT_OK)break;
        if(received==0) {complete=true;break;}
        received_total+=received;
        if(http_wire::response_complete(bytes,received_total)) {complete=true;break;}
    }
    client.close();
    bytes[received_total]=0;
    if(!complete) {
        clear_bytes(bytes,WIRE_CAPACITY+1);free(bytes);
        record("HTTPS RECEIVE",(int)(error==xboxtls::XT_OK ?
               xboxtls::XT_ERR_BUFFER_TOO_SMALL:error));
        return false;
    }
    http_wire::Response parsed;
    if(!http_wire::parse_response(bytes,received_total,&parsed)) {
        clear_bytes(bytes,WIRE_CAPACITY+1);free(bytes);
        record("HTTP HEADER",-3);return false;
    }
    const size_t body_bytes=received_total-parsed.body_offset;
    size_t final_bytes=body_bytes;
    bool okay=true;
    if(parsed.chunked) {
        okay=http_wire::decode_chunked(bytes+parsed.body_offset,body_bytes,
                           response->body,secure_http::RESPONSE_CAP-1,
                           &final_bytes);
    } else if(parsed.has_length) {
        okay=parsed.content_length==body_bytes;
        final_bytes=parsed.content_length;
        if(okay && final_bytes<secure_http::RESPONSE_CAP)
            memcpy(response->body,bytes+parsed.body_offset,final_bytes);
    } else if(final_bytes<secure_http::RESPONSE_CAP) {
        memcpy(response->body,bytes+parsed.body_offset,final_bytes);
    }
    if(!okay || final_bytes>=secure_http::RESPONSE_CAP) {
        clear_bytes(bytes,WIRE_CAPACITY+1);free(bytes);
        clear_bytes(response,sizeof(*response));
        record("HTTP BODY",-3);return false;
    }
    response->body[final_bytes]=0;
    response->length=final_bytes;
    response->http_status=parsed.status;
    response->peer_certificate_verified=true;
    response->hostname_verified=true;
    clear_bytes(bytes,WIRE_CAPACITY+1);
    free(bytes);
    record("HTTPS COMPLETE",0);
    return true;
}
bool XboxTlsBackend::cloud_request(void* context,const char* method,
                                  const char* host,const char* path,
                                  const char* bearer,const char* json,size_t bytes,
                                  secure_http::Response* result) {
    if(!context || !bearer || !*bearer)return false;
    return static_cast<XboxTlsBackend*>(context)->execute(
        host,path,json,bytes,result,true,method,bearer);
}
