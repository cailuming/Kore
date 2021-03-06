#include "pch.h"

#include "Direct3D12.h"

#include <Kore/Graphics5/Shader.h>
#include <Kore/Math/Core.h>
#include <Kore/SystemMicrosoft.h>

using namespace Kore;

Shader5Impl::Shader5Impl() {}

Graphics5::Shader::Shader(void* _data, int length, ShaderType type) {
	unsigned index = 0;
	u8* data = (u8*)_data;

	int attributesCount = data[index++];
	for (int i = 0; i < attributesCount; ++i) {
		char name[256];
		for (unsigned i2 = 0; i2 < 255; ++i2) {
			name[i2] = data[index++];
			if (name[i2] == 0) break;
		}
		attributes[name] = data[index++];
	}

	u8 texCount = data[index++];
	for (unsigned i = 0; i < texCount; ++i) {
		char name[256];
		for (unsigned i2 = 0; i2 < 255; ++i2) {
			name[i2] = data[index++];
			if (name[i2] == 0) break;
		}
		textures[name] = data[index++];
	}

	u8 constantCount = data[index++];
	constantsSize = 0;
	for (unsigned i = 0; i < constantCount; ++i) {
		char name[256];
		for (unsigned i2 = 0; i2 < 255; ++i2) {
			name[i2] = data[index++];
			if (name[i2] == 0) break;
		}
		ShaderConstant constant;
		constant.offset = *(u32*)&data[index];
		index += 4;
		constant.size = *(u32*)&data[index];
		index += 4;
#ifdef KORE_WINDOWS
		index += 2; // columns and rows
#endif
		constants[name] = constant;
		constantsSize = constant.offset + constant.size;
	}

	this->length = length - index;
	this->data = new u8[this->length];
	memcpy(this->data, &data[index], this->length);

	switch (type) {
	case VertexShader:
		// Microsoft::affirm(device->CreateVertexShader(this->data, this->length, nullptr, (ID3D11VertexShader**)&shader));
		break;
	case FragmentShader:
		// Microsoft::affirm(device->CreatePixelShader(this->data, this->length, nullptr, (ID3D11PixelShader**)&shader));
		break;
	case GeometryShader:
		// Microsoft::affirm(device->CreateGeometryShader(this->data, this->length, nullptr, (ID3D11GeometryShader**)&shader));
		break;
	case TessellationControlShader:
		// Microsoft::affirm(device->CreateHullShader(this->data, this->length, nullptr, (ID3D11HullShader**)&shader));
		break;
	case TessellationEvaluationShader:
		// Microsoft::affirm(device->CreateDomainShader(this->data, this->length, nullptr, (ID3D11DomainShader**)&shader));
		break;
	}
}
