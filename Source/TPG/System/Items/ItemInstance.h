#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

class UItemDefinition;

/**
 * ItemInstance：
 * - 指向 ItemDefinition
 * - 保存数量、旋转等运行时状态
 *
 * 约定：
 * - 正常创建时 StackCount >= 1
 * - 堆叠合并/转移后允许 StackCount == 0，表示该实例被丢弃
 */
UCLASS(BlueprintType)
class TPG_API UItemInstance : public UObject
{
	GENERATED_BODY()

public:
	// 物品模板（静态配置）
	UPROPERTY(BlueprintReadOnly, Category = "Item")
	TObjectPtr<const UItemDefinition> Definition = nullptr;

	// 当前堆叠数量
	// - 正常创建时 >= 1
	// - 合并后允许变成 0（表示空实例）
	UPROPERTY(BlueprintReadOnly, Category = "Item|Stack")
	int32 StackCount = 1;

	// 是否旋转（决定占格是 W×H 还是 H×W）
	UPROPERTY(BlueprintReadOnly, Category = "Item|Grid")
	bool bRotated = false;

public:
	// 初始化实例（保证创建时 StackCount >= 1）
	void Init(const UItemDefinition* InDef, int32 InStackCount = 1)
	{
		Definition = InDef;
		StackCount = FMath::Max(1, InStackCount);
		bRotated = false;
	}

	// 获取当前占格宽度（考虑旋转）
	int32 GetGridWidth() const;

	// 获取当前占格高度（考虑旋转）
	int32 GetGridHeight() const;

	// 是否可堆叠
	bool IsStackable() const;

	// 最大堆叠数
	int32 GetMaxStack() const;

	// 是否为空（被合并吃完）
	UFUNCTION(BlueprintPure, Category = "Item|Stack")
	bool IsEmpty() const { return IsStackable() && StackCount <= 0; }

	// 尝试合并堆叠：把 Other 的数量合并到自己，返回实际合并了多少
	// 约定：Other->StackCount 可能被减到 0（但不会为负数）
	int32 TryMergeFrom(UItemInstance* Other);
};