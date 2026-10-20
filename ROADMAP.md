# ROADMAP — Tudo que falta para o XCloud360 funcionar no Xbox 360 RGH 3

**Alvo único:** Xbox 360 real com RGH/RGH 3, CPU Xenon PowerPC, sem PC intermediário.  
**Último teste de rede confirmado:** v0.9 no RGH — DNS Xbox 1 IP; TCP Xbox.com:443 e 1.1.1.1:443 falharam com erro 10060, origem 8. **Novos códigos v0.10:** fonte embutida, teste porta 80, parsers e leitura do controle; estado de compilação/funcionamento ainda não confirmado.  
**Objetivo final:** autenticar uma conta autorizada, iniciar uma sessão Xbox Cloud Gaming com Fortnite, receber vídeo/áudio e enviar comandos do controle **no próprio console**.

> Importante: este checklist abrange o trabalho conhecido, não é garantia de viabilidade. O serviço e seus protocolos podem mudar, exigir permissões não disponíveis a terceiros ou recursos além do hardware. **O Fortnite ainda não funciona.** Os checkboxes são tarefas; `[x]` indica recurso implementado/compilado, não necessariamente validado no RGH.

**Painel de status resumido:** [STATUS_DO_PROJETO.md](STATUS_DO_PROJETO.md). A lista abaixo inclui tarefas de pesquisa e de implementação; marcar código escrito como concluído não comprova funcionamento no console.

## Resultado mais recente no Xbox 360 RGH 3 (20/10/2026)

**Teste v0.9 falhou nas conexões TCP**, mas o DNS Xbox resolveu **1 IP**. Tanto o teste para Xbox.com quanto o IP direto apresentaram **erro 10060, origem 8 (TCP_SELECT)**. A causa ainda não é conhecida; as possibilidades incluem restrições do XNet Retail, rota e portas. **DNS não deve ser considerado a causa confirmada.** Informações de DNS UDP não foram fornecidas nesta rodada.

Relato preservado em [docs/TESTE_RGH3_2026-10-20.md](docs/TESTE_RGH3_2026-10-20.md).

**Regras solicitadas pelo proprietário:** somente editar fontes nesta pasta, sem compilar pelo assistente, sem criar ZIP, sem gerar diretório `XCloud360-RGH3`; a compilação e exportação para `Release/` serão feitas pelo proprietário. A fonte deve ser incorporada ao `.xex` gerado localmente, sem cópia externa obrigatória.

## 0. Base já existente — não confundir com xCloud funcional

- [x] Criar projeto C++ para compilação PowerPC pelo Xbox 360 SDK/XDK.
- [x] Gerar `.xex`, converter cópia em Retail e verificar cabeçalho com XexTool.
- [x] Criar interface de diagnóstico em Direct3D e leitura básica dos botões A/B/BACK.
- [x] Criar carregador de fonte suavizada **de dentro do XEX**, com código gerado localmente a partir do XDK e bitmap interno de reserva. **Ainda não compilado/testado no hardware.**
- [x] Implementar **tentativas** de DNS XNet, DNS UDP Google, TCP para `www.xbox.com:443` e `1.1.1.1:443`.
- [x] Criar script de compilação para `Release/XCloud360.xex`. **Fluxo atualizado para não gerar ZIP nem exigir fonte externa. Ainda sem teste de compilação.**
- [x] **Executar o diagnóstico v0.9 no Xbox 360 físico:** DNS Xbox 1 IP; os dois testes TCP na porta 443 falharam com 10060/origem 8. **Teste realizado, problema de rede permanece aberto.**
- [ ] **Validar a nova base v0.10 no RGH:** fonte incluída no XEX, autotestes de parsers, controles e diagnóstico TCP 80. Aguardando compilação/teste pelo proprietário.

## 1. Decisão de plataforma e viabilidade — bloqueia as etapas seguintes

- [ ] Confirmar modelo de placa, versão da dashboard, RGH, Aurora/XeXMenu e ambiente efetivamente disponível para homebrew (sem expor CPU Key ou KV).
- [ ] Confirmar rede local do console: link Ethernet/Wi-Fi, DHCP, endereço IP, gateway, DNS, acesso à internet e relógio.
- [ ] Investigar por que a conexão TCP anterior no RGH retornou `10060`, identificando se timeout ocorre por rota, bloqueio ou segurança XNet Retail.
- [ ] Testar DNS pelo resolvedor da dashboard e consultas UDP por IP sem alterar automaticamente os DNS do console.
- [ ] Testar TCP com destinos de diagnóstico distintos, além de `xbox.com`, mantendo timeout e motivo da falha separados.
- [ ] Testar envio **e recepção** UDP, já que WebRTC depende de comunicação bidirecional e NAT.
- [ ] Verificar obtenção de IP local, interface ativa e gateway; identificar NAT e estabilidade da conexão.
- [ ] Verificar se bibliotecas XDK/XNet Retail permitem tráfego externo arbitrário em um título RGH sem depender de serviços Xbox Live legados.
- [ ] Tomar a decisão técnica **XEX/XNet ou ELF/libxenon/lwIP** com base em testes no console, não em resultados de emulador.
- [ ] Se XNet bloquear tráfego: preparar toolchain libxenon, criar prova de conceito `xenon.elf`, testar DHCP/DNS/TCP/UDP e planejar uma camada compartilhada entre ambientes.
- [ ] Confirmar que nenhuma solução proposta exige PC intermediário, proxy de streaming ou modificação arriscada de NAND.

**Marco M1 — Rede viável:** o console troca pacotes TCP e UDP com destinos externos conhecidos, com resultados repetíveis e logs compreensíveis. Sem M1, não começar OAuth real.

## Fundações novas adicionadas ao código (ainda sem compilação nesta revisão)

- [x] `http_wire.cpp`: montagem de solicitações HTTP/1.1 e validação de cabeçalhos, sem rede/TLS.
- [x] `json_fields.cpp`: extração limitada de campos JSON para respostas de dispositivo OAuth.
- [x] `oauth_device.cpp`: criação local de formulário OAuth Device Code e interpretação de resposta, **sem conexão, sem credenciais e sem login real**.
- [x] `rtp_packet.cpp`: leitura estrutural de pacotes RTP previamente autenticados/descriptografados, **sem WebRTC/SRTP**.
- [x] `h264_nal.cpp`: identificação de unidades H.264 Annex B, **não é decodificador de vídeo**.
- [x] `protocol_selftest.cpp`: verificação offline desses parsers, exibida na interface. **A executar no RGH após sua compilação.**
- [x] `controller_input.cpp`: leitura integral do controle Xbox 360 (botões, gatilhos e analógicos) para o futuro canal de entrada; **ainda sem envio pela nuvem e sem teste após esta alteração**.
- [ ] Corrigir quaisquer erros de compilação/inicialização dos módulos acima a partir do resultado do usuário.

## 2. Transporte seguro — HTTPS/TLS moderno

- [ ] Escolher uma biblioteca TLS que possa ser portada legalmente para PowerPC big-endian, com tamanho e dependências aceitáveis.
- [ ] Adaptar sockets/TCP a essa biblioteca; implementar timeouts, reconexão e tratamento de `EWOULDBLOCK`.
- [ ] Criar uma fonte de entropia/aleatoriedade criptográfica adequada para o console.
- [ ] Implementar relógio confiável: data/hora correta, expiração de certificados/tokens e estratégia de sincronização.
- [ ] Implementar TLS com SNI, versões e cifras exigidas pelos servidores acessados.
- [ ] Validar cadeias de certificados, nome do host, certificados raiz, validade e políticas de revogação quando aplicáveis. **Não desativar verificação TLS.**
- [ ] Construir um cliente HTTPS mínimo com HTTP/1.1 (métodos, cabeçalhos, redirects com segurança, `Content-Length`, chunked, erros, limites de tamanho).
- [ ] Suportar JSON com parser robusto, `UTF-8`, codificação URL/form e respostas de erro.
- [ ] Verificar necessidade de HTTP/2, WebSocket ou outros transportes **conforme os endpoints autorizados realmente exigirem**.
- [ ] Fazer `GET/HEAD` HTTPS de teste sem credenciais e registrar o status HTTP sem registrar conteúdo sensível.
- [ ] Testar TLS/HTTP com conexão ruim, perda de pacotes, DNS falhando, certificado inválido e horário incorreto.

**Marco M2 — HTTPS real:** um pedido HTTPS sai do PowerPC para um servidor público e retorna resposta válida **com certificado corretamente verificado**.

## 3. Login Microsoft seguro e autorização xCloud

- [ ] Confirmar se existe **fluxo oficialmente permitido** de autenticação e de acesso às APIs do Xbox Cloud Gaming para um cliente homebrew independente.
- [ ] Definir aplicativo cliente público registrado, ID, autoridade e escopos autorizados **sem inventar permissões Xbox indisponíveis**.
- [ ] Implementar `OAuth 2.0 Device Authorization Grant` se autorizado: pedido do código de dispositivo.
- [ ] Mostrar `verification_uri`, `user_code`, prazo de expiração e instruções na TV.
- [ ] Gerar QR Code **somente a partir da URL/código fornecidos por uma resposta válida**; usar o celular para autenticar.
- [ ] Implementar polling do endpoint de token respeitando `interval`, expiração, `authorization_pending`, `slow_down`, rejeição e cancelamento.
- [ ] Separar credenciais Microsoft de autorização específica Xbox/xCloud; **login Microsoft bem-sucedido não garante sessão de cloud gaming**.
- [ ] Gerenciar access tokens, refresh tokens (se houver), validade, renovação, cancelamento e logout.
- [ ] Guardar tokens com proteção apropriada ou exigir novo login; **nunca senha de Microsoft em TXT, código-fonte, log ou GitHub**.
- [ ] Provar, com um escopo legalmente concedido, que o cliente PowerPC consegue autenticar e efetuar uma requisição autorizada.
- [ ] Obter acesso autorizado às APIs de catálogo e sessão xCloud, conforme a disponibilidade pública e os termos do serviço.
- [ ] Lidar com conta pessoal, região, ofertas/entitlements, MFA, restrições de idade e mensagens do serviço, sem contorná-las.

**Marco M3 — Autenticação autorizada:** usuário entra pelo celular e o console recebe credenciais válidas e autorizadas para as ações relevantes. Se o serviço não permitir cliente de terceiros, o plano de uso real do xCloud pode parar aqui.

## 4. Descoberta e abertura de sessão do Fortnite

- [ ] Identificar os endpoints atuais e legais de descoberta de jogos e regiões; não depender de URLs extraídas de versões antigas.
- [ ] Validar catálogo, disponibilidade e autorização do Fortnite para a conta/região do usuário.
- [ ] Obter identificador de título/sessão por API autorizada, sem fixar identificadores não confirmados.
- [ ] Implementar criação de sessão, seleção de servidor/região e tratamento de filas, capacidade e indisponibilidade.
- [ ] Implementar o fluxo de negociação/signaling exigido pelo serviço (mensagens autenticadas, ciclo de vida, heartbeat/keepalive).
- [ ] Implementar cancelamento, encerramento, reconexão e limpeza de sessão, inclusive se o aplicativo travar.
- [ ] Testar uma sessão autorizada sem vídeo inicialmente: receber resposta válida do servidor e estados de sessão.
- [ ] Tratar mudanças de protocolo e incompatibilidade de versão sem permitir que o cliente falhe silenciosamente.

**Marco M4 — Sessão iniciada:** a Microsoft confirma uma sessão utilizável e autorizada, mesmo antes de decodificar vídeo.

## 5. WebRTC e transporte de mídia/controles

- [ ] Investigar **quais** protocolos e codecs o serviço negocia atualmente; não assumir que tudo usa WebRTC ou H.264 em todas as regiões.
- [ ] Implementar SDP/ofertas/respostas e controle de estados de negociação, se o serviço utilizar WebRTC.
- [ ] Implementar descoberta ICE, STUN, NAT traversal, candidatos e seleção de caminho; avaliar TURN se autorizado/necessário.
- [ ] Implementar segurança DTLS e SRTP com bibliotecas compatíveis e verificação correta, sem desativar proteção.
- [ ] Implementar recepção/transmissão RTP/RTCP, controle de jitter, perda, retransmissão/NACK, PLI/FIR quando negociados e estatísticas.
- [ ] Implementar canais de dados (por exemplo SCTP/DataChannel) **se** o protocolo de controles exigir.
- [ ] Converter XInput do Xbox 360 no formato de entrada esperado pela sessão e receber rumble/feedback quando disponível.
- [ ] Implementar periodicidade de envio de controle, timestamps, reconexão e controle de latência.
- [ ] Testar tráfego de sessão em rede variável, NAT restritivo e troca de região, sem introduzir tráfego não autorizado.

**Marco M5 — Transporte de mídia:** sessão segura negocia fluxos válidos e os pacotes começam a chegar ao PowerPC (mesmo que ainda sem imagem).

## 6. Vídeo na GPU e áudio do Xbox 360

- [ ] Verificar os codecs realmente oferecidos pelo serviço e escolher um codec suportável pelo hardware (H.264 quando negociado, se viável).
- [ ] Estudar acesso a aceleração de decodificação de vídeo do Xbox 360 via homebrew/XDK; não presumir API pública ou hardware decoder acessível.
- [ ] Se não houver aceleração apropriada, medir um decoder de software PowerPC/VMX antes de prometer resolução ou FPS.
- [ ] Implementar parser de pacotes, buffers, montagem de frames e sincronização com timestamps.
- [ ] Converter frames decodificados para texturas/superfícies da GPU Xenos, respeitando formato e endianness.
- [ ] Medir consumo de memória (Xbox 360 tem **512 MB de RAM compartilhada**) e evitar cópias desnecessárias.
- [ ] Implementar gerenciamento de buffers, fila de renderização, resolução, escala de imagem, mudança de orientação e tela cheia.
- [ ] Implementar áudio no codec negociado (por exemplo Opus se for oferecido), decodificação, buffer e saída pelo sistema de som.
- [ ] Sincronizar áudio/vídeo e compensar jitter, queda de frames e mudanças de bitrate.
- [ ] Provar primeiro **um frame estático**, depois vídeo contínuo, depois som e controles simultâneos no console real.
- [ ] Medir uso de CPU/GPU, FPS, resolução, latência ponta a ponta e temperatura; decidir uma qualidade alvo realista.

**Marco M6 — Reprodução:** vídeo e áudio da sessão aparecem na TV do Xbox 360 de maneira contínua, dentro dos limites do hardware.

## 7. Experiência jogável, interface e robustez

- [ ] Criar estados claros: inicialização, diagnóstico, login, aguardando usuário, catálogo, fila, conectando, streaming e erro.
- [ ] Criar botão de Fortnite **somente quando realmente abrir uma sessão**, sem fingir lançamento.
- [ ] Criar UI de QR Code, confirmação de login, tempo limite e cancelar.
- [ ] Implementar navegação por controle, botões, analógicos, gatilhos, vibração e mapeamento customizável.
- [ ] Oferecer indicador de qualidade de rede, perda, latência, bitrate, FPS e mensagens de erro traduzidas.
- [ ] Tratar desconexão de cabo/rede, mudança de DNS, token expirado e sessão interrompida sem travar a dashboard.
- [ ] Gravar logs locais opcionais sem tokens, senhas, PII ou cookies; exportar diagnóstico por USB, se desejado.
- [ ] Ter limites para cada resposta de rede e parser, memória e tempo de execução; verificar entrada não confiável.
- [ ] Validar encerramento limpo, recuperação após travamento, respeito a limites do sistema e segurança do RGH.
- [ ] Repetir testes com diferentes Xbox 360, versões de Aurora/dashboard e tipos de saída de vídeo/áudio.

**Marco M7 — Fortnite jogável:** vídeo + áudio + controle + rede em uma sessão autorizada, por vários minutos, **sem PC intermediário**.

## 8. Qualidade, distribuição e manutenção

- [ ] Reorganizar módulos em camadas testáveis: `platform`, `net`, `tls`, `auth`, `session`, `webrtc`, `video`, `audio`, `input`, `ui`.
- [ ] Definir interfaces/abstrações entre componentes para trocar XNet por libxenon/lwIP se necessário.
- [ ] Criar testes unitários de DNS, HTTP, JSON, SDP, RTP, buffers e parsers, preferencialmente sem exigir o console.
- [ ] Criar testes de integração no **RGH físico** e registrar resultado por compilação e versão.
- [ ] Criar `Release` repetível, cabeçalho Retail verificado e pacote de distribuição com dependências corretas.
- [ ] Criar checklist pré-publicação para remover XDK, SDK binário, tokens, certificados privados e outros arquivos sem permissão de redistribuição.
- [ ] Confirmar licenças das bibliotecas de TLS/WebRTC/codecs e escolher uma licença para o código próprio.
- [ ] Documentar riscos de serviço de terceiros, requisitos, limitações e casos sem acesso autorizado.
- [ ] Atualizar README e roadmap com resultados reais, sem chamar diagnóstico TCP de streaming funcional.
- [ ] Manter o código em GitHub sem enviar credenciais nem binários proprietários.

## Ordem recomendada para os próximos desenvolvimentos

1. **Primeiro:** receber um teste completo v0.9 no Xbox 360 físico e isolar o bloqueio `10060`.
2. **Se necessário:** desenvolver uma prova de rede ELF XeLL/libxenon com lwIP para evitar limitações XNet — uma trilha experimental separada, sem prometer que o XEX existente rodará no XeLL.
3. **Depois:** estabelecer HTTPS moderno validado diretamente no Xenon.
4. **Então:** verificar autorização viável ao serviço e implementar OAuth Device Code com código/QR, sem senhas.
5. **Depois:** negociar uma sessão autorizada do Fortnite.
6. **Só então:** mídia WebRTC, decodificação H.264/áudio e entrada do controle.
7. **Por último:** otimizar latência, estabilidade, interface e distribuição.

## Critérios de bloqueio (não passar adiante antes de resolver)

- **M1 não passou:** não iniciar login real; o console nem consegue transportar dados necessários.
- **M2 não passou:** não enviar credenciais nem tokens; HTTPS sem certificados validados é inseguro.
- **M3 não passou:** não simular sessões não autorizadas; as permissões do Xbox Cloud Gaming precisam ser reais.
- **M4/M5 não passaram:** não declarar Fortnite em execução porque a UI mostra um botão ou logo.
- **M6 não passou:** a recepção de pacotes sem imagem/som ainda não é uma experiência de jogo.
- **M7 não passou:** não anunciar o projeto como cliente funcional.

## Fontes de estudo

- [Device authorization grant oficial da Microsoft](https://learn.microsoft.com/en-us/azure/active-directory/develop/v2-oauth2-device-code) — descreve login de dispositivos limitados, **não concede por si só acesso ao xCloud**.
- [Greenlight](https://github.com/unknownskl/greenlight) — arquitetura de clientes de streaming em sistemas modernos.
- [libxenon (Free60)](https://github.com/Free60Project/libxenon) — projeto bare-metal PowerPC do Xbox 360.
- [XeLL](https://free60.org/Homebrew/Tools/XeLL/) — carregador de homebrew ELF para o console.

**Última revisão:** 20/10/2026. Documento de planejamento; nenhum código adicional foi compilado nesta revisão.
