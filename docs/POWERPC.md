# Arquitetura XCloud360: Xbox 360 RGH 3 / PowerPC Xenon

**Plataforma exclusiva do cliente atual:** Xbox 360 real com RGH/RGH 3, executável Retail `.xex` iniciado via Aurora/XeXMenu. O objetivo de longo prazo é executar **toda a sessão do xCloud diretamente no Xbox 360 sem PC intermediário**.

O Xenon usa CPU PowerPC de 3 núcleos e 6 threads de hardware, ABI big-endian. O XCloud360 é compilado pelo Xbox 360 SDK/XDK para PowerPC; não é um executável Windows. No atual projeto o XexTool verifica e converte a saída para formato Retail.

## Pipeline pretendido — ainda não funcional

```text
                       Xbox 360 RGH3
   Controle Xbox 360 -> cliente PowerPC (.xex)
                         |    |
                         |    +-> DNS, rede externa e TLS verificado
                         |    +-> Microsoft OAuth autorizado
                         |    +-> sessão e sinalização do Xbox Cloud Gaming
                         |    +-> WebRTC: ICE, DTLS/SRTP, canais de dados
                         |    +-> decodificação vídeo/áudio e renderização
                         v
                         TV
```

## Etapas
- **Feito:** geração do executável PowerPC Retail, interface gráfica, controle e diagnósticos de DNS UDP e TCP.
- **Ainda sem comprovação no hardware:** saída UDP/TCP livre para serviços externos sob XNet Retail.
- **Pendente:** TLS moderno com certificados, fluxo OAuth autorizado, acesso aos protocolos de sessão xCloud, WebRTC, decodificadores de vídeo/áudio, integração do controle e redução de latência.
- **Não implementado:** reprodução de Fortnite no console.

## Quando investigar XeLL/libxenon
XeLL Reloaded é um ambiente de boot homebrew que pode carregar um **ELF nativo PowerPC**, diferente do XEX da dashboard. O projeto libxenon possui pilha de rede independente e deve ser avaliado se o caminho XDK/XNet Retail limitar conexões ao serviço. Essa alternativa não converte automaticamente um XEX para ELF; exige outro compilador e outra camada de rede e gráficos.

A documentação de rede está em [RGH_NETWORK.md](RGH_NETWORK.md) e o plano de streaming em [fortnite-architecture.md](fortnite-architecture.md).
