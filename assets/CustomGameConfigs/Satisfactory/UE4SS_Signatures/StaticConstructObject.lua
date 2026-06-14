-- Satisfactory 1.0, UE 5.3
--return LoadExport("?StaticConstructObject_Internal@@YAPEAVUObject@@PEBVUClass@@PEAV1@VFName@@W4EObjectFlags@@W4EInternalObjectFlags@@1_NPEAUFObjectInstancingGraph@@5@Z")

-- Satisfactory 1.2, UE 5.6
-- class UObject * __ptr64 __cdecl StaticConstructObject_Internal(struct FStaticConstructObjectParameters const & __ptr64)
return LoadExport("?StaticConstructObject_Internal@@YAPEAVUObject@@AEBUFStaticConstructObjectParameters@@@Z")
