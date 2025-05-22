/* Checks that recent_cpu and priorities are updated for blocked
   threads.

   The main thread sleeps for 25 seconds, spins for 5 seconds,
   then releases a lock.  The "block" thread spins for 20 seconds
   then attempts to acquire the lock, which will block for 10
   seconds (until the main thread releases it).  If recent_cpu
   decays properly while the "block" thread sleeps, then the
   block thread should be immediately scheduled when the main
   thread releases the lock. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

static void block_thread(void *lock_);

void test_mlfqs_block(void)
{
  int64_t start_time; // tempo inicial para medicoes
  struct lock lock;   // lock sera utilizado para sincronizar as threads
  // thread_mlfqs eh uma variavel global q indica se o escalonador MLFQS esta ativado
  ASSERT(thread_mlfqs); // faz com q o teste falhe se o escalonador MLFQS n estiver ativado

  msg("Main thread acquiring lock."); // msg informando q thread principal esta adquirindo lock
  lock_init(&lock);                   // inicializa lock
  lock_acquire(&lock);                // thread principal adquire lock impedindo outras threads de acessá-lo

  msg("Main thread creating block thread, sleeping 25 seconds..."); // thread principal cria uma nova thread
  thread_create("block", PRI_DEFAULT, block_thread, &lock);         // thread criada executara a funcao block_thread, q recebe lock como argumento
  timer_sleep(25 * TIMER_FREQ);                                     // thread principal dorme por 25s

  msg("Main thread spinning for 5 seconds...");
  start_time = timer_ticks();                        // marca tempo inicial
  while (timer_elapsed(start_time) < 5 * TIMER_FREQ) // thread principal faz busy-waiting for 5s
    continue;

  msg("Main thread releasing lock.");
  lock_release(&lock); // thread principal libera lock

  msg("Block thread should have already acquired lock."); // verifica se thread "block" adquiriu lock
}

static void
block_thread(void *lock_)
{                            // funcao da thread secundaria
  struct lock *lock = lock_; // lock_ eh um ponteiro generico q eh convertido para struct lock* (sincronizara threads)
  int64_t start_time;        // sera usado para capturar tempo atual

  msg("Block thread spinning for 20 seconds...");
  start_time = timer_ticks();                         // tempo atual
  while (timer_elapsed(start_time) < 20 * TIMER_FREQ) // busy-waiting por 20s da thread secundaria
    continue;

  msg("Block thread acquiring lock...");
  lock_acquire(lock); // thread secundaria tenta adquirir lock apos espera ocupada
  // lock ainda estara sendo usado pela thread principal, ent thread secundaria deve ser bloqueada ate q seja liberado

  msg("...got it."); // assim q thread secundaria obter lock imprime essa msg
}

/*
Resumo do funcionamento:

1 - Thread principal inicializa um lock e o adquire
2 - Cria block thread
3 - Main thread dorme por 25s
4 - Main thread faz busy-waiting por mais 5s
5 - Enquanto isso, block thread faz busy-waiting por 20s e ent tenta adquirir lock
6 - Block thread fica bloqueada por n conseguir adquirir o lock, pois ainda esta sendo utilizado
7 - Main thread libera lock apos busy-waiting
8 - Block thread imediatamente adquire o lock e confirma q o adquiriu

O código testa o comportamento do escalonador, verificando como ele ajusta a prioridade das threads com base no
tempo de cpu usado e no tempo de bloqueio:
- A thread secundaria deve ficar bloqueada por 10s(ate q a thread principal libere o lock);
- Assim q o lock for liberado, a thread secundaria deve imediatamente ser escalonada para executá-lo;

obs: recent_cpu
qto tempo de cpu a thread tem utilizado recentemente
aumenta qdo a thread executa e decai qdo esta bloqueada ou inativa
qto maior, menor a prioridade da thread
se uma thread for bloqueada, seu recent_cpu decai gradualmente, permitindo q recupere prioridade com o tempo


Objetivos do teste:
- Atualiza corretamente o recent_cpu;
- Ajusta prioridades corretamente;
 = Qdo o lock for liberado, block thread deve imediatamente ser escalonada para rodar
 = Caso contrário, significa q recent_cpu n esta decaindo corretamente para as threads bloqueadas
*/