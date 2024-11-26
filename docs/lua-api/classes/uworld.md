# UWorld
## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### SpawnActor(UClass ActorClass, FVector Location, FRotator Rotation)
This function uses `UGameplayStatics:BeginDeferredActorSpawnFromClass` and `UGameplayStatics:FinishSpawningActor` to spawn an actor.  
- **Return type:** `AActor`
- **Returns:** Spawned actor object or an invalid object.
