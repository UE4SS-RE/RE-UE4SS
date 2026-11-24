# GetAsyncThreadId

This deprecated function returns a [ThreadId](../classes/threadid.md) representing an empty C++ thread ID. UE4SS no longer starts a worker thread for each Lua mod. Use [`GetCurrentThreadId`](./getcurrentthreadid.md) to identify the calling thread.
