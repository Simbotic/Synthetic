#include "SyntheticCommon.h"
#include "DetectableActor.h"
#include "XmlParser/Public/XmlFile.h"
#include "Engine/StaticMesh.h"

struct FDetectableObject {
      UDetectableActor* DetectableActor;
      bool Occluded;
      bool Truncated;
      FBox2D Box2D;
      float DistanceFromCameraView;
};

bool USyntheticCommon::SaveLabelingFormat(USceneCaptureComponent2D *RenderComponent, EExportFormat Format, FString FilePath, FString FileName)
{
    uint32 imgWidth = RenderComponent->TextureTarget->SizeX;
    uint32 imgHeight = RenderComponent->TextureTarget->SizeY;
    TArray<FDetectableObject> DetectedObjects;
    FString yoloOutput;

    const FString XmlPath = FPaths::Combine(FilePath, FileName + ".xml");
    const FString ImagePath = FPaths::Combine(FilePath, FileName + ".png");

    FXmlFile *Xml = new FXmlFile("<annotation></annotation>", EConstructMethod::ConstructFromBuffer);
    FXmlNode *XmlRoot = Xml->GetRootNode();

    XmlRoot->AppendChildNode(TEXT("folder"), TEXT("Unspecified"));
    XmlRoot->AppendChildNode(TEXT("filename"), FileName);
    XmlRoot->AppendChildNode(TEXT("path"), FilePath);
    XmlRoot->AppendChildNode(TEXT("source"), TEXT(""));
    FXmlNode *XmlSource = XmlRoot->FindChildNode(TEXT("source"));
    XmlSource->AppendChildNode(TEXT("database"), TEXT("Unknown"));
    XmlRoot->AppendChildNode(TEXT("size"), TEXT(""));
    FXmlNode *XmlSize = XmlRoot->FindChildNode(TEXT("size"));
    XmlSize->AppendChildNode(TEXT("width"), FString::FromInt(imgWidth));
    XmlSize->AppendChildNode(TEXT("height"), FString::FromInt(imgHeight));
    XmlSize->AppendChildNode(TEXT("depth"), TEXT("3"));
    XmlRoot->AppendChildNode(TEXT("segmented"), TEXT("0"));

    FXmlNode *XmlNext = XmlRoot->FindChildNode(TEXT("segmented"));

    for (TActorIterator<AActor> ActorItr(RenderComponent->GetWorld()); ActorItr; ++ActorItr)
    {
       UDetectableActor *Actor = ActorItr->FindComponentByClass<UDetectableActor>();

       if (Actor)
       {   
           FBox2D BoxOut;
           bool IsTruncated;
           bool IsValid;
           float DistanceFromCamera = 0;
           bool IsInCameraView = CalcMinimumBoundingBox(*ActorItr, RenderComponent, BoxOut, DistanceFromCamera, IsTruncated, IsValid);
           
           if (IsValid && IsInCameraView)
           {   
               if (Format == EExportFormat::VE_YOLO)
               {
                   float cx = ((BoxOut.Min.X + BoxOut.Max.X) / 2) / imgWidth;
                   float cy = ((BoxOut.Min.Y + BoxOut.Max.Y) / 2) / imgHeight;
                   float w = (BoxOut.Max.X - BoxOut.Min.X) / imgWidth;
                   float h = (BoxOut.Max.Y - BoxOut.Min.Y) / imgHeight;

                   // YOLOv3 Format: {CLASS} {CX} {CY} {W} {H}, normalized [0,1]
                   yoloOutput += Actor->ClassName + " " +  FString::SanitizeFloat(cx) + " " + FString::SanitizeFloat(cy) + " "
                                + FString::SanitizeFloat(w) + " " + FString::SanitizeFloat(h) + "\n";
               } 
               else if(Format == EExportFormat::VE_PASCAL_VOC) 
               {
                   FDetectableObject Object = { Actor, false, IsTruncated, BoxOut, DistanceFromCamera };
                   DetectedObjects.Add(Object);
               }
           }
       }
    }

    if(Format == EExportFormat::VE_PASCAL_VOC)
    {
        for (size_t i = 0; i < DetectedObjects.Num(); i++)
        {
            for (size_t j = 0; j < DetectedObjects.Num(); j++)
            {
                // avoid comparing with itself
                if (i == j)
                {
                    continue;
                }

                // if marked as occluded skip checks
                if (DetectedObjects[i].Occluded)
                {
                    continue;
                }
                
                // intersects?
                if (DetectedObjects[i].Box2D.Intersect(DetectedObjects[j].Box2D))
                {
                    bool isBehind = DetectedObjects[i].DistanceFromCameraView > DetectedObjects[j].DistanceFromCameraView;

                    // is comparison box completely inside?
                    if (DetectedObjects[i].Box2D.IsInside(DetectedObjects[j].Box2D))
                    {
                        // is comparison box behind?
                        if (!isBehind)
                        {
                            DetectedObjects[j].Occluded = true;
                            continue;
                        }
                    }

                    // is completely inside?
                    if (DetectedObjects[j].Box2D.IsInside(DetectedObjects[i].Box2D))
                    {
                        // is behind?
                        if (isBehind)
                        {
                            DetectedObjects[i].Occluded = true;
                        }
                    }
                    // how many points intersect?
                    else 
                    {
                        uint8_t points = 0;

                        if (DetectedObjects[i].Box2D.IsInside(DetectedObjects[j].Box2D.Max))
                        {
                            points++;
                        }
                        if (DetectedObjects[i].Box2D.IsInside(DetectedObjects[j].Box2D.Min))
                        {
                            points++;
                        }
                        if (DetectedObjects[i].Box2D.IsInside(FVector2D(DetectedObjects[j].Box2D.Min.X, DetectedObjects[j].Box2D.Max.Y)))
                        {
                            points++;
                        }
                        if (DetectedObjects[i].Box2D.IsInside(FVector2D(DetectedObjects[j].Box2D.Max.X, DetectedObjects[j].Box2D.Min.Y)))
                        {
                            points++;
                        }

                        if (points == 0)
                        {
                            if (DetectedObjects[j].Box2D.IsInside(DetectedObjects[i].Box2D.Max))
                            {
                                points++;
                            }
                            if (DetectedObjects[j].Box2D.IsInside(DetectedObjects[i].Box2D.Min))
                            {
                                points++;
                            }
                            if (DetectedObjects[j].Box2D.IsInside(FVector2D(DetectedObjects[i].Box2D.Min.X, DetectedObjects[i].Box2D.Max.Y)))
                            {
                                points++;
                            }
                            if (DetectedObjects[j].Box2D.IsInside(FVector2D(DetectedObjects[i].Box2D.Max.X, DetectedObjects[i].Box2D.Min.Y)))
                            {
                                points++;
                            }
                        }
                        
                        // if at least 3 points intersect and is behind: occluded
                        if (points >= 3)
                        {
                            if (isBehind)
                            {
                                DetectedObjects[i].Occluded = true;
                            }
                        }
                        // if 1 or 2 points intersect and is behind: truncated
                        else if (points >= 1 && points <= 2)
                        {
                            if (isBehind)
                            {
                                DetectedObjects[i].Truncated = true;
                            }
                        }
                    }
                }
            }
        }

        for (FDetectableObject &Obj : DetectedObjects)
        {
            XmlRoot->AppendChildNode(TEXT("object"), "");
            XmlNext = (FXmlNode *)XmlNext->GetNextNode();
            XmlNext->AppendChildNode(TEXT("name"), Obj.DetectableActor->ClassName);
            XmlNext->AppendChildNode(TEXT("pose"), Obj.DetectableActor->Pose);
            XmlNext->AppendChildNode(TEXT("truncated"), Obj.Truncated ? TEXT("1") : TEXT("0"));
            XmlNext->AppendChildNode(TEXT("difficult"), FString::FromInt(Obj.DetectableActor->Difficult));
            XmlNext->AppendChildNode(TEXT("occluded"), FString::FromInt(static_cast<int>(Obj.Occluded)));
            XmlNext->AppendChildNode(TEXT("bndbox"), TEXT(""));
            FXmlNode *XmlBox = XmlNext->FindChildNode(TEXT("bndbox"));
            XmlBox->AppendChildNode(TEXT("xmin"), FString::Printf(TEXT("%f"), Obj.Box2D.Min.X));
            XmlBox->AppendChildNode(TEXT("xmax"), FString::Printf(TEXT("%f"), Obj.Box2D.Max.X));
            XmlBox->AppendChildNode(TEXT("ymin"), FString::Printf(TEXT("%f"), Obj.Box2D.Min.Y));
            XmlBox->AppendChildNode(TEXT("ymax"), FString::Printf(TEXT("%f"), Obj.Box2D.Max.Y));
        }
    }

    // Save render capture and format file
    RenderComponent->CaptureScene();
    UKismetRenderingLibrary::ExportRenderTarget(RenderComponent->GetWorld(), RenderComponent->TextureTarget, FilePath, FileName + ".png");
    
    if (Format == EExportFormat::VE_YOLO)
    {
        return WriteTxt(yoloOutput, FilePath, FileName + ".txt");
    } 
    else if(Format == EExportFormat::VE_PASCAL_VOC) 
    {
        if (!Xml->Save(XmlPath))
        {
            UE_LOG(LogTemp, Error, TEXT("%s couldn't be saved at %s"), *FileName, *FilePath );
            return false;
        }
        delete Xml;
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("%s couldn't be saved at %s"), *FileName, *FilePath );
    return false;
}

bool USyntheticCommon::CalcMinimumBoundingBox(const AActor* Actor, USceneCaptureComponent2D *RenderComponent, FBox2D &BoxOut, float &DistanceFromCameraView, bool &Truncated, bool &Valid)
{
    bool isCompletelyInView = true;
    Valid = true;
    UTextureRenderTarget2D *RenderTexture = RenderComponent->TextureTarget;
    TArray<FVector> Points;
    TArray<FVector2D> Points2D;
    FMinimalViewInfo Info;

    Info.Location = RenderComponent->GetComponentTransform().GetLocation();
    Info.Rotation = RenderComponent->GetComponentTransform().GetRotation().Rotator();
    Info.FOV = RenderComponent->FOVAngle;
    Info.ProjectionMode = RenderComponent->ProjectionType;
    Info.AspectRatio = float(RenderTexture->SizeX) / float(RenderTexture->SizeY);
    Info.OrthoNearClipPlane = 1;
    Info.OrthoFarClipPlane = 1000;
    Info.bConstrainAspectRatio = true;
    
    FTransform ViewPoint = RenderComponent->GetComponentTransform();
    
    USkinnedMeshComponent *Mesh = Actor->FindComponentByClass<USkinnedMeshComponent>();

    // Skinned Mesh
    if (Mesh)
    {
        DistanceFromCameraView = ViewPoint.InverseTransformPosition(Mesh->GetComponentLocation()).X;
        
        TArray<FFinalSkinVertex> OutVertices;
        Mesh->GetCPUSkinnedVertices(OutVertices, 0);

        FTransform const MeshWorldTransform = Mesh->GetComponentTransform();

        for (FFinalSkinVertex &Vertex : OutVertices)
        {
            Points.Add(MeshWorldTransform.TransformPosition(Vertex.Position));
        }
    } 
    else 
    {
        Valid = false;

        UStaticMeshComponent *StaticMeshComponent = Actor->FindComponentByClass<UStaticMeshComponent>();

        DistanceFromCameraView = ViewPoint.InverseTransformPosition(StaticMeshComponent->GetComponentLocation()).X;

        // Static Mesh
        if (StaticMeshComponent)
        {
            if (!StaticMeshComponent) { Valid = false; };
            if (!StaticMeshComponent->GetStaticMesh()) { Valid = false; };
            if (!StaticMeshComponent->GetStaticMesh()->RenderData) { Valid = false; };
            if (StaticMeshComponent->GetStaticMesh()->RenderData->LODResources.Num() > 0)
            {
                FPositionVertexBuffer* VertexBuffer = &StaticMeshComponent->GetStaticMesh()->RenderData->LODResources[0].VertexBuffers.PositionVertexBuffer;
                if (VertexBuffer)
                {   
                    FTransform const MeshWorldTransform = StaticMeshComponent->GetComponentTransform();

                    const int32 VertexCount = VertexBuffer->GetNumVertices();
                    
                    if (VertexCount > 0)
                    {
                        Valid = true;
                    }
                    
                    for (int32 Index = 0; Index < VertexCount; Index++)
                    {
                        Points.Add(MeshWorldTransform.TransformPosition(VertexBuffer->VertexPosition(Index)));
                    }
                } 
                else
                {
                    Valid = false;
                }     
            }
        } 
        else
        {
            Valid = false;
        }
    }

    if (!Valid)
    {
        return false;
    }

    FVector2D MinPixel(RenderTexture->SizeX, RenderTexture->SizeY);
    FVector2D MaxPixel(0, 0);
    FIntRect ScreenRect(0, 0, RenderTexture->SizeX, RenderTexture->SizeY);

    FSceneViewProjectionData ProjectionData;
    ProjectionData.ViewOrigin = Info.Location;

    ProjectionData.ViewRotationMatrix = FInverseRotationMatrix(Info.Rotation) * 
                                                                                FMatrix(
                                                                                        FPlane(0, 0, 1, 0),
                                                                                        FPlane(1, 0, 0, 0),
                                                                                        FPlane(0, 1, 0, 0),
                                                                                        FPlane(0, 0, 0, 1));

    ProjectionData.ProjectionMatrix = Info.CalculateProjectionMatrix();
    ProjectionData.SetConstrainedViewRectangle(ScreenRect);

    for (FVector &Point : Points)
    {
        FVector2D Pixel(0, 0);
        FSceneView::ProjectWorldToScreen((Point), ScreenRect, ProjectionData.ComputeViewProjectionMatrix(), Pixel);
        if (Pixel.X >= (RenderTexture->SizeX * -0.01) && Pixel.X <= (RenderTexture->SizeX + RenderTexture->SizeX * 0.01) && Pixel.Y >= (RenderTexture->SizeY * -0.01) && Pixel.Y <= (RenderTexture->SizeY + RenderTexture->SizeY * 0.01))
        {
            Points2D.Add(Pixel);
            MaxPixel.X = FMath::Max(Pixel.X, MaxPixel.X);
            MaxPixel.Y = FMath::Max(Pixel.Y, MaxPixel.Y);
            MinPixel.X = FMath::Min(Pixel.X, MinPixel.X);
            MinPixel.Y = FMath::Min(Pixel.Y, MinPixel.Y);
        }
    }

    BoxOut = FBox2D(MinPixel, MaxPixel);
    if (BoxOut.Min.X < 0)
    {
        BoxOut.Min.X = 0;
        isCompletelyInView = false;
    }
    else if (BoxOut.Min.X > RenderTexture->SizeX)
    {
        BoxOut.Min.X = RenderTexture->SizeX;
        isCompletelyInView = false;
    }
    if (BoxOut.Min.Y < 0)
    {
        BoxOut.Min.Y = 0;
        isCompletelyInView = false;
    }
    else if (BoxOut.Min.Y > RenderTexture->SizeY)
    {
        BoxOut.Min.Y = RenderTexture->SizeY;
        isCompletelyInView = false;
    }
    if (BoxOut.Max.X > RenderTexture->SizeX)
    {
        BoxOut.Max.X = RenderTexture->SizeX;
        isCompletelyInView = false;
    }
    else if (BoxOut.Max.X < 0)
    {
        BoxOut.Max.X = 0;
        isCompletelyInView = false;
    }
    if (BoxOut.Max.Y > RenderTexture->SizeY)
    {
        BoxOut.Max.Y = RenderTexture->SizeY;
        isCompletelyInView = false;
    }
    else if (BoxOut.Max.Y < 0)
    {
        BoxOut.Max.Y = 0;
        isCompletelyInView = false;
    }

    Truncated = !isCompletelyInView;

    if (BoxOut.GetSize().X < 5 || BoxOut.GetSize().Y < 5)
    {
        return false;
    }

    return true;
}

bool USyntheticCommon::ReadTxt(FString FilePath, FString FileName, FString &OutputTxt)
{
    FPaths::NormalizeDirectoryName(FilePath);
    FString full_path =  FilePath + "/" + FileName;
    FPaths::RemoveDuplicateSlashes(full_path);
    return FFileHelper::LoadFileToString(OutputTxt, *full_path);
}

bool USyntheticCommon::WriteTxt(FString inText, FString FilePath, FString FileName)
{
    FPaths::NormalizeDirectoryName(FilePath);
    FString full_path =  FilePath + "/" + FileName;
    FPaths::RemoveDuplicateSlashes(full_path);
    return FFileHelper::SaveStringToFile(inText, *full_path);
}