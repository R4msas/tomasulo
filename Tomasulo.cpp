#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
using namespace std;

class Instrucao
{
public:
    string operacao;  // Tipo de operação (ADD, SUB, MUL, DIV)
    string destino;   // Registrador de Destino
    string operando1; // Operando 1
    string operando2; // Operando 2
    int ciclosExecucaoRestantes;
    bool opEmissao;
    bool emExecucao;
    bool opCompleta;

    /*
        Instrução recebe:
        - Operação
        - Registrador de Destino
        - Operando 1 (MUL F0 *F2* F4)
        - Operando 2 (MUL F0 F2 *F4*)

        Todas as outras propriedades são marcadas como falsas, para o início da execução
    */
    Instrucao(string op, string dest, string src1, string src2)
        : operacao(op), destino(dest), operando1(src1), operando2(src2), ciclosExecucaoRestantes(0), opEmissao(false), emExecucao(false), opCompleta(false) {}
};

class UnidadeFuncional
{
public:
    string tipoUnidade; // Tipo da Unidade Funcional (ADD, SUB, MUL, DIV)
    int ciclosExecucao; // Quantidade de ciclos que a instrução demora a ser executada
    bool estaOcupada;
    Instrucao *instrucao; // Instrução em execução

    /*
        Unidade Funcional recebe:
        - Tipo da Unidade
        - Número de ciclos que ela demora a executar

        Todas as outras propriedades são marcadas como falsas, para o início da execução
    */
    UnidadeFuncional(string type, int latency)
        : tipoUnidade(type), ciclosExecucao(latency), estaOcupada(false), instrucao(nullptr) {}
};

class Registrador
{
public:
    string nomeRegistrador; // Nome do Registrador
    int valorRegistrador;   // Valor atual do Registrador
    bool ocupadoRead;       // Busy Read
    bool ocupadoWrite;      // Busy Write
    Instrucao *instrucao;   // Instrução ocupando o Registrador

    /*
        Registrador recebe:
        - Nome do Registrador
        - Valor do Registrador

        Todas as outras propriedades são marcadas como falsas, para o início da execução
    */
    Registrador(string name, int value)
        : nomeRegistrador(name), valorRegistrador(value), ocupadoRead(false), ocupadoWrite(false), instrucao(nullptr) {}
};

// Algoritmo de Tomasulo
class Tomasulo
{
public:
    vector<Instrucao *> listaInstrucoes; // Lista de Instruções
    vector<UnidadeFuncional *> unidadesADD;
    vector<UnidadeFuncional *> unidadesMUL;
    vector<UnidadeFuncional *> unidadesSW;
    vector<Registrador *> listaRegistradores;
    map<string, string> renomear;
    int x = -1;
    int ciclo; // Ciclo atual
    vector<int> memoriaCache;

    /*
        O construtor de Tomasulo é iniciado recebendo as instruções, um mapa com o tipo de operação e a duração dos ciclos execução,
        o número do ciclo inicial e a memória cache.
        A quantidade de ciclos de execução será passada pelo código na execução do 'main'.
        A memória cache foi definida como tendo 32 endereços de valores '2'.
    */
    Tomasulo(vector<Instrucao *> instrucoes, map<string, int> valores)
        : listaInstrucoes(instrucoes), ciclo(1), memoriaCache(32, 2)
    {
        /*
            Fizemos um reaproveitamento no código, tratando as unidades de SUB junto com ADD, DIV junto com MUL e LW junto com SW.
            Embora não seja um reflexo da arquitetura real, existe a vantagem de simplificar a implementação.
        */
        unidadesADD = criarUnidadeFuncional(valores["unidadeAddQtd"], "add", valores["unidadeAddLatencia"]);
        unidadesMUL = criarUnidadeFuncional(valores["unidadeMulQtd"], "mul", valores["unidadeMulLatencia"]);
        unidadesSW = criarUnidadeFuncional(valores["unidadeSwQtd"], "sw", valores["unidadeSwLatencia"]);
        listaRegistradores = criarRegistradores(valores["qtdRegistradores"]);
    }

    // Criar as Unidades Funcionais
    vector<UnidadeFuncional *> criarUnidadeFuncional(int numeroUnidades, string tipo, int latencia)
    {
        vector<UnidadeFuncional *> unidades;
        for (int i = 0; i < numeroUnidades; i++)
        {
            unidades.push_back(new UnidadeFuncional(tipo, latencia));
        }
        return unidades;
    }

    // Criar os Registradores
    vector<Registrador *> criarRegistradores(int quant)
    {
        vector<Registrador *> registrador;
        for (int i = 0; i < quant; i++)
        {
            registrador.push_back(new Registrador("F" + to_string(i), 1));
        }
        for (int i = 0; i < quant; i++)
        {
            registrador.push_back(new Registrador("R" + to_string(i), 1));
        }
        return registrador;
    }

    void executarTomasulo()
    {
        while (!execucaoCompleta()) // Loop pela execução
        {
            emissao();                // Estágio de emissão (Issue)
            executar();               // Estágio de execução (Execution)
            escrever();               // Estágio de escrita (Write-Back)
            exibirEstadoAtual(false); // Exibir apenas os Registradores que estão sendo utilizados
            ciclo++;
        }
        exibirEstadoAtual(false);

        printf("\nEXECUÇÃO FINALIZADA\n");
    }

    void exibirEstadoAtual(bool exibirApenasEmUso)
    {
        printf("\n=====X=====X=====X=====X=====\n");
        printf("\n[ CICLO %d ]\n", ciclo); // Exibir o ciclo atual

        // Instruções em Espera
        printf("\n>> Instruções em Espera:");
        for (const auto &instrucao : listaInstrucoes)
        {
            // Exibir apenas as instruções que não estão nem execução e que também não foram finalizadas
            if (!instrucao->emExecucao && !instrucao->opCompleta)
            {
                printf("\nInstrução: %s %s %s %s",
                       instrucao->operacao.c_str(),
                       instrucao->destino.c_str(),
                       instrucao->operando1.c_str(),
                       instrucao->operando2.c_str());
            }
        }

        // Exibir Estações de Reserva
        printf("\n\n>> Estações de Reserva:");
        for (const auto &unidade : unidadesADD)
        {
            exibirEstadoEstacao(unidade);
        }
        for (const auto &unidade : unidadesMUL)
        {
            exibirEstadoEstacao(unidade);
        }
        for (const auto &unidade : unidadesSW)
        {
            exibirEstadoEstacao(unidade);
        }

        // Valores dos Registradores
        printf("\n\n>> Valores dos Registradores:");
        for (const auto &registrador : listaRegistradores)
        {
            // Apenas exibe os registradores ocupados, se o booleano for verdadeiro
            if (!exibirApenasEmUso || registrador->ocupadoRead || registrador->ocupadoWrite)
            {
                printf("\n%s: %d\t\t[ Busy Read: %s ] [ Busy Write: %s ]",
                       registrador->nomeRegistrador.c_str(),
                       registrador->valorRegistrador,
                       registrador->ocupadoRead ? "Sim" : "Não",
                       registrador->ocupadoWrite ? "Sim" : "Não");
            }
        }

        printf("\n");
    }

    void exibirEstadoEstacao(UnidadeFuncional *unidade)
    {
        if (unidade->estaOcupada)
        {
            printf("\nEstação: %s", unidade->tipoUnidade.c_str());
            printf("\n\tInstrução: %s %s %s %s",
                   unidade->instrucao->operacao.c_str(),
                   unidade->instrucao->destino.c_str(),
                   unidade->instrucao->operando1.c_str(),
                   unidade->instrucao->operando2.c_str());
            printf("\n\tCiclos restantes: %d", unidade->instrucao->ciclosExecucaoRestantes);
        }
        else
        {
            printf("\nEstação: %s (vazia)", unidade->tipoUnidade.c_str());
        }
    }

    // Verifica se a execução está completa
    bool execucaoCompleta()
    {
        for (size_t i = 0; i < listaInstrucoes.size(); i++)
        {
            if (!listaInstrucoes[i]->opCompleta)
            {
                return false;
            }
        }
        return true;
    }

    void renomearRegistrador(Registrador *target, size_t index)
    {
        x++; // Aumentar o número de registradores temporários que são criados, para entrar no mapa
        size_t posicaoRegistradorTemporario = listaRegistradores.size() / 2;
        string novoNome = "X" + to_string(x);

        // Loop para encontrar registrador temporário disponível
        while (listaRegistradores[posicaoRegistradorTemporario]->ocupadoRead || listaRegistradores[posicaoRegistradorTemporario]->ocupadoWrite)
        {
            posicaoRegistradorTemporario++;
            if (posicaoRegistradorTemporario >= listaRegistradores.size())
            {
                return;
            }
            else
            {
                novoNome = listaRegistradores[posicaoRegistradorTemporario]->nomeRegistrador;
            }
        }

        if (renomear.find(target->nomeRegistrador) != renomear.end())
        {
            renomear[novoNome] = renomear[target->nomeRegistrador];
        }
        else
        {
            renomear[novoNome] = target->nomeRegistrador;
        }

        // Exibição da renomeação
        cout << "Renomeação: Registrador " << target->nomeRegistrador << " renomeado para " << novoNome << " no ciclo " << ciclo << endl;

        // Renomear próximas ocorrências do registrador alvo
        for (size_t i = index; i < listaInstrucoes.size(); i++)
        {
            if (listaInstrucoes[i]->destino == target->nomeRegistrador)
            {
                listaInstrucoes[i]->destino = novoNome;
            }

            if (listaInstrucoes[i]->operando1 == target->nomeRegistrador)
            {
                listaInstrucoes[i]->operando1 = novoNome;
            }

            if (listaInstrucoes[i]->operando2 == target->nomeRegistrador)
            {
                listaInstrucoes[i]->operando2 = novoNome;
            }
        }
    }

    // Faz o estágio de emissão (issue)
    void emissao()
    {
        size_t restricao = listaInstrucoes.size();
        if (ciclo < listaInstrucoes.size())
        {
            restricao = ciclo;
        }
        for (size_t i = 0; i < restricao; i++)
        {
            Instrucao *instrucao = listaInstrucoes[i];
            UnidadeFuncional *unidade = nullptr;
            if (instrucao->emExecucao || instrucao->opCompleta || !instrucao->opEmissao)
            {
                // Verifica se há uma unidade funcional disponível para a instrução
                instrucao->opEmissao = true;
                unidade = nullptr;
            }
            else if (instrucao->operacao == "add" || instrucao->operacao == "sub")
            {
                unidade = encontrarUnidadeDisponivel(unidadesADD);
            }
            else if (instrucao->operacao == "mul" || instrucao->operacao == "div")
            {
                unidade = encontrarUnidadeDisponivel(unidadesMUL);
            }
            else if (instrucao->operacao == "sw" || instrucao->operacao == "lw")
            {
                unidade = encontrarUnidadeDisponivel(unidadesSW);
            }

            if (unidade)
            {
                // Ajusta para caso de SW e LW
                Registrador *aux;
                if (instrucao->operacao == "sw" || instrucao->operacao == "lw")
                {
                    aux = new Registrador("aux", stoi(instrucao->operando1));
                }
                else
                {
                    aux = getRegistrador(instrucao->operando1);
                }

                // Verifica se os operandos estão disponíveis
                Registrador *registradorDestino = getRegistrador(instrucao->destino);
                Registrador *registradorOp1 = aux;
                Registrador *regsitradorOp2 = getRegistrador(instrucao->operando2);

                // Dependencia falsa
                if (registradorDestino->ocupadoRead || registradorDestino->ocupadoWrite)
                {
                    renomearRegistrador(registradorDestino, i);
                }

                // Dependencia verdadeira
                if (!registradorOp1->ocupadoWrite && !regsitradorOp2->ocupadoWrite)
                {
                    // Define a instrução como executando
                    instrucao->emExecucao = true;
                    instrucao->ciclosExecucaoRestantes = unidade->ciclosExecucao;

                    // Define a unidade funcional como ocupada
                    unidade->estaOcupada = true;
                    unidade->instrucao = instrucao;

                    // Atualiza os registradores utilizados pela instrução
                    registradorDestino->ocupadoWrite = true;
                    registradorDestino->instrucao = instrucao;

                    registradorOp1->ocupadoRead = true;
                    registradorOp1->instrucao = instrucao;

                    regsitradorOp2->ocupadoRead = true;
                    regsitradorOp2->instrucao = instrucao;
                }
            }
        }
    }

    // Fazer o estágio de execução
    void executar()
    {
        for (size_t i = 0; i < unidadesADD.size(); i++)
        {
            UnidadeFuncional *unidade = unidadesADD[i];
            if (unidade->estaOcupada)
            {
                unidade->instrucao->ciclosExecucaoRestantes--;
                if (unidade->instrucao->ciclosExecucaoRestantes == 0)
                {
                    unidade->instrucao->emExecucao = false;
                }
            }
        }

        for (size_t i = 0; i < unidadesMUL.size(); i++)
        {
            UnidadeFuncional *unit = unidadesMUL[i];
            if (unit->estaOcupada)
            {
                unit->instrucao->ciclosExecucaoRestantes--;
                if (unit->instrucao->ciclosExecucaoRestantes == 0)
                {
                    unit->instrucao->emExecucao = false;
                }
            }
        }

        for (size_t i = 0; i < unidadesSW.size(); i++)
        {
            UnidadeFuncional *unit = unidadesSW[i];
            if (unit->estaOcupada)
            {
                unit->instrucao->ciclosExecucaoRestantes--;
                if (unit->instrucao->ciclosExecucaoRestantes == 0)
                {
                    unit->instrucao->emExecucao = false;
                }
            }
        }
    }

    // Fazer o estágio de escrita
    void escrever()
    {
        escreveExecucao(unidadesADD);
        escreveExecucao(unidadesMUL);
        escreveExecucao(unidadesSW);
    }

    // Executa as funções da escrita
    void escreveExecucao(vector<UnidadeFuncional *> &tipoUnidade)
    {

        for (size_t i = 0; i < tipoUnidade.size(); i++)
        {
            UnidadeFuncional *unidade = tipoUnidade[i];
            if (unidade->estaOcupada && !unidade->instrucao->emExecucao && !unidade->instrucao->opCompleta)
            {
                unidade->instrucao->opCompleta = true;
                unidade->estaOcupada = false; // Liberar a unidade

                // Atualiza o valor do registrador de destino
                Registrador *registradorDestino = getRegistrador(unidade->instrucao->destino);
                Registrador *registradorOp1 = getRegistrador(unidade->instrucao->operando1);
                Registrador *registradorOp2 = getRegistrador(unidade->instrucao->operando2);

                if (unidade->instrucao->operacao == "sw" || unidade->instrucao->operacao == "lw")
                {
                    int offset = executaOperacaoMatematica("add", stoi(unidade->instrucao->operando1), registradorOp2->valorRegistrador);

                    if (unidade->instrucao->operacao == "sw")
                    {
                        // Evitar out of bounds com resto da divisão (%)
                        memoriaCache[offset % memoriaCache.size()] = registradorDestino->valorRegistrador;
                    }
                    else if (unidade->instrucao->operacao == "lw")
                    {
                        registradorDestino->valorRegistrador = memoriaCache[offset % memoriaCache.size()];
                    }
                }
                else
                {
                    registradorDestino->valorRegistrador = executaOperacaoMatematica(unidade->instrucao->operacao, registradorOp1->valorRegistrador, registradorOp2->valorRegistrador);

                    registradorOp1->ocupadoRead = false;
                    registradorOp1->instrucao = nullptr;
                }

                // Libera os registradores utilizados pela instrução
                registradorDestino->ocupadoWrite = false;
                registradorDestino->instrucao = nullptr;

                if (renomear.find(registradorDestino->nomeRegistrador) != renomear.end())
                {
                    retornaRenomeado(registradorDestino, i);
                }

                registradorOp2->ocupadoRead = false;
                registradorOp2->instrucao = nullptr;
            }
        }
    }

    void retornaRenomeado(Registrador *target, size_t index)
    {
        // Renomear os valores na lista de instrução
        for (size_t i = 0; i < listaInstrucoes.size(); i++)
        {
            if (listaInstrucoes[i]->destino == target->nomeRegistrador)
            {
                listaInstrucoes[i]->destino = renomear[target->nomeRegistrador];
            }

            if (listaInstrucoes[i]->operando1 == target->nomeRegistrador)
            {
                listaInstrucoes[i]->operando1 = renomear[target->nomeRegistrador];
            }

            if (listaInstrucoes[i]->operando2 == target->nomeRegistrador)
            {
                listaInstrucoes[i]->operando2 = renomear[target->nomeRegistrador];
            }
        }

        // Apaga da lista de renomeados
        renomear.erase(target->nomeRegistrador);
    }

    // Encontra uma unidade funcional disponível
    UnidadeFuncional *encontrarUnidadeDisponivel(vector<UnidadeFuncional *> &units)
    {
        for (size_t i = 0; i < units.size(); i++)
        {
            if (!units[i]->estaOcupada)
            {
                return units[i];
            }
        }
        return nullptr;
    }

    // Obtém o registro pelo nome
    Registrador *getRegistrador(const string &name)
    {
        // voltarAqui

        for (size_t i = 0; i < listaRegistradores.size(); i++)
        {
            if (listaRegistradores[i]->nomeRegistrador == name)
            {
                return listaRegistradores[i];
            }
        }

        if (name.at(0) == 'X')
        {
            int pos = 0;
            string nomeNovo = renomear[name];

            if (nomeNovo.at(0) == 'R')
            {
                pos = 16;
            }

            string numero = nomeNovo.substr(1);
            pos += stoi(numero);

            return listaRegistradores[pos];
        }
        else
        {
            return nullptr;
        }
    }

    // Executar a Operação Matemática
    int executaOperacaoMatematica(const string &op, int src1, int src2)
    {
        if (op == "add")
        {
            return src1 + src2;
        }
        else if (op == "sub")
        {
            return src1 - src2;
        }
        else if (op == "mul")
        {
            return src1 * src2;
        }
        else if (op == "div")
        {
            return src1 / src2;
        }

        return 0;
    }
};

// Função para liberar a memória alocada para as instruções
void liberarMemoriaAlocada(vector<Instrucao *> &instructions)
{
    for (const auto &instrucao : instructions)
    {
        delete instrucao;
    }
}

int main()
{
    // Leitura das instruções a partir de um arquivo de texto
    vector<Instrucao *> instrucoes;
    ifstream arquivoInput("instrucoes.txt"); // Nome do arquivo de texto

    if (!arquivoInput.is_open())
    {
        cerr << "Erro ao abrir o arquivo." << endl;
        return 1;
    }

    string linha;
    while (getline(arquivoInput, linha))
    {
        istringstream iss(linha);
        string op, dest, src1, src2;
        iss >> op >> dest >> src1 >> src2;
        instrucoes.push_back(new Instrucao(op, dest, src1, src2));
    }

    arquivoInput.close();

    // Preset de latências por instrução, quantidade de registradores e quantidade de unidades lógicas
    map<string, int> valores = {
        {"unidadeAddLatencia", 3},
        {"unidadeMulLatencia", 10},
        {"unidadeSwLatencia", 2},
        {"unidadeAddQtd", 3},
        {"unidadeMulQtd", 2},
        {"unidadeSwQtd", 2},
        {"qtdRegistradores", 16}};

    vector<int> memoriaCache(32, 2); // Inicialização do cache com valores diferentes

    Tomasulo tomasulo(instrucoes, valores);
    tomasulo.executarTomasulo();

    // Liberar a memória alocada para as instruções
    liberarMemoriaAlocada(instrucoes);

    return 0;
}