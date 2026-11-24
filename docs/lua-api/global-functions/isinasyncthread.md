# IsInAsyncThread

This deprecated function always returns `false`. UE4SS no longer starts a worker thread for each Lua mod. Use [`IsInGameThread`](./isingamethread.md) to check whether the current callback is running on the game thread.
