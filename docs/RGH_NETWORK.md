# Xbox 360 RGH/RGH 3 network investigation

This is a **network diagnostic**, not a cloud streaming client.

## Last owner-reported RGH test — 2026-10-20

| Probe | Result |
| --- | --- |
| Xbox XNet DNS | **1 IPv4 address** |
| Xbox.com:443 | **10060**, source **8** |
| 1.1.1.1:443 direct IP | **10060**, source **8** |
| Google DNS over UDP | No result supplied for this hardware run |
| 1.1.1.1:80 | Source prepared, RGH result still missing |

**Error 10060** is a TCP connection timeout; **source 8** maps to `TcpProbe::TCP_SELECT` in the project's source. DNS did resolve a hostname. No TLS, OAuth or xCloud request was made.

Potential explanations: XNet Retail security or socket restrictions, IP route, firewall/NAT behavior or platform-specific socket initialization. None is proven. DNS switching alone is not a verified solution.

## Network diagnostic source

- `XNetStartup` / `WSAStartup` initialization.
- `XNetDnsLookup("www.xbox.com")` uses the console's existing network settings.
- Optional fallback UDP DNS query to `8.8.8.8:53`, followed by `8.8.4.4:53` if necessary. The program does not change the dashboard's DNS settings.
- TCP connection to a resolved `www.xbox.com` IPv4 address, port 443.
- Direct TCP connection to `1.1.1.1` on **port 443** and **port 80** as independent comparison tests.
- Each TCP attempt has a deadline. An open TCP connection does **not** prove TLS/HTTPS support or xCloud access.

## Error source codes

| Code | Internal stage |
| --- | --- |
| 1 | XNet startup |
| 2 | Winsock startup |
| 3 | DNS API call |
| 4 | DNS status/result |
| 5 | Socket creation |
| 6 | Nonblocking socket setup |
| 7 | TCP connect call |
| **8** | **TCP select/wait for connect completion** |
| 9 | TCP socket result |
| 10 | Worker thread creation |
| 11–15 | UDP DNS phases |

### What to validate next on real RGH

1. Ensure the interface has a local IP address, subnet, gateway and working DNS.
2. Compare TCP port 80 vs 443, noting exact error, source and elapsed time.
3. Confirm UDP send/receive behavior with a test endpoint the developer is authorized to use.
4. Check whether Retail XNet allows outbound arbitrary TCP/UDP in a homebrew title.
5. If XNet blocks traffic, compile and run the separate libxenon/XeLL boot experiment, then implement a test using the exact available lwIP version.

The diagnostic screen is optional: press **X** from the home screen. No test is started automatically at launch.

**Safety:** never send credentials before validated HTTPS is available, and do not bypass TLS certificate verification.

See [the dated report](TESTE_RGH3_2026-10-20.md) and [the research references](REFERENCES.md).
