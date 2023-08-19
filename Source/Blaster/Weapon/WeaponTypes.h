#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayNames = "Assult Rifle"),
	EWT_RocketLauncher UMETA(DisplayNames = "RocketLauncher"),

	EWT_MAX UMETA(DisplayName = "DefaulMAX")
};
