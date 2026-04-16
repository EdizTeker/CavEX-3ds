/*
	Copyright (c) 2023 ByteBit/xtreme8000

	This file is part of CavEX.

	CavEX is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	CavEX is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with CavEX.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>

#include "thread.h"

#ifdef PLATFORM_PC

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void thread_create(struct thread* t, void* (*entry)(void* arg), void* arg,
				   uint8_t priority) {
	assert(t && entry);
	pthread_create(&t->native, NULL, entry, arg);
}

void thread_join(struct thread* t) {
	assert(t);
	pthread_join(t->native, NULL);
}

void thread_msleep(size_t ms) {
	usleep(ms * 1000);
}

void tchannel_init(struct thread_channel* c, size_t count) {
	assert(c && count > 0);

	c->count = 0;
	c->length = count;
	c->data = malloc(c->length * sizeof(void*));

	pthread_mutex_init(&c->lock, NULL);
	pthread_cond_init(&c->signal, NULL);
}

void tchannel_close(struct thread_channel* c) {
	assert(c);

	free(c->data);
	pthread_cond_destroy(&c->signal);
	pthread_mutex_destroy(&c->lock);
}

bool tchannel_receive(struct thread_channel* c, void** msg, bool block) {
	assert(c && msg);

	pthread_mutex_lock(&c->lock);

	if(block) {
		while(!c->count)
			pthread_cond_wait(&c->signal, &c->lock);
	} else {
		if(!c->count) {
			pthread_mutex_unlock(&c->lock);
			return false;
		}
	}

	*msg = c->data[0];
	c->count--;

	memmove(c->data, c->data + 1, c->count * sizeof(void*));

	pthread_cond_signal(&c->signal);
	pthread_mutex_unlock(&c->lock);
	return true;
}

bool tchannel_send(struct thread_channel* c, void* msg, bool block) {
	assert(c && msg);

	pthread_mutex_lock(&c->lock);

	if(block) {
		while(c->count >= c->length)
			pthread_cond_wait(&c->signal, &c->lock);
	} else {
		if(c->count >= c->length) {
			pthread_mutex_unlock(&c->lock);
			return false;
		}
	}

	c->data[c->count++] = msg;

	pthread_cond_signal(&c->signal);
	pthread_mutex_unlock(&c->lock);
	return true;
}

#endif

#ifdef PLATFORM_WII

void thread_create(struct thread* t, void* (*entry)(void* arg), void* arg,
				   uint8_t priority) {
	assert(t && entry);
	LWP_CreateThread(&t->native, entry, arg, NULL, 0, priority);
}

void thread_join(struct thread* t) {
	assert(t);

	void* unused;
	LWP_JoinThread(t->native, &unused);
}

void thread_msleep(size_t ms) {
	usleep(ms * 1000);
}

void tchannel_init(struct thread_channel* c, size_t count) {
	assert(c && count > 0);
	MQ_Init(&c->native, count);
}

void tchannel_close(struct thread_channel* c) {
	assert(c);
	MQ_Close(c->native);
}

bool tchannel_receive(struct thread_channel* c, void** msg, bool block) {
	return MQ_Receive(c->native, msg, block ? MQ_MSG_BLOCK : MQ_MSG_NOBLOCK);
}

bool tchannel_send(struct thread_channel* c, void* msg, bool block) {
	return MQ_Send(c->native, msg, block ? MQ_MSG_BLOCK : MQ_MSG_NOBLOCK);
}

#endif

#ifdef PLATFORM_3DS
#include <3ds.h>
#include <stdlib.h>
#include <string.h>

struct thread_args_wrapper {
    void* (*engine_entry)(void*);
    void* actual_arg;
};

static void libctru_thread_wrapper(void* wrapper_arg) {
    struct thread_args_wrapper* wrapper = (struct thread_args_wrapper*)wrapper_arg;
    
    wrapper->engine_entry(wrapper->actual_arg);
    
    free(wrapper); 
}

void thread_create(struct thread* t, void* (*entry)(void* arg), void* arg, uint8_t priority) {
    s32 sys_priority = 0x3F - priority; 
    if (sys_priority < 0x18) sys_priority = 0x18;
    if (sys_priority > 0x3F) sys_priority = 0x3F;
    
    struct thread_args_wrapper* wrapper = malloc(sizeof(struct thread_args_wrapper));
    wrapper->engine_entry = entry;
    wrapper->actual_arg = arg;

    t->native = threadCreate(libctru_thread_wrapper, wrapper, 32 * 1024, sys_priority, -2, false);
}

void thread_join(struct thread* t) {
    threadJoin(t->native, U64_MAX);
    threadFree(t->native);
}

void thread_msleep(size_t ms) {
    svcSleepThread((s64)ms * 1000000LL); 
}

void tchannel_init(struct thread_channel* c, size_t count) {
    c->count = 0;
    c->length = count;
    c->data = malloc(c->length * sizeof(void*));
    LightLock_Init(&c->lock);
    LightEvent_Init(&c->signal, RESET_ONESHOT);
}

void tchannel_close(struct thread_channel* c) {
    free(c->data);
}

bool tchannel_receive(struct thread_channel* c, void** msg, bool block) {
    LightLock_Lock(&c->lock);
    if(block) {
        while(!c->count) {
            LightLock_Unlock(&c->lock);
            LightEvent_Wait(&c->signal);
            LightLock_Lock(&c->lock);
        }
    } else {
        if(!c->count) {
            LightLock_Unlock(&c->lock);
            return false;
        }
    }
    
    *msg = c->data[0];
    c->count--;
    memmove(c->data, c->data + 1, c->count * sizeof(void*));
    LightLock_Unlock(&c->lock);
    return true;
}

bool tchannel_send(struct thread_channel* c, void* msg, bool block) {
    LightLock_Lock(&c->lock);
    if(block) {
        while(c->count >= c->length) {
            LightLock_Unlock(&c->lock);
            svcSleepThread(1000000LL); 
            LightLock_Lock(&c->lock);
        }
    } else {
        if(c->count >= c->length) {
            LightLock_Unlock(&c->lock);
            return false;
        }
    }
    
    c->data[c->count++] = msg;
    LightEvent_Signal(&c->signal);
    LightLock_Unlock(&c->lock);
    return true;
}
#endif
