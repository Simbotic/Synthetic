// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DetectableActor.generated.h"

UCLASS( ClassGroup=(Synthetic), meta=(BlueprintSpawnableComponent) )
class SYNTHETIC_API UDetectableActor : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDetectableActor();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Synthetic")
	FString ClassName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Synthetic")
	FString Pose;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Synthetic")
	int Difficult;
	
};
