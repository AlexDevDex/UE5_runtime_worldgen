// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "WorldGenerator.generated.h"

UCLASS()
class TRAINGAME_API AWorldGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AWorldGenerator();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int XVertexCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int YVertexCount = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float CellSize = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int NumOfXSections = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		int NumOfYSections = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		int MeshSectionIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
		UProceduralMeshComponent * TerrainMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UMaterialInterface* TerrainMaterial = nullptr;

	UPROPERTY(BlueprintReadOnly)
		bool GeneratorBusy = false;

	UPROPERTY(BlueprintReadOnly)
		bool TileDataReady = false;

	UPROPERTY(BlueprintReadWrite)
	TMap<FIntPoint, int> QueuedTile;

	int SectionIndexX = 0;
	int SectionIndexY = 0;

	//Subset mesh data
	TArray<FVector> SubVertices;
	TArray<FVector2D> SubUVs;
	TArray<int32> SubTriangles;
	TArray<FVector> SubNormals;
	TArray<FProcMeshTangent> SubTangents;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
		void GenerateTerrain(const int InSectionIndexX, const int InSectionIndexY);

	UFUNCTION(BlueprintCallable)
		void GenerateTerrainAsync(const int InSectionIndexX, const int InSectionIndexY);

	UFUNCTION(BlueprintCallable)
		void DrawTile();

	UFUNCTION(BlueprintCallable)
		FVector GetPlayerLocation();

	UFUNCTION(BlueprintCallable)
		FVector2D GetTileLoc(FIntPoint TileCoords);

	UFUNCTION(BlueprintCallable)
		FIntPoint GetClosestQueuedTile();

	float GetHeight(FVector2D Location);
	float PerlinNoiseExtended(const FVector2D Location, const float Frequency, const float Amplitude, const FVector2D offset);

};

class FAsyncWorldGenerator : public FNonAbandonableTask
{
public:
	FAsyncWorldGenerator(AWorldGenerator* InWorldGenerator): WorldGenerator(InWorldGenerator) {}
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FAsyncWorldGenerator, STATGROUP_ThreadPoolAsyncTasks);
	}
	void DoWork();
private:
	AWorldGenerator* WorldGenerator;
};