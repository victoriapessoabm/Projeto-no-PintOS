/* Measures the correctness of the "nice" implementation.

   The "fair" tests run either 2 or 20 threads all niced to 0.
   The threads should all receive approximately the same number
   of ticks.  Each test runs for 30 seconds, so the ticks should
   also sum to approximately 30 * 100 == 3000 ticks.

   The mlfqs-nice-2 test runs 2 threads, one with nice 0, the
   other with nice 5, which should receive 1,904 and 1,096 ticks,
   respectively, over 30 seconds.

   The mlfqs-nice-10 test runs 10 threads with nice 0 through 9.
   They should receive 672, 588, 492, 408, 316, 232, 152, 92, 40,
   and 8 ticks, respectively, over 30 seconds.

   (The above are computed via simulation in mlfqs.pm.) */

#include <stdio.h>
#include <inttypes.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static void test_mlfqs_fair(int thread_cnt, int nice_min, int nice_step);
// funcao q implementa o teste para diferentes valores de nice e de numero de threads

void test_mlfqs_fair_2(void)
{ // executa o teste com 2 threads, todas com nice = 0
  test_mlfqs_fair(2, 0, 0);
} // verifica se ambas as threads recebem aproximadamente o mesmo numero de ticks

void test_mlfqs_fair_20(void)
{ // executa o teste com 20 threads, todas com nice = 0
  test_mlfqs_fair(20, 0, 0);
} // verifica se todas as threads recebem aproximadamente o mesmo numero de ticks

void test_mlfqs_nice_2(void)
{ // cria duas threads
  test_mlfqs_fair(2, 0, 5);
} // thread com nice = 0 (prioridade mais alta, maior qtd de ticks de cpu); thread com nice = 5 (menor prioridade, menos ticks)

void test_mlfqs_nice_10(void)
{ // 10 threads - verifica se as threads com nice menor recebem mais ticks de cpu qdo comparadas as com nice maior
  test_mlfqs_fair(10, 0, 1);
} // primeira com nice = 0, segunda com nice = 1, terceira com nice = 2, ..., decima com nice = 9

#define MAX_THREAD_CNT 20 // numero max de threads = 20

struct thread_info
{                     // armazenada dados de cada thread
  int64_t start_time; // momento q a thread comecou a rodar
  int tick_count;     // numero de ticks da cpu recebidos
  int nice;           // valor do nice da thread
};

static void load_thread(void *aux);

static void
test_mlfqs_fair(int thread_cnt, int nice_min, int nice_step)
{                                          //(numero de threads criadas, valor minimo de nice para a primeira thread, diferenca de nice entre threads consec)
  struct thread_info info[MAX_THREAD_CNT]; // vetor de structs para armazenar dados de cada thread
  int64_t start_time;
  int nice;
  int i;

  ASSERT(thread_mlfqs);                                  // verifica se escalonador mlfqs esta ativado
  ASSERT(thread_cnt <= MAX_THREAD_CNT);                  // verifica se a qtd de threads criadas eh a maxima permitida
  ASSERT(nice_min >= -10);                               // garante q valores de nice sao validos
  ASSERT(nice_step >= 0);                                // garante q valores de nice sao validos
  ASSERT(nice_min + nice_step * (thread_cnt - 1) <= 20); // garante q valores de nice sao validos

  thread_set_nice(-20); // configura a thread principal para a prioridade maxima (menor nice possivel = -20)

  start_time = timer_ticks();                // marca tempo inicial
  msg("Starting %d threads...", thread_cnt); // mostra qtas threads serao inicializadas
  nice = nice_min;                           // nice recebe valor de nice da primeira thread
  for (i = 0; i < thread_cnt; i++)
  {
    struct thread_info *ti = &info[i]; // armazena info de cada thread na estrutura thread_info
    char name[16];

    ti->start_time = start_time; // passa tempo inicial
    ti->tick_count = 0;          // zera qtd de ticks da cpu recebidos pela thread
    ti->nice = nice;             // passa valor do nice da thread criada

    snprintf(name, sizeof name, "load %d", i);         // da nome a thread
    thread_create(name, PRI_DEFAULT, load_thread, ti); // chama load_thread para executar cada thread criada

    nice += nice_step; // incrementa valor do nice da prox thread
  }
  msg("Starting threads took %" PRId64 " ticks.", timer_elapsed(start_time));

  msg("Sleeping 40 seconds to let threads run, please wait...");
  timer_sleep(40 * TIMER_FREQ); // aguarda 40s para permitir q todas as threads rodem (dormem enquanto isso)

  for (i = 0; i < thread_cnt; i++) // exibe numero de ticks recebidos por cada thread
    msg("Thread %d received %d ticks.", i, info[i].tick_count);
}

static void
load_thread(void *ti_)
{
  struct thread_info *ti = ti_;
  int64_t sleep_time = 5 * TIMER_FREQ;              // define tempo de sono inicial da thread como 5s
  int64_t spin_time = sleep_time + 30 * TIMER_FREQ; // tempo final de execucao da thread = tempo dormindo(5s) e tempo fazendo espera ocupada (30s)
  int64_t last_time = 0;                            // contador q indica qtos ticks a thread recebeu de cpu

  thread_set_nice(ti->nice);                               // configura valor de nice da thread
  timer_sleep(sleep_time - timer_elapsed(ti->start_time)); // garante q todas as threads comecem ao msm tempo
  while (timer_elapsed(ti->start_time) < spin_time)
  {
    int64_t cur_time = timer_ticks();
    if (cur_time != last_time)
      ti->tick_count++;
    last_time = cur_time;
  } // contabiliza ticks de cpu recebidos pela thread
}

/*
obs: nice
- representa a gentileza de uma thread ao compartilhar tempo de cpu com outras threads
- varia de -20 a 20 = menos gentil(thread recebe mais tempo de cpu) a mais gentil(thread recebe menos tempo de cpu)
- escalonador usa esse valor para ajudar no calculo de prioridade da thread
- qto menor o nice, menos gentil, recebe mais tempo de cpu e possui prioridade maior
- qto maior o nice, mais gentil, recebe menos tempo de cpu e possui prioridade menor
- testes fair - todas as threads com mesmo valor de nice (nice = 0) devem receber aprox mesma qtd de ticks da cpu
- testes nice - as threads tem diferentes valores de nice, o q afeta a qtd de tempo de cpu q cada uma recebe
- define o quao educada uma thread eh ao compartilhar a cpu
- o valor de nice eh herdado na criacao da thread, mas pode ser modificado em thread_set_nice()

obs: TIMER_FREQ
- representa o numero de tics por segundo
- qdo multiplicado por x, representa o numero total de ticks de cpu por x segundos

objetivo:
- testes medem se o escalonador esta distribuindo os ticks corretamente com base no nice das threads
- mesmo nice -> mesma prioridade -> tempo de cpu semelhante
- nice menor -> maior prioridade -> mais tempo de cpu
- nice maior -> menor prioridade -> menor tempo de cpu

*/