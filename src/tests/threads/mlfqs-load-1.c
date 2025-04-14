/* Verifies that a single busy thread raises the load average to
   0.5 in 38 to 45 seconds.  The expected time is 42 seconds, as
   you can verify:
   perl -e '$i++,$a=(59*$a+1)/60while$a<=.5;print "$i\n"'

   Then, verifies that 10 seconds of inactivity drop the load
   average back below 0.5 again. */

#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"

void test_mlfqs_load_1(void)
{
  int64_t start_time; // inicio do teste
  int elapsed;        // tempo decorrido desde o inicio do teste
  int load_avg;       // guarda valor da media de carga do sistema

  ASSERT(thread_mlfqs); // verifica se escalonador esta ligado

  msg("spinning for up to 45 seconds, please wait...");

  start_time = timer_ticks();
  for (;;) // loop infinito q seja quebrado qdo a carga atingir o valor esperado
  {
    load_avg = thread_get_load_avg(); // obtem valor atual da carga do sistema
    ASSERT(load_avg >= 0);
    elapsed = timer_elapsed(start_time) / TIMER_FREQ; // tempo decorrido desde inicio do sistema
    if (load_avg > 100)                               // load_avg > 1.0 (100 em escala inteira)
      fail("load average is %d.%02d "
           "but should be between 0 and 1 (after %d seconds)",
           load_avg / 100, load_avg % 100, elapsed);
    else if (load_avg > 50) // load_avg > 0.5 (50 na escala inteira) -> sai do loop
      break;
    else if (elapsed > 45)
      fail("load average stayed below 0.5 for more than 45 seconds");
  }

  if (elapsed < 38)
    fail("load average took only %d seconds to rise above 0.5", elapsed);
  msg("load average rose to 0.5 after %d seconds", elapsed);

  msg("sleeping for another 10 seconds, please wait...");
  timer_sleep(TIMER_FREQ * 10);

  load_avg = thread_get_load_avg();
  if (load_avg < 0)
    fail("load average fell below 0");
  if (load_avg > 50)
    fail("load average stayed above 0.5 for more than 10 seconds");
  msg("load average fell back below 0.5 (to %d.%02d)",
      load_avg / 100, load_avg % 100);

  pass();
}

/*
obs: load_avg
- load_avg = load average
- mede o numero medio de threads ativas no sistema
- eh uma metrica q mede o numero de threads em execucao ou esperando para ser executadas
- load_avg = 0 -> nenhuma thread esta sendo executada
- load_avg > 0 -> existem threads sendo executadas ou aguardando para serem executadas
- load_avg > 100 -> sistema esta sobrecarregado(mais threads aguardando para usar cpu do q suportado)
- o valor de load_avg esta sendo monitorado durante o teste
- qdo uma thread esta em execucao, o valor de load_avg aumenta
- qto mais tempo uma thread ocupa a cpu, mais o load_avg aumenta, pq o sistema esta ocupado
- o tempo influencia diretamente o load_avg, pois seu valor leva em conta o tempo q as threads estao ativas ou esperando para serem executadas

Primeira parte - Spin (atividade)

O código “fica girando” (fazendo uma thread ocupada) durante um período de até 45 segundos.
O load_avg aumenta porque a thread está ocupada (e a CPU está sendo usada).
O teste espera que o load_avg suba para 0.5 (50 em termos inteiros) em aproximadamente 42 segundos.
Se o load_avg subir para 0.5 antes de 38 segundos ou demorar mais de 45 segundos para chegar a 0.5, o teste falha.

Segunda parte - Inatividade (dormindo)

Depois de a thread ter sido ocupada, o teste faz o sistema "dormir" por 10 segundos.
Durante esse período de inatividade, a carga do sistema diminui porque não há threads ativas consumindo CPU.
O teste espera que, após 10 segundos, o load_avg caia abaixo de 0.5 (porque o sistema está inativo e não há threads sendo executadas).


condicoes
- se load_avg for maior do q 1.0, teste falha. Valor esperado [0,1]
- se passagam mais de 45s e o load_avg ainda n subiu para 0.5, teste falha
- verifica se load_avg subiu mto rapido
- se passou entre 38 a 45s para load_avg atingir 0.5, teste continua normalmente
- apos 10s de inatividade load_avg deve ficar abaixo de 0.5

objetivo
- load_avg aumenta a medida q tempo passa pq o sistema tem threads sendo executadas ou aguardando para serem executadas
- qdo n ha threads executando, load_avg comeca a cair
- codigo verifica se load_avg atinge 0.5 dentro do intervalo 38 a 45s de ocupacao da cpu
- codigo verifica se load_avg cai para abaixo de 0.5 apos inatividade da cpu de 10s

*/