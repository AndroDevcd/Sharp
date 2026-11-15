/*
 * RunScheduler - decides WHEN a run starts: debounce, one at a time, one queued.
 *
 * WHAT THIS IS FOR
 *   Saves arrive in bursts (auto-save, Save All, a refactoring touching ten files) and a
 *   compiler run takes seconds. The rules (doc/ide.md section 3.1):
 *
 *     - DEBOUNCE: after a request, wait `delay` with no further request before starting,
 *       so a burst of saves is one run.
 *     - ONE AT A TIME: while a run is going, nothing else starts.
 *     - ONE QUEUED: a request that arrives during a run is remembered (a flag, not a
 *       list: ten requests are one) and a new run follows when the current one ends.
 *       The running compiler is NOT killed: a run takes seconds, continuous auto-save
 *       would starve it, and its result is still right for every file that did not change.
 *
 *   The three states and the moves between them:
 *
 *        IDLE --request--> WAITING --delay passes--> RUNNING --finished--> IDLE
 *                           ^  |request: wait again     |request: remember "again"
 *                           +--+                         +--> finished --> WAITING
 *
 *   The scheduler knows nothing of compilers or editors: `start` is a function that
 *   begins a run in the background and calls `finished` when it is over. That keeps it
 *   testable with a fake `start` and a short delay. All state is under one lock; the
 *   lock is never held while `start` runs.
 */
package sharp.ide.run

import java.util.concurrent.ScheduledExecutorService
import java.util.concurrent.ScheduledFuture
import java.util.concurrent.TimeUnit

class RunScheduler(
    private val executor: ScheduledExecutorService,
    private val delayMillis: () -> Long,
    /** Begins a run in the background; must call the given function exactly once when the run is over. */
    private val start: (finished: () -> Unit) -> Unit,
) {
    enum class State { IDLE, WAITING, RUNNING }

    private val lock = Any()
    private var state = State.IDLE
    private var again = false
    private var pending: ScheduledFuture<*>? = null
    private var disposed = false

    fun currentState(): State = synchronized(lock) { state }

    /** A run is wanted. `immediate` skips the debounce (the "Run Sharp check now" action). */
    fun request(immediate: Boolean = false) {
        synchronized(lock) {
            if (disposed) return
            when (state) {
                State.RUNNING -> again = true
                State.IDLE, State.WAITING -> arm(if (immediate) 0 else delayMillis())
            }
        }
    }

    /** Cancels a waiting run and stops accepting requests. A run already going is left to its own cancellation. */
    fun dispose() {
        synchronized(lock) {
            disposed = true
            pending?.cancel(false)
            pending = null
            if (state == State.WAITING) state = State.IDLE
        }
    }

    /** (Re)starts the countdown. Called with the lock held. */
    private fun arm(delay: Long) {
        pending?.cancel(false)
        state = State.WAITING
        pending = executor.schedule({ begin() }, delay, TimeUnit.MILLISECONDS)
    }

    /** The countdown ended: move to RUNNING and start. */
    private fun begin() {
        synchronized(lock) {
            if (disposed || state != State.WAITING) return
            state = State.RUNNING
            again = false
            pending = null
        }
        try {
            start { finish() }
        } catch (e: Throwable) {
            finish()                                          // a start that throws must not leave the scheduler stuck
            throw e
        }
    }

    private fun finish() {
        synchronized(lock) {
            if (state != State.RUNNING) return                 // called twice: ignore the second
            state = State.IDLE
            if (again && !disposed) {
                again = false
                arm(delayMillis())
            }
        }
    }
}
