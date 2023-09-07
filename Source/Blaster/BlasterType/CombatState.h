#pragma once

UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),	// 버어있는
	ECS_Reloading UMETA(DisplayName = "Reloading"),	// 리로딩
	ECS_ThrowingGrenade UMETA(DisplayName = "ThrowingGrenade"),	// 수류탄 투척
	ECS_SwappingWeapons UMETA(DisplayName = "SwappingWeapons"),	// 무기 스왑

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};