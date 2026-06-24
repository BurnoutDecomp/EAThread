// ===========================================================================
// eathread_x360align.cpp — X360-ALIGNMENT additions to the forked vendor EAThread.
//
// The committed vendor EAThread is the single EAThread linked into the PC binary
// (the reconstructed X360 EA::Thread set is mutually exclusive with it — both define
// EA::Thread::Thread / Semaphore — so it stays REFERENCE-only). Where the
// reconstructed CgsFileSystem device layer issues an X360-shaped EAThread call, the
// matching overload is added here so it links against this forked vendor EAThread.
// See the eathread-strategy memory.
//
// This TU is compiled UNCONDITIONALLY (it does not sit inside the vendor's
// platform #if/#else selection), so the symbols are always emitted regardless of
// which ThreadSleep(ThreadTime) implementation (native vs cpp11) the vendor selects.
// ===========================================================================

#include <eathread/eathread.h>            // EA::Thread::ThreadSleep(const ThreadTime&), ThreadTime
#include <eathread/eathread_semaphore.h>  // EA::Thread::Semaphore (forked-vendor Wait overload)

#if defined(_WIN32)
	#include <Windows.h>          // SleepEx / SwitchToThread
#endif

namespace EA
{
namespace Thread
{
	// X360 ABI: EA::Thread::ThreadSleep(const u32* pMilliseconds) @0x82B42610.
	// SleepEx(ms, TRUE) (alertable) for ms>0, else SwitchToThread() (yield). Additive
	// overload — distinct mangling from vendor's ThreadSleep(const ThreadTime&). Used by
	// CgsFileSystem::Device::CallErrorCallback (its faithful 5ms error-path yield).
	unsigned int ThreadSleep(const unsigned int* pMilliseconds)
	{
		const unsigned int luMilliseconds = *pMilliseconds;
	#if defined(_WIN32)
		if (luMilliseconds)
			return static_cast<unsigned int>(SleepEx(luMilliseconds, TRUE));
		return static_cast<unsigned int>(SwitchToThread());
	#else
		// Portable fallback: delegate to the vendor relative-time sleep.
		ThreadSleep(static_cast<ThreadTime>(luMilliseconds));
		return 0;
	#endif
	}

	// X360 ABI: EA::Thread::Semaphore::Wait(const u32* pTimeoutAbsolute) @0x82B42A40.
	// *p is an absolute deadline: 0xFFFFFFFF (-1) == INFINITE (wait forever), 0 == poll.
	// Mapped onto the forked vendor Semaphore: INFINITE -> Wait() (kTimeoutNone, wait
	// forever) which is the only value CgsFileSystem::OperationPool::ReadOperation passes;
	// a finite deadline is forwarded as a vendor absolute ThreadTime (NOTE: the X360 build
	// applied a 10s relative reference-window here — not reproduced, as the only live caller
	// waits forever). Returns the vendor Wait result (>=0 count, or kResultTimeout/Error).
	int Semaphore::Wait(const unsigned int* pTimeoutAbsolute)
	{
		const unsigned int luTimeoutAbsolute = *pTimeoutAbsolute;
		if (luTimeoutAbsolute == 0xFFFFFFFFu)
			return Wait();   // kTimeoutNone: wait forever
		return Wait(static_cast<ThreadTime>(luTimeoutAbsolute));
	}
}
}
