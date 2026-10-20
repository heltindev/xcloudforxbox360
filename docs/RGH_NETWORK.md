# Rede nativa no Xbox 360 RGH 3 — diagnóstico v0.9

## Alvo
Testar acesso real a redes públicas a partir do **Xbox 360 RGH 3**, com as funções XNet/Winsock do SDK do próprio console. O teste **não autentica na Microsoft, não negocia HTTPS/TLS e não abre Fortnite**.

## O aplicativo testa
- `XNetStartup` e `WSAStartup`.
- `XNetDnsLookup("www.xbox.com")`, usando a configuração de rede do Xbox 360.
- **Somente se o DNS XNet falhar**, uma consulta DNS UDP A pela porta 53 em `8.8.8.8`; em caso de falha, tenta `8.8.4.4`. Isso **não altera os DNS** da dashboard.
- Conexão TCP com os IPs obtidos para `www.xbox.com:443` (até três endereços) e controle independente para `1.1.1.1:443`.
- Tempo limite por tentativa TCP de até seis segundos; o teste pode durar vários segundos.

## Interpretação de falhas

| Resultado no Xbox 360 | Significado |
| --- | --- |
| DNS XNet retorna endereço IPv4 | O resolvedor padrão do console respondeu |
| DNS UDP retorna endereço IPv4 | Consulta DNS pública por UDP funcionou no RGH |
| DNS UDP erro `10060` na fase `3` | Resposta UDP não chegou dentro do prazo |
| TCP erro `10060` | `connect` esgotou o prazo — não significa erro de OAuth |
| TCP IP direto OK, `xbox.com` falha | Possível problema DNS, destino ou rota |
| Ambos os destinos TCP falham | Investigar saída TCP e regras/limites do XNet Retail |
| Ambos TCP OK | É possível criar sockets TCP; **não** comprova HTTPS ou xCloud |

Origem no programa: `1` inicializar XNet, `2` inicializar Winsock, `3` chamada DNS XNet, `4` estado DNS XNet, `5` criar socket, `6` modo não bloqueante, `7` connect, `8` select, `9` verificar conexão, `10` thread; `11–15` representam fases do DNS UDP. Fases UDP: `1` socket, `2` envio, `3` espera, `4` recebimento, `5` validação.

## Observação anterior
Um teste na versão antiga do XEX deu `10060` na etapa de conexão TCP no Xbox 360 real. O DNS do console estava automático, com `181.213.132.2` informado anteriormente; foi proposta troca manual para `8.8.8.8 / 8.8.4.4`. Ainda **não temos o diagnóstico v0.9 executado no hardware real**.

## Limitação do XNet Retail
A flag `XNET_STARTUP_BYPASS_SECURITY` é voltada ao desenvolvimento; em bibliotecas retail, pode ser ignorada. Consequentemente, mesmo o console sendo RGH, os sockets XDK podem ser limitados e os testes podem falhar. Se isso se confirmar no RGH, uma opção de pesquisa é testar **XeLL Reloaded/libxenon** e a sua pilha de rede independente (lwIP), como explicado em [XELL_ALTERNATIVE.md](XELL_ALTERNATIVE.md). É outro ambiente e ainda não possui um build deste cliente.

## Próximos marcos
1. Confirmar no Xbox 360 físico os testes DNS XNet, DNS UDP e TCP por IP.
2. Verificar possibilidades reais de saída UDP/TCP da pilha de rede no console.
3. Somente após transporte externo confiável, estudar TLS moderno **com validação de certificados**.
4. Então estudar OAuth Microsoft autorizado e negociação de sessão xCloud.

Nenhuma credencial Microsoft é lida, armazenada ou enviada pelo diagnóstico.
