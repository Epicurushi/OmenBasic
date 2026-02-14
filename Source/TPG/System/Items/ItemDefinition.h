#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

class UItemFragment;
class UTexture2D;

/**
 * 物品类别
 */
UENUM(BlueprintType)
enum class EItemCategory : uint8
{
	Weapon		UMETA(DisplayName = "Weapon"),
	SpellBook	UMETA(DisplayName = "Spell Book"),
	Potion		UMETA(DisplayName = "Potion"),
	Utility		UMETA(DisplayName = "Utility"),
};

/**
 * ItemDefinition：物品静态配置
 * - 背包 UI：DisplayName/Icon/Description
 * - 网格背包：GridWidth/GridHeight/bCanRotate
 * - 堆叠：bStackable/MaxStack
 * - 扩展能力：Fragments
 */
UCLASS(BlueprintType)
class TPG_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ---------- UI / 基础信息 ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Basic")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Basic", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|UI")
	TObjectPtr<UTexture2D> Icon = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Basic")
	EItemCategory Category = EItemCategory::Utility;

	// ---------- 网格背包 ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridWidth = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid", meta = (ClampMin = "1"))
	int32 GridHeight = 1;

	// 是否允许旋转（交换宽高）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Grid")
	bool bCanRotate = true;

	// ---------- 堆叠 ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stack")
	bool bStackable = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item|Stack", meta = (ClampMin = "1", EditCondition = "bStackable"))
	int32 MaxStack = 1;

	// ---------- Fragment ----------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Item|Fragments")
	TArray<TObjectPtr<UItemFragment>> Fragments;
};