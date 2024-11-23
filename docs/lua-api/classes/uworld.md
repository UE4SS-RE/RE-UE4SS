# UWorld
## Inheritance
[RemoteObject](./remoteobject.md)

## Methods

### SpawnActor(UClass ActorClass, FVector Location, FRotator Rotation)
It's a self-implemented function that is automatically added to every UWorld object by UE4SS.  
The function uses `UGameplayStatics:BeginDeferredActorSpawnFromClass` and `UGameplayStatics:FinishSpawningActor` to spawn an actor.  
- **Return type:** `AActor`
- **Returns:** Spawned actor object or an invalid object.
