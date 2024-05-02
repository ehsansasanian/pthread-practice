/*------------------------------------------------------------------------*/
// Copyright (C) 2019-2020, Armin Biere, Johannes Kepler University Linz
// Copyright (C) 2020, .... [ put your name here ] ....
/*------------------------------------------------------------------------*/

#define program "bogus"	// CHANGE THE NAME OF THE PROGRAM!!

static const char *usage =
  "usage: " program " <workers> <operations>\n"
  "(operations logarithmically in base 10)\n";

/*------------------------------------------------------------------------*/

#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>

/*------------------------------------------------------------------------*/

static unsigned num_workers;
static uint64_t total_operations;

/*------------------------------------------------------------------------*/

// Error and verbose messages.

static void die (const char *fmt, ...) {
  fprintf (stderr, "%s: error: ", program);
  va_list ap;
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  exit (1);
}

static void msg (const char *fmt, ...) {
  va_list ap;
  va_start (ap, fmt);
  vprintf (fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  fflush (stdout);
}

/*========================================================================*/

// This part is the real 'meat' of the exercise:

static bool go;
static uint64_t global_result;

typedef struct worker worker;

struct worker {
  pthread_t thread;
  uint64_t operations;
};

static worker *workers;

static void * run (void *ptr) {
  worker *worker = ptr;
  while (!go)
    ;
  const uint64_t operations = worker->operations;
  uint64_t *p = &global_result;
  for (uint64_t i = 0; i < operations; i++){
    *p = *p + 1;
  }
  return 0;
}

/*========================================================================*/

// Get current wall-clock and process time in seconds.

static double wall_clock_time () {
  double res = 0;
  struct timeval tv;
  if (!gettimeofday (&tv, 0)) {
    res = 1e-6 * tv.tv_usec + tv.tv_sec;
  }
  return res;
}

static double process_time () {
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u)) {
    return 0;
  }
  double res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

/*------------------------------------------------------------------------*/

int main (int argc, char **argv) {

  // Parse command line option.
  if (argc != 3) {
    fprintf (stderr, usage, program);
	exit (1);
  }
  int tmp = atoi (argv[1]);
  if (tmp <= 0) {
    die ("non-positive number of workers");
  }
  num_workers = tmp;
  tmp = atoi (argv[2]);
  if (tmp > 20) {
    die ("operations exponent too large");
  }
  total_operations = 1;
  while (tmp--) {
    total_operations *= 10;
  }

  msg ("testing %s version", program);
  msg ("initializing %u workers", num_workers);
  msg ("executing %" PRIu64 " operations in total", total_operations);
  uint64_t rest = total_operations % num_workers;
  uint64_t operations_per_worker = total_operations / num_workers;
  msg ("executing %" PRIu64 " operations per worker", operations_per_worker);

  workers = calloc (num_workers, sizeof *workers);

  for (unsigned i = 0; i < num_workers; i++){
    worker *worker = workers + i;
    worker->operations = operations_per_worker;
    if (rest){
	  worker->operations++;
	  rest--;
	}
  }

  for (unsigned i = 0; i < num_workers; i++){
    if (pthread_create (&workers[i].thread, 0, run, workers + i)) {
      die ("failed to create worker thread %u", i);
    }
  }

  double w = wall_clock_time ();
  double p = process_time ();

  go = true;

  for (unsigned i = 0; i < num_workers; i++) {
    if (pthread_join (workers[i].thread, 0)) {
      die ("failed to join worker thread %u", i);
    }
  }

  free (workers);

  if (global_result == total_operations) {
    msg ("SUCCESS: result is %" PRIu64 " as expected", total_operations);
  } else {
    msg ("ERROR: result is %" PRIu64 " but expected %" PRIu64,
	 global_result, total_operations);
  }

  w = wall_clock_time () - w;
  p = process_time () - p;

  msg ("used %.3f seconds wall-clock and %.3f process time", w, p);
  msg ("utilization %.0f%%, %.1f million operations per second",
       (w ? 100 * p / w : 0), w ? total_operations / 1e6 / w : 0);

  return 0;
}
