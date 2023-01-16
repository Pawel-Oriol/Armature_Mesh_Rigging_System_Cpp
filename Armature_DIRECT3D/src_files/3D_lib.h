//Copyright © 2023 by Pawel Oriol

//A C++ Implementation of The Armature and Mesh Rigging System "from scratch".
//A blender file of the used 3D animated model and blender expoters written in python by the author are also included in this project.
//You can watch the demo video of this project under the following adress:
//https://www.youtube.com/watch?v=yJpmEfbqp9k&t=47s

//Uses Direct3D11 and DirectXTK(both properties of Microsoft Corp - Visit the DirectXTK subfolder for the respective license):
//https://github.com/microsoft/DirectXTK
// 
//Model& Animation:
//Megan & Walkcycle2 obtained from mixamo.com:
//https://www.mixamo.com/#/?page=1&query=walk&type=Character
//https://www.youtube.com/watch?v=yJpmEfbqp9k



#include <time.h>
#include <iostream>
#include <cstdio>
#include <io.h>
#include<fcntl.h>
#include <dinput.h>
#include <windows.h>
#include <d3d11.h>
#include <vector>

#include <d3dcompiler.h>
#include <DirectXPackedVector.h>
#include <DirectXMath.h>
#include <dxgidebug.h>


#include "../DirectXTK/WICTextureLoader.h"
#include "../DirectXTK/DDSTextureLoader.h"
#include "d3d_wrappers.h"


using namespace DirectX;
struct Vertex
{
	Vertex() {};
	Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) : pos(x, y, z), texCoord(u, v), normal(nx, ny, nz) {}


	XMFLOAT3 pos;
	XMFLOAT2 texCoord;
	XMFLOAT3 normal;
};

void NormalizeVector(float* vSrc, float* vDst, int len);

void AddVectors(float* v1, float* v2, float* result, int len);

void SubVectors(float* v1, float* v2, float* result, int len);

float DotVectors(float* u, float* v, int len);

void CrossVectors(float* u, float* v, float* result);

void ScaleVector(float* v, float scale, int len);


struct Quaternion
{
	void Init(float w, float x, float y, float z);
	void InitAxisAngle(float* axis, float angle);
	Quaternion Conjugation();
	float Norm();
	void Normalize();
	Quaternion Reciprocal();
	

	float w;
	float x;
	float y;
	float z;
};

Quaternion HamiltonProd(Quaternion& q1, Quaternion& q2);

void Rotate(Quaternion* q, float* vSrc, float* vDst);

float QuaternionDot(Quaternion* q1, Quaternion* q2);

Quaternion QuaternionSlerp(Quaternion* q1, Quaternion* q2, float t);


struct VertexGroup
{
public:
	std::string boneName;
	int boneIndex;
	float weight;

	VertexGroup(const char *bName, float w): boneName(bName), weight(w)
	{

	}

};


//A structure representing a unique vertex for mesh rigging purposes.
//As a vertex can be shared by many triangles and thus present many times on a vertex array 
//(if the drawing is nonindexed as is the case in this implementation)
//it's more efficient to have a unique instance of it for armature tranformation purposes and than copy
//its current final coordinates to every adress on the vertex array when it is present
//The adresses where to copy it are stored in vPointers
//The copying is accomplished by the SetVertices() method;
struct VertexSkinned
{

	float posLocal[3];
	float posTrans[3];

	//in this list are listed all the bones that affect the vertex transformations and the influence they have via weights
	std::vector<VertexGroup> vGroups;
	std::vector<DirectX::XMFLOAT3*> vPointers;


	void SetVertices();
	void LoadVertexGroups(std::vector <VertexSkinned>& vSkinnedList, const char* fname);
};

void LoadVertexGroups(std::vector <VertexSkinned>& vSkinnedList, const char* fname);

class Object3D
{
public:
	std::vector<float> vList;
	std::vector<float> UVList;
	std::vector<float> normalList;
	std::vector<float> normalListTrans;

	std::vector<int> indexList;

	int numVertices = 0;
	Vertex* vLocal = NULL;
	Vertex* vTrans = NULL;

	std::vector <VertexSkinned> vSkinnedList;

	ID3D11Buffer *dataBuffer = NULL;
	ID3D11ShaderResourceView *objTexture = NULL;

	//we need to call it before destructor!
	void ReleaseD3D();
	Object3D();

	~Object3D();

	Object3D& operator=(Object3D&& other);


	void Load(ID3D11Device* devicePtr, const char* fname, bool vertexGroups = false, const char* vertexGroupsFname = NULL);

	void LoadTexture(ID3D11Device* devicePtr, ID3D11DeviceContext* devConPtr, const wchar_t* fname);

	//translation by a vector of all the vertices
	void TranslateByVector(float* vec);

	//rotation by a queternion of all the vertices
	void RotateByQuaternion(Quaternion* q);

	//combined rotation and translation
	void RotateAndTranslate(Quaternion* q, float* vec);


	
	void Scale(float scale_factor);

	//The algorithm is the same as the one used in Blender for smooth shading - the wider the angle 
	//is between the edges originating from the vertex the greater the influence the triangle will have on the
	//final values of the normal coordinates for tihs vertex
	void RecalculateNormals();
	void DrawObject(ID3D11DeviceContext* devConPtr);

};

struct TransformPair
{
	Quaternion orient;
	float pos[3];
};


struct FRAME
{
	float numFrame;
	Quaternion orientation;
};

struct Bone
{

	std::string name;
	int ID;
	std::string parentName;
	Bone* parent;

	float size; 
	Quaternion qLocal;
	float posLocal[3];

	Quaternion qBasis;
	float posBasis[3];

	Quaternion qBasisCurrent;
	float posBasisCurrent[3];

	Quaternion qFinal;
	float posFinal[3];

	Object3D object3d;

	int numFrames;
	std::vector<FRAME> frameList;

	Bone()
	{

	}

	Bone(Bone&& other);

	//Compute the final tranformations of all the bones
	//It's a recursive process. For each bone it's final transformatin equals: a composition of it's basis transformation, reverse local transformation  
	//and it's parents final tranformation (so we in turn need to compute is. The recursive process end's up once we arriave at the root bone).
	//For a better explanation check this thread on blender.stackexchange.com (I could never have explained it better myself):
	//https://blender.stackexchange.com/questions/44637/how-can-i-manually-calculate-bpy-types-posebone-matrix-using-blenders-python-ap
	TransformPair ComputeFinalOrientationPos();

	

	//Transformation of a vertex by the bone. At first the relative position of a vertex to the bone is computed, when
	//the bone is in its "rest pose" and then this position is transformed by the final orientation and position of the bone.
	//A very innefficient way of doing it. The better way would be to convert all the translations and rotation to matrices
	//and stack them together. Although I intended to keep this as simple as possible - it can not be stressed enough
	//that this it the worst way of conducting this operation!
	void TransformVertexByBone(float* vSrc, float* vDst);

};



class Armature
{

	int numBones = 0;
	std::vector<Bone> boneList;

	float lastFrame;
	float currFrame = 1;

public:
	void ReleaseD3D();

	void Load(ID3D11Device* devicePtr, const char* filename, const char* modelFilename, bool anim = true);

	void Animate(float progress);



	//This method computes the current value of the basis transformation.
	//For every bone we find two frames between which the current frame counter happens to
	//be and compute interpolate between them. The interpolation coefficient "t" of this interpolation
	//is the distance beetween the frame counter and the previous frame divided by the distance of these
	//two frames. The resulting interpolations (SLERP for orientation and LERP for position) constitute
	//the current basis transformation.
	void ComputeCurrBasis();

	//a method computing the final orientations and positions of all the bones
	void ComputeFinalOrientationPos();

	void Draw(ID3D11DeviceContext* devConPtr);

	void DrawFinal(ID3D11DeviceContext* devConPtr);

	void AssignBoneIndicesToVertexGroups(Object3D* objPtr);


	//The algorith is as follows:
	//Do for every vertex: Transform the vertex local (i.e. starting) position by TransformVertexByBone of every bone
	//that has any influence (i.e. weight) over it and multiply the result by the bones weight. Sum all the
	//results to achieve the final vertex position. Basically a weighted sum of all the transformations off all the bones
	//for that vertex.
	void MeshDeform(Object3D* objPtr);

};

