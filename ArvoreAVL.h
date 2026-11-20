/*
(Paulo) Arvore AVL generica: indice dos atendimentos por data.
Chave: data no formato aaaammdd. Valor: os atendimentos daquela data.
Responde "quais atendimentos entre duas datas" em O(log n + k).
O balanceamento mantem a altura em O(log n) mesmo com as datas chegando em ordem.
*/

#ifndef ARVORE_AVL_H
#define ARVORE_AVL_H

#include <vector>
using namespace std;

template <typename K, typename V>
class ArvoreAVL {
private:
    // cada no guarda uma data e todos os atendimentos dessa data.
    struct No {
        K chave;
        vector<V> valores;
        No* esquerda;
        No* direita;
        int altura;     // altura da subarvore (folha = 1)

        No(const K& c, const V& v) : chave(c), esquerda(nullptr), direita(nullptr), altura(1) {
            valores.push_back(v);
        }
    };

    No* raiz;
    int totalChaves;        // quantas datas diferentes
    int totalValores;       // quantos atendimentos no total
    int visitados;          // nos visitados na ultima consulta por intervalo

    // copia proibida: duas copias liberariam os mesmos nos.
    ArvoreAVL(const ArvoreAVL&);
    ArvoreAVL& operator=(const ArvoreAVL&);

    // altura de um no; no nulo tem altura 0.
    int alturaDe(No* no) const {
        return (no == nullptr) ? 0 : no->altura;
    }

    int maior(int a, int b) const {
        return (a > b) ? a : b;
    }

    void atualizarAltura(No* no) {
        no->altura = 1 + maior(alturaDe(no->esquerda), alturaDe(no->direita));
    }

    // fator de balanceamento: esquerda - direita. fora de -1..1, rotaciona.
    int fatorDe(No* no) const {
        return alturaDe(no->esquerda) - alturaDe(no->direita);
    }

    // rotacao a direita: usada quando a esquerda ficou alta demais.
    No* rotacaoDireita(No* y) {
        No* x = y->esquerda;
        No* b = x->direita;

        x->direita = y;
        y->esquerda = b;

        atualizarAltura(y);
        atualizarAltura(x);
        return x;               // nova raiz da subarvore
    }

    // rotacao a esquerda: usada quando a direita ficou alta demais.
    No* rotacaoEsquerda(No* x) {
        No* y = x->direita;
        No* b = y->esquerda;

        y->esquerda = x;
        x->direita = b;

        atualizarAltura(x);
        atualizarAltura(y);
        return y;
    }

    // aplica a rotacao necessaria (quatro casos; dois exigem rotacao dupla).
    No* balancear(No* no) {
        atualizarAltura(no);
        int fator = fatorDe(no);

        if (fator > 1) {                                // pesado a esquerda
            if (fatorDe(no->esquerda) < 0) {            // caso esquerda-direita
                no->esquerda = rotacaoEsquerda(no->esquerda);
            }
            return rotacaoDireita(no);                  // caso esquerda-esquerda
        }
        if (fator < -1) {                               // pesado a direita
            if (fatorDe(no->direita) > 0) {             // caso direita-esquerda
                no->direita = rotacaoDireita(no->direita);
            }
            return rotacaoEsquerda(no);                 // caso direita-direita
        }
        return no;                                      // ja esta balanceado
    }

    // insere como numa arvore de busca e rebalanceia na volta da recursao.
    No* inserir(No* no, const K& chave, const V& valor) {
        if (no == nullptr) {
            totalChaves++;
            return new No(chave, valor);
        }

        if (chave < no->chave) {
            no->esquerda = inserir(no->esquerda, chave, valor);
        } else if (no->chave < chave) {
            no->direita = inserir(no->direita, chave, valor);
        } else {
            // data ja existe: acrescenta o atendimento no mesmo no.
            no->valores.push_back(valor);
            return no;
        }

        return balancear(no);
    }

    // percorre em ordem (resultado ja sai ordenado por data) e ignora
    // os galhos que estao fora do intervalo.
    void intervalo(No* no, const K& minimo, const K& maximo, vector<V>& saida) {
        if (no == nullptr) {
            return;
        }
        visitados++;

        if (minimo < no->chave) {
            intervalo(no->esquerda, minimo, maximo, saida);
        }
        if (!(no->chave < minimo) && !(maximo < no->chave)) {
            for (size_t i = 0; i < no->valores.size(); i++) {
                saida.push_back(no->valores[i]);
            }
        }
        if (no->chave < maximo) {
            intervalo(no->direita, minimo, maximo, saida);
        }
    }

    // libera os nos: filhos antes do pai.
    void destruir(No* no) {
        if (no == nullptr) {
            return;
        }
        destruir(no->esquerda);
        destruir(no->direita);
        delete no;
    }

public:
    ArvoreAVL() : raiz(nullptr), totalChaves(0), totalValores(0), visitados(0) {}

    ~ArvoreAVL() {
        destruir(raiz);
    }

    void inserir(const K& chave, const V& valor) {
        raiz = inserir(raiz, chave, valor);
        totalValores++;
    }

    // valores com chave entre minimo e maximo (inclusive), em ordem.
    vector<V> buscarIntervalo(const K& minimo, const K& maximo) {
        vector<V> saida;
        visitados = 0;
        intervalo(raiz, minimo, maximo, saida);
        return saida;
    }

    // esvazia a arvore (usada antes de recarregar do arquivo).
    void limpar() {
        destruir(raiz);
        raiz = nullptr;
        totalChaves = 0;
        totalValores = 0;
        visitados = 0;
    }

    int altura() const          { return alturaDe(raiz); }
    int quantidadeChaves() const { return totalChaves; }
    int quantidadeValores() const { return totalValores; }
    int nosVisitados() const    { return visitados; }
};

#endif
