# EInternalObjectFlags

A table of internal object flags that can be or'd together by using `|`

Field Name | Field Value Type
--------- | --------------
ReachableInCluster               | 0x00800000
ClusterRoot                      | 0x01000000
Native                           | 0x02000000
Async                            | 0x04000000
AsyncLoading                     | 0x08000000
Unreachable                      | 0x10000000
PendingKill                      | 0x20000000
RootSet                          | 0x40000000
GarbageCollectionKeepFlags       | 0x0E000000
AllFlags                         | 0x7F800000