// Copyright (c) 2026 CCP Games

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct TrinityStandaloneCmfVertex
{
	float position[3];
	float normal[3];
	float tangent[4];
	float texcoord[2];
	float color[4];
};

struct TrinityStandaloneEveV5Vertex
{
	float position[3];
	uint16_t joints[4];
	float texcoord[2];
	float packedTangent[4];
};

struct TrinityStandaloneCmfTexture
{
	uint32_t width = 1;
	uint32_t height = 1;
	std::vector<uint8_t> rgba = { 255, 255, 255, 255 };
};

class TrinityStandaloneCmfModel
{
public:
	TrinityStandaloneCmfModel();
	~TrinityStandaloneCmfModel();

	bool Load( const std::string& path, std::string& error );
	uint32_t VertexCount() const;
	uint32_t IndexCount() const;
	const std::vector<TrinityStandaloneCmfVertex>& Vertices() const;
	const std::vector<TrinityStandaloneEveV5Vertex>& EveV5Vertices() const;
	const std::vector<uint32_t>& Indices() const;
	void GetCenterAndScale( float output[4] ) const;
	const TrinityStandaloneCmfTexture& BaseColorTexture() const;
	const TrinityStandaloneCmfTexture& NormalTexture() const;
	const TrinityStandaloneCmfTexture& RoughnessTexture() const;
	const TrinityStandaloneCmfTexture& MaterialTexture() const;
	const TrinityStandaloneCmfTexture& GlowTexture() const;
	const TrinityStandaloneCmfTexture& DirtTexture() const;
	const TrinityStandaloneCmfTexture& MaskTexture() const;
	const TrinityStandaloneCmfTexture& PaintMaskTexture() const;

private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};
