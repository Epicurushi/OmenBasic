#include "System/Inventory/GridInventoryComponent.h"
#include "System/Items/ItemInstance.h"
#include "System/Items/ItemDefinition.h"

// Public
// ----------------- Manual Test / UI Helpers -----------------

UItemInstance* UGridInventoryComponent::CreateItemInstance(const UItemDefinition* Definition, int32 StackCount)
{
	if (!Definition)
	{
		return nullptr;
	}

	UItemInstance* NewInst = NewObject<UItemInstance>(this);
	NewInst->Init(Definition, StackCount);
	return NewInst;
}

// Public
// ----------------- Inventory API -----------------

void UGridInventoryComponent::Initialize(int32 InWidth, int32 InHeight)
{
	Width = FMath::Max(1, InWidth);
	Height = FMath::Max(1, InHeight);

	Cells.SetNum(Width * Height);
	for (int32& Cell : Cells)
	{
		Cell = -1;
	}

	Records.Empty();
	NextItemId = 1;

	// Reset 通知（UI：全量重建）
	FGridInvChangeEvent Ev;
	Ev.Type = EGridInvChangeType::Reset;
	Ev.ItemId = -1;
	BroadcastChange(Ev);
}

int32 UGridInventoryComponent::GetItemIdAtCell(int32 X, int32 Y) const
{
	if (!InBounds(X, Y))
	{
		return -1;
	}

	const int32 Idx = Index(X, Y);
	if (!Cells.IsValidIndex(Idx))
	{
		return -1;
	}

	return Cells[Idx];
}

bool UGridInventoryComponent::GetItemRecord(int32 ItemId, FGridItemRecord& OutRecord) const
{
	OutRecord = FGridItemRecord();

	const FPlacedItemRecord* Rec = Records.Find(ItemId);
	if (!Rec || !Rec->Instance || !Rec->Instance->Definition)
	{
		return false;
	}

	const UItemDefinition* Def = Rec->Instance->Definition;

	const int32 GridW = Rec->bRotated ? Def->GridHeight : Def->GridWidth;
	const int32 GridH = Rec->bRotated ? Def->GridWidth : Def->GridHeight;

	OutRecord.ItemId = ItemId;
	OutRecord.Instance = Rec->Instance;
	OutRecord.Pos = Rec->Pos;
	OutRecord.bRotated = Rec->bRotated;
	OutRecord.GridW = GridW;
	OutRecord.GridH = GridH;

	return true;
}

TArray<int32> UGridInventoryComponent::GetAllItemIds() const
{
	TArray<int32> Keys;
	Records.GetKeys(Keys);

	Keys.RemoveAll([this](int32 Id)
		{
			const FPlacedItemRecord* Rec = Records.Find(Id);
			return (!Rec || !Rec->Instance || !Rec->Instance->Definition);
		});

	Keys.Sort();
	return Keys;
}

bool UGridInventoryComponent::PlaceAt(UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated, int32& OutItemId)
{
	OutItemId = -1;

	// 检查能否放下（边界/碰撞）
	if (!CanPlaceInternal(Instance, TopLeft, bRotated))
	{
		return false;
	}

	// 分配 ItemId
	const int32 ItemId = NextItemId++;
	OutItemId = ItemId;

	// 写入记录（Records 是权威）
	FPlacedItemRecord NewRec;
	NewRec.Instance = Instance;
	NewRec.Pos = TopLeft;
	NewRec.bRotated = bRotated;
	Records.Add(ItemId, NewRec);

	// 同步 Instance 的旋转状态（辅助一致性/调试；权威仍是 Records）
	Instance->bRotated = bRotated;

	// 写入 Cells 占格
	FillCells(ItemId, Instance, TopLeft, bRotated);

	// Added 通知（UI：只加这一件）
	FGridInvChangeEvent Ev;
	Ev.Type = EGridInvChangeType::Added;
	Ev.ItemId = ItemId;
	Ev.NewPos = TopLeft;
	Ev.bNewRotated = bRotated;
	BroadcastChange(Ev);

	return true;
}

bool UGridInventoryComponent::MoveItem(int32 ItemId, const FIntPoint& NewTopLeft, bool bNewRotated)
{
	FPlacedItemRecord* Rec = Records.Find(ItemId);
	if (!Rec || !Rec->Instance)
	{
		return false;
	}

	UItemInstance* Instance = Rec->Instance;

	const FIntPoint OldPos = Rec->Pos;
	const bool bOldRotated = Rec->bRotated;

	// 1先清旧占格（否则新位置检测会被自己挡住）
	ClearCells(ItemId, Instance, OldPos, bOldRotated);

	// 检查新位置是否可放（IgnoreItemId=自己，双保险）
	if (!CanPlaceInternal(Instance, NewTopLeft, bNewRotated, ItemId))
	{
		// 失败回滚：恢复旧占格
		FillCells(ItemId, Instance, OldPos, bOldRotated);
		return false;
	}

	// 更新记录
	Rec->Pos = NewTopLeft;
	Rec->bRotated = bNewRotated;

	// 同步 Instance（辅助一致性）
	Instance->bRotated = bNewRotated;

	// 写入新占格
	FillCells(ItemId, Instance, NewTopLeft, bNewRotated);

	// Moved 通知（UI：只更新这一件的位置/尺寸/旋转）
	FGridInvChangeEvent Ev;
	Ev.Type = EGridInvChangeType::Moved;
	Ev.ItemId = ItemId;
	Ev.OldPos = OldPos;
	Ev.NewPos = NewTopLeft;
	Ev.bOldRotated = bOldRotated;
	Ev.bNewRotated = bNewRotated;
	BroadcastChange(Ev);

	return true;
}

bool UGridInventoryComponent::RemoveItem(int32 ItemId)
{
	FPlacedItemRecord RemovedRec;
	if (!Records.RemoveAndCopyValue(ItemId, RemovedRec))
	{
		return false;
	}

	if (!RemovedRec.Instance || !RemovedRec.Instance->Definition)
	{
		return false;
	}

	// 清除占格
	ClearCells(ItemId, RemovedRec.Instance, RemovedRec.Pos, RemovedRec.bRotated);

	// Removed 通知
	FGridInvChangeEvent Ev;
	Ev.Type = EGridInvChangeType::Removed;
	Ev.ItemId = ItemId;
	Ev.OldPos = RemovedRec.Pos;
	Ev.bOldRotated = RemovedRec.bRotated;
	BroadcastChange(Ev);

	return true;
}

bool UGridInventoryComponent::RotateItem(int32 ItemId)
{
	FPlacedItemRecord* Rec = Records.Find(ItemId);
	if (!Rec || !Rec->Instance || !Rec->Instance->Definition)
	{
		return false;
	}

	UItemInstance* Instance = Rec->Instance;
	const UItemDefinition* Def = Instance->Definition;

	if (!Def->bCanRotate)
	{
		return false;
	}

	const FIntPoint Pos = Rec->Pos;
	const bool bOldRotated = Rec->bRotated;
	const bool bNewRotated = !bOldRotated;

	// 清旧占格
	ClearCells(ItemId, Instance, Pos, bOldRotated);

	// 检查旋转后能否原地放下
	if (!CanPlaceInternal(Instance, Pos, bNewRotated, ItemId))
	{
		// 失败回滚：恢复旧占格
		FillCells(ItemId, Instance, Pos, bOldRotated);
		return false;
	}

	// 更新记录 & 同步 Instance
	Rec->bRotated = bNewRotated;
	Instance->bRotated = bNewRotated;

	// 写入新占格
	FillCells(ItemId, Instance, Pos, bNewRotated);

	// Rotated 通知
	FGridInvChangeEvent Ev;
	Ev.Type = EGridInvChangeType::Rotated;
	Ev.ItemId = ItemId;
	Ev.OldPos = Pos;
	Ev.NewPos = Pos;
	Ev.bOldRotated = bOldRotated;
	Ev.bNewRotated = bNewRotated;
	BroadcastChange(Ev);

	return true;
}

bool UGridInventoryComponent::TryAddItem(UItemInstance* NewInstance, int32& OutItemId, FIntPoint& OutPlacedPos)
{
	OutItemId = -1;
	OutPlacedPos = FIntPoint::ZeroValue;

	if (!NewInstance || !NewInstance->Definition)
	{
		return false;
	}

	// 先尝试堆叠合并
	if (NewInstance->IsStackable())
	{
		for (auto& Pair : Records)
		{
			const int32 ExistingId = Pair.Key;
			FPlacedItemRecord& Rec = Pair.Value;

			if (!Rec.Instance || !Rec.Instance->Definition)
			{
				continue;
			}

			// 只有同一个 Definition 才能堆叠
			if (Rec.Instance->Definition != NewInstance->Definition)
			{
				continue;
			}

			// 记录旧数量 -> 合并 -> 新数量，用于 StackChanged
			const int32 OldStack = Rec.Instance->StackCount;
			Rec.Instance->TryMergeFrom(NewInstance);
			const int32 NewStack = Rec.Instance->StackCount;

			// 只有真的变化才广播，避免 UI 无意义刷新
			if (NewStack != OldStack)
			{
				FGridInvChangeEvent Ev;
				Ev.Type = EGridInvChangeType::StackChanged;
				Ev.ItemId = ExistingId;
				Ev.OldStack = OldStack;
				Ev.NewStack = NewStack;
				BroadcastChange(Ev);
			}

			// 如果 NewInstance 被完全吃完：本次添加完成（没有新增格子物品）
			if (NewInstance->IsEmpty())
			{
				OutItemId = -1;
				return true;
			}
		}
	}

	// 还有剩余 -> 找空位放入
	FIntPoint FitPos;
	bool bFitRotated = false;

	const bool bFound = FindFirstFit(NewInstance, /*bTryRotate=*/true, FitPos, bFitRotated);
	if (!bFound)
	{
		return false;
	}

	int32 NewId = -1;
	if (!PlaceAt(NewInstance, FitPos, bFitRotated, NewId))
	{
		return false;
	}

	OutItemId = NewId;
	OutPlacedPos = FitPos;
	return true;
}

bool UGridInventoryComponent::IsCellEmpty(int32 X, int32 Y) const
{
	if (!InBounds(X, Y))
	{
		return false;
	}

	const int32 Idx = Index(X, Y);
	if (!Cells.IsValidIndex(Idx))
	{
		return false;
	}

	return Cells[Idx] == -1;
}

// Private
// ----------------- Helpers -----------------

bool UGridInventoryComponent::CanPlaceInternal(UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated, int32 IgnoreItemId) const
{
	if (!Instance || !Instance->Definition)
	{
		return false;
	}

	const UItemDefinition* Def = Instance->Definition;

	// 防御：不允许旋转的物品，传 bRotated=true 直接失败
	if (bRotated && !Def->bCanRotate)
	{
		return false;
	}

	// 不要依赖 Instance->bRotated（外部可能还没同步），用参数 bRotated 算尺寸
	const int32 W = bRotated ? Def->GridHeight : Def->GridWidth;
	const int32 H = bRotated ? Def->GridWidth : Def->GridHeight;

	if (W <= 0 || H <= 0)
	{
		return false;
	}

	// 边界：TopLeft 和右下角都必须在范围内
	if (!InBounds(TopLeft.X, TopLeft.Y))
	{
		return false;
	}
	if (!InBounds(TopLeft.X + W - 1, TopLeft.Y + H - 1))
	{
		return false;
	}

	// 碰撞：覆盖区域每一格必须为空(-1)，或被 IgnoreItemId 占用
	for (int32 y = TopLeft.Y; y < TopLeft.Y + H; ++y)
	{
		for (int32 x = TopLeft.X; x < TopLeft.X + W; ++x)
		{
			const int32 Occ = Cells[Index(x, y)];
			if (Occ != -1 && Occ != IgnoreItemId)
			{
				return false;
			}
		}
	}

	return true;
}

void UGridInventoryComponent::FillCells(int32 ItemId, UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated)
{
	if (!Instance || !Instance->Definition)
	{
		return;
	}

	const UItemDefinition* Def = Instance->Definition;
	const int32 W = bRotated ? Def->GridHeight : Def->GridWidth;
	const int32 H = bRotated ? Def->GridWidth : Def->GridHeight;

	for (int32 y = TopLeft.Y; y < TopLeft.Y + H; ++y)
	{
		for (int32 x = TopLeft.X; x < TopLeft.X + W; ++x)
		{
			Cells[Index(x, y)] = ItemId;
		}
	}
}

void UGridInventoryComponent::ClearCells(int32 ItemId, UItemInstance* Instance, const FIntPoint& TopLeft, bool bRotated)
{
	if (!Instance || !Instance->Definition)
	{
		return;
	}

	const UItemDefinition* Def = Instance->Definition;
	const int32 W = bRotated ? Def->GridHeight : Def->GridWidth;
	const int32 H = bRotated ? Def->GridWidth : Def->GridHeight;

	for (int32 y = TopLeft.Y; y < TopLeft.Y + H; ++y)
	{
		for (int32 x = TopLeft.X; x < TopLeft.X + W; ++x)
		{
			const int32 Idx = Index(x, y);
			if (Cells.IsValidIndex(Idx) && Cells[Idx] == ItemId)
			{
				Cells[Idx] = -1;
			}
		}
	}
}

bool UGridInventoryComponent::FindFirstFit(UItemInstance* Instance, bool bTryRotate, FIntPoint& OutPos, bool& OutRotated) const
{
	if (!Instance || !Instance->Definition)
	{
		return false;
	}

	const UItemDefinition* Def = Instance->Definition;
	const bool bAllowRotatePass = bTryRotate && Def->bCanRotate;

	const int32 PassCount = bAllowRotatePass ? 2 : 1;

	for (int32 Pass = 0; Pass < PassCount; ++Pass)
	{
		const bool bRot = (Pass == 1);

		for (int32 y = 0; y < Height; ++y)
		{
			for (int32 x = 0; x < Width; ++x)
			{
				const FIntPoint P(x, y);
				if (CanPlaceInternal(Instance, P, bRot))
				{
					OutPos = P;
					OutRotated = bRot;
					return true;
				}
			}
		}
	}

	return false;
}

void UGridInventoryComponent::BroadcastChange(const FGridInvChangeEvent& E)
{
	OnInventoryChanged.Broadcast(E);
}
