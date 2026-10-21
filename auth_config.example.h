#pragma once
// Copy this file to auth_config.h in the project root.
// Set this to YOUR OWN public Application (client) ID registered with
// Microsoft and configured to support device-code authentication.
// Never put account passwords, client secrets or access tokens in files.
// This placeholder intentionally fails closed until configured.
#define XCLOUD360_CLIENT_ID "00000000-0000-0000-0000-000000000000"
// Optional opt-in: only enable if your app is registered/authorized for
// the `xboxlive.signin` scope. Enabling it may make code
// issuance/consent fail if Microsoft's permission checks reject your app.
// THIS DOES NOT PROVIDE an Xbox XSTS token or xCloud session by itself.
#define XCLOUD360_ENABLE_XBOX_LIVE_SCOPE 0
