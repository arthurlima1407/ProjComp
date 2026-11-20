# Triagem PS

Sistema de triagem de pronto-socorro em C++, executado no terminal. Projeto em grupo da disciplina **Algoritmos e Estrutura de Dados** da Universidade de Brasília, desenvolvido de agosto a novembro de 2026.

O objetivo é aplicar estruturas de dados implementadas à mão em um problema concreto: cadastrar pacientes, classificar o risco, organizar a fila de atendimento e consultar o histórico de cada paciente.

## Funcionalidades

- Cadastro de pacientes com validação de CPF (11 dígitos), nome, idade e telefone
- Triagem em cinco níveis de gravidade: Vermelho, Laranja, Amarelo, Verde e Azul
- Duas filas de atendimento: prioritária (Vermelho e Laranja) e normal (Amarelo, Verde e Azul), cada uma ordenada por gravidade
- Chamada padrão (prioritária primeiro) ou personalizada (escolhendo a fila)
- Prontuário por CPF, navegação pelo histórico e recorrência de diagnósticos do paciente
- Consulta de atendimentos por período de datas
- Relatórios ordenados e estatísticas por diagnóstico
- Gravação e leitura dos dados em arquivos de texto

## Estruturas de dados e algoritmos

| Componente | Implementação |
|---|---|
| Índice de pacientes (CPF → paciente) | Tabela hash genérica (`template`) com encadeamento separado e função de dispersão polinomial (base 31) |
| Histórico por paciente | Lista duplamente encadeada |
| Atendimentos por data | Árvore AVL genérica: consulta por período em O(log n + k) |
| Filas de atendimento | `std::queue`, reordenada por gravidade com insertion sort estável |
| Relatórios por diagnóstico e por CPF | Merge Sort  |

## Como compilar e executar

Requer `g++` com suporte a C++11 e `make`.

```bash
make triagem        # compila o sistema
./triagem           # executa

make benchmark      # compila o benchmark da tabela hash (com -O2)
./benchmark         # executa os experimentos

make clean          # remove os executáveis
```

### Dados de exemplo

O repositório traz `pacientes_exemplo.txt` e `atendimentos_exemplo.txt`. Para usá-los, copie-os com os nomes que o programa lê e carregue pela opção 10 do menu:

```bash
cp pacientes_exemplo.txt pacientes.txt
cp atendimentos_exemplo.txt atendimentos.txt
```

## Persistência

Os dados são gravados e lidos pela opção 10 do menu, em dois arquivos de texto com campos separados por `;`:

- `pacientes.txt`: `cpf;nome;idade;telefone`
- `atendimentos.txt`: `cpf;data;gravidade;diagnostico;observacoes`

O estado das filas não é gravado: ao reiniciar, a triagem de quem está aguardando é refeita. Linhas corrompidas são detectadas e ignoradas na leitura.

## Benchmark da tabela hash

O `benchmark.cpp` mede a `TabelaHash.h` sem alterá-la. Os CPFs de teste são válidos (com dígitos verificadores) e gerados com semente fixa, então os resultados são reproduzíveis. A saída completa está em [`resultados_benchmark.txt`](resultados_benchmark.txt).

**1. Colisões x capacidade** (2.000 CPFs)

| Capacidade | Fator de carga | Colisões |
|---|---|---|
| 101 | 19,80 | 95,0% |
| 1009 | 1,98 | 56,9% |
| 4211 | 0,47 | 20,9% |

**2. Função de dispersão** (2.000 CPFs em 2.111 posições)

| Função | Maior lista | Comparações por busca |
|---|---|---|
| Polinomial (base 31) | 5 | 1,45 |
| Soma dos dígitos | 636 | 223,84 |

**3. Busca linear x tabela hash** (tempo médio de uma busca)

| Pacientes | Linear | Hash | Ganho |
|---|---|---|---|
| 100 | 140 ns | 9,6 ns | 15x |
| 1.000 | 1.695 ns | 10,1 ns | 168x |
| 10.000 | 24.009 ns | 14,0 ns | 1.714x |

Cada medição dura pelo menos 200 ms e é repetida 3 vezes, valendo a menor. O tempo da busca linear cresce com o número de pacientes, enquanto o da hash praticamente não muda.

## Limitações conhecidas

- A tabela hash tem capacidade fixa e não faz rehash. Com muitos pacientes em uma tabela pequena o fator de carga passa de 1 e a busca degrada, como mostra o experimento 1.
- A fila é reordenada inteira a cada triagem. Para o volume deste projeto é suficiente, mas uma fila de prioridade (heap) escalaria melhor.

## Estrutura do repositório

```
main.cpp                   menu e fluxo principal
Paciente.h / .cpp          dados do paciente
Atendimento.h / .cpp       registro de um atendimento
Gravidade.h                enum de níveis de gravidade
Historico.h                histórico (lista duplamente encadeada)
TabelaHash.h               tabela hash genérica
ArvoreAVL.h                árvore AVL genérica (índice por data)
benchmark.cpp              experimentos de desempenho da tabela hash
resultados_benchmark.txt   saída do benchmark
pacientes_exemplo.txt      dados de exemplo
atendimentos_exemplo.txt   dados de exemplo
Makefile
```

## Divisão do trabalho

- **Arthur:** tabela hash, cadastro e validação das entradas, persistência em arquivo, benchmark e integração dos módulos
- **Mateus:** triagem, filas de atendimento, algoritmos de ordenação e estatísticas por diagnóstico
- **Paulo:** histórico do paciente (lista duplamente encadeada) e índice por data (árvore AVL)
