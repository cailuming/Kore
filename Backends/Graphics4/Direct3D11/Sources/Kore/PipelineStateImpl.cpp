#include "pch.h"

#include "Direct3D11.h"
#include "PipelineStateImpl.h"

#include <Kore/Graphics4/PipelineState.h>
#include <Kore/Graphics4/Shader.h>
#include <Kore/SystemMicrosoft.h>
#include <Kore/Log.h>

#include <malloc.h>

using namespace Kore;
using namespace Kore::Graphics4;

namespace Kore {
	PipelineState* currentPipeline = nullptr;

	D3D11_CULL_MODE convert(Graphics4::CullMode cullMode) {
		switch (cullMode) {
		case Graphics4::Clockwise:
			return D3D11_CULL_FRONT;
		case Graphics4::CounterClockwise:
			return D3D11_CULL_BACK;
		case Graphics4::NoCulling:
		default:
			return D3D11_CULL_NONE;
		}
	}

	D3D11_BLEND convert(Graphics4::BlendingOperation operation) {
		switch (operation) {
		case Graphics4::BlendOne:
			return D3D11_BLEND_ONE;
		case Graphics4::BlendZero:
			return D3D11_BLEND_ZERO;
		case Graphics4::SourceAlpha:
			return D3D11_BLEND_SRC_ALPHA;
		case Graphics4::DestinationAlpha:
			return D3D11_BLEND_DEST_ALPHA;
		case Graphics4::InverseSourceAlpha:
			return D3D11_BLEND_INV_SRC_ALPHA;
		case Graphics4::InverseDestinationAlpha:
			return D3D11_BLEND_INV_DEST_ALPHA;
		case Graphics4::SourceColor:
			return D3D11_BLEND_SRC_COLOR;
		case Graphics4::DestinationColor:
			return D3D11_BLEND_DEST_COLOR;
		case Graphics4::InverseSourceColor:
			return D3D11_BLEND_INV_SRC_COLOR;
		case Graphics4::InverseDestinationColor:
			return D3D11_BLEND_INV_DEST_COLOR;
		default:
			//	throw Exception("Unknown blending operation.");
			return D3D11_BLEND_SRC_ALPHA;
		}
	}

	D3D11_COMPARISON_FUNC getComparison(Graphics4::ZCompareMode compare) {
		switch (compare) {
		default:
		case Graphics4::ZCompareAlways:
			return D3D11_COMPARISON_ALWAYS;
		case Graphics4::ZCompareNever:
			return D3D11_COMPARISON_NEVER;
		case Graphics4::ZCompareEqual:
			return D3D11_COMPARISON_EQUAL;
		case Graphics4::ZCompareNotEqual:
			return D3D11_COMPARISON_NOT_EQUAL;
		case Graphics4::ZCompareLess:
			return D3D11_COMPARISON_LESS;
		case Graphics4::ZCompareLessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
		case Graphics4::ZCompareGreater:
			return D3D11_COMPARISON_GREATER;
		case Graphics4::ZCompareGreaterEqual:
			return D3D11_COMPARISON_GREATER_EQUAL;
		}
	}

	D3D11_STENCIL_OP getStencilAction(Graphics4::StencilAction action) {
		switch (action) {
		default:
		case Graphics4::Keep:
			return D3D11_STENCIL_OP_KEEP;
		case Graphics4::Zero:
			return D3D11_STENCIL_OP_ZERO;
		case Graphics4::Replace:
			return D3D11_STENCIL_OP_REPLACE;
		case Graphics4::Increment:
			return D3D11_STENCIL_OP_INCR;
		case Graphics4::IncrementWrap:
			return D3D11_STENCIL_OP_INCR_SAT;
		case Graphics4::Decrement:
			return D3D11_STENCIL_OP_DECR;
		case Graphics4::DecrementWrap:
			return D3D11_STENCIL_OP_DECR_SAT;
		case Graphics4::Invert:
			return D3D11_STENCIL_OP_INVERT;
		}
	}
}

void PipelineStateImpl::setConstants() {
	if (currentPipeline->vertexShader->constantsSize > 0) {
		context->UpdateSubresource(currentPipeline->vertexConstantBuffer, 0, nullptr, vertexConstants, 0, 0);
		context->VSSetConstantBuffers(0, 1, &currentPipeline->vertexConstantBuffer);
	}
	if (currentPipeline->fragmentShader->constantsSize > 0) {
		context->UpdateSubresource(currentPipeline->fragmentConstantBuffer, 0, nullptr, fragmentConstants, 0, 0);
		context->PSSetConstantBuffers(0, 1, &currentPipeline->fragmentConstantBuffer);
	}
	if (currentPipeline->geometryShader != nullptr && currentPipeline->geometryShader->constantsSize > 0) {
		context->UpdateSubresource(currentPipeline->geometryConstantBuffer, 0, nullptr, geometryConstants, 0, 0);
		context->GSSetConstantBuffers(0, 1, &currentPipeline->geometryConstantBuffer);
	}
	if (currentPipeline->tessellationControlShader != nullptr && currentPipeline->tessellationControlShader->constantsSize > 0) {
		context->UpdateSubresource(currentPipeline->tessControlConstantBuffer, 0, nullptr, tessControlConstants, 0, 0);
		context->HSSetConstantBuffers(0, 1, &currentPipeline->tessControlConstantBuffer);
	}
	if (currentPipeline->tessellationEvaluationShader != nullptr && currentPipeline->tessellationEvaluationShader->constantsSize > 0) {
		context->UpdateSubresource(currentPipeline->tessEvalConstantBuffer, 0, nullptr, tessEvalConstants, 0, 0);
		context->DSSetConstantBuffers(0, 1, &currentPipeline->tessEvalConstantBuffer);
	}
}

PipelineStateImpl::PipelineStateImpl() : d3d11inputLayout(nullptr), fragmentConstantBuffer(nullptr), vertexConstantBuffer(nullptr), geometryConstantBuffer(nullptr), tessEvalConstantBuffer(nullptr), tessControlConstantBuffer(nullptr),
	depthStencilState(nullptr), rasterizerState(nullptr), rasterizerStateScissor(nullptr), blendState(nullptr) {}

PipelineStateImpl::~PipelineStateImpl() {
	if (d3d11inputLayout != nullptr) d3d11inputLayout->Release();
	if (fragmentConstantBuffer != nullptr) fragmentConstantBuffer->Release();
	if (vertexConstantBuffer != nullptr) vertexConstantBuffer->Release();
	if (geometryConstantBuffer != nullptr) geometryConstantBuffer->Release();
	if (tessEvalConstantBuffer != nullptr) tessEvalConstantBuffer->Release();
	if (tessControlConstantBuffer != nullptr) tessControlConstantBuffer->Release();
	if (depthStencilState != nullptr) depthStencilState->Release();
	if (rasterizerState != nullptr) rasterizerState->Release();
	if (rasterizerStateScissor != nullptr) rasterizerStateScissor->Release();
	if (blendState != nullptr) blendState->Release();
}

void PipelineStateImpl::set(PipelineState* pipeline, bool scissoring) {
	currentPipeline = pipeline;

	context->OMSetDepthStencilState(depthStencilState, pipeline->stencilReferenceValue);
	float blendFactor[] = {0, 0, 0, 0};
	UINT sampleMask = 0xffffffff;
	context->OMSetBlendState(blendState, blendFactor, sampleMask);
	setRasterizerState(scissoring);

	context->VSSetShader((ID3D11VertexShader*)pipeline->vertexShader->shader, nullptr, 0);
	context->PSSetShader((ID3D11PixelShader*)pipeline->fragmentShader->shader, nullptr, 0);
	
	context->GSSetShader(pipeline->geometryShader != nullptr ? (ID3D11GeometryShader*) pipeline->geometryShader->shader : nullptr, nullptr, 0);
	context->HSSetShader(pipeline->tessellationControlShader != nullptr ? (ID3D11HullShader*) pipeline->tessellationControlShader->shader : nullptr, nullptr, 0);
	context->DSSetShader(pipeline->tessellationEvaluationShader != nullptr ? (ID3D11DomainShader*) pipeline->tessellationEvaluationShader->shader : nullptr, nullptr, 0);		

	context->IASetInputLayout(pipeline->d3d11inputLayout);
}

void PipelineStateImpl::setRasterizerState(bool scissoring) {
	if (scissoring && rasterizerStateScissor != nullptr)
		context->RSSetState(rasterizerStateScissor);
	else if (rasterizerState != nullptr)
		context->RSSetState(rasterizerState);
}

Graphics4::ConstantLocation Graphics4::PipelineState::getConstantLocation(const char* name) {
	ConstantLocation location;

	if (vertexShader->constants.find(name) == vertexShader->constants.end()) {
		location.vertexOffset = 0;
		location.vertexSize = 0;
		location.vertexColumns = 0;
		location.vertexRows = 0;
	}
	else {
		ShaderConstant constant = vertexShader->constants[name];
		location.vertexOffset = constant.offset;
		location.vertexSize = constant.size;
		location.vertexColumns = constant.columns;
		location.vertexRows = constant.rows;
	}

	if (fragmentShader->constants.find(name) == fragmentShader->constants.end()) {
		location.fragmentOffset = 0;
		location.fragmentSize = 0;
		location.fragmentColumns = 0;
		location.fragmentRows = 0;
	}
	else {
		ShaderConstant constant = fragmentShader->constants[name];
		location.fragmentOffset = constant.offset;
		location.fragmentSize = constant.size;
		location.fragmentColumns = constant.columns;
		location.fragmentRows = constant.rows;
	}

	if (geometryShader == nullptr || geometryShader->constants.find(name) == geometryShader->constants.end()) {
		location.geometryOffset = 0;
		location.geometrySize = 0;
		location.geometryColumns = 0;
		location.geometryRows = 0;
	}
	else {
		ShaderConstant constant = geometryShader->constants[name];
		location.geometryOffset = constant.offset;
		location.geometrySize = constant.size;
		location.geometryColumns = constant.columns;
		location.geometryRows = constant.rows;
	}

	if (tessellationControlShader == nullptr || tessellationControlShader->constants.find(name) == tessellationControlShader->constants.end()) {
		location.tessControlOffset = 0;
		location.tessControlSize = 0;
		location.tessControlColumns = 0;
		location.tessControlRows = 0;
	}
	else {
		ShaderConstant constant = tessellationControlShader->constants[name];
		location.tessControlOffset = constant.offset;
		location.tessControlSize = constant.size;
		location.tessControlColumns = constant.columns;
		location.tessControlRows = constant.rows;
	}

	if (tessellationEvaluationShader == nullptr || tessellationEvaluationShader->constants.find(name) == tessellationEvaluationShader->constants.end()) {
		location.tessEvalOffset = 0;
		location.tessEvalSize = 0;
		location.tessEvalColumns = 0;
		location.tessEvalRows = 0;
	}
	else {
		ShaderConstant constant = tessellationEvaluationShader->constants[name];
		location.tessEvalOffset = constant.offset;
		location.tessEvalSize = constant.size;
		location.tessEvalColumns = constant.columns;
		location.tessEvalRows = constant.rows;
	}

	if (location.vertexSize == 0 && location.fragmentSize == 0 && location.geometrySize == 0 && location.tessControlSize && location.tessEvalSize == 0) {
		log(Warning, "Uniform %s not found.", name);
	}

	return location;
}

Graphics4::TextureUnit Graphics4::PipelineState::getTextureUnit(const char* name) {
	char unitName[64];
	int unitOffset = 0;
	size_t len = strlen(name);
	if (len > 63) len = 63;
	strncpy(unitName, name, len + 1);
	if (unitName[len - 1] == ']') { // Check for array - mySampler[2]
		unitOffset = int(unitName[len - 2] - '0'); // Array index is unit offset
		unitName[len - 3] = 0; // Strip array from name
	}
	
	TextureUnit unit;
	if (vertexShader->textures.find(unitName) == vertexShader->textures.end()) {
		if (fragmentShader->textures.find(unitName) == fragmentShader->textures.end()) {
			unit.unit = -1;
#ifndef NDEBUG
			static int notFoundCount = 0;
			if (notFoundCount < 10) {
				log(Warning, "Sampler %s not found.", unitName);
				++notFoundCount;
			}
			else if (notFoundCount == 10) {
				log(Warning, "Giving up on sampler not found messages.", unitName);
				++notFoundCount;
			}
#endif
		}
		else {
			unit.unit = fragmentShader->textures[unitName] + unitOffset;
			unit.vertex = false;
		}
	}
	else {
		unit.unit = vertexShader->textures[unitName] + unitOffset;
		unit.vertex = true;
	}
	return unit;
}

namespace {
	char stringCache[1024];
	int stringCacheIndex = 0;

	int getMultipleOf16(int value) {
		int ret = 16;
		while (ret < value) ret += 16;
		return ret;
	}

	void setVertexDesc(D3D11_INPUT_ELEMENT_DESC& vertexDesc, int attributeIndex, int index, int stream, bool instanced, int subindex = -1) {
		if (subindex < 0) {
			vertexDesc.SemanticName = "TEXCOORD";
			vertexDesc.SemanticIndex = attributeIndex;
		}
		else {
			// SPIRV_CROSS uses TEXCOORD_0_0,... for split up matrices
			int stringStart = stringCacheIndex;
			strcpy(&stringCache[stringCacheIndex], "TEXCOORD");
			stringCacheIndex += (int)strlen("TEXCOORD");
			_itoa(attributeIndex, &stringCache[stringCacheIndex], 10);
			stringCacheIndex += (int)strlen(&stringCache[stringCacheIndex]);
			strcpy(&stringCache[stringCacheIndex], "_");
			stringCacheIndex += 2;
			vertexDesc.SemanticName = &stringCache[stringStart];
			vertexDesc.SemanticIndex = subindex;
		}
		vertexDesc.InputSlot = stream;
		vertexDesc.AlignedByteOffset = (index == 0) ? 0 : D3D11_APPEND_ALIGNED_ELEMENT;
		vertexDesc.InputSlotClass = instanced ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
		vertexDesc.InstanceDataStepRate = instanced ? 1 : 0;
	}
}

namespace {
	const int usedCount = 32;

	int getAttributeLocation(std::map<std::string, int>& attributes, const char* name, bool* used) {
		if (attributes.find(name) != attributes.end()) {
			return attributes[name];
		}
		else {
			for (int i = 0; i < usedCount; ++i) {
				if (!used[i]) {
					used[i] = true;
					return i;
				}
			}
		}
		return 0;
	}
}

void Graphics4::PipelineState::compile() {
	if (vertexShader->constantsSize > 0)
		Kore_Microsoft_affirm(device->CreateBuffer(&CD3D11_BUFFER_DESC(getMultipleOf16(vertexShader->constantsSize), D3D11_BIND_CONSTANT_BUFFER), nullptr,
		                                       &vertexConstantBuffer));
	if (fragmentShader->constantsSize > 0)
		Kore_Microsoft_affirm(device->CreateBuffer(&CD3D11_BUFFER_DESC(getMultipleOf16(fragmentShader->constantsSize), D3D11_BIND_CONSTANT_BUFFER), nullptr,
		                                       &fragmentConstantBuffer));
	if (geometryShader != nullptr && geometryShader->constantsSize > 0)
		Kore_Microsoft_affirm(device->CreateBuffer(&CD3D11_BUFFER_DESC(getMultipleOf16(geometryShader->constantsSize), D3D11_BIND_CONSTANT_BUFFER), nullptr,
		                                       &geometryConstantBuffer));
	if (tessellationControlShader != nullptr && tessellationControlShader->constantsSize > 0)
		Kore_Microsoft_affirm(device->CreateBuffer(&CD3D11_BUFFER_DESC(getMultipleOf16(tessellationControlShader->constantsSize), D3D11_BIND_CONSTANT_BUFFER),
		                                       nullptr, &tessControlConstantBuffer));
	if (tessellationEvaluationShader != nullptr && tessellationEvaluationShader->constantsSize > 0)
		Kore_Microsoft_affirm(device->CreateBuffer(&CD3D11_BUFFER_DESC(getMultipleOf16(tessellationEvaluationShader->constantsSize), D3D11_BIND_CONSTANT_BUFFER),
		                                       nullptr, &tessEvalConstantBuffer));

	int all = 0;
	for (int stream = 0; inputLayout[stream] != nullptr; ++stream) {
		for (int index = 0; index < inputLayout[stream]->size; ++index) {
			if (inputLayout[stream]->elements[index].data == Float4x4VertexData) {
				all += 4;
			}
			else {
				all += 1;
			}
		}
	}

	bool used[usedCount];
	for (int i = 0; i < usedCount; ++i) used[i] = false;
	for (auto attribute : vertexShader->attributes) {
		used[attribute.second] = true;
	}
	stringCacheIndex = 0;
	D3D11_INPUT_ELEMENT_DESC* vertexDesc = (D3D11_INPUT_ELEMENT_DESC*)alloca(sizeof(D3D11_INPUT_ELEMENT_DESC) * all);
	int i = 0;
	for (int stream = 0; inputLayout[stream] != nullptr; ++stream) {
		for (int index = 0; index < inputLayout[stream]->size; ++index) {
			switch (inputLayout[stream]->elements[index].data) {
			case Float1VertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R32_FLOAT;
				++i;
				break;
			case Float2VertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R32G32_FLOAT;
				++i;
				break;
			case Float3VertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R32G32B32_FLOAT;
				++i;
				break;
			case Float4VertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				++i;
				break;
			case Short2NormVertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R16G16_SNORM;
				++i;
				break;
			case Short4NormVertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R16G16B16A16_SNORM;
				++i;
				break;
			case ColorVertexData:
				setVertexDesc(vertexDesc[i], getAttributeLocation(vertexShader->attributes, inputLayout[stream]->elements[index].name, used), index, stream,
				              inputLayout[stream]->instanced);
				vertexDesc[i].Format = DXGI_FORMAT_R8G8B8A8_UINT;
				++i;
				break;
			case Float4x4VertexData:
				char name[101];
				strcpy(name, inputLayout[stream]->elements[index].name);
				strcat(name, "_");
				size_t length = strlen(name);
				_itoa(0, &name[length], 10);
				name[length + 1] = 0;
				int attributeLocation = getAttributeLocation(vertexShader->attributes, name, used);

				for (int i2 = 0; i2 < 4; ++i2) {
					setVertexDesc(vertexDesc[i], attributeLocation, index + i2, stream,
					              inputLayout[stream]->instanced, i2);
					vertexDesc[i].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					++i;
				}
				break;
			}
		}
	}

	Kore_Microsoft_affirm(device->CreateInputLayout(vertexDesc, all, vertexShader->data, vertexShader->length, &d3d11inputLayout));

	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.DepthEnable = depthMode != ZCompareAlways;
		desc.DepthWriteMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = getComparison(depthMode);

		desc.StencilEnable = stencilMode != ZCompareAlways;
		desc.StencilReadMask = stencilReadMask;
		desc.StencilWriteMask = stencilWriteMask;
		desc.FrontFace.StencilFunc = desc.BackFace.StencilFunc = getComparison(stencilMode);
		desc.FrontFace.StencilDepthFailOp = desc.BackFace.StencilDepthFailOp = getStencilAction(stencilDepthFail);
		desc.FrontFace.StencilPassOp = desc.BackFace.StencilPassOp = getStencilAction(stencilBothPass);
		desc.FrontFace.StencilFailOp = desc.BackFace.StencilFailOp = getStencilAction(stencilFail);

		device->CreateDepthStencilState(&desc, &depthStencilState);
	}

	{
		D3D11_RASTERIZER_DESC rasterDesc;
		rasterDesc.CullMode = convert(cullMode);
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		rasterDesc.FrontCounterClockwise = FALSE;
		rasterDesc.DepthBias = 0;
		rasterDesc.SlopeScaledDepthBias = 0.0f;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.ScissorEnable = FALSE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = FALSE;

		device->CreateRasterizerState(&rasterDesc, &rasterizerState);
		rasterDesc.ScissorEnable = TRUE;
		device->CreateRasterizerState(&rasterDesc, &rasterizerStateScissor);

		// We need d3d11_3 for conservative raster
		// D3D11_RASTERIZER_DESC2 rasterDesc;
		// rasterDesc.ConservativeRaster = conservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		// device->CreateRasterizerState2(&rasterDesc, &rasterizerState);
		// rasterDesc.ScissorEnable = TRUE;
		// device->CreateRasterizerState2(&rasterDesc, &rasterizerStateScissor);
	}

	{
		D3D11_RENDER_TARGET_BLEND_DESC rtbd;
		ZeroMemory(&rtbd, sizeof(rtbd));

		rtbd.BlendEnable = blendSource != Graphics4::BlendOne || blendDestination != Graphics4::BlendZero || alphaBlendSource != Graphics4::BlendOne ||
		                   alphaBlendDestination != Graphics4::BlendZero;
		rtbd.SrcBlend = convert(blendSource);
		rtbd.DestBlend = convert(blendDestination);
		rtbd.BlendOp = D3D11_BLEND_OP_ADD;
		rtbd.SrcBlendAlpha = convert(alphaBlendSource);
		rtbd.DestBlendAlpha = convert(alphaBlendDestination);
		rtbd.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtbd.RenderTargetWriteMask = (((colorWriteMaskRed[0] ? D3D11_COLOR_WRITE_ENABLE_RED : 0) |
		                               (colorWriteMaskGreen[0] ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0)) |
		                               (colorWriteMaskBlue[0] ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0)) |
		                               (colorWriteMaskAlpha[0] ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0);

		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));
		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.RenderTarget[0] = rtbd;
		device->CreateBlendState(&blendDesc, &blendState);
	}
}
