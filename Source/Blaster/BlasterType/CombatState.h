#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),	// 버어있는
	ECS_Reloading UMETA(DisplayName = "Reloading"),	// 리로딩

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};