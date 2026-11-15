# FWeakObjectPtr

## 继承
[LocalObject](./localobject.md)

## 方法

### Get() / get()

- **返回类型：** `UObjectDerivative`
- **返回：** 所指向的 `UObject` 或 `UObject` 派生对象。

> 返回值可能无效，因此在调用该函数后请调用 `UObject:IsValid` 进行校验。

## 属性赋值

UObject 实例上已反射的 `FWeakObjectProperty` 字段可以直接从 Lua 赋值：

- 赋一个 `UObject`：把弱指针设为引用目标对象。
- 赋一个 `FWeakObjectPtr`：复制该弱指针引用。
- 赋 `nil`：清除该弱指针引用。

```lua
-- 把一个 actor 赋给弱对象属性
TargetActor.WeakTarget = CurrentEnemy

-- 清除弱对象属性
TargetActor.WeakTarget = nil
```
