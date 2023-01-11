#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>
#include <sys/select.h>
#include <signal.h>
#include <pthread.h>

#define FIFO_SRV "bal" // fifo para escrever no balcao
#define FIFO_SRV2 "bal2" // fifo para escrever no balcao2
#define FIFO_SRV3 "bal3" // fifo para receber o aviso de 20 em 20 segundos do médico
#define FIFO_CLI "cli%d" // fifo recebe cliente
#define FIFO_CLI2 "cli2%d" // fifo do cliente onde escreve o medico
#define FIFO_MED "med%d" // fifo recebe medico
#define FIFO_MED2 "med2%d" // fifo do medico onde escreve o cliente

typedef struct {
    int id; // id do utente (pode haver nomes e especialidades repetidas)
    char nome[80]; // nome do utente
    char sintomas[80]; // sintomas do utente
    bool atendido; // o cliente está a ser atendido?
    bool max; // já foi atingido o numero máximo de clientes?
    char especialidade[40]; // especialidade em que o utente se vai enquadrar
    int urgencia; // urgencia do utente
    int posicao; // numero de pessoas à sua frente (1 = significa que esta em primeiro)
    int idBalcao; // id do Balcao (serve para avisar o balcao quando o cliente fecha)
    int idMedico; // id do Medico que o vai atender

} utente;

typedef struct {
    int id; // id do médico (pode haver nomes e especialidades repetidas)
    char nome[80]; // nome do médico
    char especialidade[40]; // especialidade do médico
    bool atender; // o especialista está a atender alguém?
    bool max; // já foi atingido o numero máximo de medicos?
    int idBalcao; // id do Balcao (serve para avisar o balcao quando o medico fecha)
    int idCliente; // id do Cliente que vai atender
    int continua; // flag para dizer que o medico ja atendeu alguem anteriormente

} medico;

typedef struct {
    char msg[255]; // texto que vai ser enviado entre cliente-medico
} mensagem;

typedef struct {
    int idRemetente; // pid() do processo que mandou o aviso
} alarme;