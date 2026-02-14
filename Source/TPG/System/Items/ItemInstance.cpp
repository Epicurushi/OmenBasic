#include "System/Items/ItemInstance.h"
#include "System/Items/ItemDefinition.h"

int32 UItemInstance::GetGridWidth() const
{
	if (!Definition) return 0;
	return bRotated ? Definition->GridHeight : Definition->GridWidth;
}

int32 UItemInstance::GetGridHeight() const
{
	if (!Definition) return 0;
	return bRotated ? Definition->GridWidth : Definition->GridHeight;
}

bool UItemInstance::IsStackable() const
{
	return Definition ? Definition->bStackable : false;
}

int32 UItemInstance::GetMaxStack() const
{
	if (!Definition) return 1;
	return Definition->bStackable ? FMath::Max(1, Definition->MaxStack) : 1;
}

int32 UItemInstance::TryMergeFrom(UItemInstance* Other)
{
	if (!Other || Other == this) return 0;
	if (!Definition || Other->Definition != Definition) return 0;
	if (!IsStackable()) return 0;

	const int32 MaxStack = GetMaxStack();
	const int32 SpaceLeft = MaxStack - StackCount;
	if (SpaceLeft <= 0) return 0;

	// Other 可能已经是 0，在这里钳制
	const int32 OtherCount = FMath::Max(0, Other->StackCount);
	if (OtherCount <= 0) return 0;

	const int32 Move = FMath::Min(SpaceLeft, OtherCount);

	StackCount += Move;
	Other->StackCount = OtherCount - Move;

	return Move;
}