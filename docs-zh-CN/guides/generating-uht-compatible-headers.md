# 生成与 UHT 兼容的头文件

## 支持的版本

虽然 UHT 头文件生成器只在 `4.25+` 上获得官方支持，但它也能用于更老的游戏版本（已在 `4.18.3` 上测试；`4.17` 存在一些默认属性的问题，应该很快修复）。它也适用于 `5.0+`。

## 使用方法

生成头文件的快捷键默认为 `CTRL` + `Numpad 9`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。

要充分发挥生成头文件的作用，请参见 Archengius 的 [UE4GameProjectGenerator](https://github.com/Buckminsterfullerene02/UE4GameProjectGenerator)（这里链接到 Buck 的 fork，因为修复了 Arch 懒得合并的几个问题）。

> 该项目生成器只能为 `4.22` 及更高版本的 UE 编译。开发者对引擎所做的自定义可能导致意外结果。
> 如果要为低于 `4.22` 的引擎版本生成项目，请先用 `4.22` 或更高版本编译项目生成器，再进行生成。

在编译 projectgencommandlet 之前，请打开 `GameProjectGenerator.uproject`，以及你游戏的 pluginmanifest 或 `.uproject`，把游戏使用的所有默认引擎插件，以及你找到开源版本或已购买的插件（不建议把已购买的插件放进公开的 uproject）加入该 commandlet 的 uproject 文件。

编译完 commandlet 并对你的游戏文件运行之后，只需把生成的 `.uproject` 中的引擎版本改成与你游戏匹配的正确版本即可。

![image](https://user-images.githubusercontent.com/73571427/184372672-60d8579e-51c3-4d1e-9839-73af5d96ff21.png)

这个[由 Spuds 提供的 commandlet](https://cdn.discordapp.com/attachments/1005879578419531947/1075109443445985462/UE4ProjectGen-GUI.exe) 会替你输入项目生成的命令行命令，并生成一个批处理文件，以便用相同设置重新生成（例如游戏大更新后重新生成）。

## 可能出现的生成不准确问题

UE4SS 有两种不同的生成器：UHT 兼容生成器与所谓的 CXX 生成器。

使用 UE4GameProjectGenerator 创建 `.uproject` 文件时使用的是 UHT 兼容生成器；而 CXX 生成器是一个做得相当粗糙的生成器，它不生成 UHT 宏或正确的 `#include` 语句，但它会为 UHT 生成器不处理的 UE 核心类生成头文件。

> 注意，UE4SS 的 CXX 转储目前填充（padding）并不准确。如果另一来源生成的 SDK 转储具有正确的填充，那么它可能是确定下述修正的更好依据，尤其是位域检查。

在较老的引擎版本中，某些默认属性可能无法正确生成。例如在 <`4.17` 中，`SoftObjectProperty` 被称为 `AssetObjectProperty`，`SoftClassProperty` 被称为 `AssetClassProperty`。建议同时生成一份 SDK/CXX 转储，以检查并修正这些属性。

位域总是会被生成为 `uint8`。但它们原本可能被声明为 `uint32`。你可以根据 CXX/SDK 转储尝试确定实际大小并修正。在 CXX 转储中，位域会显示相同的偏移。如果同一偏移处有多个位域，而下个属性位于该偏移之后 4 字节处，那么该位域应改为 uint32。

## 处理可能遇到的错误

以下是生成项目的一般说明，同时涵盖了若干你很可能遇到的错误。

以下的错误与解决方案来自为多个游戏生成项目时的实际经验。

注意，即使你的游戏不在下表中，也可以在这里查找解决方案。错误列表由 Buckminsterfullerene、CheatingMuppet、Narknon 与 Blubb 汇总。

### 继承的虚函数

如果继承的虚函数未被反射，UE4SS 无法生成它们。这常常是 ```LNK2001: unresolved external symbol``` 错误的来源，尤其是当某个类继承自接口时。构建日志通常无法帮助确定需要把这些虚函数加到哪个文件。

要确定它们应被加到哪个文件，可以搜索错误中列出的虚函数，或在引擎中搜索该函数所属的类。例如，对于 ```Module.AkAudio.cpp.obj : error LNK2001: unresolved external symbol "public: virtual class FString const __cdecl UInterpTrack::GetEdHelperClassName(void)const```，你可以搜索 `GetEdHelperClassName` 或 `UInterpTrack`。找到父函数后，再找出你项目中继承同一虚函数的类。最好在引擎中找到一个继承这些虚函数的其他类的样例，以其为基础进行修复，并把实现复制到受影响的项目文件中，注意把类名改成你项目中的类名。

通常你还会希望删掉实现中的逻辑，只返回正确类型的数据或 "null"，而不实际执行任何逻辑。

### 游戏 Target 的生成

project gen commandlet 不会生成游戏 target 文件。复制一份你的 `GameNameEditor.target.cs` 到同一位置，从文件名中去掉 Editor。打开该文件，删除红叉标记处的 "Editor"，并把高亮处的 "Editor" 替换为 "Game"。

![image](https://user-images.githubusercontent.com/73571427/184372165-a365e6a9-4665-469d-a481-ef2b300240e7.png)

### Deep Rock Galactic

```
========================== 首先执行： ==========================
用 commandlet 生成项目
然后在 Rider/VS 中打开它。

========================== 然后按任意顺序执行： ==========================
查明 mod.io 游戏当前使用的版本。撰写本文时是 https://github.com/modio/modio-ue4/releases/tag/v2.16.1792。先删除现有的 'Modio' 文件夹。把其中的 'Modio'、'ModioTests' 与 'ThirdParty' 文件夹粘贴到 Plugins/Modio/Source，替换现有的 'Modio' 文件夹。不要替换 .uplugin 文件，而是从 .uplugin 文件中删除 ModioEx 段落。

在：
- CharacterSightSensor.h 的 FCharacterSightSensorDelegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterSightSensorDelegate);
- FSDProjectileMovementComponent.h 顶部的委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectilePenetrateDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnProjectileOutOfPropulsion);
在 UCLASS 上方添加宏 DECLARE_DYNAMIC_MULTICAST_DELEGATE(<委托名>);

在：
- SubHealthComponent.h 第 56 行
- HealthComponentBase.h 第 117 行
- HealthComponent.h 第 98 行
- EnemyHealthComponent.h 第 39 行
- FriendlyHealthComponent.h 第 33 行
注释掉 UFUNCTION

形如这样的错误："ActorFunctionLibrary.gen.cpp(153): [C2664] 'void UActorFunctionLibrary::DissolveMaterials(UObject *,const UMeshComponent *&,float)': cannot convert argument 2 from 'UMeshComponent *' to 'const UMeshComponent *&'"：
删除出错参数前的 const（记得在定义桩中也要删除）
或使用正则 (const) ((\w+)\*\&) 并替换为 $2

在 "ShowroomStage.cpp" 的构造函数实现中，注释掉 "this->SceneCapture = CreateDefaultSubobject<\USceneCaptureComponent2D>(TEXT("SceneCapture"));"

把支持的平台设为 windows
```

### cyubeVR

```
在生成的 "cyubeVR.uproject" 的 "Plugins" 段落中添加以下 4 行：
{
    "Name": "ChaosEditor",
    "Enabled": false
}

复制并粘贴 cyubeVREditor.Target.cs 文件（位于 Source 文件夹中），命名为 cyubeVRGame.Target.cs。然后把该新文件中所有出现 "editor" 的地方替换为 "game"

右键点击生成的项目并用 IDE（例如 Rider）打开

在 ReceiveLightActor.h 中注释掉 UFUNCTION()
    - UseActorCustomLocation
    - GetActorCustomLocation

把 "_MAX UMETA(Hidden)," 改为 "_MAX = 0xFF UMETA(Hidden),"：
    - EUGCMatchingUGCTypeBP.h
    - EItemPreviewTypeBP.h

删除 IpNetDriverUWorks.h 与 cpp 文件中的构造函数。

删除 TEnumAsByte<>（但保留其中的类型）：
    - VRGripInterface.h 中的 OnInput
    - VRGripScriptBase.h 中的 OnEndPlay 及其 .cpp 文件中的 _Implementation 版本
    - DeerCPP.h 中的 SetMobilityAllEvent 及其 .cpp 文件中的 _Implementation 版本

然后右键点击 .uproject 并选择 "regenerate solution files"。

如果在构建或打包时遇到 "failed to create version memory for PCH" 错误，再重做一次即可。
```

### 游戏 3

```
错误 1
在一个枚举类中：
System.ArgumentException - String cannot contain a minus sign if the base is not 10.

修复：
删除该枚举上的 BlueprintType 元标记以及 uint8 覆盖 ': uint8'。


错误 2
Unable to find 'class', 'delegate', 'enum', or 'struct' with name 'XYZ'，其中 XYZ 是某个类内使用但没有单独 UStruct 声明的 FStruct。

修复：
在头文件顶部附近添加 DECLARE_DYNAMIC_MULTICAST_DELEGATE(XYZ);


错误 3
"is not supported by blueprint."

修复：
-> 删除 BlueprintReadWrite
-> 或删除 BlueprintCallable


错误 4
cannot instantiate abstract class

修复：

cpp 中是这样的：

 UAbilitySystemComponent* AActorWithGAS::GetAbilitySystemComponent() const {
    return nullptr;
 }

 转到头文件并添加：

 UAbilitySystemComponent* GetAbilitySystemComponent() const override;


错误 5
modifiers not allowed on static member functions

修复：
删除修饰符，例如 "const"

示例：
static TSoftObjectPtr<Test> SomeFunction(some args) const;  <- 删除 const

在 h 与 cpp 文件中都要修改。


错误 6
'AAkAMbientSound' no appropriate default consturctor available.

修复：
-------
头文件
-------
AkAmbientSound();

->

AkAmbientSound(const class FObjectInitializer& ObjectInitializer);

-------
CPP 文件
-------
AkAmbientSound::AkAmbientSound() {
    this->AkEvent = NULL;
}

->

AkAmbientSound::AkAmbientSound(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)  {
    this->AkEvent = NULL;
}
```

### Astro Colony

```
========================== 首先执行： ==========================
用 commandlet 生成项目
然后在 Rider/VS 中打开它。

========================== 然后按任意顺序执行： ==========================
复制 EditorTarget 文件，重命名为 AstroColonyGame.Target，并在其中把 target 类型改为 Game

在：
- VoxelPhysicsPartSpawner_VoxelWorlds.h 的 FConfigureVoxelWorld;
- TGNamedSlot.h 的 FOnNamedSlotAdded/Removed
- EHLogicObject.h 的 FOnSelectedResourcesChanged
- EHSignalObject.h 的 FOnResourcesSignalOutChanged/FOnSelectedDeviceChanged
- EHInteractableServiceObject 的 FOnAIInsideChanged
- EHModsBrowsedOptionViewModel 的 FOnInstalProgressChanged/FOnInstalCompleted
- EHSaveLoadListViewModel 的 FOnScenarioDetailsUpdated
- EHTrainingObject 的 FOnTrainedChanged
- EHSchoolObject 的 FOnAwaitingSpecialistTrainingsChange
- EHSignalReceiver 的 FOnSignalSendChanged
- EHModsListViewModel 的 FOnModsOptionSelected
- EHSignalNetwork 的 FOnSignalChanged
- AbilityAsync_WaitGameplayTagAdded 的 FAsyncWaitGameplayTagDelegate（把它放到 AbilityAsync_WaitGameplayTag 内部）
在 UCLASS 上方添加宏 DECLARE_DYNAMIC_MULTICAST_DELEGATE(<委托名>);

在：
- AbilityAsync_WaitGameplayTagRemoved.h
- AbilityAsync_WaitGameplayTagAdded.h
从每个成员前面删除 UAbilityAsync_WaitGameplayTag::

在 EHSummaryViewModel.h 中添加 #include "EHSaveLoadListViewModel.h"

在：
- MaterialExpressionBlendMaterialAttributesBarycentric.h（每个属性）
- MaterialExpressionUnpack.h（FExpressionInput Input）
- GameplayCueInterface.h（ForwardGameplayCueToParent）
从 'UPROPERTY' 宏中删除 BlueprintReadWrite/BlueprintCallable（按需）。

在 MaterialPackInput.h 中，添加 #include "MaterialExpressionIO.h"，并从 FExpressionInput Input 的 'UPROPERTY' 宏中删除 BlueprintReadWrite 标记；

在 EAbilityTaskWaitState.h 中，给枚举添加 None = 0

在：
- AbilityTask.h/.cpp
- UMovieSceneGameplayCueTriggerSection
- UMovieSceneGameplayCueSection
注释掉构造函数/定义

在 AbilitySystemComponent.h/.cpp 中注释掉：
- 构造函数
- ServerSetReplicatedEventWithPayload
- ServerSetReplicatedEvent
- ClientSetReplicatedEvent

在 EHBaseButtonWidget.h 中添加：
#include "Components/HorizontalBox.h"
#include "Components/BackgroundBlur.h"
#include "Components/SizeBox.h"
然后删除 UHorizontalBox、UBackgroundBlur、USizeBox 的前向声明。
然后注释掉
	UFUNCTION(BlueprintImplementableEvent)
    void OnInputControllerChanged(TEnumAsByte<ETGInputControllerType> InputControllerType);

在：
- EHPlanetoidDestructibleItem.h
- EHPlanetoidVisualItem.h（同时从 SpawnDensity 中删除数组）
- EHGridComponent.h 的 BillboardTextures
- EHHUDGame.h 的 PopMenuClasses/HUDMenuClasses（同时修改 GetPopMenuClass 的返回类型）
- EHScenarioParams.h 的 TerrainTypeSpawnChances/ShapeTypeSpawnChances
- EHDataProvider.h 中的每个数组
把数组声明替换为 TArray<>，并在 'UPROPERTY' 宏中添加 BlueprintReadWrite 及其他常规标记。然后更新 .cpp 构造函数。

在 VoxelProceduralMeshComponent.h/.cpp 中，添加 UPrimitiveComponent 接口，例如像这样：
VoxelProceduralMeshComponent.h:
#pragma once
#include "CoreMinimal.h"
#include "Components/ModelComponent.h"
#include "VoxelIntBox.h"
#include "VoxelProceduralMeshComponent.generated.h"

class UBodySetup;
class UStaticMeshComponent;
class AVoxelWorld;
class UModelComponent;

UCLASS(Blueprintable, ClassGroup=Custom, meta=(BlueprintSpawnableComponent))
class VOXEL_API UVoxelProceduralMeshComponent : public UModelComponent {
    GENERATED_BODY()
public:
private:
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, meta=(AllowPrivateAccess=true))
    UBodySetup* BodySetup;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Transient, meta=(AllowPrivateAccess=true))
    UBodySetup* BodySetupBeingCooked;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Export, Transient, meta=(AllowPrivateAccess=true))
    UStaticMeshComponent* StaticMeshComponent;

public:
    UVoxelProceduralMeshComponent(const FObjectInitializer& ObjectInitializer);
    UFUNCTION(BlueprintCallable)
    static void SetVoxelCollisionsFrozen(const AVoxelWorld* VoxelWorld, bool bFrozen);

    UFUNCTION(BlueprintImplementableEvent)
    void InitChunk(uint8 ChunkLOD, FVoxelIntBox ChunkBounds);

    UFUNCTION(BlueprintCallable, BlueprintPure)
    static bool AreVoxelCollisionsFrozen(const AVoxelWorld* VoxelWorld);

//~ Begin UPrimitiveComponent Interface.
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual bool GetLightMapResolution( int32& Width, int32& Height ) const override;
	virtual int32 GetStaticLightMapResolution() const override;
	virtual void GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override;
#if WITH_EDITOR
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options) override;
	virtual void AddMapBuildDataGUIDs(TSet<FGuid>& InGUIDs) const override;
#endif
	virtual ELightMapInteractionType GetStaticLightingType() const override	{ return LMIT_Texture;	}
	virtual void GetStreamingRenderAssetInfo(FStreamingTextureLevelContext& LevelContext, TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingRenderAssets) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual class UBodySetup* GetBodySetup() override { return ModelBodySetup; };
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	virtual bool IsPrecomputedLightingValid() const override;
	//~ End UPrimitiveComponent Interface.

	//~ Begin UActorComponent Interface.
	virtual void InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly) override;
	virtual void PropagateLightingScenarioChange() override;
	//~ End UActorComponent Interface.

	//~ Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual bool IsNameStableForNetworking() const override;
#if WITH_EDITOR
	virtual void PostEditUndo() override;
#endif // WITH_EDITOR
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~ End UObject Interface.

	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface

//#if WITH_EDITOR
	/**
	 *	Generate the Elements array.
	 *
	 *	@param	bBuildRenderData	If true, build render data after generating the elements.
	 *
	 *	@return	bool				true if successful, false if not.
	 */
	virtual bool GenerateElements(bool bBuildRenderData);
//#endif // WITH_EDITOR
};

VoxelProceduralMeshComponent.cpp:
#include "VoxelProceduralMeshComponent.h"

class AVoxelWorld;

void UVoxelProceduralMeshComponent::SetVoxelCollisionsFrozen(const AVoxelWorld* VoxelWorld, bool bFrozen) {

}

bool UVoxelProceduralMeshComponent::AreVoxelCollisionsFrozen(const AVoxelWorld* VoxelWorld) {
    return false;
}

UVoxelProceduralMeshComponent::UVoxelProceduralMeshComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    this->BodySetup = NULL;
    this->BodySetupBeingCooked = NULL;
    this->StaticMeshComponent = NULL;
}

void UVoxelProceduralMeshComponent::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	/*UVoxelProceduralMeshComponent* This = CastChecked<UVoxelProceduralMeshComponent>(InThis);
	Collector.AddReferencedObject( This->StaticMeshComponent, This );
	AddReferencedObjects( This, Collector );*/
}

void UVoxelProceduralMeshComponent::Serialize(FArchive& Ar)
{
	/*Serialize(Ar);

	Ar << StaticMeshComponent;*/
}

void UVoxelProceduralMeshComponent::PostLoad()
{
	/*PostLoad();

	// Fix for old StaticMeshComponent components which weren't created with transactional flag.
	SetFlags( RF_Transactional );

	// BuildRenderData relies on the StaticMeshComponent having been post-loaded, so we ensure this by calling ConditionalPostLoad.
	check(StaticMeshComponent);
	StaticMeshComponent->ConditionalPostLoad();*/

}

bool UVoxelProceduralMeshComponent::IsNameStableForNetworking() const
{
	// UVoxelProceduralMeshComponent is always persistent for the duration of a game session, and so can be considered to have a stable name
	return true;
}

void UVoxelProceduralMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{

}

int32 UVoxelProceduralMeshComponent::GetNumMaterials() const
{
	return 0;
}

UMaterialInterface* UVoxelProceduralMeshComponent::GetMaterial(int32 MaterialIndex) const
{
	UMaterialInterface* Material = nullptr;

	return Material;
}

UMaterialInterface* UVoxelProceduralMeshComponent::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	UMaterialInterface* Result = nullptr;
	SectionIndex = 0;
	return Result;
}

bool UVoxelProceduralMeshComponent::IsPrecomputedLightingValid() const
{
	return false;
}

void UVoxelProceduralMeshComponent::GetStreamingRenderAssetInfo(FStreamingTextureLevelContext& LevelContext, TArray<FStreamingRenderAssetPrimitiveInfo>& OutStreamingRenderAssets) const
{

}

void UVoxelProceduralMeshComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{

}

void UVoxelProceduralMeshComponent::DestroyRenderState_Concurrent()
{

}

FPrimitiveSceneProxy* UVoxelProceduralMeshComponent::CreateSceneProxy()
{
	return NULL;
}

bool UVoxelProceduralMeshComponent::ShouldRecreateProxyOnUpdateTransform() const
{
	return true;
}

FBoxSphereBounds UVoxelProceduralMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
}

void UVoxelProceduralMeshComponent::InvalidateLightingCacheDetailed(bool bInvalidateBuildEnqueuedLighting, bool bTranslationOnly)
{

}

void UVoxelProceduralMeshComponent::PropagateLightingScenarioChange()
{

}

bool UVoxelProceduralMeshComponent::GetLightMapResolution( int32& Width, int32& Height ) const
{
	return false;
}

int32 UVoxelProceduralMeshComponent::GetStaticLightMapResolution() const
{
	/*int32 Width;
	int32 Height;
	GetLightMapResolution(Width, Height);

	return FMath::Max<int32>(Width, Height);*/
	return NULL;
}

void UVoxelProceduralMeshComponent::GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const
{
	/*return;*/
}

#if WITH_EDITOR
void UVoxelProceduralMeshComponent::GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options)
{
	/*check(0);*/
}

void UVoxelProceduralMeshComponent::AddMapBuildDataGUIDs(TSet<FGuid>& InGUIDs) const
{

}

void UVoxelProceduralMeshComponent::PostEditUndo()
{
	/*PostEditUndo();*/
}
#endif // WITH_EDITOR

bool UVoxelProceduralMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	return false;
}

bool UVoxelProceduralMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return false;
}

bool UVoxelProceduralMeshComponent::GenerateElements(bool bBuildRenderData)
{
	return false;
}

把支持的平台设为 windows
```
