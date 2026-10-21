# Roadmap e checklist — 21/10/2026

Objetivo: Xbox Cloud Gaming diretamente no Xbox 360 RGH/JTAG, sem PC intermediário durante o uso.

## Concluído nesta base

- [x] Interface nativa verde, com poucos textos e navegação por controle.
- [x] Código Microsoft Device Code e troca Xbox User/XSTS/login xCloud.
- [x] Leitura de JSON aninhado e validação das regiões retornadas.
- [x] gsToken mantido somente em memória, com expiração e limpeza.
- [x] Cancelamento entre as etapas de autenticação.
- [x] Leitura HTTPS termina ao receber Content-Length ou chunked completos.
- [x] Testes simulados de autenticação, credenciais e HTTP aprovados.
- [x] Compilação PowerPC Release e verificação do XEX Retail aprovadas.

Os checks de código e compilação não significam validação com conta real ou console físico.

## Checklist solicitado

| Item | Situação atual |
| --- | --- |
| ☐ Validar autenticação real | Consentimento Microsoft, Xbox User, XSTS e login xCloud ainda não validados nesta revisão |
| ☐ Validar rede no Xbox 360 | Timeout TCP 10060 ainda sem diagnóstico conclusivo no hardware; correção de leitura HTTP não comprova correção da conexão TCP |
| ☐ Implementar sessão de jogo | Solicitar partida, tratar fila, conectar e encerrar ainda pendentes |
| ◐ Gerenciar credenciais da sessão | Retenção em memória, expiração e limpeza concluídas; consumo por sessão de jogo e renovação ainda pendentes |
| ☐ Integrar WebRTC | Negociação e transporte ICE/DTLS/SRTP/SCTP ainda não integrados |
| ☐ Decodificar vídeo | H.264 ainda sem decodificador/renderização integrados |
| ☐ Reproduzir áudio | Opus e sincronização com vídeo ainda não integrados |
| ☐ Enviar comandos do controle | Formatadores locais existem; falta conectar ao canal remoto real |
| ☐ Testar e otimizar no console | Memória, desempenho, latência, reconexão e estabilidade ainda não validados |

## Próximos passos

- [ ] Validar rede e autenticação reais no Xbox 360.
- [ ] Integrar as credenciais ao ciclo de vida da sessão e definir renovação.
- [ ] Implementar criação, fila, conexão e encerramento da sessão.
- [ ] Integrar WebRTC.
- [ ] Integrar vídeo, áudio e controle.
- [ ] Testar e otimizar no console físico.

Só considerar streaming funcional após uma partida real com imagem, áudio e controle no Xbox 360. A revisão atual ainda não reproduz jogos.

Veja [notas da pré-release](RELEASE_NOTES.md) e [status](STATUS_DO_PROJETO.md).
