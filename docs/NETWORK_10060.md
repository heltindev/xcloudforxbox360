# TCP 10060 diagnostics — NET-10060-A1

This revision investigates the console timeout. Confirming a fix requires testing on physical Xbox 360 hardware.

## Changes

- The probe and HTTPS backend wait up to 10 seconds for IP acquisition within the same SYSAPP socket context.
- Status distinguishes IP ready, wait timeout, XNet unavailable, and configuration trouble.
- Missing Ethernet flags do not block a valid address; Wi-Fi and emulators may report different flags.
- If an emulator does not expose XNADDR, diagnostics still run actual socket tests rather than inventing an IP address.
- HTTPS tries up to four distinct DNS-resolved IPv4 addresses within a total TCP wait budget of 12 seconds.
- Logs record destination public IP, port, attempt, elapsed time, error, and stage without tokens.
- X / NETWORK screen displays local address, flags, DNS, TCP/HTTP probes, and WSA/TLS/HTTP stage.
- Certificate, hostname, and SNI validation remain mandatory. Existing `SO_MARKINSECURE` configures the XNet socket; it does not disable TLS.

IP waiting addresses a startup condition identified in the code. Address rotation avoids depending on one destination IP. Neither change proves the cause of a previously observed hardware timeout.

## Testing on Xbox 360

1. Use the new `Release/XCloud360.xex`.
2. From Home, press **X** to open NETWORK.
3. Press **Y** to run only network diagnostics.
4. Let the test finish; it may take approximately one minute.
5. Record the **NET-10060-A1** screen including IP, LINK, XNADDR, DNS, results, and origins.
6. Return with X/B, then press A to test HTTPS sign-in. If it fails, reopen X / NETWORK and record the last HTTPS line.

Y does not start another test during active authentication/session work. Closing diagnostics does not cancel sign-in. Do not disclose login codes or authentication responses.

## Interpreting the results

| Result | Meaning |
| --- | --- |
| IP WAIT TIMEOUT / IP 0.0.0.0 | Address acquisition was not confirmed within the deadline; inspect network configuration |
| XNET ADDRESS UNAVAILABLE | API did not report an address; socket tests continue to distinguish emulator limitations |
| XNET CONFIG TROUBLE | Stack signaled a potential configuration problem |
| DNS fails, direct IP works | Investigate DNS resolution |
| Origin 8, error 10060 | TCP connection wait timed out |
| HTTP:80 result 1 | Server returned an HTTP prefix, proving bidirectional traffic to that destination/port |
| 443 result 3 | Socket became writable; HTTPS must still complete and validate TLS |
| HTTP80 works, HTTPS fails | Investigate port 443, TLS, certificates, clock, and endpoint separately |
| Service HTTP 401/403 | Transport reached the application; investigate authorization, not a TCP timeout |

The control test uses 1.1.1.1 without changing the console's DNS settings. One failed public control endpoint does not prove there is no internet connection.

## Build and verification for this revision

- Visual Studio **2010 10.0.40219.1**, **Release | Xbox 360** configuration.
- `devenv.com` build: passed.
- Retail XEX preparation and inspection: passed.
- Previous host suites for authentication, renewal, and session: passed.
- New readiness suite for delayed DHCP, exact deadline, timeout, flags, and 32-bit clock rollover: passed.
- Xenia and physical Xbox 360 execution: not performed for this revision.

Artifact: `Release/XCloud360.xex`.
SHA-256: **E59FC1F6CC00523D7155D6710E7DB3C01845E54F304EF1AC9D50AE51A14F84D9**

Local logs: `obj/vs2010_network_final.log`, `obj/build_net10060_retail.log`, `obj/test_network_10060.log`.

The console connection error remains unvalidated. This revision does not implement xCloud playback.
