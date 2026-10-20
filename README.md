# XCloud360 — Xbox Cloud Gaming no Xbox 360 (PowerPC / RGH3)

> **Status: protótipo experimental v0.9.** Executável homebrew `.xex` nativo para o Xbox 360 desbloqueado (RGH/RGH 3). **Não executa Fortnite e não se conecta a sessões do xCloud nesta versão.** Não é afiliado à Microsoft, Xbox ou Epic Games.

## Objetivo

Desenvolver um cliente Xbox Cloud Gaming que rode **diretamente no processador Xenon (PowerPC) do Xbox 360**, sem usar PC intermediário. O console deverá autenticar a conta de maneira autorizada, estabelecer uma sessão de streaming, receber vídeo/áudio e enviar entradas do controle.

A implementação atual comprova a compilação de um aplicativo PowerPC para RGH, desenha uma interface e realiza **diagnósticos de DNS, UDP e TCP**. Não existe navegador web, OAuth, TLS moderno ou WebRTC funcional.

## Estado real dos componentes

| Componente | Estado | Próxima validação |
| --- | --- | --- |
| C++/PowerPC e formato `.xex` Retail | Implementado, build verificado | Executar build atual no RGH físico |
| Interface Direct3D / controle Xbox 360 | Implementado | Verificar fonte e entradas no console |
| DNS via XNet | Implementado como diagnóstico | Confirmar resposta do DNS configurado no Xbox |
| DNS UDP via 8.8.8.8 / 8.8.4.4 | Implementado como fallback experimental | Verificar se recebe pacotes no RGH |
| TCP porta 443 | Diagnóstico implementado | Investigar timeout `10060` no RGH |
| Transporte UDP/TCP sem restrições | **Não demonstrado** | Avaliar limites do XNet Retail e alternativa XeLL/libxenon |
| TLS moderno e validação de certificados | **Não implementado** | Selecionar biblioteca TLS portada para PowerPC |
| Microsoft OAuth Device Code / QR | **Não implementado** | Registrar app autorizado e confirmar suporte do serviço |
| Autorização para sessões xCloud | **Não implementado** | Investigar API e termos aplicáveis |
| WebRTC (ICE/DTLS/SRTP/data channel) | **Não implementado** | Portar/implementar protocolos e testar no hardware |
| Decodificação H.264/áudio em tempo real | **Não implementado** | Validar APIs e desempenho do Xbox 360 |
| Fortnite jogável pela nuvem | **Não disponível** | Depende de todos os módulos anteriores |

## Estrutura do repositório

```text
XCLOUDFORXBOX360/
├── src/                        # Código C++ nativo para PowerPC Xbox 360
├── docs/
│   ├── POWERPC.md              # Arquitetura e requisitos
│   ├── RGH_NETWORK.md          # Diagnósticos de rede e erros
│   ├── XELL_ALTERNATIVE.md     # Alternativa via XeLL/libxenon
│   └── fortnite-architecture.md
├── XCLOUDFORXBOX360.sln
├── XCloud360.vcxproj
├── build_release.ps1           # Gera .xex Retail e pacote para RGH
├── package_github.ps1          # Cria ZIP somente com fontes e documentação
├── .gitignore
└── README.md
```

**Não publique** executáveis proprietários do SDK, a pasta `tools/`, arquivos `.xpr` do XDK, `obj/`, `bin/`, `Release/` ou `dist/`. O ZIP de código-fonte é gerado separadamente do ZIP para instalar no console.

## Pré-requisitos de compilação

- Windows com Xbox 360 SDK/XDK e compilador Xbox 360 para PowerPC, usados com a devida autorização.
- MSBuild compatível e variável de ambiente `XEDK` configurada.
- [XexTool](https://github.com/Team-Resurgent/XexTool/releases), instalado **localmente** em `tools/xextool/XexTool-windows-x64/XexTool.exe`; não incluído no repositório.
- Bundler e recursos de fonte instalados no próprio XDK. A compilação gera localmente a fonte `.xpr`; o código mantém um fallback bitmap se a fonte externa não carregar.

### Compilar para RGH 3

Abra PowerShell na pasta do projeto:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1
```

O script gera um `.xex` **Retail** em `Release/XCloud360.xex`, um pacote em `dist/` e `XCloud360-RGH3.zip`.

### Instalar e testar no Xbox 360 físico

1. Extraia **`XCloud360-RGH3.zip`** para HD ou USB do Xbox 360.
2. Mantenha `XCloud360.xex` e `Media/Fonts/SegoeUI_24.xpr` na estrutura original; a fonte bitmap embutida é um fallback.
3. Inicie o `.xex` pelo Aurora ou XeXMenu.
4. Pressione **A** para testar a rede, **B** para voltar e **BACK** para sair.
5. Registre **DNS Xbox, DNS UDP, ERRO XBOX, ERRO IP, ETAPA e ORIGEM**. Os resultados mais importantes são os obtidos no **console real**.

**DNS:** o Xbox pode usar DNS automático ou os endereços manuais `8.8.8.8` / `8.8.4.4`. O aplicativo **não altera as configurações do console**. O fallback UDP consulta esses endereços como teste somente quando o resolvedor XNet falha.

**Histórico observado no console:** uma versão anterior retornou `10060` (timeout na tentativa TCP). Isso não demonstra falta de internet nem garante que trocar DNS resolverá. O XNet Retail pode impor restrições a destinos externos.

A explicação das etapas e possíveis erros está em [docs/RGH_NETWORK.md](docs/RGH_NETWORK.md).

## O que falta para conectar ao xCloud de verdade

**Primeiro:** validar acesso de rede do Xbox 360 RGH a servidores externos, incluindo DNS, TCP e UDP. **Depois:** TLS moderno com verificação de certificado e hostname (nunca desativar essa segurança), OAuth autorizado sem senha em TXT, negociação de sessão e protocolos WebRTC, decodificação de áudio/vídeo e envio dos controles. Mesmo com o login Microsoft funcionando, o acesso ao streaming não está garantido; dependerá das APIs/autorizações do serviço e da viabilidade técnica no hardware.

Se XNet Retail impedir rede externa, a rota de pesquisa é **XeLL Reloaded/libxenon com lwIP**, que exigirá outra cadeia de compilação e produzirá ELF em vez de XEX. Não é necessário modificar NAND ou atualizar XeLL para simplesmente avaliar esta alternativa. Veja [docs/XELL_ALTERNATIVE.md](docs/XELL_ALTERNATIVE.md).

Referências: [Greenlight](https://github.com/unknownskl/greenlight) (pesquisa de protocolos, não port direto para PowerPC), [Better xCloud](https://github.com/redphx/better-xcloud), [libxenon](https://github.com/Free60Project/libxenon).

## Colaborar

Issues e pull requests sobre **PowerPC, rede RGH, TLS, autenticação autorizada e vídeo** são bem-vindos. Informe a versão do XEX, se o teste ocorreu no Xbox 360 físico, erro e etapa. Não publique credenciais, tokens, executáveis de SDK, arquivos de firmware ou conteúdo de jogos.

A licença do código deve ser escolhida pelo mantenedor antes de redistribuição por terceiros.
