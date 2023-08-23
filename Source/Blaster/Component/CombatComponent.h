// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "../HUD/BlasterHUD.h"
#include "../Weapon/WeaponTypes.h"
#include "../BlasterType/CombatState.h"
#include "CombatComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class BLASTER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class ABlasterCharacter;

public:	
	UCombatComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void EquipWeapon(class AWeapon* WeaponToEquip);
	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();

	void FireButtonPressed(bool bPressed);

	UFUNCTION(BlueprintCallable)
	void ShotgunShellReload();

	void JumpToShotgunEnd();

	UFUNCTION(BlueprintCallable)
	void ThrowGrenadeFinished();

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);

	UFUNCTION()
	void OnRep_EquippedWeapon();

	void Fire();

	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	// 크로스 헤어 부분
	void SetHUDCrosshairs(float DeltaTime);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	// 서버와 클라이언트에서 발생 모든 컴퓨터에서 발생해야할 것을 처리
	void HandleReload();
	
	int32 AmountToReload();

	void ThrowGrenade();

	UFUNCTION(Server, Reliable)
	void ServerThrowGrenad();

	void DropEquippedWeapon();
	void AttachActorToRightHand(AActor* ActorToAttach);
	void AttachActorToLeftHand(AActor* ActorToAttach);
	void UpdateCarriedAmmo();
	void PlayEquipWeaponSound();
	void ReloadEmptyWeapon();

private:
	UPROPERTY()
	class ABlasterCharacter* Character;
	UPROPERTY()
	class ABlasterPlayerController* Controller;
	UPROPERTY()
	ABlasterHUD* HUD;
	// 장착된 무기를 저정하기 위함

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	class AWeapon* EquippedWeapon;

	UPROPERTY(Replicated)
	bool bAiming;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;


	UPROPERTY(EditAnywhere)
	float AimWalkSpeed;

	bool bFireButtonPressed;

	// HUD and Crosshairs
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;

	FVector HitTarget;

	FHUDPackage HUDPackage;

	// Aiming and FOV

	// 조준하지 않을때의 FOV값(시야) BeginPlay에서 카메라의 기본 FOV로 설정
	float DefaultFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;

	float CurrentFOV;

	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;

	void InterpFOV(float DeltaTime);

	// 자동 라이플 발사
	FTimerHandle FireTimer;

	bool bCanFire = true;

	void StartFireTimer();
	// 타이머 끝났을때의 콜백함수
	void FireTimerFinished();

	bool CanFire();

	// 현재 장착된 무기의 소지 탄약
	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;

	UFUNCTION()
	void OnRep_CarriedAmmo();

	// TMap은 복제가 안된다
	TMap<EWeaponType, int32> CarriedAmmoMap;

	// 라이플 첫 시작 총알 개수
	UPROPERTY(EditAnywhere)
	int32 StartingARAmmo = 30;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 4;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 15;

	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 15;

	UPROPERTY(EditAnywhere)
	int32 StartingShotgunAmmo = 20;

	UPROPERTY(EditAnywhere)
	int32 StartingSniperAmmo = 10;

	UPROPERTY(EditAnywhere)
	int32 StartingGrenadeLauncherAmmo = 5;

	void InitializeCarriedAmmo();

	// 모든 클라이언트는 전투 상태를 알아야한다
	UPROPERTY(ReplicatedUsing = OnRep_CombatSTate)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
	void UpdateShotgunAmmoValues();

public:	

		
};
