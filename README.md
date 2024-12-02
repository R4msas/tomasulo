# Simulador do Algoritmo de Tomasulo

Implementação do simulador para o **Algoritmo de Tomasulo**,  utilizado para executar instruções fora de ordem em processadores. Apesar de incluir renomeação de registradores para evitar dependências falsas, este simulador é um **pseudo-superscalar**, pois realiza a emissão de instruções de forma sequencial, não aproveitando completamente o paralelismo.

## Objetivo

O objetivo principal do trabalho é reproduzir os estágios do Algoritmo de Tomasulo, que incluem:
1. **Emissão (Issue):** Verifica dependências e aloca recursos.
2. **Execução (Execution):** Realiza as operações nas Unidades Funcionais.
3. **Escrita (Write-Back):** Atualiza os resultados nos registradores e libera os recursos.

Apesar de sua implementação simplificada, este simulador busca preservar a essência do algoritmo, incorporando funcionalidades como renomeação de registradores para reduzir dependências e evitar **WAR (Write-After-Read)** e **WAW (Write-After-Write)**.

## Detalhes da Implementação

### Estruturas Principais

1. **Instrução (`Instrucao`):** Representa uma operação a ser executada, incluindo seu destino e operandos, além de estados como "em execução" ou "completa".

2. **Unidade Funcional (`UnidadeFuncional`):** Modela as unidades que executam operações aritméticas e de memória. Cada unidade possui um tipo (e.g., ADD, MUL), latência, e estado ocupado ou livre.

3. **Registrador (`Registrador`):** Representa os registradores utilizados pelas instruções. Suporta renomeação para evitar conflitos de dependência.

4. **Simulador (`Tomasulo`):** Coordena o fluxo do algoritmo em ciclos, gerenciando a emissão, execução, e escrita das instruções.

5. **Renomeação de Registradores:**
  - Implementada para reduzir dependências artificiais entre as instruções.
  - Utiliza registradores temporários (`X`) para resolver conflitos.

6. **Estágios de Execução:**
  - **Emissão:** Aloca unidades funcionais disponíveis e verifica a disponibilidade de operandos.
  - **Execução:** Realiza a operação associada após verificar dependências reais.
  - **Escrita:** Atualiza os resultados nos registradores e memória, liberando recursos ocupados.

### Configuração

O simulador é configurado a partir de:
- Um arquivo de texto contendo as instruções a serem simuladas.
- Parâmetros de execução, como latências e quantidade de registradores e unidades funcionais.

Exemplo de configuração no código:
    
    Instruções =
                add R2 R3 R2
                lw F6 34 R2
                lw F2 45 R3
                mul F0 F2 F4
                sub F8 F6 F2
                div F10 F0 F6
    
    Parâmetros = 
                {{"unidadeAddLatencia", 3},
                {"unidadeMulLatencia", 10},
                {"unidadeSwLatencia", 2},
                {"unidadeAddQtd", 3},
                {"unidadeMulQtd", 2},
                {"unidadeSwQtd", 2},
                {"qtdRegistradores", 16}};


## Extensões Possíveis

O simulador pode ser expandido para incluir funcionalidades mais avançadas, como:
1. **Emissão paralela de instruções:**
   - Permitir a emissão simultânea de múltiplas instruções, simulando o comportamento de processadores superscalars reais.

2. **Modelagem de múltiplos estágios de execução:**
   - Introduzir pipelines mais complexos para simular o tempo necessário em diferentes estágios de uma unidade funcional.

3. **Cache de memória avançado:**
   - Implementar hierarquias de memória, como caches L1, L2 e L3, para simular o impacto da localidade temporal e espacial.

4. **Detecção e resolução de dependências estruturais:**
   - Adicionar verificações para recursos de hardware, como portas de leitura/escrita limitadas nos registradores.

5. **Simulação de interrupções:**
   - Introduzir a capacidade de lidar com interrupções ou mudanças no fluxo de execução.

## Referências

- **"Computer Architecture: A Quantitative Approach"** - John L. Hennessy e David A. Patterson.
- **"Computer Organization and Design"** - David A. Patterson e John L. Hennessy.

