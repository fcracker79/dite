#include <stdio.h>
#include <stdlib.h>
#include "time_queue.h"

int
main () {
	time_queue t;
	int id;
	int i;

	tq_init ( &t );

	tq_push ( &t, 10, 0 );
	tq_push ( &t, 2, 1 );
	tq_push ( &t, 1, 2 );
	tq_push ( &t, 0, 3 );
	tq_push ( &t, 10, 4 );
	tq_push ( &t, 2, 5 );
	tq_push ( &t, 16, 6 );
	tq_push ( &t, 2, 7 );
	tq_push ( &t, 3, 8 );
	tq_push ( &t, 5, 9 );
	
#ifdef TQ_DEBUG
	   tq_print ( &t );
#endif

	for ( i = 0 ; tq_test ( &t ) >= 0 ; i++ ) {
		tq_tick ( &t );
		while ( tq_test ( &t ) == 1 ) {
			id = tq_pop ( &t );
			printf ("%.2d: pop %d\n", i, id);
		}
	}

#ifdef TQ_DEBUG
	tq_print ( &t );
#endif

	if ( tq_empty ( &t ) )
		printf ("queue is empty\n");

	tq_clear ( &t );

#ifdef TQ_DEBUG
	   tq_print ( &t );
#endif
	
	exit (1);
}
