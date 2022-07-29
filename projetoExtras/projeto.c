#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include </usr/include/semaphore.h>
#include <syslog.h>
#include <omp.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

//define variaveis globais
#define SHARED_SIZE 5
#define BUFF_SIZE 5
#define ProducerInstances 1
#define CP1Instances 5
#define CP2Instances 4
#define CP3Instances 3
#define ConsumerInstances 1
#define Lines 10
#define Cols 10
#define vectorSize 10

//FileStructure S
typedef struct {
    char Name[100];
    double matrixA[Lines][Cols];
    double matrixB[Lines][Cols];
    double matrixC[Lines][Cols];
    double vectorV[vectorSize];
    double resultE;
} S;

typedef struct {
    S *buffer[BUFF_SIZE];
    int in;
    int out;
    sem_t full;
    sem_t empty;
    sem_t mutex;
} shared_t;

shared_t shared[SHARED_SIZE];

//Chamada das funções de matriz
void matrixReadFile(FILE *matrixFile, double matrix[Lines][Cols]);
void matrixMultiplier(double matrixA[Lines][Cols], double matrixB[Lines][Cols], double matrixC[Lines][Cols]);
void sumMatrixCols(double matrix[Lines][Cols], double vectorSums[vectorSize]);
void printMatrix(double matrix[Lines][Cols]);
void printMatrixFile(FILE *out, double Matrix[Lines][Cols]);
//chamada das funções do arquivo
void printFile(FILE *out, S *s);
//Chamada das funções de array
void printArray(double array[vectorSize]);
double sumArray(double array[vectorSize]);
void printArrayFile(FILE *out, double array[vectorSize]);
void print_to_syslog(const int level, const char* message);
void print_to_file(const int level, const char* message);

/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void *Producer(void *arg)  
{
    syslog(LOG_INFO, "operação do Producer\n");

    char *entryFilename = (char *) arg;

    FILE *entryFile = fopen(entryFilename, "r");
    if (entryFile == NULL) 
    {
        syslog(LOG_ERR, "Erro ao abrir %s\n", entryFilename);
        exit(EXIT_FAILURE);
    }

    while (! feof(entryFile) ) 
    {
        syslog(LOG_INFO, "iniciada leitura de matriz\n");
        // cria ponteiro com o nome do arquivo e as matrizes
        S *s = (S *) malloc(sizeof(S));

        // lê o nome do próximo arquivo de matrizes a ser processado
        fscanf(entryFile, "%s\n", s->Name);
        FILE *matrixFile = fopen(s->Name, "r");
        syslog(LOG_INFO, "iniciada leitura da matriz %s\n", s->Name);
        if (matrixFile == NULL) 
        {
            syslog(LOG_ERR, "Erro ao abrir %s\n", s->Name);
        }
        matrixReadFile(matrixFile, s->matrixA);
        matrixReadFile(matrixFile, s->matrixB);
        syslog(LOG_INFO, "entrada da sessão crítica\n");
        // seção crítica: adiciona s a shared
        sem_wait(&shared[0].empty);
        sem_wait(&shared[0].mutex);
        shared[0].buffer[shared[0].in] = s;
        shared[0].in = (shared[0].in + 1) % BUFF_SIZE;
        sem_post(&shared[0].mutex);
        sem_post(&shared[0].full);
        syslog(LOG_INFO, "saída da sessão crítica\n");
        fclose(matrixFile);
    }
    syslog(LOG_INFO, "finalização do Producer \n");
    fclose(entryFile);
}

void *ConsumerProducer1(void *arg) 
{
    int index = *((int *) arg);
    syslog(LOG_INFO, "ConsumerProducer1 Thread N %d\n", index);
    while (1) 
    {
        syslog(LOG_INFO, " Iniciação do Calculo C = A * B\n na da thread N %d\n", index);
        // pega um elemento de shared[0]
        sem_wait(&shared[0].full);
        sem_wait(&shared[0].mutex);
        S *s = shared[0].buffer[shared[0].out];
        shared[0].out = (shared[0].out + 1) % BUFF_SIZE;
        matrixMultiplier(s->matrixA, s->matrixB, s->matrixC);
        sem_post(&shared[0].mutex);
        sem_post(&shared[0].empty);

        syslog(LOG_INFO,"Transfere elemento de shared[0] para shared[1]\n");
        sem_wait(&shared[1].empty);
        sem_wait(&shared[1].mutex);
        shared[1].buffer[shared[1].in] = s;
        shared[1].in = (shared[1].in + 1) % BUFF_SIZE;
        sem_post(&shared[1].full);
        sem_post(&shared[1].mutex);
    }
}

void *ConsumerProducer2(void *arg) 
{
    int index = *((int *) arg);
    syslog(LOG_INFO, "ConsumerProducer2 Thread N %d\n", index);
    while (1) 
    {
        syslog(LOG_INFO,"Iniciação da soma as colunas de C da thread N %d\n", index);
        sem_wait(&shared[1].full);
        sem_wait(&shared[1].mutex);
        S *s = shared[1].buffer[shared[1].out];
        shared[1].out = (shared[1].out + 1) % BUFF_SIZE;
        sumMatrixCols(s->matrixC, s->vectorV);
        sem_post(&shared[1].mutex);
        sem_post(&shared[1].empty);

        syslog(LOG_INFO,"Transfere o resultado de shared[1] para shared[2]\n");
        sem_wait(&shared[2].empty);
        sem_wait(&shared[2].mutex);
        shared[2].buffer[shared[2].in] = s;
        shared[2].in = (shared[2].in + 1) % BUFF_SIZE;
        sem_post(&shared[2].mutex);
        sem_post(&shared[2].full);
    }
}

void *ConsumerProducer3(void *arg)
{
    int index = *((int *) arg);
    syslog(LOG_INFO, "ConsumerProducer3 Thread N %d\n", index);
    while (1) 
    {
        syslog(LOG_INFO, "Iniciação da soma do vetor V da thread N %d\n", index);
        // pega um elemento de shared[2] 
        sem_wait(&shared[2].full);
        sem_wait(&shared[2].mutex);
        S *s = shared[2].buffer[shared[2].out];
        shared[2].out = (shared[2].out + 1) % BUFF_SIZE;
        s->resultE = sumArray(s->vectorV);
        sem_post(&shared[2].mutex);
        sem_post(&shared[2].empty);
        syslog(LOG_INFO, "Transfere o resultado de shared[2] para shared[3]\n");
        // passa o elemento para shared[3]
        sem_wait(&shared[3].empty);
        sem_wait(&shared[3].mutex);
        shared[3].buffer[shared[3].in] = s;
        shared[3].in = (shared[3].in + 1) % BUFF_SIZE;
        sem_post(&shared[3].mutex);
        sem_post(&shared[3].full);
    }
}

void *Consumer(void *arg) 
{
    syslog(LOG_INFO, "operação do Consumer\n");

    char *resultFilename = (char *) arg;
    FILE *resultFile = fopen(resultFilename, "w");
    syslog(LOG_INFO, "Preenche o arquivo saida.out com os valores obtidos\n");
    int i;
    for (i = 0; i < 50; i++) 
    {
        sem_wait(&shared[3].full);
        sem_wait(&shared[3].mutex);
        S *s = shared[3].buffer[shared[3].out];
        shared[3].out = (shared[3].out + 1) % BUFF_SIZE;
        printFile(resultFile, s);
        sem_post(&shared[3].mutex);
        sem_post(&shared[3].empty);
    }
    fclose(resultFile);
    syslog(LOG_INFO, "Finalização do Consumer\n");
    
}

//Funcoes de Matrix
void matrixReadFile(FILE *matrix_file, double matrix[Lines][Cols]) 
{
    int i, j;
    for (i = 0; i < Lines; i++) 
    {
        for (j = 0; j < Cols; j++) 
        {
            fscanf(matrix_file, "%lf", &matrix[i][j]);
        }
    }
}

void matrixMultiplier(double matrixA[Lines][Cols], double matrixB[Lines][Cols], double matrixC[Lines][Cols]) 
{
    int i, tid;
    #pragma omp parallel num_threads(2) private(i, tid) shared(matrixA, matrixB, matrixC)
    {
    tid = omp_get_thread_num();
        int lineStart = tid * (Lines / 2);
        int lineEnd = lineStart + (Lines / 2);
        int j, k;
        #pragma omp parallel for
        for (i = lineStart; i < lineEnd; i++) 
        {
            for (j = 0; j < Cols; j++) {
                matrixC[i][j] = 0;
                for (k = 0; k < Cols; k++) 
                {
                    matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
                }
            }
        }
    }
}

void printMatrix(double matrix[Lines][Cols])
{
    int i, j;
    for (i = 0; i < Lines; i++) 
    {
        for (j = 0; j < Cols; j++) 
        {
            printf("%lf ", matrix[i][j]);
        }
        printf("\n");
    }
}

//Funcoes de Soma
double sumArray(double array[vectorSize]) 
{
    int j;
    double sum = 0;
    for (j = 0; j < vectorSize; j++) 
    {
        sum += array[j];
    }
    return sum;
}

void sumMatrixCols(double matrix[Lines][Cols], double vectorSums[vectorSize]) 
{
    int i, j, tid;
    #pragma omp parallel num_threads(2) private(i, j, tid) shared(matrix, vectorSums)
    {
        tid = omp_get_thread_num();
        int colStart = tid * (Cols / 2);
        int colEnd = colStart + (Cols / 2);

        #pragma omp parallel for
        for (j = colStart; j < colEnd; j++) 
        {
            double sum = 0;
            for (i = 0; i < Lines; i++) 
            {
                sum += matrix[i][j];
            }
            vectorSums[j] = sum;
        }
    }
}

//Funcoes de Print
void printFile(FILE *out, S *s) 
{
    fprintf(out,"================================\n");
    fprintf(out, "Entrada %s;\n", s->Name);
    fprintf(out, "——————————–\n");
    printMatrixFile(out, s->matrixA);
    fprintf(out, "——————————–\n");
    printMatrixFile(out, s->matrixB);
    fprintf(out, "——————————–\n");
    printMatrixFile(out, s->matrixC);
    fprintf(out, "——————————–\n");
    printArrayFile(out, s->vectorV);
    fprintf(out, "——————————–\n");
    fprintf(out, "%lf\n", s->resultE);
    fprintf(out, "——————————–\n");
    fprintf(out,"================================\n");
}

void printArrayFile(FILE *out, double array[vectorSize]) 
{
    int j;
    for (j = 0; j < vectorSize; j++) 
    {
        fprintf(out, "%lf\n", array[j]);
    }
}

void printArray(double array[vectorSize]) 
{
    int j;
    for (j = 0; j < vectorSize; j++) 
    {
        printf("%lf ", array[j]);
    }
    printf("\n");
}

void printMatrixFile(FILE *out, double matrix[Lines][Cols]) 
{
    int i, j;
    for (i = 0; i < Lines; i++) {
        for (j = 0; j < Cols; j++) {
            fprintf(out, "%lf ", matrix[i][j]);
        }
        fprintf(out, "\n");
    }
}

int main(int argc, char *argv[]) 
{
    pid_t pid;
    pid = fork();
    if (pid < 0)
    {
        printf("PID menor que 0\n");
        exit(EXIT_FAILURE);
    }
    if (pid > 0)
    {
        printf("sucesso, PID %d\n", pid);
        exit(EXIT_SUCCESS);
    }
    umask(0);
    if (setsid() < 0)
    {
        printf("Setsid Falhou, SID menor que 0\n");
        exit(EXIT_FAILURE);
    }
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    int process;
    openlog("Logs",LOG_NDELAY, LOG_DAEMON);
    syslog(LOG_INFO, "projeto da dupla João Vítor Oliveira e Gregory Santos\n");
    syslog(LOG_INFO, "Iniciando aplicação\n");

    pthread_t id_p, id_cp1, id_cp2, id_cp3, id_c;
    int s_p[ProducerInstances], s_cp1[CP1Instances], s_cp2[CP2Instances], s_cp3[CP3Instances], s_c[ConsumerInstances];
    int idx_shared;
    syslog(LOG_INFO, "Iniciando os semáforos dos elementos\n");
    for (idx_shared = 0; idx_shared < SHARED_SIZE; idx_shared++) 
    {
        sem_init(&shared[idx_shared].empty, 0, BUFF_SIZE);
        sem_init(&shared[idx_shared].full, 0, 0);
        sem_init(&shared[idx_shared].mutex, 0, 1);
    }

    syslog(LOG_INFO, "Iniciando o Producer\n");
    s_p[0] = 0;
    pthread_create(&id_p, NULL, Producer, argv[1]);
    syslog(LOG_INFO, "Iniciando o Consumer Producer 1\n");
    int idx_cp1;
    for (idx_cp1 = 0; idx_cp1 < CP1Instances; idx_cp1++) 
    {
        s_cp1[idx_cp1] = idx_cp1;
        pthread_create(&id_cp1, NULL, ConsumerProducer1, &s_cp1[idx_cp1]);
    }

    syslog(LOG_INFO, "Iniciando o Consumer Producer 2\n");
    int idx_cp2;
    for (idx_cp2 = 0; idx_cp2 < CP2Instances; idx_cp2++) 
    {
        s_cp2[idx_cp2] = idx_cp2;
        pthread_create(&id_cp2, NULL, ConsumerProducer2, &s_cp2[idx_cp2]);
    }

    syslog(LOG_INFO, "Iniciando o Consumer Producer 3\n");
    int idx_cp3;
    for (idx_cp3 = 0; idx_cp3 < CP3Instances; idx_cp3++) 
    {
        s_cp3[idx_cp3] = idx_cp3;
        pthread_create(&id_cp3, NULL, ConsumerProducer3, &s_cp3[idx_cp3]);
    }

    syslog(LOG_INFO, "Iniciando o Consumer\n");
    s_c[0] = 0;
    pthread_create(&id_c, NULL, Consumer, argv[2]);
    syslog(LOG_INFO, "Finalização, espera o final do Consumer para matar ConsumerProducer1, ConsumerProducer2 e ConsumerProducer3\n");
    pthread_join(id_c, NULL);
    pthread_cancel(id_cp1);
    pthread_cancel(id_cp2);
    pthread_cancel(id_cp3);

    syslog(LOG_INFO, "Finalizado o processo\n");
    closelog();

    return EXIT_SUCCESS;
}