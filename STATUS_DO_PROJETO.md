# STATUS DO PROJETO — XCloud360 / Xbox 360 RGH 3

**Última revisão:** 20/10/2026. **Alvo exclusivo:** Xbox 360 PowerPC Xenon, RGH/RGH 3, sem PC intermediário.

> Esta lista diferencia **código escrito**, **comprovado no hardware** e **ainda não implementado**. O projeto NÃO se conecta ao xCloud e NÃO executa Fortnite. O erro 10060 não foi corrigido.

## Resultado do seu teste no RGH 3

| Teste no Xbox 360 | Resultado | O que significa |
| --- | --- | --- |
| DNS Xbox | **1 IP** | Resolução de nome ocorreu |
| TCP Xbox.com porta 443 | **10060 / origem 8** | Timeout na espera pela conexão (`TCP_SELECT`) |
| TCP 1.1.1.1 porta 443 | **10060 / origem 8** | Timeout também usando endereço IP direto |
| DNS UDP Google | Não informado neste teste | Sem resultado conclusivo |
| TCP 1.1.1.1 porta 80 | Código adicionado, resultado não informado | Teste comparativo, não corrige a conexão |

**Conclusão provisória:** DNS funcionou; ambas as conexões TCP externas falharam. O motivo ainda é desconhecido. Investigar XNet Retail, rota e restrições de rede; não assumir que mudar DNS resolve.

## A. Implementado e anteriormente observado funcionando

- [x] Projeto C++ nativo para Xbox 360 / PowerPC Xenon.
- [x] Geração do executável `.xex` Retail em versões anteriores.
- [x] Interface gráfica básica Direct3D e navegação A/B/BACK.
- [x] Leitura da configuração DNS por XNet (encontrou 1 IP no RGH).
- [x] Código de diagnóstico com sockets TCP, que executou e registrou o erro 10060 no hardware.

## B. Código escrito — ainda precisa de compilação, teste ou integração

- [ ] **Fonte suavizada dentro do XEX:** carregador de XPR em memória + `embed_font.ps1`, com bitmap reserva. Ainda precisa confirmação de inicialização, link e renderização no RGH.
- [ ] **Diagnóstico TCP porta 80** além de TCP 443. Ainda precisamos do resultado no RGH.
- [ ] **Consulta DNS UDP direta aos servidores Google** com resposta validada; resultado no RGH ainda incerto.
- [ ] **Captura XInput completa:** botões, analógicos e gatilhos, ainda sem envio remoto ao xCloud.
- [ ] **`http_wire`**: montagem HTTP/1.1 e análise limitada de respostas; sem transporte HTTPS e sem suporte completo a HTTP/chunked.
- [ ] **`json_fields`**: parser restrito a campos simples; não é um parser JSON geral.
- [ ] **`oauth_device`**: formatação e interpretação local de mensagens Device Code; não abre login e não possui conexão OAuth.
- [ ] **`rtp_packet`**: leitura de cabeçalhos RTP já recebidos/descriptografados; não implementa WebRTC ou SRTP.
- [ ] **`h264_nal`**: identificação de NAL units H.264 Annex B; não decodifica vídeo.
- [ ] **`protocol_selftest`**: testes offline de parsers; ainda precisam ser conferidos no RGH.
- [ ] Corrigir eventuais erros do compilador/linker desses módulos com base no log completo do usuário.

## C. Tudo que falta para Fortnite abrir pelo xCloud

| Etapa | Requisito | Estado |
| --- | --- | --- |
| 1 | Rede externa confiável: DNS, TCP, UDP, gateway, NAT, reconexão | **BLOQUEADO: erro 10060** |
| 2 | Determinar se XNet Retail aceita tráfego; se não, avaliar XeLL/libxenon/lwIP | Pesquisa pendente |
| 3 | Biblioteca TLS moderna portada para PowerPC, relógio e aleatoriedade seguros | Não implementado |
| 4 | Verificação de certificados e hostname, SNI e HTTPS robusto | Não implementado |
| 5 | Cliente HTTP/JSON completo, adequado aos serviços autorizados | Parcial, apenas parsers |
| 6 | Autorização Microsoft OAuth Device Code, cliente registrado e permissões | Parcial, apenas parsing offline |
| 7 | Exibição de código/QR, polling, tokens protegidos, refresh/logout | Não implementado |
| 8 | Descoberta de catálogo, autorização da conta, região e acesso legal ao xCloud | Não implementado; acesso de terceiros não garantido |
| 9 | Criar sessão real do Fortnite e tratar fila, estados e signaling | Não implementado |
| 10 | WebRTC conforme protocolo vigente: SDP, ICE/STUN/TURN, DTLS/SRTP e canal de dados | Não implementado |
| 11 | Rede de mídia: RTP/RTCP, jitter, perda, reconexão e sincronização | Apenas parser RTP preliminar |
| 12 | Decodificação de H.264 ou codec realmente negociado, sem exceder RAM/CPU/GPU | Apenas scanner de NAL; sem decoder |
| 13 | Decodificação de áudio, buffers e saída de som | Não implementado |
| 14 | Envio dos controles do Xbox 360 à sessão, latência e feedback | Somente captura XInput |
| 15 | Interface para conta, QR, fila, vídeo, estado da conexão e erros | Só interface diagnóstica básica |
| 16 | Segurança, limites, logs sem credenciais, tratamento de travamentos | Parcial e não auditado |
| 17 | Teste real no RGH, desempenho, FPS, latência, estabilidade e compatibilidade | Pendente |

## Podemos pular o diagnóstico TCP?

**Sim, como tela ou etapa de desenvolvimento**: ele pode ficar opcional, e podemos continuar escrevendo outros módulos. **Não é possível pular a necessidade de transporte de rede funcional**, porque o streaming precisa receber mídia e enviar controles. Antes de autenticar de verdade, a conexão TLS validada também é indispensável.

**Trilha recomendada:** continuar os componentes offline em paralelo; quando for possível testar rede no RGH, comparar TCP 80/443 e decidir entre XNet e uma alternativa nativa XeLL/libxenon. **Não afirmar que o xCloud funciona enquanto essas dependências estiverem ausentes.**

## Regras deste projeto

- O usuário faz a compilação; o assistente não compila, não gera ZIP e não modifica `Release/`.
- A fonte precisa estar dentro do `.xex` final; não depender de uma pasta `Media/Fonts` no console.
- Não criar pasta `XCloud360-RGH3` e não criar pacotes ZIP.
- Manter lista detalhada em `ROADMAP.md` e esta visão geral em `STATUS_DO_PROJETO.md`.
- Não copiar para o GitHub binários proprietários do XDK, recursos de fonte gerados, credenciais, tokens, CPU Key ou KV.

**Checklist detalhado de todas as subtarefas:** [ROADMAP.md](ROADMAP.md).
**Relato original dos erros:** [docs/TESTE_RGH3_2026-10-20.md](docs/TESTE_RGH3_2026-10-20.md).
