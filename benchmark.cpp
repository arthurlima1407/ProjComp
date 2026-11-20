/*
benchmark.cpp
Medicao de desempenho da tabela hash do projeto TriagemPS.

Este programa NAO faz parte do sistema e NAO altera nenhum arquivo dele:
ele inclui a TabelaHash.h exatamente como esta e mede o comportamento dela.

Tres experimentos:
  1. colisoes e fator de carga conforme a capacidade da tabela
  2. comparacao entre duas funcoes de dispersao
  3. tempo de busca: varredura linear contra tabela hash

Os CPFs de teste sao gerados por um algoritmo deterministico (semente fixa),
entao rodar duas vezes produz exatamente os mesmos numeros. Isso e o que
permite repetir a medicao e colocar os resultados no relatorio.

compilar:
    g++ -Wall -Wextra -std=c++11 -O2 -o benchmark benchmark.cpp
rodar:
    ./benchmark
*/

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio>
#include "TabelaHash.h"

using namespace std;
using namespace std::chrono;

// ============================================================
// GERADOR DE CPFs
// ============================================================

// gerador de numeros pseudoaleatorios proprio (congruencia linear).
// nao uso rand() porque a implementacao dele muda de compilador para
// compilador: com este, os numeros sao os mesmos em qualquer maquina.
unsigned long semente = 42;

int proximoDigito() {
    semente = semente * 1103515245UL + 12345UL;
    return (int)((semente >> 16) % 10);
}

// monta um CPF estruturalmente valido: 9 digitos sorteados mais os 2
// digitos verificadores calculados pelo algoritmo oficial.
//
// gerar CPFs validos, e nao strings aleatorias, importa: os dois ultimos
// digitos sao funcao dos nove primeiros, entao a distribuicao real e menos
// uniforme do que a de um numero qualquer. medir com CPF de verdade e o
// que torna o resultado honesto.
string gerarCpf() {
    int d[11];
    for (int i = 0; i < 9; i++) {
        d[i] = proximoDigito();
    }

    int soma = 0;
    for (int i = 0; i < 9; i++) {
        soma += d[i] * (10 - i);
    }
    int resto = soma % 11;
    d[9] = (resto < 2) ? 0 : 11 - resto;

    soma = 0;
    for (int i = 0; i < 10; i++) {
        soma += d[i] * (11 - i);
    }
    resto = soma % 11;
    d[10] = (resto < 2) ? 0 : 11 - resto;

    string cpf;
    for (int i = 0; i < 11; i++) {
        cpf += (char)('0' + d[i]);
    }
    return cpf;
}

vector<string> gerarBase(int quantidade) {
    semente = 42;               // reinicia para a base ser sempre a mesma
    vector<string> base;
    for (int i = 0; i < quantidade; i++) {
        base.push_back(gerarCpf());
    }
    return base;
}

// ============================================================
// FUNCOES DE DISPERSAO EM COMPARACAO
// ============================================================

// a do projeto: polinomial base 31.
unsigned long hashPolinomial(const string& chave) {
    unsigned long h = 0;
    for (size_t i = 0; i < chave.size(); i++) {
        h = h * 31 + (unsigned char)chave[i];
    }
    return h;
}

// alternativa ingenua: soma simples dos caracteres.
// e a primeira ideia que ocorre a qualquer um, e serve justamente para
// mostrar POR QUE a polinomial e melhor. como ela ignora a posicao dos
// digitos, "12345678901" e "10987654321" caem na mesma posicao.
unsigned long hashSoma(const string& chave) {
    unsigned long h = 0;
    for (size_t i = 0; i < chave.size(); i++) {
        h += (unsigned char)chave[i];
    }
    return h;
}

// ============================================================
// EXPERIMENTO 1 - colisoes conforme a capacidade
// ============================================================

void experimento1(const vector<string>& base) {
    cout << "\n";
    cout << "============================================================\n";
    cout << " EXPERIMENTO 1 - colisoes e fator de carga por capacidade\n";
    cout << "============================================================\n";
    cout << " base fixa de " << base.size() << " CPFs, variando a capacidade da tabela\n\n";

    cout << left  << setw(12) << "capacidade"
         << right << setw(12) << "colisoes"
         << setw(10) << "% colis."
         << setw(14) << "fator carga" << "\n";
    cout << "------------------------------------------------\n";

    int capacidades[] = {101, 211, 503, 1009, 2111, 4211};

    for (int c = 0; c < 6; c++) {
        int cap = capacidades[c];

        // escopo proprio: a tabela proibe copia, entao ela nasce e morre aqui
        TabelaHash<string, int> tabela(cap);
        for (size_t i = 0; i < base.size(); i++) {
            tabela.inserir(base[i], (int)i);
        }

        double porcentagem = 100.0 * tabela.colisoes() / tabela.tamanho();

        cout << left  << setw(12) << cap
             << right << setw(12) << tabela.colisoes()
             << setw(9)  << fixed << setprecision(1) << porcentagem << "%"
             << setw(14) << fixed << setprecision(2) << tabela.fatorDeCarga() << "\n";
    }

    cout << "\n Leitura: cada insercao numa posicao ja ocupada conta como colisao.\n";
    cout << " Quanto menor o fator de carga, mais curtas as listas e mais\n";
    cout << " proxima de O(1) fica a busca.\n";
}

// ============================================================
// EXPERIMENTO 2 - qualidade das funcoes de dispersao
// ============================================================

// distribui as chaves nas posicoes e devolve as metricas que importam.
// o calculo e feito aqui fora para nao precisar alterar a TabelaHash.h,
// que tem a funcao de dispersao fixa por dentro.
void medirDispersao(const vector<string>& base, int capacidade,
                    unsigned long (*funcaoHash)(const string&),
                    const string& nome) {
    vector<int> ocupacao(capacidade, 0);

    for (size_t i = 0; i < base.size(); i++) {
        int pos = (int)(funcaoHash(base[i]) % (unsigned long)capacidade);
        ocupacao[pos]++;
    }

    int vazias = 0;
    int maiorLista = 0;
    long somaComparacoes = 0;

    for (int i = 0; i < capacidade; i++) {
        int k = ocupacao[i];
        if (k == 0) {
            vazias++;
        }
        if (k > maiorLista) {
            maiorLista = k;
        }
        // numa lista de k elementos, achar o 1o custa 1 comparacao, o 2o
        // custa 2, e assim por diante: 1+2+...+k = k*(k+1)/2
        somaComparacoes += (long)k * (k + 1) / 2;
    }

    double mediaComparacoes = (double)somaComparacoes / base.size();

    cout << left  << setw(22) << nome
         << right << setw(10) << vazias
         << setw(14) << maiorLista
         << setw(16) << fixed << setprecision(2) << mediaComparacoes << "\n";
}

void experimento2(const vector<string>& base) {
    const int capacidade = 2111;

    cout << "\n";
    cout << "============================================================\n";
    cout << " EXPERIMENTO 2 - qualidade da funcao de dispersao\n";
    cout << "============================================================\n";
    cout << " " << base.size() << " CPFs distribuidos em " << capacidade << " posicoes\n\n";

    cout << left  << setw(22) << "funcao"
         << right << setw(10) << "vazias"
         << setw(14) << "maior lista"
         << setw(16) << "comp. media" << "\n";
    cout << "--------------------------------------------------------------\n";

    medirDispersao(base, capacidade, hashPolinomial, "polinomial (base 31)");
    medirDispersao(base, capacidade, hashSoma,       "soma dos digitos");

    cout << "\n Leitura: 'comp. media' e quantas comparacoes que uma busca bem\n";
    cout << " sucedida custa em media. Quanto mais proximo de 1, melhor.\n";
    cout << " 'maior lista' e o pior caso: o paciente mais caro de achar.\n";
}

// ============================================================
// EXPERIMENTO 3 - busca linear contra busca hash
// ============================================================

// quanto tempo, no minimo, cada medicao precisa durar.
//
// o relogio do computador tem uma resolucao: no windows ele costuma andar
// em passos de cerca de 1 milissegundo. 20000 buscas na tabela hash levam
// menos do que isso, e a medicao saia como "0.00 ms". a solucao e repetir
// o lote de buscas ate o tempo total passar com folga da resolucao do
// relogio, e so entao dividir pelo numero de buscas feitas.
const double DURACAO_MINIMA_MS = 200.0;
const int LOTE = 20000;     // buscas por rodada

// tempo decorrido em milissegundos desde 'inicio'
double msDesde(steady_clock::time_point inicio) {
    return duration_cast<microseconds>(steady_clock::now() - inicio).count() / 1000.0;
}

// mede a busca linear: percorre o vetor ate achar cada CPF procurado.
// devolve o tempo medio de UMA busca, em nanossegundos.
double medirLinear(const vector<string>& base, const vector<string>& procurados, long& achados) {
    int n = (int)base.size();
    long buscasFeitas = 0;
    achados = 0;

    steady_clock::time_point inicio = steady_clock::now();
    do {
        for (int i = 0; i < LOTE; i++) {
            for (int j = 0; j < n; j++) {
                if (base[j] == procurados[i]) {
                    achados++;
                    break;
                }
            }
        }
        buscasFeitas += LOTE;
    } while (msDesde(inicio) < DURACAO_MINIMA_MS);

    return msDesde(inicio) * 1000000.0 / buscasFeitas;
}

// mede a busca na tabela hash: calcula a posicao e percorre so aquela lista.
// devolve o tempo medio de UMA busca, em nanossegundos.
double medirHash(TabelaHash<string, int>& tabela, const vector<string>& procurados, long& achados) {
    long buscasFeitas = 0;
    achados = 0;

    steady_clock::time_point inicio = steady_clock::now();
    do {
        for (int i = 0; i < LOTE; i++) {
            if (tabela.buscar(procurados[i]) != nullptr) {
                achados++;
            }
        }
        buscasFeitas += LOTE;
    } while (msDesde(inicio) < DURACAO_MINIMA_MS);

    return msDesde(inicio) * 1000000.0 / buscasFeitas;
}

void experimento3() {
    cout << "\n";
    cout << "============================================================\n";
    cout << " EXPERIMENTO 3 - busca linear contra tabela hash\n";
    cout << "============================================================\n";
    cout << " tempo medio de UMA busca de CPF existente, em nanossegundos\n";
    cout << " (1 ns = 1 bilionesimo de segundo). cada medicao dura pelo menos\n";
    cout << " " << (int)DURACAO_MINIMA_MS << " ms; sao 3 repeticoes e vale a menor.\n\n";

    cout << left  << setw(12) << "pacientes"
         << right << setw(16) << "linear (ns)"
         << setw(14) << "hash (ns)"
         << setw(16) << "quantas vezes" << "\n";
    cout << "----------------------------------------------------------\n";

    int tamanhos[] = {100, 500, 1000, 2000, 5000, 10000};

    // guarda as linhas para o bloco CSV no fim
    vector<string> linhasCsv;

    for (int t = 0; t < 6; t++) {
        int n = tamanhos[t];
        vector<string> base = gerarBase(n);

        // a capacidade acompanha a base para o fator de carga ficar perto de 0.5
        TabelaHash<string, int> tabela(n * 2 + 1);
        for (int i = 0; i < n; i++) {
            tabela.inserir(base[i], i);
        }

        // escolhe de antemao quais CPFs procurar, para a escolha nao entrar
        // na conta do tempo
        vector<string> procurados;
        for (int i = 0; i < LOTE; i++) {
            procurados.push_back(base[(i * 7919) % n]);
        }

        // cada medicao e repetida 3 vezes e fica valendo a MENOR.
        // o computador esta rodando outras coisas ao mesmo tempo, e isso so
        // pode deixar a medicao mais lenta, nunca mais rapida. o menor tempo
        // e portanto o mais proximo do custo real do algoritmo.
        double nsLinear = 0;
        double nsHash = 0;
        long achadosLinear = 0;
        long achadosHash = 0;

        for (int rodada = 0; rodada < 3; rodada++) {
            double t1 = medirLinear(base, procurados, achadosLinear);
            if (rodada == 0 || t1 < nsLinear) {
                nsLinear = t1;
            }
            double t2 = medirHash(tabela, procurados, achadosHash);
            if (rodada == 0 || t2 < nsHash) {
                nsHash = t2;
            }
        }

        // toda busca tem que achar o CPF, nos dois metodos. alem de conferir
        // o resultado, usar os contadores impede o compilador de apagar os
        // lacos por achar que eles nao servem para nada.
        if (achadosLinear % LOTE != 0 || achadosHash % LOTE != 0) {
            cout << " ERRO: alguma busca nao encontrou o CPF procurado!\n";
        }

        double vezes = (nsHash > 0) ? nsLinear / nsHash : 0;

        cout << left  << setw(12) << n
             << right << setw(16) << fixed << setprecision(1) << nsLinear
             << setw(14) << fixed << setprecision(1) << nsHash
             << setw(15) << fixed << setprecision(1) << vezes << "x" << "\n";

        char linha[120];
        snprintf(linha, sizeof(linha), "%d;%.1f;%.1f", n, nsLinear, nsHash);
        linhasCsv.push_back(string(linha));
    }

    cout << "\n Leitura: o tempo da busca linear cresce junto com o numero de\n";
    cout << " pacientes (O(n)). O da hash fica praticamente parado (O(1)),\n";
    cout << " porque ela nao percorre a base: calcula a posicao e vai direto.\n";

    cout << "\n--- dados para o grafico (copie para uma planilha) ---\n";
    cout << "pacientes;linear_ns;hash_ns\n";
    for (size_t i = 0; i < linhasCsv.size(); i++) {
        cout << linhasCsv[i] << "\n";
    }
}

// ============================================================

int main() {
    cout << "\n";
    cout << "############################################################\n";
    cout << "#  TriagemPS - benchmark da tabela hash                    #\n";
    cout << "############################################################\n";

    vector<string> base = gerarBase(2000);

    experimento1(base);
    experimento2(base);
    experimento3();

    cout << "\n";
    return 0;
}
