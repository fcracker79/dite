#include "time_queue.h"

void
tq_init ( tq_elem** top ) {
	if ( !top ) return;
	*top = 0;
}

void
tq_tick ( tq_elem** top ) {
	if ( !top ) return;
	if ( *top && (*top)->ttl > 0 )
		--(*top)->ttl;
}

void
tq_push ( tq_elem** top, int ttl, int id ) {
	tq_elem* p;
	tq_elem* q;
	tq_elem* x;

	if ( !top || ttl < 0 ) return;

	x = (tq_elem*) malloc ( sizeof (tq_elem) );
	x->ttl = ttl;
	x->id = id;
	x->next = 0;

	/* empty list */
	if ( ! (*top) ) {
		*top = x;
	}
	else {
		/* insert on top of the list */
		if ( x->ttl <= (*top)->ttl ) {
			p = *top;
			*top = x;
			x->next = p;
			p->ttl -= x->ttl;
		}
		/* insert amid list elements */
		else {
			p = *top;
			x->ttl -= p->ttl;
			q = p->next;
			while ( q  && x->ttl > q->ttl ) {
				p = p->next;
				q = q->next;
				x->ttl -= p->ttl;
			}
			p->next = x;
			x->next = q;
			if ( q )
				q->ttl -= x->ttl;
		}
	}
}

int
tq_pop ( tq_elem** top ) {
	int id;
	tq_elem* p;

	if ( !top ) return -1;
	if ( *top ) {
		id = (*top)->id;
		p = *top;
		*top = (*top)->next;
		free (p);
		return id;
	}
	return -1;
}

int
tq_test ( tq_elem** top ) {
	if ( !top ) return -1;
	if ( *top ) {
		if ( ! (*top)->ttl ) return 1;
		return 0;
	}
	return -1;
}

void
tq_clear ( tq_elem** top ) {
	tq_elem* p;
	tq_elem* q;
	
	if ( !top ) return;
	p = *top;
	while ( p ) {
		q = p->next;
		free (p);
		p = q;
	}
	*top = 0;
}

int
tq_empty ( tq_elem** top ) {
	if ( !top ) return -1;
	if ( *top ) return 0;
	return 1;
}

#ifdef TQ_DEBUG
void
tq_print ( tq_elem** top ) {
	tq_elem* p;
	int i;
	
	if ( !top ) return;
	p = *top;
	for ( i = 0; p ; i++ ) {
		printf ("%.2d: %d [%d] -> %p\n", i, p->id, p->ttl, p->next);
		p = p->next;
	}
}
#endif
