#pragma once

#define TRACE_LENGTH 80000

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayNames = "Assult Rifle"),
	EWT_RocketLauncher UMETA(DisplayNames = "RocketLauncher"),
	EWT_Pistol UMETA(DisplayNames = "Pistol"),
	EWT_SMG UMETA(DisplayNames = "SMG"),
	EWT_Shotgun UMETA(DisplayNames = "Shotgun"),

	EWT_MAX UMETA(DisplayName = "DefaulMAX")
};
