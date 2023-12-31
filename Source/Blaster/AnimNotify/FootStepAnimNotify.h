// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify_PlaySound.h"
#include "FootStepAnimNotify.generated.h"

/**
 * 
 */
UCLASS()
class BLASTER_API UFootStepAnimNotify : public UAnimNotify_PlaySound
{
	GENERATED_BODY()

public:
	UFootStepAnimNotify();

	EPhysicalSurface m_StepSurface;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Tile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Grass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Metal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Dirt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Snow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UserContents", meta = (AllowPrivateAccess = "true"))
		TObjectPtr<USoundBase> m_Sand;

protected:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

private:
	TObjectPtr<USoundBase> SelectSoundBase();
	
};
