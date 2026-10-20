# XCloud360

**Protótipo experimental de um cliente Xbox Cloud Gaming para Xbox 360 com RGH/RGH 3.**

O objetivo de longo prazo é investigar a possibilidade de acessar o Xbox Cloud Gaming diretamente pelo Xbox 360, sem um computador intermediário. Hoje o projeto **ainda não abre o site do xCloud, não autentica contas Microsoft e não transmite jogos**. A versão atual, **v0.8**, é um aplicativo nativo `.xex` com interface, suporte ao controle e diagnósticos de DNS/TCP.

> [!IMPORTANT]
> Este é um projeto de pesquisa e desenvolvimento homebrew, **não um cliente xCloud funcional**. Não é afiliado à Microsoft ou à Xbox. Não coloque senhas, tokens ou dados de conta Microsoft nos arquivos do projeto.

![Interface XCloud360 v0.8 no Xenia Canary](docs/xenia-preview.png)

## Estado do projeto

| Recurso | Estado |
| --- | --- |
| Aplicativo nativo PowerPC / `.xex` | Compila |
| Execução e interface no Xenia Canary | Verificadas visualmente |
| Build para RGH 3 (cabeçalho Retail) | Gerado e verificado com XexTool |
| Interface com Segoe UI suavizada | Funciona no Xenia; teste no RGH pendente |
| Controles A, B e BACK | Implementados |
| Diagnóstico DNS e conexão TCP porta 443 | Implementado; resultados diferentes entre Xenia e RGH |
| HTTPS/TLS moderno com verificação de certificados | Não implementado |
| Login Microsoft por OAuth Device Code/QR | Não implementado |
| WebRTC, vídeo H.264, áudio e controles para xCloud | Não implementados |
| Fortnite jogável pelo xCloud | **Não disponível** |

## Estrutura do repositório

```text
XCLOUDFORXBOX360/
├── src/                     # Aplicativo C++ (interface, vídeo, controles, testes TCP)
├── docs/xenia-preview.png   # Captura real da interface no Xenia
├── XCloud360.vcxproj         # Projeto Xbox 360 / Visual Studio
├── XCLOUDFORXBOX360.sln      # Solução Visual Studio 2010
├── build_release.ps1        # Compila, converte para Retail e cria ZIP
├── test_xenia.ps1           # Recompila apenas se necessário e inicia o Xenia
├── run_xenia.ps1            # Abre o último .xex compilado no Xenia
├── .gitignore
└── README.md
```

As pastas `obj/`, `bin/`, `Release/`, `dist/`, o executável `.xex`, a fonte `.xpr` gerada e as ferramentas binárias locais **não são incluídos no repositório**.

## Requisitos para compilar

- Windows com ambiente **Visual Studio 2010 / Xbox 360 SDK (XDK)** configurado e `XEDK` definido.
- MSBuild 4.x compatível com o projeto Xbox 360.
- [XexTool (Team Resurgent)](https://github.com/Team-Resurgent/XexTool/releases) para transformar a saída de desenvolvimento em um arquivo destinado ao RGH. Baixe a versão para Windows x64 e coloque o executável em `tools/xextool/XexTool-windows-x64/XexTool.exe`.
- A fonte Segoe UI para Xbox 360 é gerada **localmente**, utilizando o Bundler e os recursos do SDK. Esses arquivos proprietários não são distribuídos no GitHub.
- [Xenia Canary](https://github.com/xenia-canary/xenia-canary), opcional para testes de interface. O script pressupõe `Documents/xenia_canary_windows/xenia_canary.exe`; ajuste o caminho, se necessário.

Use apenas ferramentas e SDKs que você esteja autorizado a utilizar.

### Compilar para RGH 3

No PowerShell, dentro da pasta do projeto:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build_release.ps1
```

O script compila a biblioteca ATG se estiver faltando, gera os recursos de fonte a partir do SDK, compila o PowerPC, converte o `.xex` com o XexTool e cria `XCloud360-RGH3.zip`. **A partir da v0.8, também substitui `Release/XCloud360.xex` por uma cópia Retail verificada**, adequada para exportar diretamente para o RGH 3.

O pacote produzido contém:

```text
dist/
├── XCloud360.xex
├── LEIA_PRIMEIRO.txt
└── Media/Fonts/SegoeUI_24.xpr
```

### Testar rapidamente no Xenia

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\test_xenia.ps1
```

O script reaproveita o último build quando nenhum arquivo relevante mudou. Também é possível usar `run_xenia.ps1` para abrir diretamente o `.xex` já compilado.

**O Xenia não substitui testes no console real.** A emulação de rede, DNS e XHTTP pode estar incompleta e retornar resultados diferentes dos obtidos em um RGH 3.

### Instalar no Xbox 360 RGH / RGH 3

1. Extraia o `XCloud360-RGH3.zip` em uma pasta do HD ou USB.
2. Mantenha `XCloud360.xex` e `Media/Fonts/SegoeUI_24.xpr` na mesma estrutura.
3. Abra o `.xex` pelo Aurora ou XeXMenu.
4. Pressione **A** para iniciar o diagnóstico, **B** para voltar e **BACK** para sair.

Se a fonte externa falhar, há um renderizador bitmap de reserva embutido no executável.

## Diagnóstico de rede

A v0.8 faz duas tentativas de conexão **TCP** à porta 443:

1. **`www.xbox.com`**: resolve DNS e testa endereços retornados.
2. **`1.1.1.1`**: utiliza um IP direto, como teste de controle sem DNS.

**TCP 443 conectado não comprova HTTPS, autenticação, WebRTC ou acesso ao xCloud.**

### Resultados observados

| Ambiente | Resultado |
| --- | --- |
| Xenia Canary | DNS de `www.xbox.com` retornou 0 endereços; o teste por IP `1.1.1.1:443` indicou conexão |
| Xbox 360 RGH 3 (v0.6) | Erro `10060` (timeout) durante `connect`, etapa 5 |
| Xenia Canary (v0.5) | `XHttpOpen` falhou com erro `50` porque a API XHTTP não é totalmente implementada |

No teste mais recente do Xenia, o valor `ERRO XBOX: 1` é usado internamente para representar ausência de endereços DNS; **não** é um código HTTP ou prova de falha do site.

O timeout `10060` no console físico indica que a conexão TCP não foi concluída no prazo. Na v0.8, cada tentativa TCP dispõe de até 6 segundos, em vez dos 2,5 segundos da v0.7. Pode estar relacionado à rota, bloqueios da pilha XNet retail ou características do destino; **não está demonstrado que o DNS é a causa**. Os DNS informados para o Xbox 360 foram `181.213.132.2` (primário e secundário) e a configuração manual proposta foi **Google DNS: `8.8.8.8` primário e `8.8.4.4` secundário**. A aplicação utiliza o resolvedor DNS do Xbox 360; **não precisa nem deve impor essas configurações via código**. A troca é um teste válido, mas não contorna limitações da biblioteca de rede.

> No Xbox 360, a flag `XNET_STARTUP_BYPASS_SECURITY` é documentada para devkits; com bibliotecas retail, pode ser ignorada. Portanto, um RGH não garante conexões TCP arbitrárias a destinos externos.

## Fortnite: objetivo de streaming (ainda experimental)

A descrição dos módulos necessários para chegar à execução real está em [docs/fortnite-architecture.md](docs/fortnite-architecture.md). **A versão atual não abre Fortnite.** O projeto continua focado no Xbox 360 RGH 3; o Xenia é apenas apoio aos testes visuais.

## Próximas etapas de pesquisa

1. Validar a pilha de rede no RGH físico e diferenciar restrições XNet de falhas de conectividade.
2. Investigar transporte TCP/UDP compatível e TLS moderno com verificação de certificados.
3. Avaliar um fluxo OAuth oficialmente autorizado, com código de dispositivo, sem armazenar senhas.
4. Pesquisar protocolos de negociação de sessão, decodificação H.264, áudio e entrada de controle, respeitando limites do hardware e das APIs.
5. Somente então avaliar se Fortnite poderia ser transmitido com latência aceitável.

## Contribuindo

Relatórios de testes no RGH e no Xenia, documentação, correções de código e pesquisas sobre rede do Xbox 360 são bem-vindos. Ao relatar um erro, informe a versão do `.xex`, se foi testado no Xenia ou no console real, e os campos **ETAPA**, **ORIGEM** e **ERRO**.

A licença de código deste repositório ainda precisa ser definida pelo mantenedor. Não inclua arquivos do Xbox 360 SDK, senhas, tokens ou binários proprietários em contribuições.
