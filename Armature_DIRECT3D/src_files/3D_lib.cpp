//Copyright © 2023 by Pawel Oriol


//Biblioteka implementujaca system armatury wraz z rigiem mesha
//Wersja stabilna, ale prototypowa, niezoptymalizowana pod katem wydajnosci
//wczytuje armature i wagi mesha zapisane w formacie autorskim eskportowanego z blendera
//przy uzyciu wlasnego exportera mesh wczystywany jest z plikow w formacie *.obj
//Link do demonstracji dzialania programu:
//https://www.youtube.com/watch?v=yJpmEfbqp9k


#include "3D_lib.h"



void NormalizeVector(float* vSrc, float* vDst, int len)
{
	float length = 0;

	for (int i = 0; i < len; i++)
	{
		length += vSrc[i] * vSrc[i];
	}
	length = sqrt(length);

	for (int i = 0; i < len; i++)
	{
		vDst[i] = vSrc[i] / length;
	}
}

void AddVectors(float* v1, float* v2, float* result, int len)
{
	for (int i = 0; i < len; i++)
	{
		result[i] = v1[i] + v2[i];
	}
}

void SubVectors(float* v1, float* v2, float* result, int len)
{
	for (int i = 0; i < len; i++)
	{
		result[i] = v1[i] - v2[i];
	}
}

float DotVectors(float* u, float* v, int len)
{
	float result = 0;
	for (int vi = 0; vi < len; vi++)
	{
		result += u[vi] * v[vi];
	}

	return result;
}

void CrossVectors(float* u, float* v, float* result)
{
	result[0] = u[1] * v[2] - u[2] * v[1];
	result[1] = u[2] * v[0] - u[0] * v[2];
	result[2] = u[0] * v[1] - u[1] * v[0];
}

void ScaleVector(float* v, float scale, int len)
{
	for (int vi = 0; vi < len; vi++)
	{
		v[vi] *= scale;
	}

}

void Quaternion:: Init(float w, float x, float y, float z)
{
	this->w = w;
	this->x = x;
	this->y = y;
	this->z = z;
}

void Quaternion::InitAxisAngle(float* axis, float angle)
{
	float axis_norm[3];
	NormalizeVector(axis, axis_norm, 3);
	this->w = cosf(angle / 2.0f);
	this->x = axis_norm[0] * sinf(angle / 2.0f);
	this->y = axis_norm[1] * sinf(angle / 2.0f);
	this->z = axis_norm[2] * sinf(angle / 2.0f);
}

Quaternion Quaternion::Conjugation()
{
	Quaternion conj;
	conj.w = this->w;
	conj.x = -this->x;
	conj.y = -this->y;
	conj.z = -this->z;

	return conj;
}

float Quaternion::Norm()
{
	float norm = sqrt(pow(this->w, 2.0f) + pow(this->x, 2.0f) + pow(this->y, 2.0f) + pow(this->z, 2.0f));
	return norm;
}

void Quaternion::Normalize()
{
	float norm = this->Norm();
	this->w /= norm;
	this->x /= norm;
	this->y /= norm;
	this->z /= norm;

}

Quaternion  Quaternion::Reciprocal()
{
	Quaternion rec = this->Conjugation();
	float norm = this->Norm();
	rec.w /= norm;
	rec.x /= norm;
	rec.y /= norm;
	rec.z /= norm;
	return rec;
}


Quaternion HamiltonProd(Quaternion& q1, Quaternion& q2)
{
	Quaternion result;

	result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
	result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
	result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
	result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;

	return result;
}

void Rotate(Quaternion* q, float* vSrc, float* vDst)
{
	Quaternion recip = q->Reciprocal();

	Quaternion qv;
	qv.Init(0, vSrc[0], vSrc[1], vSrc[2]);

	Quaternion temp = HamiltonProd(*q, qv);
	temp = HamiltonProd(temp, recip);

	vDst[0] = temp.x;
	vDst[1] = temp.y;
	vDst[2] = temp.z;
}

float QuaternionDot(Quaternion* q1, Quaternion* q2)
{
	float result = q1->w * q2->w + q1->x * q2->x + q1->y * q2->y + q1->z * q2->z;

	return result;
}

Quaternion QuaternionSlerp(Quaternion* q1, Quaternion* q2, float t)
{
	float dot = QuaternionDot(q1, q2);
	if (dot > 1.0)
		dot = 1.0;
	float theta = acos(dot);

	if (theta == 0.0f)
	{
		theta = 0.0001f;
	}

	float sin_theta = sin(theta);
	float st = sin(t * theta);
	float st_1_minus_t = sin((1.0f - t) * theta);

	Quaternion result;
	result.w = q1->w * (st_1_minus_t / sin_theta) + q2->w * (st / sin_theta);
	result.x = q1->x * (st_1_minus_t / sin_theta) + q2->x * (st / sin_theta);
	result.y = q1->y * (st_1_minus_t / sin_theta) + q2->y * (st / sin_theta);
	result.z = q1->z * (st_1_minus_t / sin_theta) + q2->z * (st / sin_theta);
	//result.Normalize();

	return result;
}

void VertexSkinned:: SetVertices()
{
	for (int vi = 0; vi < this->vPointers.size(); vi++)
	{
		memcpy(this->vPointers[vi], this->posTrans, sizeof(float) * 3);
	}
}


void Load_Vertex_Groups(std::vector <VertexSkinned>& vSkinnedList, const char* fname)
{
	char line[256];
	FILE* file = fopen(fname, "r");


	//fgets(line, 256, file);

	char bone_name[256];
	float weight;
	fgets(line, 256, file);
	for (int vi = 0; vi < vSkinnedList.size(); vi++)
	{
		fgets(line, 256, file);
		fgets(line, 256, file);
		while (strncmp(line, "vertex", 5) != 0)
		{
			sscanf(line, "%s %f", bone_name, &weight);
			vSkinnedList[vi].vGroups.push_back(VertexGroup(bone_name, weight));
			if (fgets(line, 256, file) == NULL)
				break;;
		}
	}

	fclose(file);
}

Object3D::Object3D()
{
	this->vLocal = NULL;
	this->vTrans = NULL;
};
Object3D:: ~Object3D()
{
	if (this->vLocal != NULL)
	{
		free(this->vLocal);
		this->vLocal = NULL;
	}
	if (this->vTrans != NULL)
	{
		free(this->vTrans);
		this->vTrans = NULL;
	}
}
void Object3D::ReleaseD3D()
{
	if (this->dataBuffer != NULL)
	{
		this->dataBuffer->Release();
		this->dataBuffer = NULL;
	}

	if (this->objTexture != NULL)
	{
		this->objTexture->Release();
		this->objTexture = NULL;
	}
}


Object3D& Object3D:: operator=(Object3D&& other)
{
	this->vLocal = other.vLocal;
	other.vLocal = NULL;
	this->vTrans = other.vTrans;
	other.vTrans = NULL;

	this->vList = other.vList;
	this->normalList = other.normalList;
	this->normalListTrans = other.normalListTrans;
	this->indexList = other.indexList;
	this->numVertices = other.numVertices;

	this->dataBuffer = other.dataBuffer;
	other.dataBuffer = NULL;
	this->objTexture = other.objTexture;
	other.objTexture = NULL;

	return *this;
}
void Object3D::Load(ID3D11Device* devicePtr, const char* fname, bool vertexGroups, const char* vertexGroupsFname)
{

	int num_unique_vertices = 0;
	char line[256];
	char* line_ptr;

	FILE* file = fopen(fname, "r");

	line_ptr = fgets(line, 256, file);

	while (line[0] != 'v')
	{
		line_ptr = fgets(line, 256, file);
	}

	//////////////pos data ////////////////////////////////
	num_unique_vertices = 0;

	while (strncmp(line, "vt", 2) != 0)
	{
		this->vList.resize((num_unique_vertices + 1) * 3);
		sscanf(line, "%*s %f %f %f", &this->vList[num_unique_vertices * 3 + 0],
			&this->vList[num_unique_vertices * 3 + 1],
			&this->vList[num_unique_vertices * 3 + 2]);
		num_unique_vertices++;
		line_ptr = fgets(line, 256, file);
	}

	if (vertexGroups)
	{
		this->vSkinnedList.resize(this->vList.size() / 3);
		for (int vi = 0; vi < this->vSkinnedList.size(); vi++)
		{
			memcpy(this->vSkinnedList[vi].posLocal, &this->vList[vi * 3], sizeof(float) * 3);
		}
	}
	//////////////UV data ////////////////////////////////
	num_unique_vertices = 0;

	while (strncmp(line, "vn", 2) != 0)
	{
		this->UVList.resize((num_unique_vertices + 1) * 2);
		sscanf(line, "%*s %f %f", &this->UVList[num_unique_vertices * 2 + 0],
			&this->UVList[num_unique_vertices * 2 + 1]);
		num_unique_vertices++;
		line_ptr = fgets(line, 256, file);
	}

	//////////////normal data ////////////////////////////////
	num_unique_vertices = 0;

	while (strncmp(line, "s", 1) != 0)
	{
		this->normalList.resize((num_unique_vertices + 1) * 3);
		sscanf(line, "%*s %f %f %f", &this->normalList[num_unique_vertices * 3 + 0],
			&this->normalList[num_unique_vertices * 3 + 1],
			&this->normalList[num_unique_vertices * 3 + 2]);
		num_unique_vertices++;
		line_ptr = fgets(line, 256, file);
	}

	this->normalListTrans.resize(this->normalList.size());

	//////////////face data ////////////////////////////////

	//wczytaj pierwsza linie
	line_ptr = fgets(line, 256, file);

	int num_polys = 0;
	while (line_ptr != NULL)
	{
		this->indexList.resize((num_polys + 1) * 9);
		for (int i = 0; i < strlen(line); i++)
		{
			if (line[i] == '/')
			{
				line[i] = ' ';
			}
		}
		sscanf(line, "%*s %d %d %d %d %d %d  %d %d %d",
			&this->indexList[num_polys * 9 + 0],
			&this->indexList[num_polys * 9 + 1],
			&this->indexList[num_polys * 9 + 2],
			&this->indexList[num_polys * 9 + 3],
			&this->indexList[num_polys * 9 + 4],
			&this->indexList[num_polys * 9 + 5],
			&this->indexList[num_polys * 9 + 6],
			&this->indexList[num_polys * 9 + 7],
			&this->indexList[num_polys * 9 + 8]);
		num_polys++;
		line_ptr = fgets(line, 256, file);
	}

	//odejmij 1 od kazdego indeksu, jako, ze blender numeruje od jedynki
	for (int vi = 0; vi < num_polys * 3 * 3; vi++)
	{
		this->indexList[vi] -= 1;
	}

	vLocal = (Vertex*)malloc(sizeof(float) * 8 * 3 * num_polys);
	vTrans = (Vertex*)malloc(sizeof(float) * 8 * 3 * num_polys);

	this->numVertices = num_polys * 3;
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		int index_pos = this->indexList[vi * 3 + 0];
		int index_UV = this->indexList[vi * 3 + 1];
		int index_normal = this->indexList[vi * 3 + 2];

		this->vLocal[vi] = Vertex(this->vList[index_pos * 3 + 0], this->vList[index_pos * 3 + 1], this->vList[index_pos * 3 + 2],
			this->UVList[index_UV * 2 + 0], this->UVList[index_UV * 2 + 1],
			this->normalList[index_normal * 3 + 0], this->normalList[index_normal * 3 + 1], this->normalList[index_normal * 3 + 2]
		);
		this->vTrans[vi] = Vertex(this->vList[index_pos * 3 + 0], this->vList[index_pos * 3 + 1], this->vList[index_pos * 3 + 2],
			this->UVList[index_UV * 2 + 0], this->UVList[index_UV * 2 + 1],
			this->normalList[index_normal * 3 + 0], this->normalList[index_normal * 3 + 1], this->normalList[index_normal * 3 + 2]
		);

		if (vertexGroups)
		{
			this->vSkinnedList[index_pos].vPointers.push_back(&this->vTrans[vi].pos);
		}

	}
	if (vertexGroups)
		Load_Vertex_Groups(this->vSkinnedList, vertexGroupsFname);


	fclose(file);

	this->dataBuffer = CreateVertexBuffer(devicePtr, (unsigned char*)this->vTrans, sizeof(float) * 8 * this->numVertices);

}

void Object3D::LoadTexture(ID3D11Device* devicePtr, ID3D11DeviceContext* devConPtr, const wchar_t* fname)
{
	HRESULT hr = CreateWICTextureFromFileEx(devicePtr, devConPtr, fname, 0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, WIC_LOADER_IGNORE_SRGB,
		(ID3D11Resource**)NULL, &this->objTexture);

}



void Object3D::TranslateByVector(float* vec)
{
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		this->vTrans[vi].pos.x = this->vLocal[vi].pos.x + vec[0];
		this->vTrans[vi].pos.y = this->vLocal[vi].pos.y + vec[1];
		this->vTrans[vi].pos.z = this->vLocal[vi].pos.z + vec[2];
	}
}

//obrot obiektu o kwaternion
void Object3D::RotateByQuaternion(Quaternion* q)
{
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		Rotate(q, &this->vLocal[vi].pos.x, &vTrans[vi].pos.x);
	}

}
//translacja obiektu o wektor oraz obrot obiektu o kwaternion
void Object3D::RotateAndTranslate(Quaternion* q, float* vec)
{
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		Rotate(q, &this->vLocal[vi].pos.x, &vTrans[vi].pos.x);

		this->vTrans[vi].pos.x = this->vTrans[vi].pos.x + vec[0];
		this->vTrans[vi].pos.y = this->vTrans[vi].pos.y + vec[1];
		this->vTrans[vi].pos.z = this->vTrans[vi].pos.z + vec[2];

	}
}


//skalowanie obiektu
void Object3D::Scale(float scale_factor)
{
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		this->vLocal[vi].pos.x *= scale_factor;
		this->vLocal[vi].pos.y *= scale_factor;
		this->vLocal[vi].pos.z *= scale_factor;

	}
}

//metoda liczaca wektory normalne na nowo
void Object3D::RecalculateNormals()
{
	//petla po trojkatach
	for (int pi = 0; pi < this->numVertices / 3; pi++)
	{
		float u[3], v[3], normal_temp[3];
		int normal_index;

		//pobierz vertexy
		float v0[3];
		float v1[3];
		float v2[3];

		/*memcpy(v0, &this->v_local[pi * 3 + 0].pos.x, sizeof(float) * 3);
		memcpy(v1, &this->v_local[pi * 3 + 1].pos.x, sizeof(float) * 3);
		memcpy(v2, &this->v_local[pi * 3 + 2].pos.x, sizeof(float) * 3);*/

		memcpy(v0, &this->vTrans[pi * 3 + 0].pos.x, sizeof(float) * 3);
		memcpy(v1, &this->vTrans[pi * 3 + 1].pos.x, sizeof(float) * 3);
		memcpy(v2, &this->vTrans[pi * 3 + 2].pos.x, sizeof(float) * 3);

		//pierwszy vertex (v0)
		SubVectors(v0, v2, u, 3);
		SubVectors(v0, v1, v, 3);

		NormalizeVector(u, u, 3);
		NormalizeVector(v, v, 3);

		//normalny tylko raz trzeba policzyc, i tylko normalziowac i skalowac od nowa, ale na razie liczymy od nowa
		CrossVectors(v, u, normal_temp);
		NormalizeVector(normal_temp, normal_temp, 3);

		float dot_u_v = DotVectors(u, v, 3);
		float alpha = acos(dot_u_v);

		ScaleVector(normal_temp, alpha, 3);

		normal_index = this->indexList[pi * 9 + 2 + 0];

		AddVectors(&this->normalListTrans[normal_index * 3], normal_temp, &this->normalListTrans[normal_index * 3], 3);


		//drugi vertex (v1)
		SubVectors(v1, v0, u, 3);
		SubVectors(v1, v2, v, 3);

		NormalizeVector(u, u, 3);
		NormalizeVector(v, v, 3);

		//normalny tylko raz trzeba policzyc, i tylko normalziowac i skalowac od nowa, ale na razie liczymy od nowa
		CrossVectors(v, u, normal_temp);
		NormalizeVector(normal_temp, normal_temp, 3);

		dot_u_v = DotVectors(u, v, 3);
		alpha = acos(dot_u_v);

		ScaleVector(normal_temp, alpha, 3);

		normal_index = this->indexList[pi * 9 + 2 + 3];

		AddVectors(&this->normalListTrans[normal_index * 3], normal_temp, &this->normalListTrans[normal_index * 3], 3);

		//trzeci vertex (v2)
		SubVectors(v2, v1, u, 3);
		SubVectors(v2, v0, v, 3);

		NormalizeVector(u, u, 3);
		NormalizeVector(v, v, 3);

		//normalny tylko raz trzeba policzyc, i tylko normalziowac i skalowac od nowa, ale na razie liczymy od nowa
		CrossVectors(v, u, normal_temp);
		NormalizeVector(normal_temp, normal_temp, 3);

		dot_u_v = DotVectors(u, v, 3);
		alpha = acos(dot_u_v);

		ScaleVector(normal_temp, alpha, 3);

		normal_index = this->indexList[pi * 9 + 2 + 6];

		AddVectors(&this->normalListTrans[normal_index * 3], normal_temp, &this->normalListTrans[normal_index * 3], 3);
	}

	//znormalizuj wszystkie wektory
	for (int ni = 0; ni < this->normalListTrans.size() / 3; ni++)
	{
		NormalizeVector(&this->normalListTrans[ni * 3], &this->normalListTrans[ni * 3], 3);
	}


	//ustaw wektory na liscie do renderowania
	for (int vi = 0; vi < this->numVertices; vi++)
	{
		int normal_index = this->indexList[vi * 3 + 2];
		memcpy(&this->vTrans[vi].normal, &this->normalListTrans[normal_index * 3], sizeof(float) * 3);

	}

}

void Object3D::DrawObject(ID3D11DeviceContext* devConPtr)
{
	UINT  stride = sizeof(float) * 8;
	UINT  offset = 0;

	devConPtr->UpdateSubresource(this->dataBuffer, 0, NULL, this->vTrans, 0, 0);
	devConPtr->IASetVertexBuffers(0, 1, &this->dataBuffer, &stride, &offset);
	devConPtr->Draw(this->numVertices, 0);
}




Bone::Bone(Bone&& other)
{
	this->name = other.name;
	this->parentName = other.parentName;
	this->parent = other.parent;
	this->size = other.size;

	this->qLocal = other.qLocal;
	memcpy(this->posLocal, other.posLocal, sizeof(float) * 3);

	this->qBasis = other.qBasis;
	memcpy(this->posBasis, other.posBasis, sizeof(float) * 3);

	this->qBasisCurrent = other.qBasisCurrent;
	memcpy(this->posBasisCurrent, other.posBasisCurrent, sizeof(float) * 3);

	this->numFrames = other.numFrames;
	this->frameList = other.frameList;

	this->object3d = std::move(other.object3d);
}


//liczenie finalnej orientacji wszystkich kosci
//algorytm przebiega tak, ze najpierw kosc jest transformowana o
//transformate basis, nastepnie local, nastepnie liczona jest jej pozycja
//wzgledem rodzica i na koncu jest transformowana o pozycje ostateczna rodzica
//ktora dopiero musimy rekursywnie policzyc
TransformPair Bone::ComputeFinalOrientationPos()
{
	Quaternion q_temp = HamiltonProd(this->qLocal, this->qBasisCurrent);
	float pos_temp[3];
	Rotate(&this->qLocal, this->posBasis, pos_temp);

	TransformPair result;
	AddVectors(pos_temp, this->posLocal, pos_temp, 3);

	if (this->parent == NULL)
	{
		result.orient = q_temp;
		memcpy(result.pos, pos_temp, sizeof(float) * 3);

		this->qFinal = result.orient;
		memcpy(this->posFinal, result.pos, sizeof(float) * 3);
		return result;
	}
	else
	{
		Quaternion parent_inverse_orient = this->parent->qLocal.Reciprocal();
		float parent_inverse_pos[] = { -this->parent->posLocal[0], -this->parent->posLocal[1], -this->parent->posLocal[2] };

		TransformPair parent_final_transform = parent->ComputeFinalOrientationPos();

		q_temp = HamiltonProd(parent_inverse_orient, q_temp);

		result.orient = HamiltonProd(parent_final_transform.orient, q_temp);

		AddVectors(pos_temp, parent_inverse_pos, pos_temp, 3);

		Rotate(&parent_inverse_orient, pos_temp, pos_temp);
		Rotate(&parent_final_transform.orient, pos_temp, pos_temp);

		AddVectors(pos_temp, parent_final_transform.pos, result.pos, 3);

		this->qFinal = result.orient;
		memcpy(this->posFinal, result.pos, sizeof(float) * 3);

		return result;
	}

}

//Transformacja wierzcholka o przez kosc.
//Najpierw liczona jest pozycja wierzcholka wzgledem kosci, gdy ta jest
//w spoczynku, a nastepnie owa pozycja jest transformowana o pozycje finalna kosci
void Bone::TransformVertexByBone(float* vSrc, float* vDst)
{
	float v_temp[3];
	SubVectors(vSrc, this->posLocal, v_temp, 3);
	Quaternion q_temp = this->qLocal.Reciprocal();
	Rotate(&q_temp, v_temp, v_temp);
	Rotate(&this->qFinal, v_temp, v_temp);
	AddVectors(v_temp, this->posFinal, vDst, 3);
}




void Armature::ReleaseD3D()
{
	for (int bi = 0; bi < this->boneList.size(); bi++)
	{
		this->boneList[bi].object3d.ReleaseD3D();
	}

}

void Armature::Load(ID3D11Device* devicePtr, const char* filename, const char* modelFilename, bool anim)
{
	char line[256];
	FILE* f = fopen(filename, "r");

	fgets(line, 256, f);
	sscanf(line, "%*s %d", &this->numBones);

	//petla wczytujaca kosci
	for (int bi = 0; bi < this->numBones; bi++)
	{

		boneList.push_back(Bone());
		Bone& curr_bone = boneList.back();

		//wczytaj model obj kosci
		curr_bone.object3d.Load(devicePtr, modelFilename);

		curr_bone.ID = bi;
		//pomin te linie oddzielajaca
		fgets(line, 256, f);

		//linia z nazwa kosci
		fgets(line, 256, f);

		char* start = std::find(line, line + strlen(line), ' ');
		start++;
		char* end = std::find(start, start + strlen(start), '/');

		curr_bone.name = std::string(start, end);

		//linia z nazwa rodzica
		fgets(line, 256, f);
		start = line + strlen("Parent_ID: ");

		curr_bone.parentName = std::string(start, strlen(start) - 1);


		//linia z rozmiarami kosci
		fgets(line, 256, f);
		sscanf(line, "%*s %f", &curr_bone.size);

		//przeskaluj model kosci o rozmiar
		curr_bone.object3d.Scale(curr_bone.size);

		//linia z orientacją lokalną
		fgets(line, 256, f);
		sscanf(line, "%*s %*s %f %f %f %f", &curr_bone.qLocal.w, &curr_bone.qLocal.x, &curr_bone.qLocal.y, &curr_bone.qLocal.z);

		//linia z pozycją lokalną
		fgets(line, 256, f);
		sscanf(line, "%*s %*s %f %f %f", &curr_bone.posLocal[0], &curr_bone.posLocal[1], &curr_bone.posLocal[2]);

		//linia z orientacją basis
		fgets(line, 256, f);
		sscanf(line, "%*s %*s %f %f %f %f", &curr_bone.qBasis.w, &curr_bone.qBasis.x, &curr_bone.qBasis.y, &curr_bone.qBasis.z);

		//linia z pozycją basis
		fgets(line, 256, f);
		sscanf(line, "%*s %*s %f %f %f", &curr_bone.posBasis[0], &curr_bone.posBasis[1], &curr_bone.posBasis[2]);


		//zamiana wspolrzednych z i y
		float temp_coord;
		temp_coord = curr_bone.qLocal.y;

		curr_bone.qLocal.y = curr_bone.qLocal.z;
		curr_bone.qLocal.z = -temp_coord;

		temp_coord = curr_bone.posLocal[1];
		curr_bone.posLocal[1] = curr_bone.posLocal[2];
		curr_bone.posLocal[2] = -temp_coord;

		temp_coord = curr_bone.qBasis.y;
		curr_bone.qBasis.y = curr_bone.qBasis.z;
		curr_bone.qBasis.z = -temp_coord;

		temp_coord = curr_bone.posBasis[1];
		curr_bone.posBasis[1] = curr_bone.posBasis[2];
		curr_bone.posBasis[2] = -temp_coord;


		
			
		if (anim)
		{

			//wspolrzedna w (tutaj ustalamy tez ilosc klatek)
			fgets(line, 256, f);
			fgets(line, 256, f);
			while (line[0] != 'x')
			{
				curr_bone.frameList.push_back(FRAME());
				FRAME& curr_frame = curr_bone.frameList.back();
				sscanf(line, "%f %*s %f", &curr_frame.numFrame, &curr_frame.orientation.w);
				fgets(line, 256, f);
			}

			//teraz wiemy juz ile frame'ow jest
			size_t temp_num_frames = curr_bone.frameList.size();

			this->lastFrame = curr_bone.frameList.back().numFrame;

			//wsporzedna "x"
			for (int fi = 0; fi < temp_num_frames; fi++)
			{
				FRAME& curr_frame = curr_bone.frameList[fi];
				fgets(line, 256, f);
				sscanf(line, "%f %*s %f", &curr_frame.numFrame, &curr_frame.orientation.x);
			}
			//pomin te linie
			fgets(line, 256, f);

			//wsporzedna "y"
			for (int fi = 0; fi < temp_num_frames; fi++)
			{
				FRAME& curr_frame = curr_bone.frameList[fi];
				fgets(line, 256, f);
				sscanf(line, "%f %*s %f", &curr_frame.numFrame, &curr_frame.orientation.y);
			}

			//pomin te linie
			fgets(line, 256, f);

			//wsporzedna "z"
			for (int fi = 0; fi < temp_num_frames; fi++)
			{
				FRAME& curr_frame = curr_bone.frameList[fi];
				fgets(line, 256, f);
				sscanf(line, "%f %*s %f", &curr_frame.numFrame, &curr_frame.orientation.z);
			}

			//zamiana y i z
			for (int fi = 0; fi < temp_num_frames; fi++)
			{
				temp_coord = curr_bone.frameList[fi].orientation.y;
				curr_bone.frameList[fi].orientation.y = curr_bone.frameList[fi].orientation.z;
				curr_bone.frameList[fi].orientation.z = -temp_coord;

			}

		}
		
	} 

	//ustaw hierarchie kosci przypisujac wskazniki polom rodzicow kazdej kosci
	for (int bi = 0; bi < this->numBones; bi++)
	{
		Bone& curr_bone = this->boneList[bi];
		curr_bone.parent = NULL;
		for (int bi2 = 0; bi2 < this->numBones; bi2++)
		{
			if (curr_bone.parentName == this->boneList[bi2].name)
			{
				curr_bone.parent = &this->boneList[bi2];
				break;
			}
		}

	}

}

void Armature::Animate(float progress)
{
	this->currFrame += progress;
	if (this->currFrame > this->lastFrame)
	{
		this->currFrame = 1;
	}
}

//Metoda liczaca aktualna wartosc tranformaty basis
//dla kazdej kosci znajdujemy dwie klatki pomiedzy ktorymi
//akurat znajduje sie licznik aktualnego frame'a
//i liczemy wartosc interpolowana miedzy tymi klatkami
//gdzie wspolczynnikiem interpolacji bedzie procentowa 
//odleglosc licznika od obu klatek
//dla translacyjnej czesci tranformaty uzywamy LERP-a,
//zas dla orientacyjnej - SLERP -a, jako, ze ten zachowuje
//uniformowe tempo rotacji
void Armature::ComputeCurrBasis()
{
	for (int bi = 0; bi < this->numBones; bi++)
	{
		Bone& curr_bone = this->boneList[bi];

		for (int fi = 0; fi < curr_bone.frameList.size(); fi++)
		{
			if (this->currFrame < curr_bone.frameList[fi].numFrame)
			{
				float frame_full_dist = curr_bone.frameList[fi].numFrame - curr_bone.frameList[fi - 1].numFrame;
				float frame_dist = this->currFrame - curr_bone.frameList[fi - 1].numFrame;
				float t = frame_dist / frame_full_dist;
				curr_bone.qBasisCurrent = QuaternionSlerp(&curr_bone.frameList[fi - 1].orientation, &curr_bone.frameList[fi].orientation, t);
				break;

			}
		}

	}

}

//metoda liczaca ostateczne orientacje i pozycje kosci
void Armature::ComputeFinalOrientationPos()
{
	for (int bi = 0; bi < this->numBones; bi++)
	{
		this->boneList[bi].ComputeFinalOrientationPos();
	}

}


void Armature::Draw(ID3D11DeviceContext* devConPtr)
{
	for (int bi = 0; bi < this->numBones; bi++)
	{
		this->boneList[bi].object3d.RotateAndTranslate(&this->boneList[bi].qLocal, this->boneList[bi].posLocal);
		this->boneList[bi].object3d.DrawObject(devConPtr);

	}
}

void Armature::DrawFinal(ID3D11DeviceContext* devConPtr)
{
	for (int bi = 0; bi < this->numBones; bi++)
	{
		this->boneList[bi].object3d.RotateAndTranslate(&this->boneList[bi].qFinal, this->boneList[bi].posFinal);
		this->boneList[bi].object3d.DrawObject(devConPtr);

	}
}


void Armature::AssignBoneIndicesToVertexGroups(Object3D* objPtr)
{
	for (int vi = 0; vi < objPtr->vSkinnedList.size(); vi++)
	{
		VertexSkinned& curr_vert = objPtr->vSkinnedList[vi];
		for (int gi = 0; gi < curr_vert.vGroups.size(); gi++)
		{
			VertexGroup& curr_v_group = curr_vert.vGroups[gi];

			for (int bi = 0; bi < this->boneList.size(); bi++)
			{
				if (this->boneList[bi].name == curr_v_group.boneName)
				{
					curr_v_group.boneIndex = bi;
				}
			}
		}
	}

}


//algorytm transformuje kazdy vertex o transformate "reverse local" kosci
//aby policzyc jego polozenie wzgledem niej gdy jest ona w "rest pose"
//a nastepnie transformuje o jej ostateczna transformate
//wszystkie transfomacje kosci ktore maja "udzialy w vertexie" zostaja finalnie zsumowane
//z uwzglednieniem wag - suma wazona
void Armature::MeshDeform(Object3D* objPtr)
{
	for (int vi = 0; vi < objPtr->vSkinnedList.size(); vi++)
	{
		VertexSkinned& curr_ver = objPtr->vSkinnedList[vi];

		float v_result[3];
		memset(v_result, 0, sizeof(float) * 3);
		for (int gi = 0; gi < curr_ver.vGroups.size(); gi++)
		{
			Bone& curr_bone = this->boneList[curr_ver.vGroups[gi].boneIndex];
			float v_temp[3];

			curr_bone.TransformVertexByBone(curr_ver.posLocal, v_temp);
			ScaleVector(v_temp, curr_ver.vGroups[gi].weight, 3);
			AddVectors(v_result, v_temp, v_result, 3);

		}
		memcpy(curr_ver.posTrans, v_result, sizeof(float) * 3);
		curr_ver.SetVertices();
	}


}