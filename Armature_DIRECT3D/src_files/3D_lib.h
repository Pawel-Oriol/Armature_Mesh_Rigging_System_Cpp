//Copyright © 2023 by Pawel Oriol


//Biblioteka implementujaca system armatury wraz z rigiem mesha
//Wersja stabilna, ale prototypowa, niezoptymalizowana pod katem wydajnosci
//wczytuje armature i wagi mesha zapisane w formacie autorskim eskportowanego z blendera
//przy uzyciu wlasnego exportera mesh wczystywany jest z plikow w formacie *.obj
//Link do demonstracji dzialania programu:
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

//vertex niepowtarzalny przypisany do kości
//posiada adresy vertexów odpowiadające mu na liście trójkatow do podmiany 
struct VertexSkinned
{

	float posLocal[3];
	float posTrans[3];

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

	//wywolac przed destruktorem!!
	void ReleaseD3D();
	Object3D();

	~Object3D();

	Object3D& operator=(Object3D&& other);


	void Load(ID3D11Device* devicePtr, const char* fname, bool vertexGroups = false, const char* vertexGroupsFname = NULL);

	void LoadTexture(ID3D11Device* devicePtr, ID3D11DeviceContext* devConPtr, const wchar_t* fname);

	//translacja obiektu o wektor
	void TranslateByVector(float* vec);

	//obrot obiektu o kwaternion
	void RotateByQuaternion(Quaternion* q);

	//translacja obiektu o wektor oraz obrot obiektu o kwaternion
	void RotateAndTranslate(Quaternion* q, float* vec);


	//skalowanie obiektu
	void Scale(float scale_factor);

	//metoda liczaca wektory normalne na nowo
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

	TransformPair ComputeFinalOrientationPos();

	//Transformacja wierzcholka o przez kosc.
	//Najpierw liczona jest pozycja wierzcholka wzgledem kosci, gdy ta jest
	//w spoczynku, a nastepnie owa pozycja jest transformowana o pozycje finalna kosci
	//Bardzo zle rozwiazanie pod katem wydajnosci. Trzeba dodac konwersje na ekwiwalent macierzowy
	//- macierz 4x4 mieszczaca w sobie zarowno rotacje jak i translacje
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


	//Metoda liczaca aktualna wartosc tranformaty basis
	//dla kazdej kosci znajdujemy dwie klatki pomiedzy ktorymi
	//akurat znajduje sie licznik aktualnego frame'a
	//i liczemy wartosc interpolowana miedzy tymi klatkami
	//gdzie wspolczynnikiem interpolacji bedzie procentowa 
	//odleglosc licznika od obu klatek
	//dla translacyjnej czesci tranformaty uzywamy LERP-a,
	//zas dla orientacyjnej - SLERP -a, jako, ze ten zachowuje
	//uniformowe tempo rotacji
	void ComputeCurrBasis();

	//liczenie finalnej orientacji wszystkich kosci
	//algorytm przebiega tak, ze najpierw kosc jest transformowana o
	//transformate basis, nastepnie local, nastepnie liczona jest jej pozycja
	//wzgledem rodzica i na koncu jest transformowana o pozycje ostateczna rodzica
	//ktora dopiero musimy rekursywnie policzyc
	void ComputeFinalOrientationPos();

	void Draw(ID3D11DeviceContext* devConPtr);

	void DrawFinal(ID3D11DeviceContext* devConPtr);

	void AssignBoneIndicesToVertexGroups(Object3D* objPtr);

	void MeshDeform(Object3D* objPtr);

};

