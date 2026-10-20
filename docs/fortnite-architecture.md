# Fortnite no Xbox 360: metas técnicas do XCloud360

A meta é transmitir Fortnite **dos servidores de jogo para o Xbox 360 RGH 3**, com vídeo/áudio e controle executados no próprio console. **O executável atual NÃO realiza isso.** O jogo não foi portado nem existe um executável nativo de Fortnite para Xbox 360 neste projeto.

## Etapas necessárias

| Módulo | Estado |
| --- | --- |
| Interface nativa C++/PowerPC para Xbox 360 | Implementada |
| XNet, DNS UDP e TCP (diagnóstico) | Implementado; resultados RGH ainda precisam ser confirmados |
| Comunicação externa UDP/TCP confiável | Não comprovada |
| TLS moderno com certificados verificados | Não implementado |
| Login Microsoft OAuth autorizado (código/QR) | Não implementado |
| Autorização de sessão no Xbox Cloud Gaming | Não implementado |
| Sinalização WebRTC, ICE, DTLS/SRTP | Não implementado |
| Receber/decodificar vídeo e áudio em tempo real | Não implementado |
| Enviar controles com baixa latência | Não implementado |
| Fortnite jogável | Não disponível |

## Referências
- [Greenlight](https://github.com/unknownskl/greenlight) e [Better xCloud](https://github.com/redphx/better-xcloud): referência de arquitetura para clientes modernos. O código não é diretamente compatível com o PowerPC/XDK do Xbox 360.
- [libxenon](https://github.com/Free60Project/libxenon): alternativa homebrew para investigar limitações XNet do RGH.

## Como colaborar
Investigar as limitações de rede do Xbox 360 real, a disponibilidade de TLS moderno no PowerPC e caminhos de vídeo/áudio na GPU Xenos. Nunca inclua senhas, tokens ou chaves privadas em repositórios e arquivos TXT.

O próximo teste no RGH usa `XCloud360-RGH3.zip`. Veja [RGH_NETWORK.md](RGH_NETWORK.md).
