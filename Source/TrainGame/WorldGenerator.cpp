// Fill out your copyright notice in the Description page of Project Settings.


#include "WorldGenerator.h"
#include "KismetProceduralMeshLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

// Sets default values
AWorldGenerator::AWorldGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
	//RootComponent = TerrainMesh;
	// 
	// New in UE 4.17, multi-threaded PhysX cooking.
	//TerrainMesh->bUseAsyncCooking = true;

	//TerrainMesh->SetupAttachment(GetRootComponent());

}

// Called when the game starts or when spawned
void AWorldGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AWorldGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AWorldGenerator::GenerateTerrain(const int InSectionIndexX, const int InSectionIndexY)
{
	FVector Offset = FVector(InSectionIndexX * (XVertexCount - 1), InSectionIndexY * (YVertexCount - 1), 0.f) * CellSize;

	TArray<FVector> Vertices;
	FVector Vertex;

	TArray<FVector2D> UVs;
	FVector2D UV;

	TArray<int32> Triangles; 
	//FJsonSerializableArrayInt Triangles;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;

	//Calculate Vertices and UVs
	for (int32 iVY = -1; iVY <= YVertexCount; iVY++)
	{
		for (int32 iVX = -1; iVX <= XVertexCount; iVX++)
		{
			//Calculate Vertex
			Vertex.X = iVX * CellSize + Offset.X;
			Vertex.Y = iVY * CellSize + Offset.Y;
			Vertex.Z = GetHeight(FVector2D(Vertex.X, Vertex.Y));
			Vertices.Add(Vertex);

			//Calculate UV
			UV.X = (iVX + (InSectionIndexX * (XVertexCount - 1))) * (CellSize / 100);
			UV.Y = (iVY + (InSectionIndexY * (YVertexCount - 1))) * (CellSize / 100);
			UVs.Add(UV);
		}
	}

	//Calculate Triangles
	for (int32 iTY = 0; iTY <= YVertexCount; iTY++)
	{
		for (int32 iTX = 0; iTX <= XVertexCount; iTX++)
		{
			Triangles.Add(iTX + iTY * (XVertexCount + 2));
			Triangles.Add((iTX + iTY * (XVertexCount + 2)) + (XVertexCount + 2)); //Triangles.Add(iTX + (iTY+1) * (XVertexCount + 2));
			Triangles.Add(iTX + iTY * (XVertexCount + 2) + 1);

			Triangles.Add((iTX + iTY * (XVertexCount + 2)) + (XVertexCount + 2));
			Triangles.Add((iTX + iTY * (XVertexCount + 2)) + (XVertexCount + 2) + 1);
			Triangles.Add(iTX + iTY * (XVertexCount + 2) + 1);
		}
	}
	//Calculate Subset Mesh (to prevent seams)
	//TArray<FVector> SubVertices;
	//TArray<FVector2D> SubUVs;
	//TArray<int32> SubTriangles;
	////FJsonSerializableArrayInt SubTriangles;
	//TArray<FVector> SubNormals;
	//TArray<FProcMeshTangent> SubTangents;

	int VertexIndex = 0;

	//Calculate Normals
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Vertices, Triangles, UVs, Normals, Tangents);

	//Calculate Vertices and UVs
	for (int32 isVY = -1; isVY <= YVertexCount; isVY++)
	{
		for (int32 isVX = -1; isVX <= XVertexCount; isVX++)
		{
			if (-1 < isVY && isVY < YVertexCount && -1 < isVX && isVX < XVertexCount)
			{
				SubVertices.Add(Vertices[VertexIndex]);
				SubUVs.Add(UVs[VertexIndex]);
				SubNormals.Add(Normals[VertexIndex]);
				SubTangents.Add(Tangents[VertexIndex]);
			}
			VertexIndex++;
		}
	}

	//Calculate Triangles
	if (SubTriangles.Num()==0)
	{
		for (int32 isTY = 0; isTY <= (YVertexCount-2); isTY++)
		{
			for (int32 isTX = 0; isTX <= (XVertexCount-2); isTX++)
			{
				SubTriangles.Add(isTX + isTY * XVertexCount);
				SubTriangles.Add(isTX + isTY * XVertexCount + XVertexCount);
				SubTriangles.Add(isTX + isTY * XVertexCount + 1);

				SubTriangles.Add(isTX + isTY * XVertexCount + XVertexCount);
				SubTriangles.Add(isTX + isTY * XVertexCount + XVertexCount + 1);
				SubTriangles.Add(isTX + isTY * XVertexCount + 1);
			}
		}
	}
	TileDataReady = true;
}

float AWorldGenerator::GetHeight(FVector2D Location)
{
	return
	{
		PerlinNoiseExtended(Location, .00001f, 10000, FVector2D(.1f)) +
		PerlinNoiseExtended(Location, .0001f, 5000, FVector2D(.2f)) +
		PerlinNoiseExtended(Location, .001f, 500, FVector2D(.3f)) +
		PerlinNoiseExtended(Location, .01f, 100, FVector2D(.4f))
	};
}

void AWorldGenerator::GenerateTerrainAsync(const int InSectionIndexX, const int InSectionIndexY)
{
	GeneratorBusy = true;
	SectionIndexX = InSectionIndexX;
	SectionIndexY = InSectionIndexY;
	QueuedTile.Add(FIntPoint(InSectionIndexX, InSectionIndexY), MeshSectionIndex);

	AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [&]()
		{
			auto WorldGenTask = new FAsyncTask<FAsyncWorldGenerator>(this);
			WorldGenTask->StartBackgroundTask();
			WorldGenTask->EnsureCompletion();
			delete WorldGenTask;
		}
	);
}

void AWorldGenerator::DrawTile()
{
	TileDataReady = false;
	//Create Mesh Section
	//TerrainMesh->CreateMeshSection(MeshSectionIndex, SubVertices, SubTriangles, SubNormals, SubUVs, TArray<FColor>(), SubTangents, true);
	TerrainMesh->CreateMeshSection_LinearColor(MeshSectionIndex, SubVertices, SubTriangles, SubNormals, SubUVs, TArray<FLinearColor>(), SubTangents, true);
	//TerrainMesh->ContainsPhysicsTriMeshData(true);
	if (TerrainMaterial)
	{
		TerrainMesh->SetMaterial(MeshSectionIndex, TerrainMaterial);
	}
	MeshSectionIndex++;

	SubVertices.Empty();
	SubNormals.Empty();
	SubUVs.Empty();
	SubTangents.Empty();

	GeneratorBusy = false;
}

FVector AWorldGenerator::GetPlayerLocation()
{	
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (PlayerCharacter)
	{
		return PlayerCharacter->GetActorLocation();
	}
	return FVector(0,0,0);
}


FVector2D AWorldGenerator::GetTileLoc(FIntPoint TileCoords)
{
	return FVector2D(TileCoords*FIntPoint(XVertexCount-1, YVertexCount-1)*CellSize) + FVector2D(XVertexCount-1, YVertexCount-1)*CellSize/2;
}

FIntPoint AWorldGenerator::GetClosestQueuedTile()
{
	float ClosestDistance = TNumericLimits<float>::Max();
	FIntPoint ClosestTile;
	for (const auto& Entry : QueuedTile)
	{
		const FIntPoint& Key = Entry.Key;
		int Value = Entry.Value;
		if (Value==-1)
		{
			FVector2D TileLocation = GetTileLoc(Key);
			FVector PlayerLocation = GetPlayerLocation();
			float Distance = FVector2D::Distance(TileLocation, FVector2D(PlayerLocation));
			if (Distance<ClosestDistance)
			{
				ClosestDistance = Distance;
				ClosestTile = Key;
			}
		}
	}
	return ClosestTile;
}

float AWorldGenerator::PerlinNoiseExtended(const FVector2D Location, const float Scale, const float Amplitude, const FVector2D offset)
{
	return FMath::PerlinNoise2D(Location * Scale + FVector2D(.1f, .1f) + offset) * Amplitude;
}

void FAsyncWorldGenerator::DoWork()
{
	WorldGenerator->GenerateTerrain(WorldGenerator->SectionIndexX, WorldGenerator->SectionIndexY);
}
