# Pesquisa alternativa: XeLL Reloaded + libxenon

O projeto usa por enquanto **XEX Retail no Aurora/XeXMenu** para executar no Xbox 360 RGH 3. XeLL/libxenon é uma alternativa potencial de acesso mais direto ao hardware se o XNet Retail bloquear conexões externas. Isso continua inteiramente focado no **Xbox 360 físico**.

| Ambiente Xbox 360 | Artefato | Forma de execução |
| --- | --- | --- |
| XDK / XEX atual | `XCloud360.xex` | Aurora ou XeXMenu |
| XeLL Reloaded / libxenon | `xenon.elf` / ELF PowerPC | XeLL já instalado, de acordo com o método de carregamento disponível |

O XeLL usa uma implementação de rede diferente, com lwIP, mas **não oferece automaticamente TLS atualizado, OAuth, WebRTC ou Fortnite**. Nenhum ELF do XCloud360 foi produzido ou testado; o compilador libxenon não está preparado na máquina de desenvolvimento.

Se os testes no hardware confirmarem restrições da pilha XNet, o plano é instalar uma toolchain adequada à libxenon em ambiente de compilação separado e criar um **teste mínimo de DHCP, DNS, UDP e TCP**, antes de portar a UI e qualquer streaming.

**Segurança:** não é necessário modificar NAND, flashear o console nem alterar o XeLL para o atual teste em XEX. Trabalhe apenas com homebrew autorizado e procedimentos reversíveis.

Referências: [XeLL Reloaded](https://github.com/Free60Project/xell-reloaded), [libxenon](https://github.com/Free60Project/libxenon), [Xenon examples](https://github.com/Free60Project/xenon-examples).
