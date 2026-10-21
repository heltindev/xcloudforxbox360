#include "region_catalog.h"
#include "cJSON.h"
#include <string.h>
#include <ctype.h>

namespace region_catalog {
namespace {
bool copy(char* out,size_t cap,const char* s) {
    if(!out||!s || strlen(s)>=cap)return false;
    strcpy_s(out,cap,s);return true;
}
bool suffix(const char* host,const char* required) {
    const size_t n=strlen(host),k=strlen(required);
    return n>k && strcmp(host+n-k,required)==0 &&
           host[n-k-1]=='.';
}
bool value_string(const cJSON* object,const char* key,
                  char* out,size_t cap) {
    const cJSON* item=cJSON_GetObjectItemCaseSensitive(object,key);
    return cJSON_IsString(item) && item->valuestring &&
           copy(out,cap,item->valuestring);
}
}
bool extract_xbox_hostname(const char* url,char* out,size_t cap) {
    if(!url || !out||cap<4)return false;
    out[0]=0;
    if(strncmp(url,"https://",8)!=0)return false;
    const char* domain=url+8;
    size_t used=0;
    while(domain[used] && domain[used]!='/' && used<127) {
        unsigned char ch=(unsigned char)domain[used];
        if(!((ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') ||
             (ch>='0'&&ch<='9') || ch=='-'||ch=='.')) return false;
        if(used+1>=cap)return false;
        out[used]=(char)tolower(ch);
        ++used;
    }
    if(used==127 || used==0 || used>=cap)return false;
    out[used]=0;
    // No trailing-dot bypass, untrusted IP addresses or host lookalikes.
    if(!suffix(out,"xboxlive.com"))return false;
    for(size_t i=8+used;url[i];++i) {
        unsigned char ch=(unsigned char)url[i];
        if(ch<0x21 || ch>=0x7f || ch=='@' ||
           ch=='\\' || ch=='#')return false;
    }
    return true;
}
bool parse(const char* json,size_t bytes,Catalog* output) {
    if(!json || !output || bytes==0 || bytes>32768)return false;
    memset(output,0,sizeof(*output));
    const char* parsed_end=NULL;
    cJSON* root=cJSON_ParseWithLengthOpts(json,bytes,&parsed_end,0);
    if(!root)return false;
    // With an explicit byte length, reject any unexpected trailing bytes.
    // cJSON's require_null_terminated flag is not valid with an exact
    // byte count unless the caller includes a trailing NUL in that count.
    while(parsed_end && parsed_end<json+bytes &&
          (*parsed_end==' '||*parsed_end=='\n'||
           *parsed_end=='\r'||*parsed_end=='\t'))++parsed_end;
    if(parsed_end!=json+bytes) {cJSON_Delete(root);return false;}
    const cJSON* opts=cJSON_GetObjectItemCaseSensitive(root,"offeringSettings");
    const cJSON* regions=cJSON_GetObjectItemCaseSensitive(opts,"regions");
    bool okay=cJSON_IsObject(root) && cJSON_IsArray(regions);
    unsigned int preferred=0;
    int saw_default=0;
    if(okay) {
        const cJSON* item=NULL;
        cJSON_ArrayForEach(item,regions) {
            if(output->count==MAX_REGIONS)break;
            if(!cJSON_IsObject(item))continue;
            Region next;
            memset(&next,0,sizeof(next));
            if(!value_string(item,"name",next.name,sizeof(next.name))||
               !value_string(item,"baseUri",next.base_uri,sizeof(next.base_uri))||
               !extract_xbox_hostname(next.base_uri,next.host,
                                      sizeof(next.host)))continue;
            const cJSON* def=cJSON_GetObjectItemCaseSensitive(item,"isDefault");
            next.is_default=cJSON_IsTrue(def)!=0;
            if(next.is_default && !saw_default) {
                preferred=output->count;
                saw_default=1;
            }
            output->regions[output->count++]=next;
        }
    }
    output->default_index=preferred;
    okay=okay && output->count>0;
    if(!okay)memset(output,0,sizeof(*output));
    cJSON_Delete(root);
    return okay;
}
bool offline_selftest() {
    const char json[]=
        "{\"offeringSettings\":{\"regions\":["
        "{\"name\":\"WestEurope\","
        "\"baseUri\":\"https://weu.gssv-play-prod.xboxlive.com\","
        "\"isDefault\":true},"
        "{\"name\":\"Bad\","
        "\"baseUri\":\"https://evil.com@xboxlive.com.fake\"}]}}";
    static Catalog list; // avoid approx. 8 KiB on main thread stack
    if(!parse(json,sizeof(json)-1,&list) ||
       list.count!=1 || list.default_index!=0 ||
       strcmp(list.regions[0].name,"WestEurope")!=0 ||
       strcmp(list.regions[0].host,"weu.gssv-play-prod.xboxlive.com")!=0)
        return false;
    char host[128];
    if(extract_xbox_hostname("http://weu.xboxlive.com",host,sizeof(host)) ||
       extract_xbox_hostname("https://foo.xboxlive.com.fake",host,sizeof(host)) ||
       extract_xbox_hostname("https://user@foo.xboxlive.com",host,sizeof(host)))
        return false;
    return true;
}
}
