// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "../BlasterType/TurningInPlace.h"
#include "../Interfaces/InteractWithCrosshairsInterface.h"
#include "Components/TimelineComponent.h"
#include "../BlasterType/CombatState.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter, public IInteractWithCrosshairsInterface
{
	GENERATED_BODY()

public:
	ABlasterCharacter();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void PostInitializeComponents()	override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void PlayFireMontage(bool bAiming);
	void PlayReloadMontage();
	void PlayElimMontage();
	void PlayThrowGrenadeMontage();
	// �ùķ��̼� ���Ͻÿ� ���� ĳ���� ȸ���� ��Ÿ�� Ȯ���� �� ƽ ��� ��� �̰��� ���
	virtual void OnRep_ReplicatedMovement() override;

	// ���� ���� �������� �����ϰ� �������� Elim�� �����ϱ� ������ ���� ������ �ش�
	void Elim();
	// �״°��� �ŷ��ؾߵ� �߿����۾�
	// ���������� ����Ǹ� �ȵȴ�.
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElim();
	
	/*UFUNCTION(Server, Reliable)
	void ServerElimDestroyed();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimDestroyed();*/


	virtual void Destroyed() override;

	// BlueprintImplementableEvent : �������Ʈ���� ������ �� ����
	UFUNCTION(BlueprintImplementableEvent)
	void ShowSniperScopeWidget(bool bShowScope);

	void UpdateHUDHealth();
	void UpdateHUDShield();
	void UpdateHUDAmmo();

	void SpawnDefaultWeapon();

protected:
	virtual void BeginPlay() override;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	// E ��ư Ŭ��(��� ����)
	void EquipButtonPressed();
	void CrouchButtonPressed();
	void ReloadButtonPressed();
	void GrenadeButtonPressed();
	void AimButtonPressed();
	void AimButtonReleased();
	float CalculateSpeed();
	void AimOffset(float DeltaTime);
	void CalculateAO_Pitch();
	void SimproxiesTurn();
	virtual void Jump() override;
	void FireButtonPressed();
	void FireButtonReleased();
	void PlayHitReactMontage();
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);

	// ���õ� ��� Ŭ������ ���� Poll, HUD�� ��ȿ�� �����͸� �ʱ�ȭ �ϴ� �Լ�
	void PollInit();
	
	void RotateInPlace(float DeltaTime);
	void DropOrDestroyWeapon(AWeapon* Weapon);
	void DropOrDestroyWeapons();

public:

protected:

private:
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
	class UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UWidgetComponent* OverheadWidget;

	UPROPERTY(EditAnywhere, Category = "Player Name")
	FString LocalPlayerName = TEXT("Unknown Player");

	// �������� ����ɶ� ��� Ŭ���̾�Ʈ���� ����ǰ� ���⿡ ���� �����͸� ������ �� �ִ�.
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
	class AWeapon* OverlappingWeapon;

	// LastWeapon���� �����Ǳ����� OverlappingWeapon�� ����
	UFUNCTION()
	void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCombatComponent* Combat;
	

	UPROPERTY(VisibleAnywhere)
	class UBuffComponent* Buff;

	// ���� RPC�� �ŷ��� �� �ֵ��� Reliable ����
	UFUNCTION(Server, Reliable)
	void ServerEquipButtonPressed();

	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	ETurningInPlace TurningInPlace;

	void TurnInPlace(float DeltaTime);

	// Anim Montage
	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ElimMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	class UAnimMontage* ThrowGrenadeMontage;

	void HideCameraIfCharacterClose();
	UPROPERTY(EditAnywhere)
	float CameraThreshold = 200.f;

	bool bRotateRootBone;
	float TurnThreshold = 0.25f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float ProxyYaw;
	float TimeSinceLastMovementReplication;

	// Player Health
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	// ���� ü��
	UPROPERTY(ReplicatedUsing = OnRep_Health, VisibleAnywhere, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health(float LastHealth);

	// Player Shield
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxShield = 100.f;

	// ���� ����
	UPROPERTY(ReplicatedUsing = OnRep_Shield, VisibleAnywhere, Category = "Player Stats")
	float Shield = 0.f;

	UFUNCTION()
	void OnRep_Shield(float LastShield);

	UPROPERTY()
	class ABlasterPlayerController* BlasterPlayerController;

	bool bElimmed = false;

	FTimerHandle ElimTimer;
	// ������ �ʱⰪ���θ� ��������
	UPROPERTY(EditDefaultsOnly)
	float ElimDelay = 3.f;
	void ElimTimerFinished();

	// Dissolve Effect
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeline;

	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;
	
	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	// ��Ÿ�ӿ� �����ϴ� ���� �ν��Ͻ��� �����ϱ� ���� ���
	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	// �������Ʈ�� ������ ��Ƽ���� �ν��Ͻ�, ���� ���͸��� �ν��Ͻ��� �Բ� ���
	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	// Elimbot
	UPROPERTY(EditAnywhere)
	class UParticleSystem* ElimBotEffect;

	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* ElimBotComponent;

	UPROPERTY(EditAnywhere)
	class USoundCue* ElimBotSound;

	UPROPERTY()
	class ABlasterPlayerState* BlasterPlayerState;

	// ����ź ����
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* AttachedGrenade;

	// ����Ʈ ����
	// ĳ���� ������ �ٷ� �����Ǵ� �⺻ ����
	UPROPERTY(EditAnywhere)
	TSubclassOf<AWeapon> DefaultWeaponClass;

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	bool IsWeaponEquipped();
	bool IsAiming();
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	AWeapon* GetEquippedWeapon();
	FORCEINLINE ETurningInPlace GetTurningInPlace() const { return TurningInPlace; }
	FVector GetHitTarget()	const;
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE bool IsElimmed() const { return bElimmed; }
	FORCEINLINE float GetHealth() const{ return Health; }
	FORCEINLINE void SetHealth(float _Health) {Health = _Health; }
	FORCEINLINE float GetMaxHealth() const{ return MaxHealth; }
	FORCEINLINE float GetShield() const { return Shield; }
	FORCEINLINE void SetShield(float _Shield) { Shield = _Shield; }
	FORCEINLINE float GetMaxShield() const { return MaxShield; }
	ECombatState GetCombatState() const;
	FORCEINLINE void SetDisableGameplay(bool Enable) { bDisableGameplay = Enable; }
	FORCEINLINE bool GetDisableGameplay() const { return bDisableGameplay; }
	FORCEINLINE class UCombatComponent* GetCombatComponent() const { return Combat; }
	FORCEINLINE UAnimMontage* GetReloadMontage() const { return ReloadMontage; }
	FORCEINLINE class UStaticMeshComponent* GetAttachedGrenade() const { return AttachedGrenade; }
	FORCEINLINE class UBuffComponent* GetBuffComponent() const { return Buff; }
public: 
	UFUNCTION(Client, Reliable)
	void ClientSetName(const FString& Name);

	UFUNCTION(Server, Reliable)
	void ServerSetPlayerName(const FString& PlayerName);


};
