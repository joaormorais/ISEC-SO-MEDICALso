#include "util1.h"

// ao criar estas variáveis globais, posso dar unlink em qualquer lado
// quando o programa precisa de fechar
char str2[40] = "a", str2b[40] = "a", strCliente[40] = "a";

void unlinkAll() {

    if (strcmp(str2, "a") != 0)
        unlink(str2);

    if (strcmp(str2b, "a") != 0)
        unlink(str2b);

    if (strcmp(strCliente, "a") != 0)
        unlink(strCliente);

}

void sigintHandler(int sig) {

    // handler do sinal para terminar o programa
    // Assim que o balcao informa que vai embora 
    // ou apaga o especialista, o medico termina
    printf("O balcao fechou %d!\n", getpid());
    fflush(stdout);
    unlinkAll();

    exit(0);

}

void *estouVivo() {

    alarme alarme;

    while (1) {

        // escreve no fifo do balcao feito especificamente
        // para receber a mensagem do medico de 20 em 20 segundos
        int fd_aviso = open(FIFO_SRV3, O_WRONLY);

        alarme.idRemetente = getpid(); // vai mandar ao balcao o seu id (processo medico)

        int n = write(fd_aviso, &alarme, sizeof (alarme)); // escrever no FIFO bal3

        if (n < 0) {
            printf("Nao foi possivel escrever no FIFO bal3!\n");
            fflush(stdout);
            exit(1);
        }

        sleep(20); // aguarda 20 segundos para repetir
    }

}

int main(int argc, char *argv[], char *envp[]) {

    signal(SIGINT, sigintHandler); // regista a função para receber o sinal de fecho do balcão
    medico med;
    mensagem mensagem;
    char comando[40];
    int fd, fd_return, fd_return2, n, fd_cliente, jaseregistou = 0;

    /* ------------------ Receber o nome e a especialidade pelo CMD ------------------- */

    if (argc != 3) {
        printf("[ERRO] Nr. de argumentos!\n");
        printf("	./medico <nome> <especialidade>\n");
        fflush(stdout);
        return (1);
    }

    strcpy(med.nome, argv[1]); // guardamos o nome do medico que recebemos pelo CMD
    strcpy(med.especialidade, argv[2]); // guardamos da mm forma a especialidade

    /* ------------------ Enviar os seus dados ao balco ------------------- */

    // verificar se o FIFO bal2 está aberto/fechado 
    if (access(FIFO_SRV2, F_OK) != 0) {
        // o medico só funciona com o balcão aberto
        printf("O Balcao esta fechado!\n");
        fflush(stdout);
        exit(1);
    }

    // o id do medico é o numero do processo
    med.id = getpid();

    // Criar o FIFO med
    sprintf(str2, FIFO_MED, med.id);
    mkfifo(str2, 0600);

    // Criar o FIFO med2
    sprintf(str2b, FIFO_MED2, med.id);
    mkfifo(str2b, 0600);

    // Abrir o FIFO bal2 para escrita
    fd = open(FIFO_SRV2, O_WRONLY); // aberto para escrita

    if (fd < 0) {
        printf("Nao foi possivel abrir o FIFO bal2!\n");
        fflush(stdout);
        unlinkAll();
        exit(1);
    }

    med.atender = false; // sabe que ainda nao foi atendido
    med.max = false; // ainda nao sabe se esta cheio ou nao, portanto assumimos que nao esta
    med.continua = 0; // ainda nao atendeu pelo menos 1 cliente
    med.idCliente = 0; // ainda nao recebeu um cliente

    // escrever no FIFO bal2
    n = write(fd, &med, sizeof (medico));

    if (n < 0) {
        printf("Nao foi possivel escrever no FIFO bal2!\n");
        fflush(stdout);
        close(fd);
        unlinkAll();
        exit(1);
    }

    /* ------------------ Mandar o aviso de 20s em 20s ao balcao ------------------- */

    pthread_t t1;
    pthread_create(&t1, NULL, &estouVivo, NULL); // criar uma thread
    // a thread vai estar a correr a funçao estouVivo

    /* ------------------ Receber a resposta do balcao.c/cliente.c ------------------- */

    // abrir o FIFO med (recebe info do Balcão)
    fd_return = open(str2, O_RDWR);

    if (fd_return < 0) {
        printf("Nao foi possivel abrir o FIFO med!\n");
        fflush(stdout);
        close(fd);
        unlinkAll();
        exit(1);
    }

    // abrir o FIFO med2 (recebe info do Cliente)
    fd_return2 = open(str2b, O_RDWR);

    if (fd_return2 < 0) {
        printf("Nao foi possivel abrir o FIFO med2!\n");
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
        // info escrita pelo cliente
        // info escrita pelo balcao
        // introduzir um comando
        // ou fazer print

        FD_ZERO(&fds); // inicializar o fds para ter 0 bits
        FD_SET(0, &fds);
        FD_SET(fd_return, &fds);
        FD_SET(fd_return2, &fds);

        tempo.tv_sec = 30;
        tempo.tv_usec = 0;

        maxfd = fd_return > fd_return2 ? fd_return : fd_return2;

        res = select(maxfd + 1, &fds, NULL, NULL, &tempo);

        if (res == 0) {

            if (med.idCliente == 0) {
                printf("Aguarde para atender um utente!\n");
                fflush(stdout);
            }

        } else if (res > 0 && FD_ISSET(0, &fds)) {

            // ler uma string
            fgets(comando, sizeof (comando) - 1, stdin);

            // essa string só é enviada ao cliente, se o medico ja tiver o cliente atribuido
            if (med.idCliente != 0) {

                // abrir o fifo do cliente (cli2)
                sprintf(strCliente, FIFO_CLI2, med.idCliente);
                fd_cliente = open(strCliente, O_WRONLY);

                if (fd_cliente < 0) {
                    printf("Nao foi possivel abrir o FIFO %s!\n", strCliente);
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    unlinkAll();
                    exit(1);
                }

                // copiar a string para a estrutura que é passada entre cliente e medico
                strcpy(mensagem.msg, comando);

                // escrever no cliente
                n = write(fd_cliente, &mensagem, sizeof (mensagem));

                if (n < 0) {
                    printf("Nao foi possivel escrever no FIFO %s!\n", strCliente);
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    close(fd_cliente);
                    unlinkAll();
                    exit(1);
                }

                // fechar o cliente
                close(fd_cliente);

                // se o medico quiser terminar a consulta sem terminar o programa
                if (strcmp(comando, "adeus\n") == 0) {

                    // dizer ao balcao que ja nao estou a atender ninguem

                    med.atender = 0;
                    med.idCliente = 0;
                    med.continua = 1;

                    // escrever no FIFO bal2
                    n = write(fd, &med, sizeof (medico));

                    if (n < 0) {
                        printf("Nao foi possivel escrever no FIFO bal2!\n");
                        fflush(stdout);
                        close(fd);
                        close(fd_return);
                        close(fd_return2);
                        unlinkAll();
                        exit(1);
                    }
                }

            }

        } else if (res > 0 && FD_ISSET(fd_return, &fds)) { // ler info escrita pelo Balcao

            // ler do FIFO med
            n = read(fd_return, &med, sizeof (medico));

            if (n < 0) {
                printf("Nao foi possivel ler o FIFO med!\n");
                fflush(stdout);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(1);
            }

            if (med.continua == 0) { // se nunca atendeu ninguem anteriormente

                if (med.max == false) { // se o balcao nao estiver cheio


                    if (jaseregistou == 0) { // se o medico estiver a entrar pela primeira vez neste programa

                        printf("Bem-vindo %s. Resgistou-se no Balcao com sucesso!\n", med.nome);
                        fflush(stdout);
                        printf("A sua especialidade: %s\n", med.especialidade);
                        fflush(stdout);
                        printf("Aguarde para atender um utente!\n");
                        fflush(stdout);
                        jaseregistou = 1;

                    }

                    if (med.idCliente != 0) { // se o medico tiver um cliente atribuido
                        printf("Voce atendeu o utente nº %d\n", med.idCliente);
                        fflush(stdout);
                    }

                } else { // se o balcao estiver cheio

                    printf("Ja existe um numero maximo de medicos!\n");
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    unlinkAll();
                    exit(1);

                }

            } else { // se ja atendeu alguem anteriormente

                if (med.idCliente != 0) { // se o medico tiver um cliente atribuido
                    printf("Voce atendeu o utente nº %d\n", med.idCliente);
                    fflush(stdout);
                } else {
                    printf("Voltou para o balcao!\nAguarde para atender um utente!\n");
                    fflush(stdout);
                }

            }

        } else if (res > 0 && FD_ISSET(fd_return2, &fds)) { // o medico mandou uma msg ao cliente

            // ler do fifo med2
            n = read(fd_return2, &mensagem, sizeof (mensagem));

            if (n < 0) {
                printf("Nao foi possivel ler do FIFO %s!\n", str2b);
                fflush(stdout);
                close(fd);
                close(fd_return);
                close(fd_return2);
                unlinkAll();
                exit(1);
            }

            // mostrar a msg escrita pelo cliente
            printf("[UTENTE] - %s\n", mensagem.msg);
            fflush(stdout);

            // se o medico terminar a consulta
            if (strcmp(mensagem.msg, "adeus\n") == 0) {

                // preparar o medico para uma nova consulta
                // dizer ao balcao que ja nao estou a atender ninguem
                med.atender = 0;
                med.idCliente = 0;
                med.continua = 1;

                // escrever no FIFO bal2
                n = write(fd, &med, sizeof (medico));

                if (n < 0) {
                    printf("Nao foi possivel escrever no FIFO bal2!\n");
                    fflush(stdout);
                    close(fd);
                    close(fd_return);
                    close(fd_return2);
                    unlinkAll();
                    exit(1);
                }
            }

        }

    } while (strcmp(comando, "sair\n") != 0);

    kill(med.idBalcao, SIGUSR2); // mandar o sinal ao balcao a dizer que estou a fechar
    close(fd);
    close(fd_return);
    close(fd_return2);
    unlinkAll();

    exit(0);

}