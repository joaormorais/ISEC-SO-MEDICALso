#include "util1.h"

// ao criar estas variáveis globais, posso dar unlink em qualquer lado
// quando o programa precisa de fechar
char str[40] = "a", strb[40] = "a", strMedico[40] = "a";

void unlinkAll() {

    if (strcmp(str, "a") != 0)
        unlink(str);

    if (strcmp(strb, "a") != 0)
        unlink(strb);

    if (strcmp(strMedico, "a") != 0)
        unlink(strMedico);

}

void sigintHandler(int sig) {

    // handler do sinal para terminar o programa
    // Assim que o balcao informa que vai embora ou apaga o utilizador, o utilizador termina

    printf("O balcao fechou!\n", getpid());
    fflush(stdout);
    unlinkAll();

    exit(0);

}

int main(int argc, char *argv[], char *envp[]) {

    signal(SIGINT, sigintHandler); // regista a função para receber o sinal de fecho do balcão
    utente ut;
    mensagem mensagem;
    char comando[40];
    int fd, fd_return, fd_return2, n, fd_medico, jaseregistou = 0;

    /* ------------------ Receber o nome pelo CMD ------------------- */

    if (argc != 2) {
        printf("[ERRO] Nr. de argumentos!\n");
        printf("	./cliente <nome> \n");
        fflush(stdout);
        return (1);
    }

    strcpy(ut.nome, argv[1]); // guardamos o nome do utilizador que recebemos pelo CMD

    /* ------------------ Pedir e guardar os sintomas ------------------- */

    printf("Ola %s quais sao os seus sintomas? ", ut.nome);
    fflush(stdout);

    fgets(ut.sintomas, sizeof (ut.sintomas) - 1, stdin);

    if (strcmp("encerra\n", ut.sintomas) == 0)
        exit(0);

    /* ------------------ Enviar os sintomas ao balcao.c ------------------- */

    // verificar se o FIFO bal está aberto/fechado 
    if (access(FIFO_SRV, F_OK) != 0) {
        // o utente só funciona com o balcão aberto
        printf("O Balcao esta fechado!\n");
        fflush(stdout);
        exit(1);
    }

    // o id do utilizador é o numero do processo
    ut.id = getpid();

    // Criar o FIFO cli
    sprintf(str, FIFO_CLI, ut.id);
    mkfifo(str, 0600);

    // Criar o FIFO cli2
    sprintf(strb, FIFO_CLI2, ut.id);
    mkfifo(strb, 0600);

    // Abrir o FIFO bal para escrita
    fd = open(FIFO_SRV, O_WRONLY);

    if (fd < 0) {
        printf("Nao foi possivel abrir o FIFO bal!\n");
        fflush(stdout);
        unlinkAll();
        exit(1);
    }

    ut.atendido = false; // sabe que ainda nao foi atendido
    ut.max = false; // ainda nao sabe se esta cheio ou nao, portanto assumimos que nao esta
    ut.idMedico = 0; // ainda nao sabe o id do medico com quem vai falar

    // escrever no FIFO bal
    n = write(fd, &ut, sizeof (utente));

    if (n < 0) {
        printf("Nao foi possivel escrever no FIFO bal!\n");
        fflush(stdout);
        close(fd);
        unlinkAll();
        exit(1);
    }

    /* ------------------ Receber a resposta do balcao.c/medico.c ------------------- */

    // abrir o FIFO cli (recebe info do Balcão)
    fd_return = open(str, O_RDWR);

    if (fd_return < 0) {
        printf("Nao foi possivel abrir o FIFO cli!\n");
        fflush(stdout);
        close(fd);
        unlinkAll();
        exit(1);
    }

    // abrir o FIFO cli2 (recebe info do Médico)
    fd_return2 = open(strb, O_RDWR);

    if (fd_return2 < 0) {
        printf("Nao foi possivel abrir o FIFO cli2!\n");
        fflush(stdout);
        close(fd);
        close(fd_return);
        unlinkAll();
        exit(1);
    }

    fd_set fds;
    struct timeval tempo;
    int res, maxfd;

    do {

        // fazer o select para receber:
        // info escrita pelo medico
        // info escrita pelo balcao
        // introduzir um comando
        // ou fazer print

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd_return, &fds);
        FD_SET(fd_return2, &fds);

        tempo.tv_sec = 10;
        tempo.tv_usec = 0;

        maxfd = fd_return > fd_return2 ? fd_return : fd_return2;

        res = select(maxfd + 1, &fds, NULL, NULL, &tempo);

        if (res == 0) {

            // se nao tiver um Medico associado, vai pedir ao utilizador para aguardar

            if (ut.idMedico == 0) {
                printf("Aguarde para ser atendido!\n");
                fflush(stdout);
            }

        } else if (res > 0 && FD_ISSET(0, &fds)) {

            // ler uma string
            fgets(comando, sizeof (comando) - 1, stdin);

            // essa string só é enviado ao medico, se o utente ja tiver o medico atribuido
            if (ut.idMedico != 0) {

                // abrir o fifo do medico (med2)
                sprintf(strMedico, FIFO_MED2, ut.idMedico);
                fd_medico = open(strMedico, O_WRONLY);

                if (fd_medico < 0) {
                    printf("Nao foi possivel abrir o FIFO %s!\n", strMedico);
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    unlinkAll();
                    exit(1);
                }

                // copiar a string para a estrutura que é passada entre cliente e medico
                strcpy(mensagem.msg, comando);

                // escrever no medico
                n = write(fd_medico, &mensagem, sizeof (mensagem));

                if (n < 0) {
                    printf("Nao foi possivel escrever no FIFO %s!\n", strMedico);
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    close(fd_medico);
                    unlinkAll();
                    exit(1);
                }

                // fechar o medico
                close(fd_medico);

            }

        } else if (res > 0 && FD_ISSET(fd_return, &fds)) { // ler info escrita pelo Balcao

            // ler do FIFO cli
            n = read(fd_return, &ut, sizeof (utente));

            if (n < 0) {
                printf("Nao foi possivel ler o FIFO cli!\n");
                fflush(stdout);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(1);
            }

            if (ut.max == false) { // se o balcao nao estiver cheio

                if (jaseregistou == 0) { // se o cliente estiver a entrar pela primeira vez neste programa

                    printf("%s, tens a especialidade %s e a prioridade %d\n", ut.nome, ut.especialidade, ut.urgencia);
                    fflush(stdout);
                    printf("Aguarde para ser atendido, estao %d pessoas na fila de espera!\n", ut.posicao);
                    fflush(stdout);
                    jaseregistou = 1;

                }

                if (ut.idMedico != 0) { // se o cliente tiver um medico atribuido
                    printf("Voce foi atendido pelo medico nº %d\n", ut.idMedico);
                    fflush(stdout);
                }

            } else { // se o balcao estiver cheio

                printf("Ja existe um numero maximo de utentes!\n");
                fflush(stdout);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(1);

            }

        } else if (res > 0 && FD_ISSET(fd_return2, &fds)) { // ler info escrita pelo medico

            // ler fifo cli2
            n = read(fd_return2, &mensagem, sizeof (mensagem));

            if (n < 0) {
                printf("Nao foi possivel ler do FIFO %s!\n", str);
                fflush(stdout);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(1);
            }

            // mostrar a msg escrita pelo medico
            printf("[MEDICO] - %s\n", mensagem.msg);
            fflush(stdout);

            // se o medico/utente terminar a consulta, acabou o programa
            if (strcmp(mensagem.msg, "adeus\n") == 0 || strcmp(mensagem.msg, "sair\n") == 0) {
                printf("A sua consulta terminou!\n");
                fflush(stdout);
                kill(ut.idBalcao, SIGUSR1);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(0);
            }
        }

    } while (strcmp(comando, "adeus\n") != 0);

    kill(ut.idBalcao, SIGUSR1); // mandar o sinal ao balcao a dizer que estou a fechar
    close(fd);
    close(fd_return);
    close(fd_return2);
    unlinkAll();

    exit(0);

}
