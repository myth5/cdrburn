/* -*- indent-tabs-mode: t; tab-width: 8; c-basic-offset: 8; -*- */

#include "message.h"
#include "libburn.h"
#include "debug.h"

#include <stdlib.h>

struct message_list
{
	struct message_list *next;

	struct burn_message *msg;
};

static struct message_list *queue;

void burn_message_free(struct burn_message *msg)
{
	free(msg);
}

void burn_message_clear_queue(void)
{
	struct burn_message *msg;

	if ((msg = burn_get_message())) {
		burn_print(0,
			   "YOU HAVE MESSAGES QUEUED FROM THE LAST OPERATION. "
			   "YOU SHOULD BE GRABBING THEM ALL!\n");
		do {
			burn_message_free(msg);
		} while ((msg = burn_get_message()));
	}
}

struct burn_message *burn_get_message()
{
	struct burn_message *msg = NULL;

	if (queue) {
		struct message_list *next;

		next = queue->next;
		msg = queue->msg;
		free(queue);
		queue = next;
	}

	return msg;
}

static void queue_push_tail(struct burn_message *msg)
{
	struct message_list *node;

	node = malloc(sizeof(struct message_list));
	node->next = NULL;
	node->msg = msg;

	if (!queue)
		queue = node;
	else {
		struct message_list *it;

		for (it = queue; it->next; it = it->next) ;
		it->next = node;
	}
}

void burn_message_info_new(struct burn_drive *drive,
			   enum burn_message_info message)
{
	struct burn_message *msg;

	msg = malloc(sizeof(struct burn_message));
	msg->drive = drive;
	msg->type = BURN_MESSAGE_INFO;
	msg->detail.info.message = message;

	queue_push_tail(msg);
}

void burn_message_warning_new(struct burn_drive *drive,
			      enum burn_message_info message)
{
	struct burn_message *msg;

	msg = malloc(sizeof(struct burn_message));
	msg->drive = drive;
	msg->type = BURN_MESSAGE_WARNING;
	msg->detail.warning.message = message;

	queue_push_tail(msg);
}

void burn_message_error_new(struct burn_drive *drive,
			    enum burn_message_info message)
{
	struct burn_message *msg;

	msg = malloc(sizeof(struct burn_message));
	msg->drive = drive;
	msg->type = BURN_MESSAGE_ERROR;
	msg->detail.error.message = message;

	queue_push_tail(msg);
}
