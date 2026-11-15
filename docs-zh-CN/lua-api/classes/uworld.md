# UWorld

## 继承
[RemoteObject](./remoteobject.md)

## 方法

### SpawnActor(UClass ActorClass, FVector Location, FRotator Rotation)

该函数使用 `UGameplayStatics:BeginDeferredActorSpawnFromClass` 与 `UGameplayStatics:FinishSpawningActor` 来生成 actor。

- **返回类型：** `AActor`
- **返回：** 生成出来的 actor 对象，或一个无效对象。
