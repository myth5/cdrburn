/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include "libburn.h"
#include "transport.h"
#include "drive.h"
#include "write.h"
#include "options.h"
#include "async.h"

#include <pthread.h>
#include <assert.h>
#include <stdlib.h>

#define SCAN_GOING() (workers && !workers->drive)

typedef void *(*WorkerFunc) (void *);

struct scan_opts
{
	struct burn_drive_info **drives;
	unsigned int *n_drives;

	int done;
};

struct erase_opts
{
	struct burn_drive *drive;
	int fast;
};

struct write_opts
{
	struct burn_drive *drive;
	struct burn_write_opts *opts;
	struct burn_disc *disc;
};

struct w_list
{
	struct burn_drive *drive;
	pthread_t thread;

	struct w_list *next;

	union w_list_data
	{
		struct scan_opts scan;
		struct erase_opts erase;
		struct write_opts write;
	} u;
};

static struct w_list *workers;

static struct w_list *find_worker(struct burn_drive *d)
{
	struct w_list *a;

	for (a = workers; a; a = a->next)
		if (a->drive == d)
			return a;
	return NULL;
}

static void add_worker(struct burn_drive *d, WorkerFunc f, void *data)
{
	struct w_list *a;
	struct w_list *tmp;

	a = malloc(sizeof(struct w_list));
	a->drive = d;
	a->u = *(union w_list_data *)data;

	/* insert at front of the list */
	a->next = workers;
	tmp = workers;
	workers = a;

	if (d)
		d->busy = BURN_DRIVE_SPAWNING;

	if (pthread_create(&a->thread, NULL, f, a)) {
		free(a);
		workers = tmp;
		return;
	}
}

static void remove_worker(pthread_t th)
{
	struct w_list *a, *l = NULL;

	for (a = workers; a; l = a, a = a->next)
		if (a->thread == th) {
			if (l)
				l->next = a->next;
			else
				workers = a->next;
			free(a);
			break;
		}
	assert(a != NULL);	/* wasn't found.. this should not be possible */
}

static void *scan_worker_func(struct w_list *w)
{
	burn_drive_scan_sync(w->u.scan.drives, w->u.scan.n_drives);
	w->u.scan.done = 1;
	return NULL;
}

int burn_drive_scan(struct burn_drive_info *drives[], unsigned int *n_drives)
{
	struct scan_opts o;
	int ret = 0;

	/* cant be anything working! */
	assert(!(workers && workers->drive));

	if (!workers) {
		/* start it */
		o.drives = drives;
		o.n_drives = n_drives;
		o.done = 0;
		add_worker(NULL, (WorkerFunc) scan_worker_func, &o);
	} else if (workers->u.scan.done) {
		/* its done */
		ret = workers->u.scan.done;
		remove_worker(workers->thread);
		assert(workers == NULL);
	} else {
		/* still going */
	}
	return ret;
}

static void *erase_worker_func(struct w_list *w)
{
	burn_disc_erase_sync(w->u.erase.drive, w->u.erase.fast);
	remove_worker(pthread_self());
	return NULL;
}

void burn_disc_erase(struct burn_drive *drive, int fast)
{
	struct erase_opts o;

	assert(drive);
	assert(!SCAN_GOING());
	assert(!find_worker(drive));

	o.drive = drive;
	o.fast = fast;
	add_worker(drive, (WorkerFunc) erase_worker_func, &o);
}

static void *write_disc_worker_func(struct w_list *w)
{
	burn_disc_write_sync(w->u.write.opts, w->u.write.disc);

	/* the options are refcounted, free out ref count which we added below 
	 */
	burn_write_opts_free(w->u.write.opts);

	remove_worker(pthread_self());
	return NULL;
}

void burn_disc_write(struct burn_write_opts *opts, struct burn_disc *disc)
{
	struct write_opts o;

	assert(!SCAN_GOING());
	assert(!find_worker(opts->drive));
	o.drive = opts->drive;
	o.opts = opts;
	o.disc = disc;

	opts->refcount++;

	add_worker(opts->drive, (WorkerFunc) write_disc_worker_func, &o);
}

void burn_async_join_all(void)
{
	void *ret;

	while (workers)
		pthread_join(workers->thread, &ret);
}
