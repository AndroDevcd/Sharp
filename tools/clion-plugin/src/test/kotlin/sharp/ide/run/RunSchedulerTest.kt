package sharp.ide.run

import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.util.concurrent.CountDownLatch
import java.util.concurrent.Executors
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger

/** Debounce, one run at a time, one queued run: the scheduler with a fake "run" and short delays. */
class RunSchedulerTest {
    private val executor = Executors.newScheduledThreadPool(2)
    private val delay = 60L

    @After fun stop() {
        executor.shutdownNow()
    }

    /** A fake run that records its start and finishes only when told. */
    private class FakeRuns {
        val started = AtomicInteger()
        val running = AtomicInteger()
        val mostAtOnce = AtomicInteger()
        var release = CountDownLatch(0)
        val firstStarted = CountDownLatch(1)

        fun start(finished: () -> Unit) {
            started.incrementAndGet()
            mostAtOnce.accumulateAndGet(running.incrementAndGet()) { a, b -> maxOf(a, b) }
            firstStarted.countDown()
            Thread {
                release.await(5, TimeUnit.SECONDS)
                running.decrementAndGet()
                finished()
            }.start()
        }
    }

    private fun waitUntil(what: String, condition: () -> Boolean) {
        val end = System.currentTimeMillis() + 5000
        while (!condition()) {
            if (System.currentTimeMillis() > end) throw AssertionError("timed out waiting for $what")
            Thread.sleep(5)
        }
    }

    @Test fun aBurstOfRequestsIsOneRun() {
        val runs = FakeRuns()
        val scheduler = RunScheduler(executor, { delay }, runs::start)
        repeat(10) { scheduler.request(); Thread.sleep(10) }                 // all within the debounce window
        waitUntil("the run") { runs.started.get() == 1 }
        waitUntil("idle") { scheduler.currentState() == RunScheduler.State.IDLE }
        Thread.sleep(delay * 3)
        assertEquals(1, runs.started.get())
    }

    @Test fun aRequestDuringARunQueuesExactlyOneMoreRunAfterIt() {
        val runs = FakeRuns()
        runs.release = CountDownLatch(1)
        val scheduler = RunScheduler(executor, { delay }, runs::start)
        scheduler.request()
        assertTrue(runs.firstStarted.await(5, TimeUnit.SECONDS))
        repeat(5) { scheduler.request() }                                      // five saves while it runs
        assertEquals(RunScheduler.State.RUNNING, scheduler.currentState())
        assertEquals(1, runs.started.get())                                    // the running one was not disturbed
        runs.release.countDown()
        waitUntil("the queued run") { runs.started.get() == 2 }
        waitUntil("idle") { scheduler.currentState() == RunScheduler.State.IDLE }
        Thread.sleep(delay * 3)
        assertEquals(2, runs.started.get())                                    // five requests made ONE more run
        assertEquals(1, runs.mostAtOnce.get())                                 // never two at once
    }

    @Test fun immediateSkipsTheDebounce() {
        val runs = FakeRuns()
        val scheduler = RunScheduler(executor, { 60_000 }, runs::start)
        scheduler.request(immediate = true)
        waitUntil("an immediate run") { runs.started.get() == 1 }
    }

    @Test fun aLongDelayHoldsTheRunBack() {
        val runs = FakeRuns()
        val scheduler = RunScheduler(executor, { 60_000 }, runs::start)
        scheduler.request()
        Thread.sleep(150)
        assertEquals(0, runs.started.get())
        assertEquals(RunScheduler.State.WAITING, scheduler.currentState())
        scheduler.dispose()
        assertEquals(RunScheduler.State.IDLE, scheduler.currentState())
    }

    @Test fun aStartThatThrowsDoesNotLeaveTheSchedulerStuck() {
        val calls = AtomicInteger()
        val scheduler = RunScheduler(executor, { 10 }) { finished ->
            if (calls.incrementAndGet() == 1) throw IllegalStateException("cannot start")
            finished()
        }
        scheduler.request()
        waitUntil("idle after the failed start") { calls.get() == 1 && scheduler.currentState() == RunScheduler.State.IDLE }
        scheduler.request()
        waitUntil("the second run") { calls.get() == 2 }
    }

    @Test fun afterDisposeNothingStarts() {
        val runs = FakeRuns()
        val scheduler = RunScheduler(executor, { 10 }, runs::start)
        scheduler.dispose()
        scheduler.request()
        Thread.sleep(100)
        assertEquals(0, runs.started.get())
    }
}
