/**
 * @file
 * @brief Link layer between timers and clock sources.
 *
 * @date 09.07.2012
 * @author Alexander Kalmuk
 */

#include <module/embox/kernel/time/slowdown.h>
#include <kernel/irq_lock.h>
#include <kernel/time/timer.h>
#include <kernel/time/clock_source.h>

#include <kernel/sched/schedee_priority.h>
#include <kernel/lthread/lthread.h>

#include <framework/mod/options.h>

#define SLOWDOWN_SHIFT OPTION_MODULE_GET(embox__kernel__time__slowdown, NUMBER, shift)
#define CLOCK_HND_PRIORITY OPTION_GET(NUMBER, hnd_priority)

static struct lthread clock_handler_lt;
/* from jiffies.c */
extern struct clock_source *cs_jiffies;

void clock_tick_handler(int irq_num, void *dev_id) {
	struct clock_source *cs = (struct clock_source *) dev_id;

	assert(cs);
	if (++cs->jiffies_cnt == (1 << SLOWDOWN_SHIFT)) {
		cs->jiffies_cnt = 0;
		cs->jiffies++;

		if (cs_jiffies->event_device &&	irq_num == cs_jiffies->event_device->irq_nr) {
			if (timer_strat_need_sched(cs_jiffies->jiffies)) {
				lthread_launch(&clock_handler_lt);
			}
		}
	}
}

static int clock_handler(struct lthread *self) {
	timer_strat_sched(cs_jiffies->jiffies);
	return 0;
}

int clock_tick_init(void) {
	static int inited = 0;

	if (!inited) {
		lthread_init(&clock_handler_lt, &clock_handler);
		schedee_priority_set(&clock_handler_lt.schedee, CLOCK_HND_PRIORITY);

		inited = 1;
	}

	return 0;
}
