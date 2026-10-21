# Credenciais e leitura HTTPS — 21/10/2026

Esta revisão prepara o login para a futura sessão de jogo. Não implementa streaming.

## Alterações

- `CloudCredentials` mantém gsToken e hostname regional somente na thread de autenticação, sem cópias na interface, nos logs ou em disco.
- A região padrão válida é priorizada; na ausência dela, usa-se a primeira região HTTPS Xbox válida.
- A validade usa o relógio monotônico de 32 bits, inclusive sua virada. A retenção é limitada à validade fornecida pelo serviço e a no máximo 24 horas. Não há renovação automática.
- Cancelar, expirar, falhar uma nova troca ou destruir o proprietário apaga a credencial.
- O cancelamento é consultado antes de cada próxima requisição Xbox/XSTS/xCloud e antes de aceitar o token. Uma chamada HTTPS já em andamento ainda depende de seus timeouts.
- O fluxo apaga as credenciais Microsoft/device após a troca; apenas gsToken permanece enquanto a tela está autenticada.
- A interface evita iniciar outro login enquanto a credencial atual está mantida.
- Respostas com Content-Length completo ou chunked completo encerram a leitura HTTPS sem esperar EOF. Codificações não suportadas, cabeçalhos de transferência duplicados e combinações ambíguas são rejeitados.

O leitor chunked continua sem suporte a extensões e trailers. Respostas sem tamanho explícito continuam dependendo de EOF.

## Verificação reproduzível

Execute no Windows com Visual Studio 2010:

```powershell
.\tests\run_host_tests.ps1
```

O parâmetro `-VcVars` permite indicar outra localização do ambiente do compilador compatível. Os testes usam respostas simuladas, sem conta e sem rede.

Cobertura: cadeia Xbox/XSTS/xCloud, preferência de região, expiração exata, virada do relógio, limpeza de credenciais, cancelamento em todas as fronteiras da troca, falhas TLS/HTTP, validade fracionária inválida, respostas fragmentadas, chunked e cabeçalhos HTTP ambíguos.

## Pendências reais

| Etapa | Situação |
| --- | --- |
| Consentimento e login real | Não validados nesta revisão |
| TCP 10060 no hardware | Ainda não diagnosticado com esta revisão |
| Credenciais em memória | Implementadas; ainda sem consumidor de sessão |
| Criar partida, fila, conectar e encerrar | Não implementados |
| WebRTC / ICE / DTLS / SRTP / SCTP | Sem transporte integrado |
| Vídeo H.264 / áudio Opus | Sem decodificação e reprodução integradas |
| Controle remoto | Sem canal real conectado |
| Xenia e Xbox 360 físico | Revisão ainda não executada |

A correção da espera HTTP só atua depois de estabelecer TLS e receber uma resposta. Ela não corrige automaticamente um timeout na conexão TCP inicial. Para localizar esse bloqueio, registrar etapa, código WSA/TLS, flags de rede e resultado DNS no console, sem publicar credenciais.

Não se aloca uma máquina de jogo nesta revisão. O próximo módulo precisa consumir a credencial na mesma thread, verificar sua validade antes de cada requisição, tratar os estados de fila e encerrar a sessão no cancelamento. Depois ainda serão necessários negociação WebRTC, decodificadores e testes reais.

## Resultado desta compilação

Testes simulados MSVC 2010: passaram. Compilação PowerPC Release e verificação Retail: passaram. Não executado no Xenia ou Xbox 360 físico.

SHA-256: 4A51BAFC00AA898709314051B659B9F897562DDF6E3B27F438602F23E5B85E36
