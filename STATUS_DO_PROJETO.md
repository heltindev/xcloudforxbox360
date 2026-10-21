# Status do projeto — 21/10/2026

**Experimental. Ainda não reproduz jogos do xCloud.**

## Revisão atual
Interface nativa verde e minimalista, fluxo Microsoft Device Code, código Xbox User/XSTS com leitura de JSON aninhado e tentativa de login no serviço xCloud. A comunicação proposta ocorre diretamente no Xbox 360; o PC serve para desenvolver e compilar.

Credenciais xCloud agora têm retenção somente em memória, expiração e limpeza ao cancelar. O leitor HTTPS conclui respostas com tamanho conhecido ou chunked completo sem esperar EOF. Ainda não há módulo de sessão consumindo o token. Veja [detalhes desta revisão](docs/CREDENTIALS_HTTP_2026-10-21.md).

## Validação
| Verificação | Resultado |
| --- | --- |
| Compilação PowerPC Release | Passou |
| Verificação Retail do XEX | Passou |
| Testes simulados da cadeia de autenticação | Passaram |
| Revisão visual atual no Xenia | Não testada |
| Xbox User/XSTS/login xCloud reais nesta revisão | Não validados |
| Xbox 360 físico com esta revisão | Não testado |
| Sessão, WebRTC, vídeo, áudio e gameplay | Não implementados |

SHA-256 do binário compilado localmente:
4A51BAFC00AA898709314051B659B9F897562DDF6E3B27F438602F23E5B85E36

O binário não integra a atualização de fontes. O erro TCP 10060 relatado no hardware continua sem validação de correção.

Documentação: [README](README.md), [alterações](CHANGELOG.md), [roadmap](ROADMAP.md) e [detalhes do login](docs/CLOUD_LOGIN_NATIVE_2026-10-21.md).

## Checklist e publicação

Consulte o [checklist com itens concluídos, parciais e pendentes](ROADMAP.md) e as [notas de pré-release](RELEASE_NOTES.md). Nenhuma release ou tag foi publicada nesta edição.
