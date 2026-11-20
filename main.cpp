// menu principal do sistema. arquivo compartilhado pelos tres integrantes.

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <cctype>
#include "Gravidade.h"
#include "Paciente.h"
#include "Atendimento.h"
#include "TabelaHash.h"
#include "Historico.h"
#include "ArvoreAVL.h"

// windows.h so existe no windows; a guarda deixa compilar no linux e no mac.
#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

// indice de pacientes: cpf -> paciente, busca em O(1). (arthur)

TabelaHash<string, Paciente> indicePacientes;

// indice de historicos: cpf -> lista duplamente encadeada do paciente. (paulo)
TabelaHash<string, Historico*> indiceHistoricos;

//Mateus: Guarda todos os atendimentos (triagens) na ordem em que aconteceram. Os relatorios da opcao 9 leem daqui.
vector<Atendimento> atendimentos;
//Mateus: Duas filas: a prioritaria (Vermelho e Laranja) e a normal (Amarelo, Verde e Azul). As duas saem por gravidade.
//Mateus: As filas guardam Atendimento (e nao Paciente), o paciente e achado depois pelo cpf na tabela hash.
queue<Atendimento> FilaNormal;
queue<Atendimento> FilaPrioritaria;

// (Paulo) indice por data: arvore AVL de data (aaaammdd) -> atendimentos. usada na opcao 7.
ArvoreAVL<int, Atendimento> indiceDatas;

// prototipos: estas funcoes sao definidas mais abaixo, mas usadas antes.
void relatoriosOrdenados();
int dataParaNumero(string data);

// desenha o menu na tela.
void exibirMenu() {
    cout << "\n======================================\n";
    cout << "     TRIAGEM PS - PRONTO SOCORRO\n";
    cout << "======================================\n";
    cout << "  1 - Cadastrar paciente\n";
    cout << "  2 - Realizar triagem (classificar risco)\n";
    cout << "  3 - Chamar proximo atendimento\n";
    cout << "  4 - Consultar prontuario por CPF\n";
    cout << "  5 - Navegar historico do paciente\n";
    cout << "  6 - Ver recorrencia de diagnosticos\n";
    cout << "  7 - Consultar atendimentos por periodo\n";
    cout << "  8 - Ver estado atual da fila\n";
    cout << "  9 - Relatorios e estatisticas\n";
    cout << " 10 - Salvar / Carregar dados\n";
    cout << "  0 - Sair\n";
    cout << "--------------------------------------\n";
    cout << "  Opcao: ";
}

// le um numero do teclado. se o usuario digitar letra, limpa o erro do cin
// e pergunta de novo, em vez de travar o menu.
int lerOpcao() {
    int valor;
    while (!(cin >> valor)) {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "Entrada invalida. Digite um numero: ";
    }
    // descarta o resto da linha para o proximo getline nao ler vazio.
    cin.ignore(10000, '\n');
    return valor;
}

// confere se o texto tem so digitos. evita passar lixo para o stoi.
bool apenasDigitos(const string& texto) {
    if (texto.empty()) {
        return false;
    }
    for (size_t i = 0; i < texto.size(); i++) {
        if (texto[i] < '0' || texto[i] > '9') {
            return false;
        }
    }
    return true;
}

// ============================================================
// ARTHUR - nucleo de dados e busca
// ============================================================

// ------------------------------------------------------------
// VALIDACAO DO CADASTRO: le tudo como texto e valida antes de converter.
// ------------------------------------------------------------

// cpf: exatamente 11 digitos, sem ponto, traco ou letra.
string lerCpf() {
    string cpf;
    while (true) {
        cout << "CPF: ";
        getline(cin, cpf);

        if (!apenasDigitos(cpf)) {
            cout << "  O CPF so pode conter numeros.\n";
            continue;
        }
        if (cpf.size() != 11) {
            cout << "  O CPF precisa ter 11 digitos. Voce digitou " << cpf.size() << ".\n";
            continue;
        }
        return cpf;
    }
}

// nome: nao pode ficar em branco nem ter numero. acento e espaco sao aceitos.
string lerNome() {
    string nome;
    while (true) {
        cout << "Nome: ";
        getline(cin, nome);

        bool temLetra = false;
        bool temDigito = false;
        for (size_t i = 0; i < nome.size(); i++) {
            if (nome[i] >= '0' && nome[i] <= '9') {
                temDigito = true;
            }
            if (nome[i] != ' ') {
                temLetra = true;
            }
        }

        if (!temLetra) {
            cout << "  O nome nao pode ficar em branco.\n";
            continue;
        }
        if (temDigito) {
            cout << "  O nome nao pode conter numeros.\n";
            continue;
        }
        return nome;
    }
}

// idade: numero inteiro de 0 a 130.
int lerIdade() {
    string texto;
    while (true) {
        cout << "Idade: ";
        getline(cin, texto);

        if (!apenasDigitos(texto)) {
            cout << "  A idade so pode conter numeros.\n";
            continue;
        }
        if (texto.size() > 3) {
            cout << "  A idade precisa estar entre 0 e 130.\n";
            continue;
        }
        int idade = stoi(texto);
        if (idade > 130) {
            cout << "  A idade precisa estar entre 0 e 130.\n";
            continue;
        }
        return idade;
    }
}

// telefone: 10 ou 11 digitos com DDD, so numeros.
string lerTelefone() {
    string telefone;
    while (true) {
        cout << "Telefone: ";
        getline(cin, telefone);

        if (!apenasDigitos(telefone)) {
            cout << "  O telefone so pode conter numeros, sem espaco, parentese ou traco.\n";
            continue;
        }
        if (telefone.size() < 10 || telefone.size() > 11) {
            cout << "  O telefone precisa ter 10 ou 11 digitos, com o DDD. Voce digitou " << telefone.size() << ".\n";
            continue;
        }
        return telefone;
    }
}

// opcao 1: le os dados do teclado e guarda o paciente na tabela hash.
void cadastrarPaciente() {
    cout << "\n";
    string cpf = lerCpf();

    // recusa cpf que ja esta cadastrado.
    if (indicePacientes.buscar(cpf) != nullptr) {
        cout << "Ja existe paciente cadastrado com este CPF.\n";
        return;
    }

    string nome = lerNome();
    int idade = lerIdade();
    string telefone = lerTelefone();

    Paciente p(cpf, nome, idade, telefone);
    indicePacientes.inserir(cpf, p);
    cout << "Paciente cadastrado.\n";
}

// ponte entre os modulos: guarda o atendimento da triagem tambem no
// historico (lista duplamente encadeada) do paciente.
void registrarNoHistorico(const Atendimento& atendimento) {
    string cpf = atendimento.getCpfPaciente();

    // cada paciente tem um unico Historico: reusa o que ja existe ou cria o primeiro.
    Historico* historico;
    Historico** ponteiro = indiceHistoricos.buscar(cpf);

    if (ponteiro == nullptr) {
        historico = new Historico();
        indiceHistoricos.inserir(cpf, historico);
    } else {
        historico = *ponteiro;
    }

    // inserir recebe ponteiro e guarda uma copia.
    Atendimento copia = atendimento;
    historico->inserir(&copia);
}

// (Paulo) guarda o atendimento tambem no indice por data (arvore AVL).
void indexarPorData(const Atendimento& atendimento) {
    int chave = dataParaNumero(atendimento.getData());
    if (chave != 0) {
        indiceDatas.inserir(chave, atendimento);
    }
}

// opcao 4: busca o paciente pelo cpf e mostra a ficha.
void consultarProntuario() {
    string cpf;
    cout << "\nCPF: ";
    cin >> cpf;

    // so acessa o ponteiro depois de conferir que nao e nullptr.
    Paciente* p = indicePacientes.buscar(cpf);
    if (p == nullptr) {
        cout << "Paciente nao encontrado.\n";
        return;
    }
    p->exibir();    // -> e o acesso a metodo por meio de ponteiro

    // (Paulo) percorre a lista duplamente encadeada do paciente
    Historico** historicoPonteiro = indiceHistoricos.buscar(cpf);

    if (historicoPonteiro != nullptr) {
        cout << "=== HISTÓRICO DO PACIENTE ===" << endl;

        Historico* historicoPaciente = *historicoPonteiro;

        if (historicoPaciente->irParaInicio()) {
            do {
                Atendimento a = historicoPaciente->atual();
                cout << "Data: " << a.getData() << endl;
                cout << "Gravidade: " << nomeGravidade(a.getGravidade()) << endl;
                cout << "Diagnóstico: " << a.getDiagnostico() << endl;
                cout << "Observações: " << a.getObservacoes() << endl;
                cout << "-----------------------------------" << endl;
            } while (historicoPaciente->proximo());
        }
    } else {
        cout << "Este paciente ainda nao tem atendimentos registrados." << endl;
    }
}

// ------------------------------------------------------------
// PERSISTENCIA EM ARQUIVO (campos separados por ';')
//   pacientes.txt      cpf;nome;idade;telefone
//   atendimentos.txt   cpf;data;gravidade;diagnostico;observacoes
// as filas nao sao gravadas: ao reiniciar, a triagem e refeita.
// ------------------------------------------------------------

// divide a linha pelos ';'. devolve false se o numero de campos estiver errado.
bool dividirLinha(const string& linha, string campos[], int esperado) {
    size_t inicio = 0;

    for (int i = 0; i < esperado; i++) {
        size_t separador = linha.find(';', inicio);

        if (i == esperado - 1) {
            // ultimo campo: vai ate o fim da linha e nao pode ter mais separador
            if (separador != string::npos) {
                return false;   // campos a mais
            }
            campos[i] = linha.substr(inicio);
        } else {
            if (separador == string::npos) {
                return false;   // campos a menos
            }
            campos[i] = linha.substr(inicio, separador - inicio);
            inicio = separador + 1;
        }
    }
    return true;
}

// apaga os Historico criados com new e esvazia o indice.
void limparHistoricos() {
    vector<string> chaves = indiceHistoricos.listarChaves();

    for (size_t i = 0; i < chaves.size(); i++) {
        Historico** ponteiro = indiceHistoricos.buscar(chaves[i]);
        if (ponteiro != nullptr) {
            delete *ponteiro;
        }
        indiceHistoricos.remover(chaves[i]);
    }
}

// grava os dados cadastrais de todos os pacientes.
void salvarPacientes() {
    ofstream arquivo("pacientes.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir pacientes.txt para escrita.\n";
        return;
    }

    vector<string> chaves = indicePacientes.listarChaves();

    for (size_t i = 0; i < chaves.size(); i++) {
        Paciente* p = indicePacientes.buscar(chaves[i]);
        if (p == nullptr) {
            continue;
        }
        arquivo << p->getCpf() << ";"
                << p->getNome() << ";"
                << p->getIdade() << ";"
                << p->getTelefone() << "\n";
    }

    arquivo.close();
    cout << chaves.size() << " paciente(s) gravados em pacientes.txt.\n";
}

// grava os atendimentos a partir do historico de cada paciente,
// que ja esta em ordem cronologica.
void salvarAtendimentos() {
    ofstream arquivo("atendimentos.txt");
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir atendimentos.txt para escrita.\n";
        return;
    }

    vector<string> chaves = indiceHistoricos.listarChaves();
    int gravados = 0;

    for (size_t i = 0; i < chaves.size(); i++) {
        Historico** ponteiro = indiceHistoricos.buscar(chaves[i]);
        if (ponteiro == nullptr) {
            continue;
        }
        Historico* historico = *ponteiro;

        // irParaInicio devolve false quando o historico esta vazio
        if (historico->irParaInicio()) {
            do {
                Atendimento a = historico->atual();
                arquivo << chaves[i] << ";"
                        << a.getData() << ";"
                        << (int)a.getGravidade() << ";"
                        << a.getDiagnostico() << ";"
                        << a.getObservacoes() << "\n";
                gravados++;
            } while (historico->proximo());
        }
    }

    arquivo.close();
    cout << gravados << " atendimento(s) gravados em atendimentos.txt.\n";
}

// le os pacientes do arquivo. cpf que ja esta na memoria e ignorado.
void carregarPacientes() {
    ifstream arquivo("pacientes.txt");
    if (!arquivo.is_open()) {
        cout << "Nao foi possivel abrir pacientes.txt.\n";
        return;
    }

    string linha;
    string campos[4];
    int carregados = 0, repetidos = 0, invalidos = 0;

    while (getline(arquivo, linha)) {
        if (linha.empty()) {
            continue;
        }

        if (!dividirLinha(linha, campos, 4) || !apenasDigitos(campos[2])) {
            invalidos++;
            continue;
        }

        Paciente p(campos[0], campos[1], stoi(campos[2]), campos[3]);

        if (indicePacientes.inserir(campos[0], p)) {
            carregados++;
        } else {
            repetidos++;
        }
    }

    arquivo.close();

    cout << carregados << " paciente(s) carregados.\n";
    if (repetidos > 0) {
        cout << repetidos << " ja estavam na memoria e foram ignorados.\n";
    }
    if (invalidos > 0) {
        cout << invalidos << " linha(s) invalidas foram puladas.\n";
    }
}

// le os atendimentos e reconstroi o vector, os historicos e a arvore.
// esvazia tudo antes, para uma segunda leitura nao duplicar os dados.
void carregarAtendimentos() {
    ifstream arquivo("atendimentos.txt");
    if (!arquivo.is_open()) {
        cout << "Nao foi possivel abrir atendimentos.txt.\n";
        return;
    }

    limparHistoricos();
    atendimentos.clear();
    indiceDatas.limpar();

    string linha;
    string campos[5];
    int carregados = 0, invalidos = 0;

    while (getline(arquivo, linha)) {
        if (linha.empty()) {
            continue;
        }

        if (!dividirLinha(linha, campos, 5) || !apenasDigitos(campos[2])) {
            invalidos++;
            continue;
        }

        int codigoGravidade = stoi(campos[2]);
        if (codigoGravidade < VERMELHO || codigoGravidade > AZUL) {
            invalidos++;
            continue;
        }

        // data fora do formato dd/mm/aaaa: linha invalida.
        if (dataParaNumero(campos[1]) == 0) {
            invalidos++;
            continue;
        }

        Atendimento a(campos[0], campos[1], (Gravidade)codigoGravidade, campos[3], campos[4]);

        atendimentos.push_back(a);      // estrutura do mateus
        registrarNoHistorico(a);        // estrutura do paulo: lista por paciente
        indexarPorData(a);              // estrutura do paulo: arvore por data
        carregados++;
    }

    arquivo.close();

    cout << carregados << " atendimento(s) carregados.\n";
    if (invalidos > 0) {
        cout << invalidos << " linha(s) invalidas foram puladas.\n";
    }
}

// opcao 10: grava e le os dados em arquivo.
void salvarCarregarDados() {
    cout << "\n  1 - Salvar dados\n";
    cout << "  2 - Carregar dados\n";
    cout << "  0 - Voltar\n";
    cout << "  Opcao: ";

    int opcao = lerOpcao();

    switch (opcao) {
        case 1:
            salvarPacientes();
            salvarAtendimentos();
            break;
        case 2:
            // pacientes primeiro: o historico depende deles.
            carregarPacientes();
            carregarAtendimentos();
            break;
        case 0:
            break;
        default:
            cout << "Opcao invalida.\n";
            break;
    }
}

// opcao 9: numeros da tabela hash e relatorios ordenados.
void relatorios() {
    cout << "\nPacientes cadastrados: " << indicePacientes.tamanho() << "\n";
    cout << "Colisoes na tabela hash: " << indicePacientes.colisoes() << "\n";
    cout << "Fator de carga: " << indicePacientes.fatorDeCarga() << "\n";

    //Mateus: submenu dos relatorios da opcao 9 (lista por diagnostico, por CPF e analise de doencas)
    relatoriosOrdenados();
}

// ============================================================
// MATEUS - fila de atendimento
// ============================================================

//Mateus: Devolve a data de hoje no formato dd/mm/aaaa, pegando do relogio do computador.
string dataAtual(){
    time_t agora = time(0);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%d/%m/%Y", localtime(&agora));
    return string(buffer);
}

//Mateus: Insertion sort pela gravidade, usado pelo organizarFila.
//Mateus: Quanto MENOR o valor do enum, MAIOR a urgencia, entao o '<' deixa os casos mais graves no inicio.
//Mateus: So troca de lugar quando o anterior e MENOS grave, entao quem chegou primeiro continua na frente no empate (FIFO).
void ordenarPorGravidade(vector<Atendimento>& v){
    for(int i = 1; i < (int)v.size(); i++){
        Atendimento chave = v[i];
        int j = i - 1;
        while(j >= 0 && chave.getGravidade() < v[j].getGravidade()){
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = chave;
    }
}

//Mateus: As duas filas saem por gravidade. Na prioritaria um Vermelho que chega depois passa na frente do Laranja,
//Mateus: e na normal o Amarelo passa na frente do Verde, que passa na frente do Azul. Empate: quem chegou primeiro.
//Mateus: A queue nao deixa mexer no meio, entao passo tudo para um vector, ordeno e devolvo para a fila na ordem certa.
//Mateus: A fila vem com & para a funcao mexer na fila de verdade (FilaPrioritaria ou FilaNormal), e nao numa copia.
void organizarFila(queue<Atendimento>& fila){
    vector<Atendimento> aux;
    while(!fila.empty()){
        aux.push_back(fila.front());
        fila.pop();
    }
    ordenarPorGravidade(aux);
    for(int i = 0; i < (int)aux.size(); i++){
        fila.push(aux[i]);
    }
}

//Mateus: Opcao 2. Busca o paciente pelo CPF, pede situacao, doenca e cor, cria o Atendimento, guarda no vector
//Mateus: e no historico do paciente e coloca na fila certa (prioritaria ou normal), ja na ordem de gravidade.
void realizarTriagem() {
    string cpf;
    string situa;
    string prob;
    int grav;

    //Mateus: pede o cpf de quem esta sendo triado e busca na tabela hash, assim a triagem nao depende do ultimo cadastrado.
    cout << "\nCPF do paciente: ";
    cin >> cpf;
    Paciente* p = indicePacientes.buscar(cpf);
    if(p == nullptr){
        cout << "Paciente nao encontrado. Cadastre o paciente antes da triagem.\n";
        return;
    }

    //Mateus: descarta o \n que o cin >> deixou no buffer, senao o primeiro getline le uma linha vazia.
    cin.ignore(1000, '\n');

    cout << "Insira uma descrição da situação do paciente:\n";
    getline(cin, situa);

    cout << "Insira o problema ou doença que o paciente tem:\n";
    getline(cin, prob);

    while(true){
    cout << "Insira o nivel de gravidade do paciente de acordo com o número de cada cor: \n0.Vermelho \n1.Laranja \n2.Amarelo \n3.Verde \n4.Azul\n";
    if(!(cin >> grav) || grav < 0 || grav > 4){
        cout << "Insira um valor entre 0 e 4!\n";
        cin.clear();
        cin.ignore(1000, '\n');
        continue;
    }
    break;
    }

    //Mateus: grav continua sendo lido como int, mas aqui ele vira o enum Gravidade oficial do projeto.
    Gravidade gravidade = (Gravidade)grav;

    //Mateus: prob vira diagnostico e situa vira observacoes.
    Atendimento novoAtendimento(p->getCpf(), dataAtual(), gravidade, prob, situa);
    atendimentos.push_back(novoAtendimento);

    // guarda tambem no historico do paciente e no indice por data.
    registrarNoHistorico(novoAtendimento);
    indexarPorData(novoAtendimento);

    if(gravidade < AMARELO){
        FilaPrioritaria.push(novoAtendimento);
        organizarFila(FilaPrioritaria);     //Mateus: Vermelho passa na frente do Laranja
    }
    else{
        FilaNormal.push(novoAtendimento);
        organizarFila(FilaNormal);          //Mateus: Amarelo, depois Verde, depois Azul
    }
    cout << "Triagem realizada: " << nomeGravidade(gravidade) << ".\n";
}

//Mateus: Mostra o nome e o CPF de quem esta sendo chamado. A fila so guarda o CPF, entao o nome vem da tabela hash.
void exibirChamada(const Atendimento& atendimento){
    Paciente* p = indicePacientes.buscar(atendimento.getCpfPaciente());
    if(p == nullptr){
        cout << "Proximo Paciente: Paciente nao encontrado\n";
    }
    else{
        cout << "Proximo Paciente: " << p->getNome() << endl;
    }
    cout << "CPF: " << atendimento.getCpfPaciente() << endl;
}

//Mateus: Opcao 3. Chamada padrao: chama primeiro a fila prioritaria e, se ela estiver vazia, a normal.
//Mateus: Chamada personalizada: o usuario escolhe de qual fila sai o proximo.
void chamarProximo(){
    //Mateus: se nao tem ninguem esperando, nao ha o que chamar (front() e pop() em fila vazia derrubam o programa).
    if(FilaPrioritaria.empty() && FilaNormal.empty()){
        cout << "Nao ha pacientes aguardando.\n";
        return;
    }

    //Mateus: o while(true) ficou so para validar a entrada. Depois de uma opcao valida ele sai com break.
    int op = 0;
    while(true){
    cout << "Escolha entre o método de chamada padrão ou a chamada personalizada:\n";
    cout << "1. Chamada Padrão\n";
    cout << "2. Chamada Personalizada\n";
    if(!(cin >> op) || op < 1 || op > 2){
        cout << "Escolha entre a opção 1 e 2!!\n";
        cin.clear();
        cin.ignore(1000, '\n');
        continue;
    }
    break;
    }

    switch(op){
        case 1:{
            //Mateus: chama primeiro da prioritaria (ja ordenada por gravidade). Se ela estiver vazia, chama da normal.
            if(!(FilaPrioritaria.empty())){
                exibirChamada(FilaPrioritaria.front());
                FilaPrioritaria.pop();

            }
            else{
                exibirChamada(FilaNormal.front());
                FilaNormal.pop();

            }
            break;
        }
        case 2:{
            int opchamada = 0;
            while(true){
            cout << "Escolha uma das seguintes opções!\n";
            cout << "1. Chamar Paciente da Fila Normal!\n";
            cout << "2. Chamar Paciente da Fila Prioritária!\n";
            if(!(cin >> opchamada) || opchamada < 1 || opchamada > 2){
                cout << "Escolha entre a opção 1 e 2!!\n";
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            break;
            }

            switch(opchamada){
                case 1:{
                    if(FilaNormal.empty()){
                        cout << "A fila normal esta vazia.\n";
                    }
                    else{
                        exibirChamada(FilaNormal.front());
                        FilaNormal.pop();
                    }
                    break;
                }
                case 2:{
                    if(FilaPrioritaria.empty()){
                        cout << "A fila prioritaria esta vazia.\n";
                    }
                    else{
                        exibirChamada(FilaPrioritaria.front());
                        FilaPrioritaria.pop();
                    }
                    break;
                }
            }
            break;
        }
    }
}

//Mateus: Imprime uma fila inteira, com a posicao, o nome, o CPF e a cor de cada um.
//Mateus: A queue so deixa ver o front(), entao para ver todo mundo tem que ir tirando com pop().
//Mateus: Para nao apagar a fila de verdade, a funcao recebe a fila SEM o & (por valor), ou seja, ela recebe uma COPIA.
//Mateus: Os pop() acontecem so na copia e a fila original continua igual.
void mostrarFila(queue<Atendimento> copia, string nomeFila){
    cout << "\n--- " << nomeFila << " (" << copia.size() << " aguardando) ---\n";
    if(copia.empty()){
        cout << "Ninguem aguardando.\n";
        return;
    }
    int cont = 1;
    while(!copia.empty()){
        Atendimento atendimento = copia.front();
        Paciente* p = indicePacientes.buscar(atendimento.getCpfPaciente());
        cout << cont << ". ";
        if(p == nullptr){
            cout << "Paciente nao encontrado";
        }
        else{
            cout << p->getNome();
        }
        cout << " | CPF: " << atendimento.getCpfPaciente();
        cout << " | Gravidade: " << nomeGravidade(atendimento.getGravidade()) << endl;
        copia.pop();
        cont++;
    }
}

//Mateus: Opcao 8. Mostra quem esta esperando nas duas filas, na ordem em que vai ser chamado, sem tirar ninguem.
void verEstadoDaFila() {
    if(FilaPrioritaria.empty() && FilaNormal.empty()){
        cout << "\nNao ha pacientes aguardando.\n";
        return;
    }
    //Mateus: Na chamada padrao a prioritaria e chamada toda antes, entao mostrar nessa ordem ja e a ordem de atendimento.
    cout << "\nOrdem de atendimento (chamada padrao): primeiro a fila prioritaria, depois a normal.\n";
    mostrarFila(FilaPrioritaria, "Fila Prioritaria");
    mostrarFila(FilaNormal, "Fila Normal");
}

// ============================================================
// MATEUS - relatorios ordenados (opcao 9)
// ============================================================

//Mateus: Os relatorios ordenam uma COPIA do vector atendimentos, para o original continuar na ordem em que os atendimentos aconteceram.

//Mateus: MERGE SORT usado nos dois relatorios: pelo diagnostico (opcao 1) e pelo CPF (opcao 2).
//Mateus: Escolhi o Merge porque ele é ESTAVEL: quando dois atendimentos empatam (mesmo diagnostico ou mesmo CPF),
//Mateus: eles continuam na ordem em que estavam no vector, ou seja, na ordem em que aconteceram.
//Mateus: O bool porCpf escolhe o campo: true = ordena pelo CPF, false = ordena pelo diagnostico.
//Mateus: intercalar junta duas metades que ja estao ordenadas: v[inicio..meio] e v[meio+1..fim].
void intercalar(vector<Atendimento>& v, int inicio, int meio, int fim, bool porCpf){
    vector<Atendimento> temp;
    int i = inicio;
    int j = meio + 1;
    while(i <= meio && j <= fim){
        bool pegaEsquerda;
        if(porCpf){
            pegaEsquerda = v[i].getCpfPaciente() <= v[j].getCpfPaciente();     //o '=' e o que deixa estavel
        }
        else{
            pegaEsquerda = v[i].getDiagnostico() <= v[j].getDiagnostico();
        }
        if(pegaEsquerda){
            temp.push_back(v[i]);
            i++;
        }
        else{
            temp.push_back(v[j]);
            j++;
        }
    }
    while(i <= meio){       //sobrou na metade esquerda
        temp.push_back(v[i]);
        i++;
    }
    while(j <= fim){        //sobrou na metade direita
        temp.push_back(v[j]);
        j++;
    }
    for(int k = 0; k < (int)temp.size(); k++){
        v[inicio + k] = temp[k];    //inicio + k porque essa parte do vector nao comeca no 0
    }
}
//Mateus: Funcao recursiva: divide no meio ate sobrar 1 elemento (que ja esta ordenado) e depois vai juntando.
void mergeSort(vector<Atendimento>& v, int inicio, int fim, bool porCpf){
    if(inicio >= fim){
        return;
    }
    int meio = (inicio + fim) / 2;
    mergeSort(v, inicio, meio, porCpf);
    mergeSort(v, meio + 1, fim, porCpf);
    intercalar(v, inicio, meio, fim, porCpf);
}

//Mateus: Mostra uma linha do relatorio: nome, CPF, data, cor e diagnostico.
void exibirAtendimento(const Atendimento& atendimento){
    Paciente* p = indicePacientes.buscar(atendimento.getCpfPaciente());
    if(p == nullptr){
        cout << "Paciente nao encontrado";
    }
    else{
        cout << p->getNome();
    }
    cout << " | CPF: " << atendimento.getCpfPaciente();
    cout << " | " << atendimento.getData();
    cout << " | " << nomeGravidade(atendimento.getGravidade());
    cout << " | Diagnostico: " << atendimento.getDiagnostico() << endl;
}

// ============================================================
// MATEUS - analise de doencas: ocorrencia, preparo e previsao
// ============================================================
//Mateus: Essa parte usa o vector atendimentos (os feitos na triagem e os carregados do arquivo na opcao 10).
//Mateus: Nao muda nada nos dados, so le e calcula.

//Mateus: Deixa o diagnostico todo em minusculo, para "Gripe", "GRIPE" e "gripe" contarem como a mesma doenca.
string padronizar(string texto){
    for(int i = 0; i < (int)texto.size(); i++){
        //tolower (da <cctype>) troca maiuscula por minuscula; o resto continua igual.
        //o (unsigned char) e porque letra com acento vira numero negativo no char, e o tolower nao aceita negativo.
        texto[i] = tolower((unsigned char)texto[i]);
    }
    return texto;
}

//Mateus: Transforma "dd/mm/aaaa" em um numero aaaammdd (ex.: 01/11/2026 vira 20261101).
//Mateus: Assim da para comparar datas com < e >, porque o ano vem primeiro, depois o mes e depois o dia.
int dataParaNumero(string data){
    // confere o formato antes de converter; data invalida devolve 0.
    if(data.size() != 10 || data[2] != '/' || data[5] != '/'){
        return 0;
    }
    if(!apenasDigitos(data.substr(0, 2)) || !apenasDigitos(data.substr(3, 2)) || !apenasDigitos(data.substr(6, 4))){
        return 0;
    }
    int dia = stoi(data.substr(0, 2));      //substr(inicio, tamanho) pega um pedaco da string; stoi converte para int
    int mes = stoi(data.substr(3, 2));
    int ano = stoi(data.substr(6, 4));
    if(dia < 1 || dia > 31 || mes < 1 || mes > 12){
        return 0;
    }
    return ano * 10000 + mes * 100 + dia;
}

//Mateus: Arredonda para 1 casa decimal (52.381 vira 52.4): multiplica por 10, soma 0.5, corta a parte quebrada com (int) e divide por 10.
double arredondar(double x){
    return (int)(x * 10 + 0.5) / 10.0;
}

//Mateus: Guarda os numeros de UMA doenca. Os atributos ficam publicos porque e so um "pacote" de contadores, igual um No.
class EstatisticaDoenca{
public:
    string nome;
    int total;
    int porCor[5];          //quantos casos em cada cor (o enum e o indice: 0 = Vermelho ... 4 = Azul)
    int somaIdade;
    int qtdComIdade;        //so conta a idade de quem foi achado na tabela hash
    EstatisticaDoenca(){
        nome = "N/A";
        total = 0;
        for(int i = 0; i < 5; i++){
            porCor[i] = 0;
        }
        somaIdade = 0;
        qtdComIdade = 0;
    }
    int graves(){           //Vermelho + Laranja
        return porCor[VERMELHO] + porCor[LARANJA];
    }
    Gravidade corMaisComum(){
        int maior = 0;
        for(int i = 1; i < 5; i++){
            if(porCor[i] > porCor[maior]){
                maior = i;
            }
        }
        return (Gravidade)maior;
    }
};

//Mateus: Percorre os atendimentos e monta um vector com uma EstatisticaDoenca para cada doenca diferente.
//Mateus: Busca linear no vector de estatisticas: se a doenca ja existe soma nela, se nao cria uma nova.
vector<EstatisticaDoenca> calcularEstatisticas(){
    vector<EstatisticaDoenca> estat;
    for(int i = 0; i < (int)atendimentos.size(); i++){
        string doenca = padronizar(atendimentos[i].getDiagnostico());
        int pos = -1;
        for(int j = 0; j < (int)estat.size(); j++){
            if(estat[j].nome == doenca){
                pos = j;
                break;
            }
        }
        if(pos == -1){
            EstatisticaDoenca nova;
            nova.nome = doenca;
            estat.push_back(nova);
            pos = estat.size() - 1;
        }
        estat[pos].total++;
        estat[pos].porCor[atendimentos[i].getGravidade()]++;
        Paciente* p = indicePacientes.buscar(atendimentos[i].getCpfPaciente());
        if(p != nullptr){
            estat[pos].somaIdade += p->getIdade();
            estat[pos].qtdComIdade++;
        }
    }
    //Mateus: ordena do mais frequente para o menos (insertion, estavel). Empate: ordem alfabetica.
    for(int i = 1; i < (int)estat.size(); i++){
        EstatisticaDoenca chave = estat[i];
        int j = i - 1;
        while(j >= 0 && (estat[j].total < chave.total || (estat[j].total == chave.total && estat[j].nome > chave.nome))){
            estat[j + 1] = estat[j];
            j--;
        }
        estat[j + 1] = chave;
    }
    return estat;
}

//Mateus: Ranking das doencas: casos, porcentagem do total, porcentagem de casos graves, idade media e cor mais comum.
void rankingDoencas(){
    vector<EstatisticaDoenca> estat = calcularEstatisticas();
    int totalGeral = atendimentos.size();
    cout << "\n--- Ocorrencia de doencas (" << totalGeral << " atendimentos) ---\n";
    for(int i = 0; i < (int)estat.size(); i++){
        //Mateus: (double) antes da divisao para nao dar divisao inteira (3 / 4 daria 0 sem ele)
        double porcentagem = (double)estat[i].total / totalGeral * 100;
        double porcGraves = (double)estat[i].graves() / estat[i].total * 100;
        cout << i + 1 << ". " << estat[i].nome << ": " << estat[i].total << " caso(s) | " << arredondar(porcentagem) << "% do total";
        cout << " | " << arredondar(porcGraves) << "% graves | cor mais comum: " << nomeGravidade(estat[i].corMaisComum());
        if(estat[i].qtdComIdade > 0){
            cout << " | idade media: " << arredondar((double)estat[i].somaIdade / estat[i].qtdComIdade);
        }
        cout << endl;
    }
}

//Mateus: Preparo: para cada doenca, quantos casos de cada cor e a porcentagem. Ajuda a saber quantos leitos de
//Mateus: emergencia separar (ex.: se 40% dos casos de "infarto" sao Vermelho, precisa de sala vermelha livre).
void preparoPorGravidade(){
    vector<EstatisticaDoenca> estat = calcularEstatisticas();
    cout << "\n--- Preparo: gravidade de cada doenca ---\n";
    for(int i = 0; i < (int)estat.size(); i++){
        cout << estat[i].nome << " (" << estat[i].total << "):";
        for(int c = 0; c < 5; c++){
            if(estat[i].porCor[c] > 0){
                cout << " " << nomeGravidade((Gravidade)c) << " " << estat[i].porCor[c];
                cout << " (" << arredondar((double)estat[i].porCor[c] / estat[i].total * 100) << "%)";
            }
        }
        cout << endl;
    }
}

//Mateus: Lista as datas diferentes que aparecem nos atendimentos, ja em ordem (insertion pela data em numero).
vector<string> datasDistintas(){
    vector<string> datas;
    for(int i = 0; i < (int)atendimentos.size(); i++){
        bool existe = false;
        for(int j = 0; j < (int)datas.size(); j++){
            if(datas[j] == atendimentos[i].getData()){
                existe = true;
                break;
            }
        }
        if(!existe){
            datas.push_back(atendimentos[i].getData());
        }
    }
    for(int i = 1; i < (int)datas.size(); i++){
        string chave = datas[i];
        int j = i - 1;
        while(j >= 0 && dataParaNumero(datas[j]) > dataParaNumero(chave)){
            datas[j + 1] = datas[j];
            j--;
        }
        datas[j + 1] = chave;
    }
    return datas;
}

//Mateus: Quantos casos de uma doenca em uma data
int casosNoDia(string doenca, string data){
    int cont = 0;
    for(int i = 0; i < (int)atendimentos.size(); i++){
        if(atendimentos[i].getData() == data && padronizar(atendimentos[i].getDiagnostico()) == doenca){
            cont++;
        }
    }
    return cont;
}

/*
Mateus: Comparacao entre os dias e previsao.
Para cada doenca:
- mostra quantos casos teve em cada dia
- compara o ULTIMO dia com a MEDIA dos dias anteriores -> tendencia (subindo, estavel ou caindo)
- previsao para o proximo dia = media dos ultimos 3 dias (media movel). E uma estimativa simples, nao e exata.
- ALERTA de possivel surto: ultimo dia com pelo menos 3 casos e pelo menos o dobro da media anterior.
Precisa de atendimentos em pelo menos 2 datas diferentes para comparar.
*/
void tendenciaEPrevisao(){
    vector<string> datas = datasDistintas();
    if(datas.size() < 2){
        cout << "\nPara comparar e prever e preciso ter atendimentos em pelo menos 2 dias diferentes.\n";
        return;
    }
    vector<EstatisticaDoenca> estat = calcularEstatisticas();
    cout << "\n--- Comparacao entre dias e previsao (" << datas.size() << " dias, de " << datas[0] << " ate " << datas[datas.size() - 1] << ") ---\n";
    for(int i = 0; i < (int)estat.size(); i++){
        string doenca = estat[i].nome;
        vector<int> casos;      //casos[d] = casos dessa doenca no dia datas[d]
        for(int d = 0; d < (int)datas.size(); d++){
            casos.push_back(casosNoDia(doenca, datas[d]));
        }
        cout << "\n" << doenca << ": ";
        for(int d = 0; d < (int)casos.size(); d++){
            cout << datas[d].substr(0, 5) << "=" << casos[d] << " ";   //substr(0, 5) mostra so dd/mm
        }
        cout << endl;

        //media dos dias anteriores ao ultimo
        int ultimo = casos[casos.size() - 1];
        double somaAnteriores = 0;
        for(int d = 0; d < (int)casos.size() - 1; d++){
            somaAnteriores += casos[d];
        }
        double mediaAnterior = somaAnteriores / (casos.size() - 1);
        cout << "   Ultimo dia: " << ultimo << " | media dos dias anteriores: " << arredondar(mediaAnterior);
        if(ultimo > mediaAnterior * 1.2){
            cout << " | tendencia: SUBINDO\n";
        }
        else if(ultimo < mediaAnterior * 0.8){
            cout << " | tendencia: CAINDO\n";
        }
        else{
            cout << " | tendencia: ESTAVEL\n";
        }

        //previsao: media dos ultimos 3 dias (ou de todos, se tiver menos de 3)
        int qtdDias = 3;
        if((int)casos.size() < 3){
            qtdDias = casos.size();
        }
        double soma = 0;
        for(int d = casos.size() - qtdDias; d < (int)casos.size(); d++){
            soma += casos[d];
        }
        cout << "   Previsao para o proximo dia: cerca de " << arredondar(soma / qtdDias) << " caso(s)\n";

        if(ultimo >= 3 && ultimo >= mediaAnterior * 2){
            cout << "   ALERTA: possivel surto de " << doenca << "! Preparar equipe e leitos.\n";
        }
    }
}

void analiseDoencas(){
    if(atendimentos.empty()){
        cout << "Nenhum atendimento para analisar.\n";
        return;
    }
    int op = 0;
    while(true){
    cout << "\nAnalise de doencas:\n";
    cout << "1. Ocorrencia e porcentagem de cada doenca\n";
    cout << "2. Preparo: gravidade de cada doenca\n";
    cout << "3. Comparacao entre dias e previsao\n";
    cout << "0. Voltar\n";
    if(!(cin >> op) || op < 0 || op > 3){
        cout << "Escolha entre a opção 0 e 3!!\n";
        cin.clear();
        cin.ignore(1000, '\n');
        continue;
    }
    break;
    }
    switch(op){
        case 1:{
            rankingDoencas();
            break;
        }
        case 2:{
            preparoPorGravidade();
            break;
        }
        case 3:{
            tendenciaEPrevisao();
            break;
        }
        case 0:{
            break;
        }
    }
}

void relatoriosOrdenados(){
    //Mateus: quantidade de atendimentos por cor. O enum vale de 0 a 4, entao ele serve direto como indice do vetor.
    int quantidade[5] = {0, 0, 0, 0, 0};
    for(int i = 0; i < (int)atendimentos.size(); i++){
        quantidade[atendimentos[i].getGravidade()]++;
    }
    cout << "\nTotal de atendimentos: " << atendimentos.size() << "\n";
    for(int i = 0; i < 5; i++){
        cout << nomeGravidade((Gravidade)i) << ": " << quantidade[i] << "\n";
    }
    if(atendimentos.empty()){
        cout << "Nenhum atendimento realizado ainda.\n";
        return;
    }

    int op = 0;
    while(true){
    cout << "\nEscolha o relatorio:\n";
    cout << "1. Atendimentos por diagnostico\n";
    cout << "2. Atendimentos por paciente/CPF\n";
    cout << "3. Analise de doencas (ocorrencia, preparo e previsao)\n";
    cout << "0. Voltar\n";
    if(!(cin >> op) || op < 0 || op > 3){
        cout << "Escolha entre a opção 0 e 3!!\n";
        cin.clear();
        cin.ignore(1000, '\n');
        continue;
    }
    break;
    }

    vector<Atendimento> copia = atendimentos;
    switch(op){
        case 1:{
            mergeSort(copia, 0, copia.size() - 1, false);    //false = pelo diagnostico
            cout << "\n--- Atendimentos por diagnostico ---\n";
            //Mateus: como o vector ja esta ordenado, os diagnosticos iguais ficam juntos. Da para contar a recorrencia so olhando o vizinho.
            int repeticoes = 1;
            for(int i = 0; i < (int)copia.size(); i++){
                exibirAtendimento(copia[i]);
                if(i + 1 < (int)copia.size() && copia[i + 1].getDiagnostico() == copia[i].getDiagnostico()){
                    repeticoes++;
                }
                else{
                    cout << "   -> " << copia[i].getDiagnostico() << ": " << repeticoes << " atendimento(s)\n";
                    repeticoes = 1;
                }
            }
            break;
        }
        case 2:{
            mergeSort(copia, 0, copia.size() - 1, true);     //true = pelo CPF (os atendimentos de cada paciente ficam juntos e em ordem de data)
            cout << "\n--- Atendimentos por paciente (CPF) ---\n";
            for(int i = 0; i < (int)copia.size(); i++){
                exibirAtendimento(copia[i]);
            }
            break;
        }
        case 3:{
            analiseDoencas();
            break;
        }
        case 0:{
            break;
        }
    }
}

// ============================================================
// PAULO - prontuario e indice temporal
// ============================================================

// opcao 5: percorre o historico do paciente nos dois sentidos. e o que justifica a lista duplamente encadeada em vez da simples.
void navegarHistorico() {
    string cpf;
    cout << "\nCPF: ";
    cin >> cpf;

    Historico** historicoPonteiro = indiceHistoricos.buscar(cpf);
    if (historicoPonteiro == nullptr || (*historicoPonteiro)->vazio()) {
        cout << "Este paciente ainda nao tem atendimentos registrados.\n";
        return;
    }

    Historico* historicoPaciente = *historicoPonteiro;

    // nao chama irParaInicio()/irParaFim() aqui: o cursor continua de onde a ultima
    // navegacao (ou insercao) deixou, sem resetar a posicao a cada vez que o menu abre.
    char opcao;
    do {
        Atendimento a = historicoPaciente->atual();
        cout << "\n--- Atendimento (" << cpf << ") ---\n";
        cout << "Data: " << a.getData() << endl;
        cout << "Gravidade: " << nomeGravidade(a.getGravidade()) << endl;
        cout << "Diagnostico: " << a.getDiagnostico() << endl;
        cout << "Observacoes: " << a.getObservacoes() << endl;

        cout << "\n[A]nterior  [P]roximo  [S]air: ";
        cin >> opcao;
        opcao = toupper(opcao);

        if (opcao == 'A') {
            if (!historicoPaciente->anterior()) {
                cout << "Ja esta no atendimento mais antigo.\n";
            }
        } else if (opcao == 'P') {
            if (!historicoPaciente->proximo()) {
                cout << "Ja esta no atendimento mais recente.\n";
            }
        }
    } while (opcao != 'S');
}

// opcao 6: conta quantas vezes cada diagnostico aparece no historico do proprio paciente.
void verRecorrencia() {
    string cpf;
    cout << "\nCPF: ";
    cin >> cpf;

    Historico** historicoPonteiro = indiceHistoricos.buscar(cpf);
    if (historicoPonteiro == nullptr || (*historicoPonteiro)->vazio()) {
        cout << "Este paciente ainda nao tem atendimentos registrados.\n";
        return;
    }

    Historico* historicoPaciente = *historicoPonteiro;

    // conta quantas vezes cada diagnostico aparece. um vector<pair<string,int>> resolve
    // aqui porque a lista e pequena (so os atendimentos DESSE paciente) - nao precisa da
    // TabelaHash, que nem teria como listar as chaves que ela mesma guarda.
    vector<pair<string,int>> contagem;

    historicoPaciente->irParaInicio();
    do {
        Atendimento a = historicoPaciente->atual();
        string diagnostico = a.getDiagnostico();

        bool achou = false;
        for (size_t i = 0; i < contagem.size(); i++) {
            if (contagem[i].first == diagnostico) {
                contagem[i].second++;
                achou = true;
                break;
            }
        }
        if (!achou) {
            contagem.push_back(make_pair(diagnostico, 1));
        }
    } while (historicoPaciente->proximo());

    cout << "\n=== RECORRENCIA DE DIAGNOSTICOS (" << cpf << ") ===\n";
    for (size_t i = 0; i < contagem.size(); i++) {
        cout << "  " << contagem[i].first << ": " << contagem[i].second << "\n";
    }
}

// le uma data dd/mm/aaaa e devolve no formato aaaammdd.
int lerData(const string& rotulo) {
    string texto;
    while (true) {
        cout << rotulo;
        if (!getline(cin, texto)) {
            return 0;
        }
        int numero = dataParaNumero(texto);
        if (numero != 0) {
            return numero;
        }
        cout << "  Use o formato dd/mm/aaaa, por exemplo 01/03/2026.\n";
    }
}

// opcao 7: atendimentos entre duas datas, usando a arvore AVL.
void consultarPorPeriodo() {
    if (indiceDatas.quantidadeValores() == 0) {
        cout << "\nNenhum atendimento registrado ainda.\n";
        return;
    }

    cout << "\n";
    int inicio = lerData("Data inicial (dd/mm/aaaa): ");
    int fim = lerData("Data final (dd/mm/aaaa): ");
    if (inicio == 0 || fim == 0) {
        return;
    }
    if (fim < inicio) {
        cout << "A data final e anterior a data inicial.\n";
        return;
    }

    vector<Atendimento> encontrados = indiceDatas.buscarIntervalo(inicio, fim);

    cout << "\n--- " << encontrados.size() << " atendimento(s) no periodo ---\n";
    for (size_t i = 0; i < encontrados.size(); i++) {
        Paciente* p = indicePacientes.buscar(encontrados[i].getCpfPaciente());
        cout << encontrados[i].getData() << " | "
             << left << setw(20) << (p == nullptr ? "(nao cadastrado)" : p->getNome()) << right << " | "
             << left << setw(8) << nomeGravidade(encontrados[i].getGravidade()) << right << " | "
             << encontrados[i].getDiagnostico() << "\n";
    }

    // altura da arvore e nos visitados, para mostrar a AVL funcionando.
    int datas = indiceDatas.quantidadeChaves();
    cout << "\nIndice por data: " << datas << " data(s), arvore com altura " << indiceDatas.altura()
         << " (sem o balanceamento AVL, poderia chegar a " << datas << ").\n";
    cout << "Nos visitados nesta consulta: " << indiceDatas.nosVisitados() << " de " << datas << ".\n";
}

// ============================================================

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);    // faz os acentos aparecerem no console do windows
#endif
    int opcao;

    // do-while: o menu aparece pelo menos uma vez.
    do {
        exibirMenu();
        opcao = lerOpcao();

        switch (opcao) {
            case 1: cadastrarPaciente();   break;
            case 2: realizarTriagem();     break;
            case 3: chamarProximo();       break;
            case 4: consultarProntuario(); break;
            case 5: navegarHistorico();    break;
            case 6: verRecorrencia();      break;
            case 7: consultarPorPeriodo(); break;
            case 8: verEstadoDaFila();     break;
            case 9: relatorios();          break;
            case 10: salvarCarregarDados(); break;
            case 0: cout << "\nEncerrando.\n"; break;

            // default trata qualquer numero fora das opcoes acima
            default: cout << "\nOpcao invalida.\n"; break;
        }
    } while (opcao != 0);

    // libera os Historico criados com new antes do programa terminar.
    limparHistoricos();

    return 0;
}