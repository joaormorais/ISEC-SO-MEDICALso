#include "util1.h"

int saiuUtente = 0; // variável global para saber se saiu um utente 
int idUtentesaiu = 0; // variável global para saber o ID do utente que saiu
int saiuMedico = 0; // variável global para saber se saiu um medico
int idMedicosaiu = 0; // variável global para saber o ID do medico que saiu
int fdBAL3; // return do fifo BAL3 (é global para poder ser usado na thread)

void *receboVivo() {

    alarme alarme;

    while (1) {

        // le do seu proprio fifo (bal3 - feito especificamente para receber apenas este read)
        int n = read(fdBAL3, &alarme, sizeof (alarme));

        if (n > 0) {
            // mostra qual foi o medico que disse estar online
            printf("O medico nº %d avisou-me que esta online!\n", alarme.idRemetente);
            fflush(stdout);
        }

    }

}

int procuraFilaespera(utente *uts, int utentesAtivos) {

    // funçao para contar o numero de utentes online

    int contador = 0;

    for (int i = 0; i < utentesAtivos; i++)
        if (uts[i].atendido == 0)
            contador++;

    return contador;

}

int procuraRepetido(medico *especialistas, medico auxiliar, int medicosAtivos) {

    // uma vez que o medico pode estar a voltar de uma consulta
    // nao queremos que o programa guarde esse medico numa nova posicao do array
    // mas sim na mesma onde ja estava
    // entao vamos procurar se ele tem um id igual aos dos medicos existentes

    int retorno = -1;

    for (int i = 0; i < medicosAtivos; i++) {

        if (especialistas[i].id == auxiliar.id) {
            retorno = i;
            break;
        }

    }

    // ou devolve -1
    // ou devolve um id repetido
    return retorno;

}

int atribuiUtente(char especialidade[40], int utentesAtivos, utente *uts, int idMedico) {

    // funçao para atribuir um utente a um medico

    utente aux[utentesAtivos];
    int j = 0, urgenciaAux = 4, idAux = 0;

    for (int i = 0; i < utentesAtivos; i++) { // procura pelos utentes com a mesma especialidade

        if (strcmp(uts[i].especialidade, especialidade) == 0 && uts[i].atendido == false) {
            aux[i] = uts[j];
            j++;
        }

    }

    for (int i = 0; i < utentesAtivos; i++) { // agora procura pelos utentes com mais urgencia

        if (aux[i].urgencia < urgenciaAux) { // se o utente for mais urgente que o outro
            urgenciaAux = aux[i].urgencia;
            if (urgenciaAux == 1) {
                idAux = aux[i].id;
                break;
            }
            idAux = aux[i].id;
        }

    }

    // depois de atribuir um cliente a um medico
    // como é obvio, tb temos de informar ao cliente qual o medico que lhe foi atribuido

    for (int i = 0; i < utentesAtivos; i++) { // procura pelo utente escolhido e coloca a sua flag a true

        if (uts[i].id == idAux) { // aqui vamos escrever no FIFO do cliente

            char str[40];

            sprintf(str, FIFO_CLI, uts[i].id); // definir o nome do fifo cli

            int tempfd_returnCLI = open(str, O_WRONLY); // abrir o fifo cli para escrita

            if (tempfd_returnCLI < 0) {
                printf("Nao foi possivel abrir o fifo cli [atribuiUtente]!\n");
                exit(1);
            }

            uts[i].atendido = true;
            uts[i].idMedico = idMedico;

            // escrever a estrutura do cliente
            int n = write(tempfd_returnCLI, &uts[i], sizeof (utente));

            if (n < 0) {
                printf("Nao foi possivel escrever no fifo cli [atribuiUtente]!\n");
                fflush(stdout);
                close(tempfd_returnCLI);
                exit(1);
            }

            // fechar o fifo cli
            close(tempfd_returnCLI);

            break;
        }

    }

    return idAux; // retorna o id do utente escolhido

}

int atribuiMedico(char especialidade[40], int medicosAtivos, medico *especialistas, int idUtente) {

    // funçao para atribuir um medico a um Utente
    // retorna o id do Medico para depois ser escrito no cliente
    // e aqui dentro escreve no medico o id do cliente

    for (int i = 0; i < medicosAtivos; i++) {

        if (strcmp(especialistas[i].especialidade, especialidade) == 0 && especialistas[i].atender == false) {

            char str2[40];

            sprintf(str2, FIFO_MED, especialistas[i].id); // definir o nome do fifo med

            int tempfd_returnMED = open(str2, O_WRONLY); // abrir o fifo med para escrita

            if (tempfd_returnMED < 0) {
                printf("Nao foi possivel abrir o fifo med [atribuiMedico]!\n");
                exit(1);
            }

            especialistas[i].atender = true;
            especialistas[i].idCliente = idUtente;

            int n = write(tempfd_returnMED, &especialistas[i], sizeof (medico)); // escrever a estrutura do medico

            if (n < 0) {
                printf("Nao foi possivel escrever no fifo med [atribuiMedico]!\n");
                fflush(stdout);
                close(tempfd_returnMED);
                exit(1);
            }

            close(tempfd_returnMED); // fechar o fifo med

            return especialistas[i].id;

        }

    }

    return 0;

}

void encerraUtilizadores(utente *utentes, medico *especialistas, int *utentesAtivos, int *medicosAtivos) {

    // desliga todos os utentes e especialistas

    for (int i = 0; i<*utentesAtivos; i++) {
        kill(utentes[i].id, SIGINT);
        printf("Encerrado o utente com o ID %d\n", utentes[i].id);
        fflush(stdout);
    }


    for (int i = 0; i<*medicosAtivos; i++) {
        kill(especialistas[i].id, SIGINT);
        printf("Encerrado o especialista com o ID %d\n", especialistas[i].id);
        fflush(stdout);
    }

}

bool verificaUTmax(int ativos, int utentesMAX) { // verificar se o número de utentes máximo foi atingido

    if (ativos == utentesMAX)
        return true;

    return false;

}

bool verificaMEDmax(int ativos, int medicosMAX) { // verificar se o número de médicos máximo foi atingido

    if (ativos == medicosMAX)
        return true;

    return false;

}

void apagaUtente(utente *utentes, int id, int *utentesAtivos) { // apagar um utente com um determinado ID

    bool despedido = false;

    for (int i = 0; i < *utentesAtivos; i++) {

        if (utentes[i].id == id) {

            for (int j = i; j < *utentesAtivos - 1; j++) {

                utentes[j] = utentes[j + 1];

            }

            (*utentesAtivos)--;
            despedido = true;
            printf("Foi apagado o utente com o ID %d.\n", id);
            kill(id, SIGINT);

            break;

        }

    }

    if (despedido == false)
        printf("Nao existe nenhum utente com esse ID!\n");

}

void apagaMedico(medico *especialistas, int id, int *medicosAtivos) { // apagar um médico com um determinado ID

    bool despedido = false;

    for (int i = 0; i < *medicosAtivos; i++) {

        if (especialistas[i].id == id) {

            for (int j = i; j < *medicosAtivos - 1; j++) {

                especialistas[j] = especialistas[j + 1];

            }

            (*medicosAtivos)--;
            despedido = true;
            printf("Foi apagado o medico com o ID %d.\n", id);
            kill(id, SIGINT);

            break;

        }

    }

    if (despedido == false)
        printf("Nao existe nenhum medico com esse ID!\n");

}

void sigusr1Handler(int sig, siginfo_t *si, void *data) {

    // handler para quando o utente desliga

    saiuUtente = 1;
    idUtentesaiu = si->si_pid;

    printf("O utente com o ID %d avisou que se vai embora!\n", si->si_pid);
    fflush(stdout);

}

void sigusr2Handler(int sig, siginfo_t *si, void *data) {

    // handler para quando o medico desliga

    saiuMedico = 1;
    idMedicosaiu = si->si_pid;

    printf("O medico com o ID %d avisou que se vai embora!\n", si->si_pid);
    fflush(stdout);

}

void lecomandos(char comando[40], int *utentesAtivos, int *medicosAtivos, utente *utentes, medico *especialistas, int *tempoalterado) { // ler os comandos do ADMIN

    char* piece = strtok(comando, " ");
    char* primeiroargumento = piece;

    piece = strtok(NULL, "\n");
    char* segundoargumento = piece;

    if (strcmp(comando, "utentes\n") == 0) {

        printf("Lista de utentes: \nUtentes Ativos = %d\n", *utentesAtivos);
        fflush(stdout);
        for (int i = 0; i < *utentesAtivos; i++) {

            printf("\n");
            fflush(stdout);
            printf("ID = %d\n", utentes[i].id);
            fflush(stdout);
            printf("Nome = %s\n", utentes[i].nome);
            fflush(stdout);
            printf("Sintomas = %s", utentes[i].sintomas);
            fflush(stdout);

            if (utentes[i].atendido == false) {
                printf("Estado: Nao atendido\n");
                fflush(stdout);
            } else {
                printf("Estado: Atendido\n");
                fflush(stdout);
            }

            printf("Especialidade = %s\n", utentes[i].especialidade);
            fflush(stdout);
            printf("Urgencia = %d\n", utentes[i].urgencia);
            fflush(stdout);
            printf("\n");
            fflush(stdout);

        }

    } else if (strcmp(comando, "especialistas\n") == 0) {

        printf("Lista de especialistas: \nEspecialistas Ativos = %d\n", *medicosAtivos);
        fflush(stdout);
        for (int i = 0; i < *medicosAtivos; i++) {

            printf("\n");
            fflush(stdout);
            printf("ID = %d\n", especialistas[i].id);
            fflush(stdout);
            printf("Nome = %s\n", especialistas[i].nome);
            fflush(stdout);
            printf("Especialidade = %s\n", especialistas[i].especialidade);
            fflush(stdout);

            if (especialistas[i].atender == false) {
                printf("Estado: Nao esta a atender\n");
                fflush(stdout);
            } else {
                printf("Estado: Esta a atender\n");
                fflush(stdout);
            }

            printf("\n");
            fflush(stdout);

        }

    } else if (strcmp(primeiroargumento, "delut") == 0) {

        int seg = atoi(segundoargumento);
        apagaUtente(utentes, seg, utentesAtivos);

    } else if (strcmp(primeiroargumento, "delesp") == 0) {

        int seg = atoi(segundoargumento);
        apagaMedico(especialistas, seg, medicosAtivos);

    } else if (strcmp(primeiroargumento, "freq") == 0) {

        int seg = atoi(segundoargumento);
        *tempoalterado = seg;
        printf("O tempo de apresentação de informação passou para %d segundos.\n", *tempoalterado);
        fflush(stdout);

    } else if (strcmp(primeiroargumento, "list\n") == 0) {

        printf("'utentes' - mostra a lista de utentes existentes\n");
        fflush(stdout);
        printf("'especialistas' - mostra a lista de especialistas existentes\n");
        fflush(stdout);
        printf("'delut ID' - apaga o utente com o ID indicado\n");
        fflush(stdout);
        printf("'delesp ID' - apaga o especialista com o ID indicado\n");
        fflush(stdout);
        printf("'freq N' - altera o tempo de intervalo entre a apresentacao de informacao para N segundos\n");
        fflush(stdout);
        printf("'encerra' - termina o balcao e desliga todos os utentes e especialistas\n");
        fflush(stdout);

    } else {

        printf("Esse comando nao existe!\n");
        fflush(stdout);

    }

}

void atribuiresposta(char respostaclassificador[40], utente *ut) { // divide a resposta do classificador entre especialidade e urgência

    char* piece = strtok(respostaclassificador, " ");
    char* primeiroargumento = piece;

    piece = strtok(NULL, "\n");
    char* segundoargumento = piece;

    int seg = atoi(segundoargumento);

    strcpy(ut->especialidade, primeiroargumento);
    ut->urgencia = seg;

}

void classifica(utente *ut, int *fd1, int *fd2) { // comunica com o classificador e devolve uma resposta consoante os sintomas

    char sintoma[80]; // declarar o sintoma do utente
    strcpy(sintoma, ut->sintomas); // inicializar o sintoma do utente
    char resposta[80]; // resposta do classificador ao sintoma do utente
    char *devolve = malloc(80); // declarar a especialidade do utente

    // PROGRAMA PAI

    close(fd1[0]); // fechar a leitura do pipe nº1
    close(fd2[1]); // fechar a escrita do pipe nº2

    if (write(fd1[1], sintoma, strlen(sintoma)) == -1) { // escrever no pipe nº1 e verificar se dá erro
        printf("[ERRO] - Não foi possível escrever no pipe 1!\n");
        fflush(stdout);
    }

    int n = read(fd2[0], resposta, sizeof (resposta) - 1); // ler o pipe nº2 e 

    if (n == -1) { // verificar se dá erro a ler o pipe nº2
        printf("[ERRO] - Não foi possível ler o pipe 2!\n");
        fflush(stdout);
    } else {
        // se não der erro, vamos atribuir a resposta do classificador ao utente
        atribuiresposta(resposta, ut);
    }

}

int main(int argc, char *argv[]) {

    int idBalcao = getpid(); // o id do Balcao é o numero do seu processo
    printf("Bem-Vindo!\nO Balcao %d foi ligado.\n\n", idBalcao);
    fflush(stdout);

    /* inicio - inicializar/abrir os pipes e fazer fork() */

    int fd1[2]; // PIPE nº1 - fd1[0] - ler    fd1[1] - escrever
    int fd2[2]; // PIPE nº2 - fd2[0] - ler    fd2[1] - escrever

    if (pipe(fd1) == -1) { // verificar se ocorre algum erro ao fazer pipe() nº1
        printf("[ERRO] - Não foi possível abrir o pipe!\n");
        fflush(stdout);
        exit(1);
    }

    if (pipe(fd2) == -1) { // verificar se ocorre algum erro ao fazer pipe() nº2
        printf("[ERRO] - Não foi possível abrir o pipe 2!\n");
        fflush(stdout);
        exit(1);
    }

    int fork_return = fork(); // receber o retorno do fork()

    if (fork_return < 0) { // verificar se ocorre algum erro ao fazer fork()
        printf("[ERRO] - Não foi possível criar um processo filho\n");
        fflush(stdout);
        exit(1);
    }

    if (fork_return == 0) { // PROGRAMA FILHO - escreve no classificador e lê do classificador

        close(0); // fechar o stdin
        dup(fd1[0]); // trocar o stdin para a leitura do pipe nº1
        close(fd1[0]); // fechar a leitura do pipe nº1    
        close(fd1[1]); // fechar a escrita do pipe nº1

        close(1); // fechar o stdout
        dup(fd2[1]); // trocar o stdout para a escrita do pipe nº2
        close(fd2[0]); // fechar a leitura do pipe nº2
        close(fd2[1]); // fechar a escrita do pipe nº2

        if (execl("classificador", "classificador", NULL) == -1) { // executar o classificador
            printf("Erro a executar o classificador!\n");
            fflush(stdout);
            exit(1);
        }

        exit(0);

    }

    /* fim - inicializar/abrir os pipes e fazer fork() */

    int n, utentesMAX, medicosMAX, fdBAL, fdBAL2, fd_returnCLI, fd_returnMED, utentesAtivos = 0, medicosAtivos = 0;
    char str[40], str2[40], comando[40];

    struct sigaction act;
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = sigusr1Handler; // regista a função para receber o sinal de fecho do cliente
    sigaction(SIGUSR1, &act, NULL);

    struct sigaction act2;
    act2.sa_flags = SA_SIGINFO;
    act2.sa_sigaction = sigusr2Handler; // regista a função para receber o sinal de fecho do medico
    sigaction(SIGUSR2, &act2, NULL);

    /* ------------------ Definir o número máximo de utentes e médicos ------------------- */

    utentesMAX = atoi(getenv("MAXCLIENTES")); // receber e converter a variável ambiente de char* para int
    medicosMAX = atoi(getenv("MAXMEDICOS")); // receber e converter a variável ambiente de char* para int

    utente utentes[utentesMAX]; // criar um array de utentes
    medico especialistas[medicosMAX]; // criar um array de médicos

    /* ------------------ Receber informação do cliente e do médico ------------------- */

    if (access(FIFO_SRV, F_OK) == 0 || access(FIFO_SRV2, F_OK) == 0) { // verificar se o FIFO bal e/ou bal2 já existem
        printf("[ERRO] O fifo bal ou o fifo bal2 ja existe!\n");
        fflush(stdout);
        exit(1);
    }

    mkfifo(FIFO_SRV, 0600); // criar o FIFO bal

    fdBAL = open(FIFO_SRV, O_RDWR); // abrir o FIFO bal para leitura e escrita

    if (fdBAL < 0) {
        printf("Nao foi possivel abrir o fifo bal1\n");
        fflush(stdout);
        unlink(FIFO_SRV);
        exit(1);
    }

    mkfifo(FIFO_SRV2, 0600); // criar o FIFO bal2

    fdBAL2 = open(FIFO_SRV2, O_RDWR); // abrir o FIFO bal2 para leitura e escrita

    if (fdBAL2 < 0) {
        printf("Nao foi possivel abrir o fifo bal2\n");
        fflush(stdout);
        unlink(FIFO_SRV);
        unlink(FIFO_SRV2);
        close(fdBAL);
        exit(1);
    }

    mkfifo(FIFO_SRV3, 0600); // criar o FIFO bal3

    fdBAL3 = open(FIFO_SRV3, O_RDWR); // abrir o FIFO bal3 para leitura e escrita
    // isto deve-se ao facto de este fifo ter sido criado especificamente para receber
    // o aviso de 20s em 20s do medico, posto isto, so precisa de ler o id do medico
    // que avisou que esta online

    if (fdBAL3 < 0) {
        printf("Nao foi possivel abrir o fifo bal3\n");
        fflush(stdout);
        unlink(FIFO_SRV);
        unlink(FIFO_SRV2);
        unlink(FIFO_SRV3);
        close(fdBAL2);
        close(fdBAL);
        exit(1);
    }

    // Thread para receber o aviso de 20 em 20 segundos do medico
    pthread_t t1;
    pthread_create(&t1, NULL, &receboVivo, NULL);
    
    // Select para receber dois FIFO's
    fd_set fds;
    struct timeval tempo;
    int res, maxfd, tempoalterado = 0;

    do {

        // mostra a lista de espera
        // le comandos
        // le o que o cliente escreveu no balcao
        // le o que o medico escreveu no balcao

        if (saiuUtente == 1) { // se saiu um utente
            apagaUtente(utentes, idUtentesaiu, &utentesAtivos); // apaga o utente
            idUtentesaiu = 0;
            saiuUtente = 0;
        } else if (saiuMedico == 1) { // se saiu um medico
            apagaMedico(especialistas, idMedicosaiu, &medicosAtivos); // apaga o medico
            idMedicosaiu = 0;
            saiuMedico = 0;
        }

        printf("\nÀ espera de utentes e especialistas...\n'list' - obter todos os comandos disponíveis\n\n");
        fflush(stdout);

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fdBAL, &fds);
        FD_SET(fdBAL2, &fds);

        // verifica se o intervalo de tempo para mostrar informaçao
        // foi alterado pelo utilizador
        if (tempoalterado == 0)
            tempo.tv_sec = 10;
        else
            tempo.tv_sec = tempoalterado;

        tempo.tv_usec = 0;

        maxfd = fdBAL > fdBAL2 ? fdBAL : fdBAL2; // se fdBAL for maior que fdBAL2, maxfd passa a ser fdBAL

        res = select(maxfd + 1, &fds, NULL, NULL, &tempo); // faz o select

        if (res == 0) {

            // verifica se saiu utente/medico
            if (saiuUtente == 1) {
                apagaUtente(utentes, idUtentesaiu, &utentesAtivos);
                idUtentesaiu = 0;
                saiuUtente = 0;
            } else if (saiuMedico == 1) {
                apagaMedico(especialistas, idMedicosaiu, &medicosAtivos);
                idMedicosaiu = 0;
                saiuMedico = 0;
            }

            // Informar a cada X segundos sobre a fila de espera (utentes)

            printf("\n-----------\nFILA DE ESPERA:\nUtentes Ativos = %d\n", utentesAtivos);
            fflush(stdout);
            for (int i = 0; i < utentesAtivos; i++) {
                if (utentes[i].atendido == false) {
                    printf("\n");
                    fflush(stdout);
                    printf("ID = %d\n", utentes[i].id);
                    fflush(stdout);
                    printf("Nome = %s\n", utentes[i].nome);
                    fflush(stdout);
                    printf("Especialidade = %s\n", utentes[i].especialidade);
                    fflush(stdout);
                    printf("Urgencia = %d\n", utentes[i].urgencia);
                    fflush(stdout);
                    printf("\n");
                    fflush(stdout);
                }
            }
            printf("\n-----------\n");
            fflush(stdout);

        } else if (res > 0 && FD_ISSET(0, &fds)) {

            // le a string
            fgets(comando, sizeof (comando) - 1, stdin);

            if (saiuUtente == 1) {
                apagaUtente(utentes, idUtentesaiu, &utentesAtivos);
                idUtentesaiu = 0;
                saiuUtente = 0;
            } else if (saiuMedico == 1) {
                apagaMedico(especialistas, idMedicosaiu, &medicosAtivos);
                idMedicosaiu = 0;
                saiuMedico = 0;
            }

            // vai executar um comando
            if (strcmp(comando, "encerra\n") != 0) {
                lecomandos(comando, &utentesAtivos, &medicosAtivos, utentes, especialistas, &tempoalterado);
                fflush(stdout);
            }

        } else if (res > 0 && FD_ISSET(fdBAL2, &fds)) { // o medico escreveu no balcao

            if (saiuUtente == 1) {
                apagaUtente(utentes, idUtentesaiu, &utentesAtivos);
                idUtentesaiu = 0;
                saiuUtente = 0;
            } else if (saiuMedico == 1) {
                apagaMedico(especialistas, idMedicosaiu, &medicosAtivos);
                idMedicosaiu = 0;
                saiuMedico = 0;
            }

            // Verificar se o utente que chegou ao balcao ja existia no mesmo
            // Isto vem do facto do medico poder terminar uma consulta e 
            // voltar para o balcao para atender outra

            int ajuste, repetido = 0;
            medico auxiliar;

            if (medicosAtivos > 0) {

                // le do bal2
                n = read(fdBAL2, &auxiliar, sizeof (medico));

                if (n < 0) {
                    printf("Nao foi possivel ler do FIFO bal2!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                // esta variavel vai servir para escrever num medico que retornou de uma consulta
                // ou seja, vai escrever por cima de um medico existente, na posicao correta
                ajuste = procuraRepetido(especialistas, auxiliar, medicosAtivos); // esta funçao procura por um medico repetido, ou seja um medico que retornou de uma consulta

                if (ajuste == -1) {
                    ajuste = medicosAtivos; // se nao encontrou um medico repetido, vai colocar na posicao normal
                } else {
                    repetido = 1;
                }

            } else {

                ajuste = medicosAtivos;

            }

            if (!verificaMEDmax(medicosAtivos, medicosMAX)) { // vai verificiar se existe um numero maximo de medicos

                if (medicosAtivos == 0) { // se nao existir medicos, é impossivel ele estar repetido

                    n = read(fdBAL2, &especialistas[ajuste], sizeof (medico));

                    if (n < 0) {
                        printf("Nao foi possivel ler do FIFO bal2!\n");
                        fflush(stdout);
                        close(fdBAL); // fechar o FIFO bal
                        close(fdBAL2); // fechar o FIFO bal2
                        close(fdBAL3); // fechar o FIFO bal2
                        unlink(FIFO_SRV); // apagar o FIFO bal
                        unlink(FIFO_SRV2); // apagar o FIFO bal2
                        unlink(FIFO_SRV3); // apagar o FIFO bal2
                        exit(1);
                    }

                } else {

                    especialistas[ajuste] = auxiliar;

                }

                printf("NOVO MEDICO DISPONIVEL! O medico %s, especialista em %s, esta pronto a atender!\n", especialistas[ajuste].nome, especialistas[ajuste].especialidade);
                fflush(stdout);

                especialistas[ajuste].max = false; // ainda nao chegamos ao maximo de medicos
                especialistas[ajuste].atender = false; // o medico ainda nao esta a atender
                especialistas[ajuste].idBalcao = idBalcao; // atribuir a uma variável do medico o id do processo do balcao

                int procurautente = atribuiUtente(especialistas[ajuste].especialidade, utentesAtivos, utentes, especialistas[ajuste].id); // vai procurar utentes para atender

                if (procurautente != 0) {

                    especialistas[ajuste].idCliente = procurautente;
                    especialistas[ajuste].atender = true;
                    printf("O medico %s vai atender o utente nº %d\n", especialistas[ajuste].nome, procurautente);
                    fflush(stdout);

                }

                /* ------------------ Enviar esta informação para o medico.c ------------------- */

                sprintf(str2, FIFO_MED, especialistas[ajuste].id); // definir o nome do fifo med

                fd_returnMED = open(str2, O_WRONLY); // abrir o fifo med para escrita

                if (fd_returnMED < 0) {
                    printf("Nao foi possivel abrir o fifo med!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                n = write(fd_returnMED, &especialistas[ajuste], sizeof (medico)); // escrever a estrutura do medico

                if (n < 0) {
                    printf("Nao foi possivel escrever no fifo med!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    close(fd_returnMED);
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                close(fd_returnMED); // fechar o fifo med

                if (repetido == 0)
                    medicosAtivos++;

            } else {

                printf("Entrou um medico, mas estamos cheios!\n");
                fflush(stdout);

                medico medAux;
                n = read(fdBAL2, &medAux, sizeof (medico));

                if (n < 0) {
                    printf("Nao foi possivel ler do FIFO bal2!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                medAux.max = true; // chegámos ao maximo de medicos
                especialistas[medicosAtivos].idBalcao = idBalcao; // atribuir a uma variável do medico o id do processo do balcao

                /* ------------------ Enviar esta informação para o medico.c ------------------- */

                sprintf(str2, FIFO_MED, medAux.id); // definir o nome do fifo med

                fd_returnMED = open(str, O_WRONLY); // abrir o fifo med para escrita

                if (fd_returnMED < 0) {
                    printf("Nao foi possivel abrir o fifo med!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                n = write(fd_returnMED, &medAux, sizeof (medico)); // escrever a estrutura do medico

                if (n < 0) {
                    printf("Nao foi possivel escrever no fifo med!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    close(fd_returnMED);
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                close(fd_returnMED); // fechar o fifo med   

            }

        } else if (res > 0 && FD_ISSET(fdBAL, &fds)) {

            if (saiuUtente == 1) {
                apagaUtente(utentes, idUtentesaiu, &utentesAtivos);
                idUtentesaiu = 0;
                saiuUtente = 0;
            } else if (saiuMedico == 1) {
                apagaMedico(especialistas, idMedicosaiu, &medicosAtivos);
                idMedicosaiu = 0;
                saiuMedico = 0;
            }

            if (!verificaUTmax(utentesAtivos, utentesMAX)) {

                n = read(fdBAL, &utentes[utentesAtivos], sizeof (utente));

                if (n < 0) {
                    printf("Nao foi possivel ler do FIFO bal!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                printf("\nEntrou um novo utente! O utente %s apresenta os seguintes sintomas: %s", utentes[utentesAtivos ].nome, utentes[utentesAtivos ].sintomas);
                fflush(stdout);

                utentes[utentesAtivos].max = false; // ainda nao chegamos ao maximo de clientes
                utentes[utentesAtivos].atendido = false; // o cliente ainda nao foi atendido
                utentes[utentesAtivos].idBalcao = idBalcao; // atribuir a uma variável do medico o id do processo do balcao
                //utentes[utentesAtivos].posicao = utentesAtivos; // na primeira instância, utentesAtivos = 0 , logo, estao zero pessoas à frente dele
                utentes[utentesAtivos].posicao = procuraFilaespera(utentes, utentesAtivos);

                classifica(&utentes[utentesAtivos], fd1, fd2);

                printf("Vamos dizer ao utente %s que a sua especialidade e: %s e a sua urgencia e: %d\n", utentes[utentesAtivos ].nome, utentes[utentesAtivos].especialidade, utentes[utentesAtivos ].urgencia);
                fflush(stdout);

                int procuramedico = atribuiMedico(utentes[utentesAtivos].especialidade, medicosAtivos, especialistas, utentes[utentesAtivos].id); // vai procurar utentes para atender

                if (procuramedico != 0) {

                    utentes[utentesAtivos].idMedico = procuramedico;
                    utentes[utentesAtivos].atendido = true;
                    printf("O utente %s vai ser atendido pelo medico nº %d\n", utentes[utentesAtivos].nome, procuramedico);
                    fflush(stdout);

                }

                /* ------------------ Enviar esta informação para o cliente.c ------------------- */

                sprintf(str, FIFO_CLI, utentes[utentesAtivos].id); // definir o nome do fifo cli

                fd_returnCLI = open(str, O_WRONLY); // abrir o fifo cli para escrita

                if (fd_returnCLI < 0) {
                    printf("Nao foi possivel abrir o fifo cli!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                n = write(fd_returnCLI, &utentes[utentesAtivos], sizeof (utente)); // escrever a estrutura do cliente

                if (n < 0) {
                    printf("Nao foi possivel escrever no fifo cli!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    close(fd_returnCLI);
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                close(fd_returnCLI); // fechar o fifo cli

                utentesAtivos++;

            } else {

                printf("Entrou um novo utente, mas estamos cheios!\n");
                fflush(stdout);

                utente utAux;
                n = read(fdBAL, &utAux, sizeof (utente));

                if (n < 0) {
                    printf("Nao foi possivel ler do FIFO bal!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                utAux.max = true;
                utentes[utentesAtivos].idBalcao = idBalcao; // atribuir a uma variável do medico o id do processo do balcao

                printf("Vamos dizer ao utente %s, que nao pode ser atendido.\n", utAux.nome);
                fflush(stdout);

                /* ------------------Enviar esta informação para o cliente.c------------------- */

                sprintf(str, FIFO_CLI, utAux.id); // definir o nome do fifo cli

                fd_returnCLI = open(str, O_WRONLY); // abrir o fifo cli para escrita

                if (fd_returnCLI < 0) {
                    printf("Nao foi possivel abrir o fifo cli!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                n = write(fd_returnCLI, &utAux, sizeof (utente)); // escrever a estrutura do cliente

                if (n < 0) {
                    printf("Nao foi possivel escrever no fifo cli!\n");
                    fflush(stdout);
                    close(fdBAL); // fechar o FIFO bal
                    close(fdBAL2); // fechar o FIFO bal2
                    close(fdBAL3); // fechar o FIFO bal2
                    close(fd_returnCLI);
                    unlink(FIFO_SRV); // apagar o FIFO bal
                    unlink(FIFO_SRV2); // apagar o FIFO bal2
                    unlink(FIFO_SRV3); // apagar o FIFO bal2
                    exit(1);
                }

                close(fd_returnCLI); // fechar o fifo cli

            }
        }

    } while (strcmp(comando, "encerra\n") != 0);

    encerraUtilizadores(utentes, especialistas, &utentesAtivos, &medicosAtivos); // encerra todos os utilizadores quando está para fechar

    /* inicio - fechar a comunicação com o classificador */

    char sintoma[80]; // declarar o sintoma do utente, no entanto, o sintoma vai ser "#fim"
    strcpy(sintoma, "#fim\n"); // o sintoma passa a ser #fim\n para que o classificador possa fechar 

    if (write(fd1[1], sintoma, strlen(sintoma)) == -1) { // escrever no pipe nº1 e verificar se dá erro
        printf("[ERRO] - Não foi possível escrever no pipe 1!\n");
        fflush(stdout);
    }

    close(fd1[1]); // fechar a escrita do pipe nº1
    close(fd2[0]); // fechar a leitura do pipe nº2

    /* fim - fechar a comunicação com o classificador */

    close(fdBAL); // fechar o FIFO bal
    close(fdBAL2); // fechar o FIFO bal2
    close(fdBAL3); // fechar o FIFO bal2
    unlink(FIFO_SRV); // apagar o FIFO bal
    unlink(FIFO_SRV2); // apagar o FIFO bal2
    unlink(FIFO_SRV3); // apagar o FIFO bal2

    exit(0);

}
