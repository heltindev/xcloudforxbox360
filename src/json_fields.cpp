#include "json_fields.h"
#include <string.h>

// Strict, flat-object JSON reader for OAuth Device Code response fields.
// Intentionally does not support nested JSON: those responses are rejected.
namespace json_fields {
namespace {
struct Cursor { const char* p; const char* end; };
enum Kind { KIND_STRING, KIND_UINT, KIND_LITERAL };
void space(Cursor& c) {
    while (c.p < c.end && (*c.p==' ' || *c.p=='\t' ||
                             *c.p=='\n' || *c.p=='\r')) ++c.p;
}
bool hex_digit(char ch, unsigned int* out) {
    if (ch>='0'&&ch<='9') {*out=(unsigned int)(ch-'0');return true;}
    if (ch>='a'&&ch<='f') {*out=(unsigned int)(ch-'a'+10);return true;}
    if (ch>='A'&&ch<='F') {*out=(unsigned int)(ch-'A'+10);return true;}
    return false;
}
bool decode_string(Cursor& c,char* out,size_t capacity) {
    if (c.p>=c.end || *c.p!='"') return false;
    ++c.p;
    size_t size=0;
    while (c.p<c.end) {
        unsigned int ch=(unsigned char)*c.p++;
        if (ch=='"') {
            if (out) {
                if (size>=capacity) return false;
                out[size]=0;
            }
            return true;
        }
        if (ch<0x20) return false;
        if (ch=='\\') {
            if (c.p>=c.end) return false;
            ch=(unsigned char)*c.p++;
            switch (ch) {
            case '"': case '\\': case '/': break;
            case 'b': ch='\b'; break;
            case 'f': ch='\f'; break;
            case 'n': ch='\n'; break;
            case 'r': ch='\r'; break;
            case 't': ch='\t'; break;
            case 'u': {
                if (c.end-c.p<4) return false;
                unsigned int v=0;
                for(int i=0;i<4;++i) {
                    unsigned int digit=0;
                    if(!hex_digit(*c.p++,&digit)) return false;
                    v=(v<<4)|digit;
                }
                // The relevant OAuth protocol fields must be ASCII. Do not
                // silently corrupt Unicode in security-sensitive fields.
                if(v==0 || (out && v>0x7f)) return false;
                ch=v<=0x7f?v:'?';
                break;
            }
            default: return false;
            }
        }
        if (out) {
            if (size+1>=capacity) return false;
            out[size]=(char)ch;
        }
        ++size;
    }
    return false;
}
bool decode_uint(Cursor& c,unsigned int* out) {
    if(c.p>=c.end || *c.p<'0'||*c.p>'9') return false;
    if(*c.p=='0' && c.p+1<c.end &&
       c.p[1]>='0' && c.p[1]<='9') return false;
    unsigned int value=0;
    while(c.p<c.end && *c.p>='0' && *c.p<='9') {
        const unsigned int digit=(unsigned int)(*c.p-'0');
        if (value>(0xffffffffu-digit)/10u) return false;
        value=value*10u+digit;
        ++c.p;
    }
    if(out) *out=value;
    return true;
}
bool decode_value(Cursor& c,char* output,size_t cap,
                  unsigned int* value,Kind* kind) {
    if(c.p>=c.end) return false;
    if(*c.p=='"') {
        *kind=KIND_STRING;
        return decode_string(c,output,cap);
    }
    if(*c.p>='0' && *c.p<='9') {
        *kind=KIND_UINT;
        return decode_uint(c,value);
    }
    const char* options[]={"true","false","null"};
    for(int i=0;i<3;++i) {
        const size_t size=strlen(options[i]);
        if((size_t)(c.end-c.p)>=size &&
           memcmp(c.p,options[i],size)==0) {
            c.p+=size;
            *kind=KIND_LITERAL;
            return true;
        }
    }
    return false;
}
bool find_field(const char* json,size_t length,const char* name,
                char* output,size_t cap,unsigned int* number,bool text) {
    if (!json || !name || length<2 || length>32767) return false;
    Cursor c={json,json+length};
    space(c);
    if(c.p>=c.end || *c.p++!='{') return false;
    bool found=false;
    while (c.p<c.end) {
        space(c);
        if (c.p<c.end && *c.p=='}') {
            ++c.p;
            space(c);
            return c.p==c.end && found;
        }
        char key[96]={0};
        if(!decode_string(c,key,sizeof(key))) return false;
        space(c);
        if(c.p>=c.end || *c.p++!=':') return false;
        space(c);

        const bool wanted=strcmp(key,name)==0;
        if(wanted && found) return false; // duplicate requested key
        Kind kind=KIND_LITERAL;
        unsigned int value=0;
        if(!decode_value(c,(wanted && text)?output:NULL,
                         (wanted && text)?cap:0,&value,&kind)) return false;
        if(wanted) {
            if ((text && kind!=KIND_STRING) ||
                (!text && kind!=KIND_UINT)) return false;
            if(!text && number) *number=value;
            found=true;
        }

        space(c);
        if(c.p>=c.end) return false;
        if(*c.p==',') {
            ++c.p;
            space(c);
            if(c.p==c.end || *c.p=='}') return false; // trailing comma
        } else if(*c.p!='}') {
            return false;
        }
    }
    return false;
}
}
bool string(const char* json,size_t size,const char* field,
            char* out,size_t capacity) {
    if(!out||capacity==0) return false;
    out[0]=0;
    if(find_field(json,size,field,out,capacity,NULL,true)) return true;
    out[0]=0;
    return false;
}
bool uint_value(const char* json,size_t size,const char* field,
                unsigned int* out) {
    if(!out) return false;
    *out=0;
    if(find_field(json,size,field,NULL,0,out,false)) return true;
    *out=0;
    return false;
}
}
