# Trilha alternativa: XeLL/libxenon, sem PC intermediário no Xbox

**Objetivo:** demonstrar inicialização de homebrew diretamente no PowerPC do Xbox 360, sem passar pela pilha XNet da dashboard. Este experimento não substitui `XCloud360.xex` e **não resolve a rede ainda**.

Arquivos: `Makefile` e `source/main.c`. O programa apenas inicializa GPU, console de texto e controle; mostra mensagem e espera Y/Guide.

## Como testar futuramente

1. Use um ambiente Linux de desenvolvimento com toolchain [libxenon](https://github.com/Free60Project/libxenon). O PC serve apenas para compilar.
2. Configure a variável `DEVKITXENON` conforme a documentação.
3. Execute `make` dentro desta pasta.
4. Depois de obter um ELF32 válido, copie para a raiz de USB FAT32 com o nome `xenon.elf`.
5. Carregue usando **XeLL Reloaded já instalado**. Não altere NAND nem use `updflash.bin` ou `updxell.bin`.
6. Confirme que o texto aparece e que Y/Guide encerra o teste.

**Ainda não foi compilado ou validado.** Não estabelece DHCP, DNS, TCP, TLS, OAuth, WebRTC ou streaming.

Se o boot funcionar, o próximo passo será adaptar um teste de DHCP/DNS/TCP/UDP à implementação de rede disponível na versão de libxenon instalada. A presença do lwIP no código-fonte não garante socket funcional nem acesso ao xCloud.

Referências:
- https://github.com/Free60Project/xenon-examples
- https://github.com/Free60Project/libxenon
- https://github.com/Free60Project/xell-reloaded
