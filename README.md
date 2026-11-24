# 32-Bit-Multi-Cycle-Emulator

Emulador em C++17 de um processador RISC-V de 32 bits. A CPU executa uma
instrução por meio de uma FSM multiciclo e inclui memórias separadas para
instruções e dados.

## Organização

Os headers ficam em `include/` e as implementações em `src/`. Os módulos
espelham os blocos funcionais do processador:

| Módulo | Arquivos | Responsabilidade |
| --- | --- | --- |
| CPU | `include/CPU.hpp`, `src/CPU.cpp` | FSM multiciclo, PC, CSRs, exceções e interrupções. |
| Decoder | `include/Decoder.hpp`, `src/Decoder.cpp` | Decodificação e validação de instruções. |
| SignalExtensionUnit | `include/SignalExtensionUnit.hpp`, `src/SignalExtensionUnit.cpp` | Imediatos dos formatos I, S, B, U e J. |
| BranchUnit | `include/BranchUnit.hpp`, `src/BranchUnit.cpp` | Comparações de branches. |
| ALU | `include/ALU.hpp`, `src/ALU.cpp` | Operações inteiras, multiplicação/divisão e manipulação de bits. |
| RegisterFile | `include/RegisterFile.hpp`, `src/RegisterFile.cpp` | 32 registradores de 32 bits; `x0` permanece zero. |
| InstructionMemory | `include/InstructionMemory.hpp`, `src/InstructionMemory.cpp` | Armazenamento e leitura de instruções. |
| DataMemory | `include/DataMemory.hpp`, `src/DataMemory.cpp` | Memória de bytes little-endian. |
| Definições compartilhadas | `include/Definitions.hpp` | Tipos, opcodes, CSRs e operações da ALU. |
| Leitor hexadecimal | `include/HexFile.hpp` | Leitura dos arquivos de imagem das memórias. |
| CLI | `src/main.cpp` | Opções, execução, impressão de estado e dump de memória. |

## Recursos implementados

- RV32I: operações aritméticas/lógicas, shifts, branches, saltos, `LUI`,
  `AUIPC`, loads e stores byte/halfword/word.
- RV32M: `MUL`, `MULH`, `MULHSU`, `MULHU`, `DIV`, `DIVU`, `REM` e `REMU`,
  incluindo divisão por zero e overflow assinado.
- Extensões RV32 Zba, Zbb e Zbs: operações de soma deslocada, lógica, contagem,
  rotações, extensão/reversão de bytes e manipulação de bits.
- `FENCE` e `FENCE.I` como no-ops.
- CSRs de máquina documentados no projeto, `ECALL`, `EBREAK`, `MRET`, traps de
  instrução/memória, interrupções de software/timer/externas e prioridade
  externa > timer > software.
- Memórias configuráveis, acesso de dados alinhado, ordem little-endian e
  traps para endereços desalinhados ou fora dos limites.

O emulador não implementa extensões A/C/F/D, modos S/U, MMU, depuração ou
periféricos. `time`/`timeh` e CSRs de identificação retornam zero. Interrupções
são entradas externas simuladas por pulsos agendados na linha de comando.

## Compilação

É necessário um compilador C++17. No PowerShell, na raiz do projeto:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude src/main.cpp src/CPU.cpp src/Decoder.cpp src/SignalExtensionUnit.cpp src/BranchUnit.cpp src/ALU.cpp src/RegisterFile.cpp src/InstructionMemory.cpp src/DataMemory.cpp -o riscv-emulator.exe
```

No Linux ou macOS:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic -Iinclude \
  src/main.cpp src/CPU.cpp src/Decoder.cpp src/SignalExtensionUnit.cpp \
  src/BranchUnit.cpp src/ALU.cpp src/RegisterFile.cpp \
  src/InstructionMemory.cpp src/DataMemory.cpp -o riscv-emulator
```

## Uso

Passe o arquivo de instruções como primeiro argumento ou use `--imem`:

```powershell
.\riscv-emulator.exe examples\sum.hex --max-cycles 100
```

O emulador mostra PC, ciclos, instruções aposentadas, último trap e os valores
dos registradores. Não há instrução de término na arquitetura; a simulação
para ao atingir o limite de ciclos, cujo padrão é 100.000.

| Opção | Padrão | Descrição |
| --- | ---: | --- |
| `--imem FILE` | obrigatório | Imagem da memória de instruções; aceita uma palavra de 32 bits por entrada. |
| `--dmem FILE` | vazia | Imagem inicial da memória de dados; aceita um byte por entrada. |
| `--imem-words N` | 256 | Número de palavras na memória de instruções. |
| `--dmem-bytes N` | 1024 | Número de bytes na memória de dados. |
| `--reset-pc N` | `0` | Endereço de reset, em decimal ou hexadecimal (`0x...`). |
| `--max-cycles N` | 100000 | Máximo de ciclos a simular. |
| `--dump-memory FILE` | — | Grava a memória de dados final, um byte hexadecimal por linha. |
| `--irq-software-at N` | — | Pulso de interrupção de software no ciclo `N`. |
| `--irq-timer-at N` | — | Pulso de interrupção de timer no ciclo `N`. |
| `--irq-external-at N` | — | Pulso de interrupção externa no ciclo `N`. |
| `--help` | — | Exibe as opções. |

Os ciclos dos pulsos de interrupção começam em zero. Uma opção de interrupção
pode ser repetida para agendar mais de um pulso.

### Formato das imagens hexadecimais

Cada palavra de instrução deve ser representada por até oito dígitos
hexadecimais, uma por linha. A memória de dados usa até dois dígitos
hexadecimais por byte. Linhas podem conter comentários iniciados por `#` ou
`//`. A diretiva `@endereco` altera o índice da próxima entrada.

Exemplo de `program.hex`:

```text
00500093
00700113
002081b3
0000006f
```

O arquivo [`examples/sum.hex`](./examples/sum.hex) calcula `x3 = 12` e fica em
um loop. Execute-o por, por exemplo, 30 ciclos:

```powershell
.\riscv-emulator.exe examples\sum.hex --max-cycles 30
```

Para inspecionar a memória de dados ao final:

```powershell
.\riscv-emulator.exe examples\sum.hex --max-cycles 30 --dump-memory memory-out.hex
```
