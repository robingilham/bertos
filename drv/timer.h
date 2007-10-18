/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2003, 2004, 2005 Develer S.r.l. (http://www.develer.com/)
 * Copyright 2000 Bernardo Innocenti <bernie@develer.com>
 *
 * -->
 *
 * \version $Id$
 *
 * \author Bernardo Innocenti <bernie@develer.com>
 *
 * \brief Hardware independent timer driver (interface)
 */

#ifndef DRV_TIMER_H
#define DRV_TIMER_H

#include <cfg/os.h>
#include <cpu/cpu.h>

/*
 * Include platform-specific binding header if we're hosted.
 * Try the CPU specific one for bare-metal environments.
 */
#if OS_HOSTED
	#include OS_HEADER(timer)
#else
	#include CPU_HEADER(timer)
#endif

#include <mware/list.h>
#include <cfg/debug.h>
#include <cfg/compiler.h>
#include <appconfig.h>


extern volatile ticks_t _clock;

/**
 * \brief Return the system tick counter (expressed in ticks)
 *
 * The result is guaranteed to increment monotonically,
 * but client code must be tolerant with respect to overflows.
 *
 * The following code is safe:
 *
 * \code
 *   ticks_t tea_start_time = timer_clock();
 *
 *   boil_water();
 *
 *   if (timer_clock() - tea_start_time > TEAPOT_DELAY)
 *       printf("Your tea, Sir.\n");
 * \endcode
 *
 * \note This function must disable interrupts on 8/16bit CPUs because the
 * clock variable is larger than the processor word size and can't
 * be copied atomically.
 */
INLINE ticks_t timer_clock(void)
{
	ticks_t result;

	ATOMIC(result = _clock);

	return result;
}

/**
 * Faster version of timer_clock(), to be called only when the timer
 * interrupt is disabled (DISABLE_INTS) or overridden by a
 * higher-priority or non-nesting interrupt.
 *
 * \sa timer_clock
 */
INLINE ticks_t timer_clock_unlocked(void)
{
	return _clock;
}

/** Convert \a ms [ms] to ticks. */
INLINE ticks_t ms_to_ticks(mtime_t ms)
{
#if TIMER_TICKS_PER_SEC < 1000
	/* Slow timer: avoid rounding down too much. */
	return (ms * TIMER_TICKS_PER_SEC) / 1000;
#else
	/* Fast timer: don't overflow ticks_t. */
	return ms * ((TIMER_TICKS_PER_SEC + 500) / 1000);
#endif
}

/** Convert \a us [us] to ticks. */
INLINE ticks_t us_to_ticks(utime_t us)
{
#if TIMER_TICKS_PER_SEC < 1000
	/* Slow timer: avoid rounding down too much. */
	return ((us / 1000) * TIMER_TICKS_PER_SEC) / 1000;
#else
	/* Fast timer: don't overflow ticks_t. */
	return (us * ((TIMER_TICKS_PER_SEC + 500) / 1000)) / 1000;
#endif
}

/** Convert \a ticks [ticks] to ms. */
INLINE mtime_t ticks_to_ms(ticks_t ticks)
{
#if TIMER_TICKS_PER_SEC < 1000
	/* Slow timer: avoid rounding down too much. */
	return (ticks * 1000) / TIMER_TICKS_PER_SEC;
#else
	/* Fast timer: avoid overflowing ticks_t. */
	return ticks / (TIMER_TICKS_PER_SEC / 1000);
#endif
}

/** Convert \a ticks [ticks] to us. */
INLINE utime_t ticks_to_us(ticks_t ticks)
{
#if TIMER_TICKS_PER_SEC < 1000
	/* Slow timer: avoid rounding down too much. */
	return ((ticks * 1000) / TIMER_TICKS_PER_SEC) * 1000;
#else
	/* Fast timer: avoid overflowing ticks_t. */
	return (ticks / (TIMER_TICKS_PER_SEC / 1000)) * 1000;
#endif
}

/** Convert \a us [us] to hpticks */
INLINE hptime_t us_to_hptime(utime_t us)
{
#if TIMER_HW_HPTICKS_PER_SEC > 10000000UL
	return us * ((TIMER_HW_HPTICKS_PER_SEC + 500000UL) / 1000000UL);
#else
	return (us * ((TIMER_HW_HPTICKS_PER_SEC + 500) / 1000UL) + 500) / 1000UL;
#endif
}

/** Convert \a hpticks [hptime] to usec */
INLINE utime_t hptime_to_us(hptime_t hpticks)
{
#if TIMER_HW_HPTICKS_PER_SEC < 100000UL
	return hpticks * ((1000000UL + TIMER_HW_HPTICKS_PER_SEC / 2) / TIMER_HW_HPTICKS_PER_SEC);
#else
	return (hpticks * 1000UL) / ((TIMER_HW_HPTICKS_PER_SEC + 500) / 1000UL);
#endif /* TIMER_HW_HPTICKS_PER_SEC < 100000UL */
}


void timer_init(void);
void timer_delayTicks(ticks_t delay);
INLINE void timer_delay(mtime_t delay)
{
	timer_delayTicks(ms_to_ticks(delay));
}

#if !defined(CONFIG_TIMER_DISABLE_UDELAY)
void timer_busyWait(hptime_t delay);
void timer_delayHp(hptime_t delay);
INLINE void timer_udelay(utime_t delay)
{
	timer_delayHp(us_to_hptime(delay));
}
#endif

#ifndef CONFIG_TIMER_DISABLE_EVENTS

#include <mware/event.h>

/**
 * The timer driver supports multiple synchronous timers
 * that can trigger an event when they expire.
 *
 * \sa timer_add()
 * \sa timer_abort()
 */
typedef struct Timer
{
	Node    link;     /**< Link into timers queue */
	ticks_t _delay;   /**< Timer delay in ms */
	ticks_t tick;     /**< Timer will expire at this tick */
	Event   expire;   /**< Event to execute when the timer expires */
	DB(uint16_t magic;)
} Timer;

/** Timer is active when Timer.magic contains this value (for debugging purposes). */
#define TIMER_MAGIC_ACTIVE    0xABBA
#define TIMER_MAGIC_INACTIVE  0xBAAB

extern void timer_add(Timer *timer);
extern Timer *timer_abort(Timer *timer);

/** Set the timer so that it calls an user hook when it expires */
INLINE void timer_set_event_softint(Timer *timer, Hook func, iptr_t user_data)
{
	event_initSoftInt(&timer->expire, func, user_data);
}

/** Set the timer delay (the time before the event will be triggered) */
INLINE void timer_setDelay(Timer *timer, ticks_t delay)
{
	timer->_delay = delay;
}

#endif /* CONFIG_TIMER_DISABLE_EVENTS */

#if defined(CONFIG_KERN_SIGNALS) && CONFIG_KERN_SIGNALS

/** Set the timer so that it sends a signal when it expires */
INLINE void timer_set_event_signal(Timer *timer, struct Process *proc, sigmask_t sigs)
{
	event_initSignal(&timer->expire, proc, sigs);
}

#endif /* CONFIG_KERN_SIGNALS */


#endif /* DRV_TIMER_H */
