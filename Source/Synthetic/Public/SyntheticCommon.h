#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "SceneView.h"
#include "Misc/FileHelper.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalRenderPublic.h"
#include "SyntheticCommon.generated.h"

UENUM(BlueprintType)
enum class EExportFormat: uint8
{
  VE_YOLO         UMETA(DisplayName="YOLOv3"),
  VE_PASCAL_VOC   UMETA(DisplayName="Pascal VOC")
};

UCLASS()
class SYNTHETIC_API USyntheticCommon : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    static bool ProjectWorldToScreen(const FVector& WorldPosition, const FIntRect& ViewRect, const FMatrix& ViewProjectionMatrix, FVector& out_ScreenPos);

  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Enum)
	  EExportFormat FormatEnum;

    UFUNCTION(BlueprintCallable, Category = "Synthetic|Util")
    static bool SaveLabelingFormat(USceneCaptureComponent2D *RenderComponent, EExportFormat Format, FString FilePath = "/tmp/synthetic_data", FString FileName = "render");

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Synthetic|Util")
    static bool CalcMinimumBoundingBox(const AActor* Actor, USceneCaptureComponent2D *RenderComponent, FBox2D &BoxOut, float &DistanceFromCameraView, bool &Truncated, bool &Valid);

    UFUNCTION(BlueprintPure, Category = "File", meta = (Keywords = "Synthetic|Util"))
    static bool ReadTxt(FString FilePath, FString FileName, FString &OutputTxt);

    UFUNCTION(BlueprintCallable, Category = "File", meta = (Keywords = "Synthetic|Util"))
    static bool WriteTxt(FString inText, FString FilePath, FString FileName);
};
