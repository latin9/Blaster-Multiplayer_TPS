#pragma once

#define TRACE_LENGTH 80000

#define CUSTOM_DEPTH_GREEN 249
#define CUSTOM_DEPTH_PURPLE 250
#define CUSTOM_DEPTH_BLUE 251
#define CUSTOM_DEPTH_TAN 252

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_AssaultRifle UMETA(DisplayNames = "Assult Rifle"),
	EWT_RocketLauncher UMETA(DisplayNames = "RocketLauncher"),
	EWT_Pistol UMETA(DisplayNames = "Pistol"),
	EWT_SMG UMETA(DisplayNames = "SMG"),
	EWT_Shotgun UMETA(DisplayNames = "Shotgun"),
	EWT_SniperRifle UMETA(DisplayNames = "SniperRifle"),
	EWT_GrenadeLauncher UMETA(DisplayNames = "GrenadeLauncher"),
	EWT_Flag UMETA(DisplayNames = "Flag"),

	EWT_MAX UMETA(DisplayName = "DefaulMAX")
};
