#include "xboxtls/x509.h"
#include <string.h>

namespace xboxtls {

X509CertificateView::X509CertificateView()
    : public_key_kind(PUBLIC_KEY_UNKNOWN), validity_present(false), not_before(0), not_after(0),
      basic_constraints_present(false), is_ca(false), path_len_present(false),
      path_len_constraint(0), key_usage_present(false), key_usage_digital_signature(false),
      key_usage_key_cert_sign(false), extended_key_usage_present(false), eku_server_auth(false) {}

struct DerTlv {
    xt_u8 tag;
    ByteSpan full;
    ByteSpan value;
};

static Error der_tlv(ByteSpan in, size_t off, DerTlv* out, size_t* next) {
    if (!out || !next || !in.data || off >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    size_t p = off;
    xt_u8 tag = in.data[p++];
    if (p >= in.size)
        return XT_ERR_BAD_HANDSHAKE;
    xt_u8 lb = in.data[p++];
    size_t len = 0;
    if ((lb & 0x80) == 0) {
        len = lb;
    } else {
        size_t n = lb & 0x7F;
        if (n == 0 || n > 4 || p + n > in.size)
            return XT_ERR_BAD_HANDSHAKE;
        if (in.data[p] == 0)
            return XT_ERR_BAD_HANDSHAKE;
        for (size_t i = 0; i < n; ++i)
            len = (len << 8) | in.data[p++];
        if (len < 128)
            return XT_ERR_BAD_HANDSHAKE;
    }
    if (len > in.size - p)
        return XT_ERR_BAD_HANDSHAKE;
    out->tag = tag;
    out->full = ByteSpan(in.data + off, (p - off) + len);
    out->value = ByteSpan(in.data + p, len);
    *next = p + len;
    return XT_OK;
}

static bool dec2(const xt_u8* p, int* v) {
    if (p[0] < '0' || p[0] > '9' || p[1] < '0' || p[1] > '9')
        return false;
    *v = (p[0] - '0') * 10 + (p[1] - '0');
    return true;
}
static bool dec4(const xt_u8* p, int* v) {
    int a, b;
    if (!dec2(p, &a) || !dec2(p + 2, &b))
        return false;
    *v = a * 100 + b;
    return true;
}
static xt_u64 days_before_year(int y) {
    int ym1 = y - 1;
    return (xt_u64)(365LL * ym1 + ym1 / 4 - ym1 / 100 + ym1 / 400);
}
static xt_u64 days_before_month(int y, int m) {
    static const int cum[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    xt_u64 d = cum[m];
    if (m > 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0))
        ++d;
    return d;
}
static bool parse_asn1_time(const DerTlv& t, xt_u64* out) {
    if (!out || (t.tag != 0x17 && t.tag != 0x18))
        return false;
    const xt_u8* p = t.value.data;
    size_t n = t.value.size;
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, se = 0;
    size_t off = 0;
    if (t.tag == 0x17) {
        if (n != 13 || !dec2(p, &y))
            return false;
        y = (y >= 50 ? 1900 + y : 2000 + y);
        off = 2;
    } else {
        if (n != 15 || !dec4(p, &y))
            return false;
        off = 4;
    }
    if (!dec2(p + off, &mo) || !dec2(p + off + 2, &d) || !dec2(p + off + 4, &h) ||
        !dec2(p + off + 6, &mi) || !dec2(p + off + 8, &se) || p[off + 10] != 'Z')
        return false;
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31 || h > 23 || mi > 59 || se > 60)
        return false;
    xt_u64 days =
        (days_before_year(y) - days_before_year(1970)) + days_before_month(y, mo) + (xt_u64)(d - 1);
    *out = days * 86400ULL + (xt_u64)h * 3600ULL + (xt_u64)mi * 60ULL + (xt_u64)se;
    return true;
}
static Error parse_validity(ByteSpan full, X509CertificateView* out) {
    DerTlv seq;
    size_t n = 0;
    Error e = der_tlv(full, 0, &seq, &n);
    if (e != XT_OK || seq.tag != 0x30 || n != full.size)
        return XT_ERR_BAD_HANDSHAKE;
    DerTlv a, b;
    size_t p = 0, np = 0;
    e = der_tlv(seq.value, p, &a, &np);
    if (e != XT_OK)
        return e;
    p = np;
    e = der_tlv(seq.value, p, &b, &np);
    if (e != XT_OK || np != seq.value.size)
        return XT_ERR_BAD_HANDSHAKE;
    if (!parse_asn1_time(a, &out->not_before) || !parse_asn1_time(b, &out->not_after) ||
        out->not_before > out->not_after)
        return XT_ERR_BAD_HANDSHAKE;
    out->validity_present = true;
    return XT_OK;
}

static bool oid_eq(ByteSpan oid, const xt_u8* bytes, size_t n) {
    return oid.size == n && memcmp(oid.data, bytes, n) == 0;
}

static Error parse_spki(ByteSpan spki_full, X509CertificateView* out) {
    DerTlv seq;
    size_t n = 0;
    Error e = der_tlv(spki_full, 0, &seq, &n);
    if (e != XT_OK || seq.tag != 0x30 || n != spki_full.size)
        return XT_ERR_BAD_HANDSHAKE;
    DerTlv alg;
    size_t p = 0;
    e = der_tlv(seq.value, p, &alg, &p);
    if (e != XT_OK || alg.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    DerTlv oid;
    size_t ap = 0;
    e = der_tlv(alg.value, ap, &oid, &ap);
    if (e != XT_OK || oid.tag != 0x06)
        return XT_ERR_BAD_HANDSHAKE;
    static const xt_u8 OID_RSA[] = {0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x01};
    static const xt_u8 OID_EC[] = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01};
    static const xt_u8 OID_P256[] = {0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07};
    if (oid_eq(oid.value, OID_RSA, sizeof(OID_RSA))) {
        out->public_key_kind = PUBLIC_KEY_RSA;
    } else if (oid_eq(oid.value, OID_EC, sizeof(OID_EC))) {
        DerTlv param;
        size_t dummy = 0;
        if (ap >= alg.value.size || der_tlv(alg.value, ap, &param, &dummy) != XT_OK ||
            param.tag != 0x06 || !oid_eq(param.value, OID_P256, sizeof(OID_P256)))
            return XT_ERR_UNSUPPORTED;
        out->public_key_kind = PUBLIC_KEY_EC_P256;
    } else
        return XT_ERR_UNSUPPORTED;
    DerTlv bits;
    size_t bp = 0;
    e = der_tlv(seq.value, p, &bits, &bp);
    if (e != XT_OK || bits.tag != 0x03 || bits.value.size < 1 || bits.value.data[0] != 0 ||
        bp != seq.value.size)
        return XT_ERR_BAD_HANDSHAKE;
    out->subject_public_key_info = spki_full;
    out->subject_public_key_bits = ByteSpan(bits.value.data + 1, bits.value.size - 1);
    return XT_OK;
}

static Error parse_extensions(ByteSpan extensions_explicit, X509CertificateView* out) {
    DerTlv exwrap;
    size_t n = 0;
    Error e = der_tlv(extensions_explicit, 0, &exwrap, &n);
    if (e != XT_OK || exwrap.tag != 0xA3 || n != extensions_explicit.size)
        return XT_ERR_BAD_HANDSHAKE;
    DerTlv seq;
    size_t sn = 0;
    e = der_tlv(exwrap.value, 0, &seq, &sn);
    if (e != XT_OK || seq.tag != 0x30 || sn != exwrap.value.size)
        return XT_ERR_BAD_HANDSHAKE;
    static const xt_u8 OID_SAN[] = {0x55, 0x1D, 0x11}, OID_BC[] = {0x55, 0x1D, 0x13},
                       OID_KU[] = {0x55, 0x1D, 0x0F}, OID_EKU[] = {0x55, 0x1D, 0x25};
    static const xt_u8 OID_SERVER_AUTH[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01};
    size_t p = 0;
    while (p < seq.value.size) {
        DerTlv ext;
        size_t np = 0;
        e = der_tlv(seq.value, p, &ext, &np);
        if (e != XT_OK || ext.tag != 0x30)
            return XT_ERR_BAD_HANDSHAKE;
        size_t q = 0, qn = 0;
        DerTlv oid;
        e = der_tlv(ext.value, q, &oid, &qn);
        if (e != XT_OK || oid.tag != 0x06)
            return XT_ERR_BAD_HANDSHAKE;
        q = qn;
        bool critical = false;
        DerTlv v;
        size_t vn = 0;
        e = der_tlv(ext.value, q, &v, &vn);
        if (e != XT_OK)
            return e;
        if (v.tag == 0x01) {
            if (v.value.size != 1)
                return XT_ERR_BAD_HANDSHAKE;
            critical = v.value.data[0] != 0;
            q = vn;
            e = der_tlv(ext.value, q, &v, &vn);
            if (e != XT_OK)
                return e;
        }
        if (v.tag != 0x04 || vn != ext.value.size)
            return XT_ERR_BAD_HANDSHAKE;
        bool known = false;
        if (oid_eq(oid.value, OID_SAN, sizeof(OID_SAN))) {
            known = true;
            DerTlv names;
            size_t nn = 0;
            e = der_tlv(v.value, 0, &names, &nn);
            if (e != XT_OK || names.tag != 0x30 || nn != v.value.size)
                return XT_ERR_BAD_HANDSHAKE;
            out->subject_alt_name = names.value;
        } else if (oid_eq(oid.value, OID_BC, sizeof(OID_BC))) {
            known = true;
            DerTlv bc;
            size_t bn = 0;
            e = der_tlv(v.value, 0, &bc, &bn);
            if (e != XT_OK || bc.tag != 0x30 || bn != v.value.size)
                return XT_ERR_BAD_HANDSHAKE;
            out->basic_constraints_present = true;
            out->is_ca = false;
            size_t bp = 0;
            if (bp < bc.value.size) {
                DerTlv ca;
                size_t cn = 0;
                e = der_tlv(bc.value, bp, &ca, &cn);
                if (e != XT_OK)
                    return e;
                if (ca.tag == 0x01) {
                    if (ca.value.size != 1)
                        return XT_ERR_BAD_HANDSHAKE;
                    out->is_ca = ca.value.data[0] != 0;
                    bp = cn;
                }
            }
            if (bp < bc.value.size) {
                DerTlv pl;
                size_t pn = 0;
                e = der_tlv(bc.value, bp, &pl, &pn);
                if (e != XT_OK || pl.tag != 0x02 || pl.value.size == 0 || pl.value.size > 4 ||
                    pn != bc.value.size || pl.value.data[0] & 0x80)
                    return XT_ERR_BAD_HANDSHAKE;
                xt_u32 vlen = 0;
                for (size_t pi = 0; pi < pl.value.size; ++pi)
                    vlen = (vlen << 8) | pl.value.data[pi];
                out->path_len_present = true;
                out->path_len_constraint = vlen;
            }
        } else if (oid_eq(oid.value, OID_KU, sizeof(OID_KU))) {
            known = true;
            DerTlv ku;
            size_t kn = 0;
            e = der_tlv(v.value, 0, &ku, &kn);
            if (e != XT_OK || ku.tag != 0x03 || kn != v.value.size || ku.value.size < 2 ||
                ku.value.data[0] > 7)
                return XT_ERR_BAD_HANDSHAKE;
            out->key_usage_present = true;
            xt_u8 b0 = ku.value.data[1];
            out->key_usage_digital_signature = (b0 & 0x80) != 0;
            out->key_usage_key_cert_sign = (b0 & 0x04) != 0;
        } else if (oid_eq(oid.value, OID_EKU, sizeof(OID_EKU))) {
            known = true;
            DerTlv es;
            size_t en = 0;
            e = der_tlv(v.value, 0, &es, &en);
            if (e != XT_OK || es.tag != 0x30 || en != v.value.size)
                return XT_ERR_BAD_HANDSHAKE;
            out->extended_key_usage_present = true;
            size_t ep = 0;
            while (ep < es.value.size) {
                DerTlv eo;
                size_t eon = 0;
                e = der_tlv(es.value, ep, &eo, &eon);
                if (e != XT_OK || eo.tag != 0x06)
                    return XT_ERR_BAD_HANDSHAKE;
                if (oid_eq(eo.value, OID_SERVER_AUTH, sizeof(OID_SERVER_AUTH)))
                    out->eku_server_auth = true;
                ep = eon;
            }
        }
        if (critical && !known)
            return XT_ERR_UNSUPPORTED;
        p = np;
    }
    return XT_OK;
}

Error x509_parse_certificate(ByteSpan der, X509CertificateView* out) {
    if (!out || !der.data || der.size == 0)
        return XT_ERR_INVALID_ARGUMENT;
    *out = X509CertificateView();
    out->der = der;
    DerTlv cert;
    size_t cn = 0;
    Error e = der_tlv(der, 0, &cert, &cn);
    if (e != XT_OK || cert.tag != 0x30 || cn != der.size)
        return XT_ERR_BAD_HANDSHAKE;
    size_t p = 0;
    DerTlv tbs;
    size_t np = 0;
    e = der_tlv(cert.value, p, &tbs, &np);
    if (e != XT_OK || tbs.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    out->tbs_certificate = tbs.full;

    size_t q = 0;
    DerTlv f;
    size_t qn = 0;
    e = der_tlv(tbs.value, q, &f, &qn);
    if (e != XT_OK)
        return e;
    if (f.tag == 0xA0)
        q = qn;
    for (int i = 0; i < 5; ++i) {
        e = der_tlv(tbs.value, q, &f, &qn);
        if (e != XT_OK)
            return e;
        if (i == 2)
            out->issuer_name = f.full;
        if (i == 3) {
            e = parse_validity(f.full, out);
            if (e != XT_OK)
                return e;
        }
        if (i == 4)
            out->subject_name = f.full;
        q = qn;
    }
    DerTlv spki;
    e = der_tlv(tbs.value, q, &spki, &qn);
    if (e != XT_OK || spki.tag != 0x30)
        return XT_ERR_BAD_HANDSHAKE;
    e = parse_spki(spki.full, out);
    if (e != XT_OK)
        return e;
    q = qn;
    while (q < tbs.value.size) {
        e = der_tlv(tbs.value, q, &f, &qn);
        if (e != XT_OK)
            return e;
        if (f.tag == 0xA3) {
            Error se = parse_extensions(f.full, out);
            if (se != XT_OK)
                return se;
        }
        q = qn;
    }
    return XT_OK;
}

static char ascii_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}
static bool dns_equal(ByteSpan pat, const char* host) {
    if (!host)
        return false;
    size_t hn = strlen(host);
    if (pat.size == hn) {
        bool same = true;
        for (size_t i = 0; i < hn; ++i)
            if (ascii_lower((char)pat.data[i]) != ascii_lower(host[i])) {
                same = false;
                break;
            }
        if (same)
            return true;
    }
    if (pat.size > 2 && pat.data[0] == '*' && pat.data[1] == '.') {
        const char* dot = strchr(host, '.');
        if (!dot || dot == host)
            return false;
        size_t suffix = strlen(dot + 1);
        if (suffix != pat.size - 2)
            return false;
        for (size_t i = 0; i < suffix; ++i)
            if (ascii_lower((char)pat.data[i + 2]) != ascii_lower(dot[1 + i]))
                return false;
        return true;
    }
    return false;
}

Error x509_verify_hostname(const X509CertificateView& cert, const char* hostname) {
    if (!hostname || !*hostname)
        return XT_ERR_INVALID_ARGUMENT;
    if (!cert.subject_alt_name.data)
        return XT_ERR_VERIFY;
    size_t p = 0;
    while (p < cert.subject_alt_name.size) {
        DerTlv gn;
        size_t np = 0;
        Error e = der_tlv(cert.subject_alt_name, p, &gn, &np);
        if (e != XT_OK)
            return e;
        if (gn.tag == 0x82 && dns_equal(gn.value, hostname))
            return XT_OK;
        p = np;
    }
    return XT_ERR_VERIFY;
}

} // namespace xboxtls
