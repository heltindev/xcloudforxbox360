# Atualizações

## 2026-10-21 — credenciais em memória e leitura HTTPS

- gsToken e região mantidos na thread de autenticação, com validade limitada a 24 horas e limpeza ao expirar/cancelar.
- Cancelamento entre as etapas Xbox/XSTS/xCloud; tokens intermediários apagados.
- Evita login duplicado enquanto autenticado.
- Respostas HTTP com tamanho conhecido ou chunked completo não dependem do fechamento da conexão.
- Rejeição de codificações HTTP não suportadas e cabeçalhos ambíguos.
- Testes públicos reproduzíveis: expiração, virada do relógio, limpeza, cancelamento, regiões e fragmentação HTTP.
- Ainda sem sessão de jogo, WebRTC e reprodução. O TCP 10060 no hardware não foi validado como corrigido.


## 2026-10-21 — interface verde e autenticação experimental

### Alterações
- Interface verde com menos textos e navegação por controle.
- Tela de login com endereço oficial, código e tempo restante.
- Correção da leitura de respostas Xbox User/XSTS com JSON aninhado.
- Tentativa de login no serviço xCloud depois de um XSTS válido.
- Validação de HTTPS, resposta de login e regiões do serviço.
- Diagnósticos de falha HTTP e XErr.
- README e roadmap alinhados com o estado atual.

### Validação
- Compilação PowerPC Release e verificação do XEX Retail: passaram.
- Testes simulados de autenticação e montagem HTTP: passaram.
- Última revisão visual: não testada no Xenia nem no Xbox 360 físico.
- Login Xbox/XSTS/xCloud real desta revisão: não validado.

### Limitações
**Esta atualização não permite jogar pelo xCloud.** Ainda faltam sessão de jogo, transporte WebRTC e decodificação/reprodução de vídeo e áudio. A arquitetura pretendida conecta o Xbox 360 diretamente ao serviço, sem PC intermediário durante o uso.

Os fontes públicos usam auth_config.example.h. A configuração de conta local, fontes de SDK, ferramentas proprietárias e binários não fazem parte desta atualização de código.
