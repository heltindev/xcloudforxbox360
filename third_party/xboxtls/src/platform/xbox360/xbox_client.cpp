#include "xboxtls/xbox_client.h"

#if defined(_XBOX)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace xboxtls {

XboxTlsOptions::XboxTlsOptions()
    : trust_store_path("game:\\roots.xts"),
      port(443),
      alpn("http/1.1"),
      require_alpn(false),
      preferred_group(GROUP_X25519),
      application_record_padding(0),
      send_timeout_ms(0),
      receive_timeout_ms(0),
      certificate_sha256_pin(0),
      auto_reconnect(true),
      send_close_notify(true),
      request_peer_key_update(false) {}

XboxTlsStats::XboxTlsStats()
    : bytes_sent(0),
      bytes_received(0),
      connect_attempts(0),
      successful_connections(0),
      failed_connections(0),
      key_updates_sent(0),
      trust_store_reloads(0) {}


static bool alpn_list_contains(const char* list, const char* selected) {
    if (!list || !selected || !*selected)
        return false;

    const size_t selected_length = strlen(selected);
    const char* p = list;

    while (*p) {
        const char* start = p;
        while (*p && *p != ',')
            ++p;

        const size_t length = (size_t)(p - start);
        if (length == selected_length && memcmp(start, selected, length) == 0)
            return true;

        if (*p == ',')
            ++p;
    }

    return false;
}

XboxTlsClient::XboxTlsClient()
    : verifier_(&platform_),
      trust_store_data_(0),
      trust_store_size_(0),
      last_error_(XT_OK),
      network_started_(false),
      initialized_(false) {}

XboxTlsClient::~XboxTlsClient() {
    shutdown();
}

Error XboxTlsClient::remember(Error error) {
    last_error_ = error;
    return error;
}

Error XboxTlsClient::load_trust_store_file(const char* path) {
    if (!path || !path[0])
        return remember(XT_ERR_INVALID_ARGUMENT);

    FILE* file = fopen(path, "rb");
    if (!file)
        return remember(XT_ERR_IO);

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return remember(XT_ERR_IO);
    }

    const long file_size = ftell(file);
    if (file_size <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return remember(XT_ERR_IO);
    }

    xt_u8* data = static_cast<xt_u8*>(malloc(static_cast<size_t>(file_size)));
    if (!data) {
        fclose(file);
        return remember(XT_ERR_BUFFER_TOO_SMALL);
    }

    const size_t size = static_cast<size_t>(file_size);
    if (fread(data, 1, size, file) != size) {
        free(data);
        fclose(file);
        return remember(XT_ERR_IO);
    }

    fclose(file);
    free_trust_store();

    free_trust_store();
    size_t loaded = 0;
    Error error = verifier_.load_trust_store(ByteSpan(data, size), &loaded);
    if (error != XT_OK || loaded == 0) {
        free(data);
        return remember(error != XT_OK ? error : XT_ERR_VERIFY);
    }

    trust_store_data_ = data;
    trust_store_size_ = size;
    ++stats_.trust_store_reloads;
    return remember(XT_OK);
}

void XboxTlsClient::free_trust_store() {
    if (trust_store_data_) {
        free(trust_store_data_);
        trust_store_data_ = 0;
    }

    trust_store_size_ = 0;
    verifier_.clear_trust_anchors();
}

Error XboxTlsClient::initialize() {
    return initialize(defaults_);
}

Error XboxTlsClient::initialize(const char* trust_store_path) {
    XboxTlsOptions options = defaults_;
    options.trust_store_path = trust_store_path;
    return initialize(options);
}

Error XboxTlsClient::initialize(const void* trust_store, size_t size) {
    if (!trust_store || !size)
        return remember(XT_ERR_INVALID_ARGUMENT);
    if (!network_started_) {
        Error error = xbox360_network_startup();
        if (error != XT_OK)
            return remember(error);
        network_started_ = true;
    }
    xt_u8* data = static_cast<xt_u8*>(malloc(size));
    if (!data) {
        shutdown();
        return remember(XT_ERR_BUFFER_TOO_SMALL);
    }
    memcpy(data, trust_store, size);
    size_t loaded = 0;
    Error error = verifier_.load_trust_store(ByteSpan(data, size), &loaded);
    if (error != XT_OK || loaded == 0) {
        free(data);
        shutdown();
        return remember(error != XT_OK ? error : XT_ERR_VERIFY);
    }
    trust_store_data_ = data;
    trust_store_size_ = size;
    ++stats_.trust_store_reloads;
    initialized_ = true;
    return remember(XT_OK);
}

Error XboxTlsClient::initialize(const XboxTlsOptions& options) {
    defaults_ = options;

    if (!network_started_) {
        Error error = xbox360_network_startup();
        if (error != XT_OK)
            return remember(error);

        network_started_ = true;
    }

    Error error = load_trust_store_file(defaults_.trust_store_path);
    if (error != XT_OK) {
        shutdown();
        return remember(error);
    }

    initialized_ = true;
    return remember(XT_OK);
}

Error XboxTlsClient::connect(const char* hostname, xt_u16 port, const char* alpn) {
    XboxTlsOptions options = defaults_;
    options.port = port;
    options.alpn = alpn;
    return connect(hostname, options);
}

Error XboxTlsClient::connect(const char* hostname, const XboxTlsOptions& options) {
    ++stats_.connect_attempts;

    if (!initialized_ || !hostname || !hostname[0]) {
        ++stats_.failed_connections;
        return remember(XT_ERR_INVALID_ARGUMENT);
    }

    if (tls_.state() == CLIENT_CONNECTED) {
        if (!options.auto_reconnect)
            return remember(XT_ERR_INVALID_ARGUMENT);

        if (options.send_close_notify)
            tls_.close();
        else
            tls_.reset();

        socket_.close();
    }

    if (tls_.state() != CLIENT_IDLE) {
        Error reset_error = tls_.reset();
        if (reset_error != XT_OK)
            return remember(reset_error);
    }

    if (options.preferred_group != GROUP_X25519 &&
        options.preferred_group != GROUP_SECP256R1) {
        ++stats_.failed_connections;
        return remember(XT_ERR_INVALID_ARGUMENT);
    }

    if (options.certificate_sha256_pin) {
        Error pin_error = verifier_.set_leaf_certificate_sha256_pin(
            ByteSpan(options.certificate_sha256_pin, 32));
        if (pin_error != XT_OK) {
            ++stats_.failed_connections;
            return remember(pin_error);
        }
    } else {
        verifier_.clear_leaf_certificate_sha256_pin();
    }

    Error error = socket_.connect_tcp(hostname, options.port);
    if (error != XT_OK) {
        ++stats_.failed_connections;
        return remember(error);
    }

    error = socket_.set_io_timeouts(options.send_timeout_ms, options.receive_timeout_ms);
    if (error != XT_OK) {
        socket_.close();
        ++stats_.failed_connections;
        return remember(error);
    }

    ClientConfig config;
    config.platform = &platform_;
    config.stream = &socket_;
    config.verifier = &verifier_;
    config.hostname = hostname;
    config.alpn = options.alpn;
    config.preferred_group = options.preferred_group;
    config.application_record_padding = options.application_record_padding;

    error = tls_.connect(config);
    if (error != XT_OK) {
        socket_.close();
        ++stats_.failed_connections;
        return remember(error);
    }

    active_options_ = options;

    if (options.require_alpn) {
        const char* negotiated = tls_.negotiated_alpn();

        if (!alpn_list_contains(options.alpn, negotiated)) {
            tls_.close();
            socket_.close();
            ++stats_.failed_connections;
            return remember(XT_ERR_BAD_HANDSHAKE);
        }
    }

    if (options.request_peer_key_update) {
        error = tls_.key_update(true);
        if (error != XT_OK) {
            ++stats_.failed_connections;
            return remember(error);
        }
        ++stats_.key_updates_sent;
    }

    ++stats_.successful_connections;
    return remember(XT_OK);
}

Error XboxTlsClient::send(ByteSpan data) {
    Error error = tls_.send(data);
    if (error == XT_OK)
        stats_.bytes_sent += (xt_u64)data.size;
    return remember(error);
}

Error XboxTlsClient::send(const void* data, size_t size) {
    if (!data && size)
        return remember(XT_ERR_INVALID_ARGUMENT);

    return send(ByteSpan(static_cast<const xt_u8*>(data), size));
}

Error XboxTlsClient::receive(MutableByteSpan out, size_t* received) {
    Error error = tls_.recv(out, received);
    if (error == XT_OK && received)
        stats_.bytes_received += (xt_u64)(*received);
    return remember(error);
}

Error XboxTlsClient::receive(void* data, size_t capacity, size_t* received) {
    if (!data && capacity)
        return remember(XT_ERR_INVALID_ARGUMENT);

    return receive(MutableByteSpan(static_cast<xt_u8*>(data), capacity), received);
}

Error XboxTlsClient::key_update(bool request_peer_update) {
    Error error = tls_.key_update(request_peer_update);
    if (error == XT_OK)
        ++stats_.key_updates_sent;
    return remember(error);
}

Error XboxTlsClient::reload_trust_store(const char* trust_store_path) {
    if (!network_started_)
        return remember(XT_ERR_INVALID_ARGUMENT);

    return load_trust_store_file(trust_store_path);
}

Error XboxTlsClient::close() {
    Error error = XT_OK;

    if (tls_.state() == CLIENT_CONNECTED && active_options_.send_close_notify)
        error = tls_.close();
    else if (tls_.state() != CLIENT_IDLE)
        error = tls_.reset();

    socket_.close();
    return remember(error);
}

void XboxTlsClient::shutdown() {
    close();
    free_trust_store();

    if (network_started_) {
        xbox360_network_cleanup();
        network_started_ = false;
    }

    initialized_ = false;
}

void XboxTlsClient::set_default_options(const XboxTlsOptions& options) {
    defaults_ = options;
}

const XboxTlsOptions& XboxTlsClient::default_options() const {
    return defaults_;
}

bool XboxTlsClient::initialized() const {
    return initialized_;
}

bool XboxTlsClient::connected() const {
    return tls_.state() == CLIENT_CONNECTED;
}

ClientState XboxTlsClient::state() const {
    return tls_.state();
}

Error XboxTlsClient::last_error() const {
    return last_error_;
}

const XboxTlsStats& XboxTlsClient::stats() const {
    return stats_;
}

void XboxTlsClient::reset_stats() {
    stats_ = XboxTlsStats();
}

const char* XboxTlsClient::negotiated_alpn() const {
    return tls_.negotiated_alpn();
}

xt_u8 XboxTlsClient::last_alert_level() const {
    return tls_.last_alert_level();
}

xt_u8 XboxTlsClient::last_alert_description() const {
    return tls_.last_alert_description();
}

} // namespace xboxtls

#endif
