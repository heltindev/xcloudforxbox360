# XCloud360

Cliente experimental de Xbox Cloud Gaming para Xbox 360 RGH/JTAG, desenvolvido em C++ para PowerPC como aplicativo nativo `.xex`.

**Ainda não reproduz jogos do xCloud.** O projeto contém interface, diagnósticos de rede e código de autenticação. Sessão de jogo, WebRTC e reprodução de áudio/vídeo ainda precisam ser implementados.

O objetivo é conectar o Xbox 360 diretamente ao serviço, **sem PC intermediário durante o uso**. O computador é usado para desenvolver e compilar; o login por código exige um navegador no celular ou em outro dispositivo. Projeto independente, sem vínculo com Microsoft, Xbox ou Epic Games.

## Interface e funcionamento direto

A interface do XEX usa fundo escuro, destaques verdes e poucos textos: Fortnite, entrada da conta e status. No login aparecem o endereço oficial, o código e o tempo restante. Diagnósticos de falha continuam disponíveis quando necessários.

**Requisito do projeto:** toda conexão de streaming deve ocorrer no Xbox 360. A implementação não usa PC, proxy ou retransmissor externo para processar o jogo. O PC serve para compilação e testes; o navegador do celular serve para aprovar o login. Esse é o objetivo de arquitetura, não uma confirmação de streaming funcional nesta versão.

## Estado atual — 21/10/2026

| Componente | Implementação e validação |
| --- | --- |
| Interface nativa | Tema verde, poucos textos, seleção de Fortnite e tela de login por código |
| Renderização | Solicita 1080p, com alternativas em 720p/480p; fonte incorporada no XEX durante a compilação local |
| Microsoft Device Code | Implementado com HTTPS; versões anteriores obtiveram código real no Xenia |
| Xbox User / XSTS | Código implementado, com leitura de respostas JSON aninhadas e testes simulados |
| Login no serviço xCloud | Novo encadeamento após XSTS válido; endpoint `/v2/login/user`, validação de token, validade e regiões retornadas |
| Sessão de jogo | Não implementada |
| WebRTC / ICE / DTLS / SRTP / SCTP | Não implementados como transporte funcional |
| Vídeo H.264 e áudio Opus | Sem decodificadores e reprodução integrados |
| Controle remoto | Formatadores e testes locais; sem canal de gameplay ativo |
| Xbox 360 físico | Última revisão não testada; timeout TCP 10060 relatado anteriormente continua sem validação de correção |

**Fortnite é apenas a entrada disponível na interface. Selecioná-lo inicia os testes e a autenticação, não uma partida.**

## Alterações mais recentes

- Interface simplificada e paleta verde.
- Correção do parser Xbox User/XSTS: respostas reais contêm objetos e listas, como `DisplayClaims`, rejeitados pelo antigo leitor de JSON plano.
- Tentativa nativa de login no serviço xCloud após obtenção de XSTS para `http://gssv.xboxlive.com/`.
- Requisição HTTPS ao endereço fixo `https://xgpuweb.gssv-play-prod.xboxlive.com/v2/login/user`, com `offeringId: xgpuweb` e cabeçalho `x-gssv-client: XboxComBrowser`.
- Validação da resposta de login e de pelo menos uma região HTTPS do domínio Xbox. Tokens ficam temporariamente na memória do fluxo e são apagados; esta etapa ainda não mantém uma sessão jogável.
- Diagnósticos de HTTP e `XErr` na tela quando uma etapa de serviço falha.

Esse endpoint é uma integração experimental baseada na referência de protocolo disponível no projeto, sem garantia de compatibilidade ou autorização pelo serviço.

## O que foi testado

| Verificação da revisão atual | Resultado |
| --- | --- |
| Compilação PowerPC Release com XDK | Passou |
| Verificação do XEX Retail | Passou |
| Testes simulados em Windows/MSVC 2010 | Passaram: Xbox → XSTS → login xCloud, montagem HTTP, recusa TLS, HTTP 403, região inválida e JSON inválido |
| Execução da revisão atual no Xenia | Não realizada nesta revisão |
| Consentimento real e Xbox User/XSTS/login xCloud da revisão atual | Não testados |
| Imagem, áudio e gameplay reais | Não implementados |

Resultados de versões anteriores no Xenia não comprovam o funcionamento desta revisão nem a compatibilidade com o console físico. Os testes simulados não acessam uma conta e não comprovam streaming.

SHA-256 do XEX compilado localmente nesta revisão:

```text
4564BADDC065991536FC183EC32DE54E464ACFCB032A5102D8BCB2DA0431C1A2
```

O binário não faz parte desta pasta de código-fonte. Detalhes em [login xCloud nativo](docs/CLOUD_LOGIN_NATIVE_2026-10-21.md).

## Compilação

É necessário configurar localmente o Xbox 360 SDK/XDK e as ferramentas de compilação compatíveis com o projeto Visual Studio 2010. `XEDK` deve apontar para a instalação do SDK. O script também espera `tools/xextool/XexTool-windows-x64/XexTool.exe`, que não é distribuído nesta pasta.

```powershell
Copy-Item .\auth_config.example.h .\auth_config.h
# Configure seu próprio Application (client) ID no arquivo local.
# Para solicitar acesso Xbox, defina XCLOUD360_ENABLE_XBOX_LIVE_SCOPE como 1.
.\build_release.ps1
```

O exemplo público mantém a solicitação Xbox desativada (`0`). Ativá-la solicita `xboxlive.signin`; a Microsoft precisa permitir e conceder o escopo. O serviço pode recusar o aplicativo ou a conta. Não substitua o ID por um de outro aplicativo.

Saída local: `Release/XCloud360.xex`. A fonte é incorporada durante o build. O script `generate_hd_font.ps1` pode gerar o atlas de maior definição quando as ferramentas e fontes locais necessárias estão disponíveis. Arquivos de fontes, SDK e ferramentas proprietárias não são incluídos no código público.

## Uso experimental

1. Execute o `.xex` em um ambiente compatível de testes.
2. Pressione **A** para iniciar a verificação de rede e solicitar o código Microsoft.
3. Se o servidor emitir um código, use exatamente o endereço mostrado e confirme a conta no navegador.
4. Se a Microsoft conceder o escopo Xbox, o fluxo tentará Xbox User, XSTS e login no serviço xCloud.
5. Consulte a etapa e os códigos de erro apresentados. Mesmo que o login seja confirmado, esta versão ainda não inicia o jogo.

| Botão | Ação |
| --- | --- |
| A | Entrar ou tentar novamente |
| B | Cancelar e voltar |
| BACK | Sair |
| X, na tela de login | Repetir diagnóstico de rede |
| Y | Repetir testes locais |

Não informe senha Microsoft no aplicativo nem publique códigos de login, tokens ou a configuração local.

## Organização do código

| Caminho | Função |
| --- | --- |
| `src/app.cpp`, `src/harissa_style_ui.*` | Interface e navegação |
| `src/platform_xbox360.cpp` | Renderização e entrada do Xbox 360 |
| `src/xbox_account.cpp`, `src/xbox_device_flow.cpp` | Login Microsoft em segundo plano |
| `src/xbox_live_exchange.cpp` | Xbox User, XSTS e tentativa de login xCloud |
| `src/xboxtls_backend.cpp`, `src/http_wire.cpp` | HTTPS e montagem de requisições |
| `src/region_catalog.cpp` | Validação das regiões do serviço |
| `src/xcloud_*`, `src/h264_rtp_receiver.cpp` | Experimentos de protocolos, controle e remontagem RTP/H.264; não incluem um player funcional |
| `src/protocol_selftest.cpp` | Testes locais executados na inicialização do aplicativo |
| `third_party/` | Dependências e respectivas licenças |
| `docs/` | Relatórios e registros técnicos, incluindo marcos históricos |

## Próximas etapas

Validar a autorização real e a rede do console, implementar o ciclo de vida da sessão xCloud e integrar WebRTC, decodificadores de vídeo/áudio e o canal de controle. Um login confirmado não substitui essas etapas.

Consulte [status](STATUS_DO_PROJETO.md), [roadmap](ROADMAP.md) e [referências](docs/REFERENCES.md). Registros antigos em `docs/` descrevem a versão da data indicada; não são confirmação de funcionamento da revisão atual.
