#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "GridInventoryComponent.generated.h"

class UItemInstance;
class UItemDefinition;

/**
 * 放置记录
 * - UI 不直接信任 Instance 的位置/旋转（因为 UI 只是表现层），以 Records 为准。
 */
USTRUCT()
struct FPlacedItemRecord
{
	GENERATED_BODY()

	// 物品实例（持有 Definition、StackCount、bRotated 等运行时数据）
	UPROPERTY()
	TObjectPtr<UItemInstance> Instance = nullptr;

	// 占格左上角坐标（TopLeft）
	UPROPERTY()
	FIntPoint Pos = FIntPoint::ZeroValue;

	// 是否旋转（以背包记录为准，写回 Instance->bRotated 只是为了方便 UI/调试读取）
	UPROPERTY()
	bool bRotated = false;
};

/**
 * 给 UI 的读取结构
 */
USTRUCT(BlueprintType)
struct FGridItemRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 ItemId = -1;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	TObjectPtr<UItemInstance> Instance = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint Pos = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bRotated = false;

	// 当前占格宽（考虑旋转后的结果）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 GridW = 0;

	// 当前占格高（考虑旋转后的结果）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 GridH = 0;
};

/**
 * 背包变化类型
 * - UI 通过 Type 决定只更新哪些 ItemWidget
 */
UENUM(BlueprintType)
enum class EGridInvChangeType : uint8
{
	// 全量重建（初始化/彻底清空等）
	Reset        UMETA(DisplayName = "Reset"),
	// 新增一个 ItemId
	Added        UMETA(DisplayName = "Added"),
	// 删除一个 ItemId
	Removed      UMETA(DisplayName = "Removed"),
	// ItemId 位置变化（包含旋转变化）
	Moved        UMETA(DisplayName = "Moved"),
	// ItemId 原地旋转（位置不变）
	Rotated      UMETA(DisplayName = "Rotated"),
	// ItemId 堆叠数量变化
	StackChanged UMETA(DisplayName = "StackChanged")
};

/**
 * UI 只根据这个事件做“最小更新”
 * - 包含变化类型 / ItemId / 新旧位置旋转 / 新旧堆叠数量
 */
USTRUCT(BlueprintType)
struct FGridInvChangeEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	EGridInvChangeType Type = EGridInvChangeType::Reset;

	// 受影响的物品 ID（Reset 时通常为 -1）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 ItemId = -1;

	// 旧位置（Removed/Moved/Rotated 会用到）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint OldPos = FIntPoint::ZeroValue;

	// 新位置（Added/Moved/Rotated 会用到）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FIntPoint NewPos = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bOldRotated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	bool bNewRotated = false;

	// 堆叠旧数量（StackChanged 使用）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 OldStack = 0;

	// 堆叠新数量（StackChanged 使用）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 NewStack = 0;
};

// 蓝图可绑定：背包发生变化 -> UI 只更新受影响部分
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGridInventoryChanged, const FGridInvChangeEvent&, Event);

/**
 * 纯数据网格背包组件
 * - 支持：矩形占格、旋转、堆叠
 * - 支持：增量事件广播（UI 只更新变化的 ItemWidget）
 */
UCLASS(ClassGroup = (TPG), meta = (BlueprintSpawnableComponent))
class TPG_API UGridInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// 背包网格宽度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Width = 10;

	// 背包网格高度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	int32 Height = 6;

public:
	/**
	 * 背包增量更新事件
	 * UI 层只需要绑定这个事件，然后按 Type 更新对应 ItemWidget 即可
	 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnGridInventoryChanged OnInventoryChanged;

public:
	// ----------------- Manual Test / UI Helpers -----------------
	/**
	 * 创建一个 ItemInstance（方便你在蓝图/编辑器里手动测试背包）
	 * @param Definition 物品定义（大小、是否可旋转、是否可堆叠等）
	 * @param StackCount 初始堆叠数量
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Test")
	UItemInstance* CreateItemInstance(const UItemDefinition* Definition, int32 StackCount = 1);

public:
	// ----------------- Inventory API -----------------

	/**
	 * 初始化背包尺寸，并清空数据
	 * - 会广播 Reset 事件（UI 应全量重建）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void Initialize(int32 InWidth, int32 InHeight);

	/**
	 * UI 查询：某格子被哪个 ItemId 占用
	 * @return -1 表示空
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetItemIdAtCell(int32 X, int32 Y) const;

	/**
	 * UI 查询：读取某个 ItemId 的信息（位置/旋转/占格尺寸/Instance）
	 * @return true 表示存在并成功输出
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool GetItemRecord(int32 ItemId, FGridItemRecord& OutRecord) const;

	/**
	 * UI 查询：获取当前背包内所有 ItemId
	 * - UI 可以用来在 Reset 后重建全部物品
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	TArray<int32> GetAllItemIds() const;

	/**
	 * 在指定位置放置物品（不会自动找位置）
	 * - 成功会分配新的 ItemId，并广播 Added 事件
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool PlaceAt(UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated, int32& OutItemId);

	/**
	 * 移动物品到新位置（可同时改变旋转）
	 * - 成功广播 Moved 事件（带旧/新位置、旧/新旋转）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool MoveItem(int32 ItemId, const FIntPoint& NewTopLeft, bool bNewRotated);

	/**
	 * 删除物品
	 * - 成功广播 Removed 事件
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(int32 ItemId);

	/**
	 * 尝试原地旋转某个物品
	 * - 放不下会回滚
	 * - 成功广播 Rotated 事件
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RotateItem(int32 ItemId);

	/**
	 * 添加物品（自动逻辑）
	 * 1) 先尝试与已有堆叠合并（会广播 StackChanged）
	 * 2) 若还有剩余，再自动找空位放下（first-fit）（会广播 Added）
	 * @return 是否成功（若放不下且无法合并则 false）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TryAddItem(UItemInstance* NewInstance, int32& OutItemId, FIntPoint& OutPlacedPos);

	/**
	 * 查询某个格子是否为空
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool IsCellEmpty(int32 X, int32 Y) const;

private:
	// ----------------- Internal Data -----------------

	/**
	 * Cells：每格存 ItemId，-1 表示空
	 * - 用来快速做占用判断和 UI hover 查询
	 */
	UPROPERTY()
	TArray<int32> Cells;

	/**
	 * Records：ItemId -> 放置记录（Instance + Pos + Rotated）
	 * - 这是背包放置的权威数据
	 */
	UPROPERTY()
	TMap<int32, FPlacedItemRecord> Records;

	// 自增生成唯一 ItemId
	int32 NextItemId = 1;

private:
	// ----------------- Helpers -----------------

	// (x,y) -> Cells 下标
	int32 Index(int32 X, int32 Y) const { return Y * Width + X; }

	// 检查坐标是否在背包范围内
	bool InBounds(int32 X, int32 Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }

	/**
	 * 检查某实例能否以 TopLeft + bRotated 的形态放置
	 * @param IgnoreItemId 用于 Move/Rotate 时忽略自己占用的格子（避免误判碰撞）
	 */
	bool CanPlaceInternal(UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated, int32 IgnoreItemId = -1) const;

	/**
	 * 把某个 ItemId 的占用写入 Cells
	 * - 只在“确定放置成功”后调用
	 */
	void FillCells(int32 ItemId, UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated);

	/**
	 * 把某个 ItemId 的占用从 Cells 清除
	 * - Move/Rotate 时先清旧占用，再尝试放新位置；失败则回滚
	 */
	void ClearCells(int32 ItemId, UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated);

	/**
	 * 自动找空位（first-fit）
	 * - 默认先不旋转尝试，再尝试旋转（如果物品允许旋转）
	 */
	bool FindFirstFit(UItemInstance* Instance, bool bTryRotate, FIntPoint& OutPos, bool& OutRotated) const;

	/**
	 * 事件广播的统一入口
	 * - 以后你要加日志、统计、或过滤规则，只改这里
	 */
	void BroadcastChange(const FGridInvChangeEvent& E);
};
