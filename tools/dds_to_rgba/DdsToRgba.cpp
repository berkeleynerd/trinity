// Copyright (c) 2026 CCP Games

#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <CCPMemory.h>
#include <ICcpStream.h>
#include <HostBitmap.h>
#include <Tr2DdsHandler.h>

#define BCDEC_IMPLEMENTATION
#include <bcdec.h>

namespace
{

class MemoryReadStream : public ICcpStream
{
public:
	explicit MemoryReadStream( std::vector<uint8_t> bytes ) :
		m_bytes( std::move( bytes ) )
	{
	}

	ptrdiff_t Read( void* destination, ptrdiff_t count ) override
	{
		if( count <= 0 )
		{
			return 0;
		}
		const size_t available = m_position < m_bytes.size() ? m_bytes.size() - m_position : 0;
		const size_t requested = static_cast<size_t>( count );
		const size_t readSize = std::min( available, requested );
		std::memcpy( destination, m_bytes.data() + m_position, readSize );
		m_position += readSize;
		return static_cast<ptrdiff_t>( readSize );
	}

	ptrdiff_t Write( const void*, size_t ) override
	{
		return 0;
	}

	ptrdiff_t Seek( ptrdiff_t distance, SeekOrigin origin ) override
	{
		ptrdiff_t base = 0;
		if( origin == SO_CURRENT )
		{
			base = static_cast<ptrdiff_t>( m_position );
		}
		else if( origin == SO_END )
		{
			base = static_cast<ptrdiff_t>( m_bytes.size() );
		}
		const ptrdiff_t target = std::clamp<ptrdiff_t>( base + distance, 0, static_cast<ptrdiff_t>( m_bytes.size() ) );
		m_position = static_cast<size_t>( target );
		return target;
	}

	ptrdiff_t GetPosition() override
	{
		return static_cast<ptrdiff_t>( m_position );
	}

	ptrdiff_t GetSize() override
	{
		return static_cast<ptrdiff_t>( m_bytes.size() );
	}

private:
	std::vector<uint8_t> m_bytes;
	size_t m_position = 0;
};

bool LoadFile( const std::string& path, std::vector<uint8_t>& bytes )
{
	std::ifstream input( path, std::ios::binary | std::ios::ate );
	if( !input )
	{
		return false;
	}
	const std::streamsize size = input.tellg();
	if( size <= 0 )
	{
		return false;
	}
	input.seekg( 0, std::ios::beg );
	bytes.resize( static_cast<size_t>( size ) );
	return static_cast<bool>( input.read( reinterpret_cast<char*>( bytes.data() ), size ) );
}

uint32_t ReadUint32( const std::vector<uint8_t>& bytes, size_t offset )
{
	uint32_t value = 0;
	std::memcpy( &value, bytes.data() + offset, sizeof( value ) );
	return value;
}

enum class BcFormat
{
	None,
	Bc1,
	Bc2,
	Bc3,
	Bc4,
	Bc5,
	Bc7,
};

// The client mixes legacy-fourcc DDS (DXT1/DXT3/DXT5, ATI1/ATI2) with
// DX10-extension files carrying the same BC payloads; map both spellings.
BcFormat GetBcFormat( const std::vector<uint8_t>& bytes, size_t& dataOffset )
{
	constexpr uint32_t DDS_MAGIC = 0x20534444;
	constexpr uint32_t DDS_HEADER_SIZE = 124;
	constexpr uint32_t FOURCC_DXT1 = 0x31545844;
	constexpr uint32_t FOURCC_DXT3 = 0x33545844;
	constexpr uint32_t FOURCC_DXT5 = 0x35545844;
	constexpr uint32_t FOURCC_ATI1 = 0x31495441;
	constexpr uint32_t FOURCC_ATI2 = 0x32495441;
	constexpr uint32_t FOURCC_DX10 = 0x30315844;
	constexpr size_t DDS_PIXEL_FORMAT_FOURCC_OFFSET = 84;
	constexpr size_t DDS_DX10_FORMAT_OFFSET = 128;

	dataOffset = 128;
	if( bytes.size() < 128 || ReadUint32( bytes, 0 ) != DDS_MAGIC || ReadUint32( bytes, 4 ) != DDS_HEADER_SIZE )
	{
		return BcFormat::None;
	}
	const uint32_t fourcc = ReadUint32( bytes, DDS_PIXEL_FORMAT_FOURCC_OFFSET );
	if( fourcc == FOURCC_DXT1 )
	{
		return BcFormat::Bc1;
	}
	if( fourcc == FOURCC_DXT3 )
	{
		return BcFormat::Bc2;
	}
	if( fourcc == FOURCC_DXT5 )
	{
		return BcFormat::Bc3;
	}
	if( fourcc == FOURCC_ATI1 )
	{
		return BcFormat::Bc4;
	}
	if( fourcc == FOURCC_ATI2 )
	{
		return BcFormat::Bc5;
	}
	if( fourcc != FOURCC_DX10 || bytes.size() < 148 )
	{
		return BcFormat::None;
	}

	dataOffset = 148;
	switch( ReadUint32( bytes, DDS_DX10_FORMAT_OFFSET ) )
	{
	case 70: case 71: case 72:
		return BcFormat::Bc1;
	case 73: case 74: case 75:
		return BcFormat::Bc2;
	case 76: case 77: case 78:
		return BcFormat::Bc3;
	case 79: case 80:
		return BcFormat::Bc4;
	case 82: case 83:
		return BcFormat::Bc5;
	case 97: case 98: case 99:
		return BcFormat::Bc7;
	default:
		return BcFormat::None;
	}
}

bool DecodeBcDds( const std::vector<uint8_t>& bytes, BcFormat format, size_t dataOffset, uint32_t& width, uint32_t& height, std::vector<uint8_t>& rgba )
{
	const size_t blockSize = format == BcFormat::Bc1 || format == BcFormat::Bc4 ?
		BCDEC_BC1_BLOCK_SIZE : BCDEC_BC7_BLOCK_SIZE;
	width = ReadUint32( bytes, 16 );
	height = ReadUint32( bytes, 12 );
	if( width == 0 || height == 0 )
	{
		return false;
	}

	const size_t blocksWide = ( width + 3 ) / 4;
	const size_t blocksHigh = ( height + 3 ) / 4;
	const size_t requiredSize = dataOffset + blocksWide * blocksHigh * blockSize;
	if( requiredSize > bytes.size() )
	{
		return false;
	}

	rgba.resize( static_cast<size_t>( width ) * height * 4 );
	uint8_t blockPixels[4 * 4 * 4] = {};
	for( size_t blockY = 0; blockY < blocksHigh; ++blockY )
	{
		for( size_t blockX = 0; blockX < blocksWide; ++blockX )
		{
			const size_t blockIndex = blockY * blocksWide + blockX;
			const uint8_t* compressed = bytes.data() + dataOffset + blockIndex * blockSize;
			if( format == BcFormat::Bc1 )
			{
				bcdec_bc1( compressed, blockPixels, 4 * 4 );
			}
			else if( format == BcFormat::Bc2 )
			{
				bcdec_bc2( compressed, blockPixels, 4 * 4 );
			}
			else if( format == BcFormat::Bc3 )
			{
				bcdec_bc3( compressed, blockPixels, 4 * 4 );
			}
			else if( format == BcFormat::Bc4 )
			{
				uint8_t scalarPixels[4 * 4];
				bcdec_bc4( compressed, scalarPixels, 4 );
				for( size_t pixel = 0; pixel < 16; ++pixel )
				{
					blockPixels[pixel * 4 + 0] = scalarPixels[pixel];
					blockPixels[pixel * 4 + 1] = scalarPixels[pixel];
					blockPixels[pixel * 4 + 2] = scalarPixels[pixel];
					blockPixels[pixel * 4 + 3] = 255;
				}
			}
			else if( format == BcFormat::Bc5 )
			{
				uint8_t vectorPixels[4 * 4 * 2];
				bcdec_bc5( compressed, vectorPixels, 4 * 2 );
				for( size_t pixel = 0; pixel < 16; ++pixel )
				{
					blockPixels[pixel * 4 + 0] = vectorPixels[pixel * 2 + 0];
					blockPixels[pixel * 4 + 1] = vectorPixels[pixel * 2 + 1];
					blockPixels[pixel * 4 + 2] = 255;
					blockPixels[pixel * 4 + 3] = 255;
				}
			}
			else
			{
				bcdec_bc7( compressed, blockPixels, 4 * 4 );
			}
			for( size_t pixelY = 0; pixelY < 4 && blockY * 4 + pixelY < height; ++pixelY )
			{
				const size_t copyWidth = std::min<size_t>( 4, width - blockX * 4 );
				const size_t destinationOffset = ( ( blockY * 4 + pixelY ) * width + blockX * 4 ) * 4;
				std::memcpy( rgba.data() + destinationOffset, blockPixels + pixelY * 4 * 4, copyWidth * 4 );
			}
		}
	}
	return true;
}

bool ParseArgs( int argc, char** argv, std::string& input, std::string& output, std::string& metadata, bool& summary )
{
	for( int i = 1; i < argc; ++i )
	{
		const std::string arg = argv[i];
		if( arg == "--summary" )
		{
			summary = true;
		}
		else if( arg == "--input" && ++i < argc )
		{
			input = argv[i];
		}
		else if( arg == "--output" && ++i < argc )
		{
			output = argv[i];
		}
		else if( arg == "--metadata" && ++i < argc )
		{
			metadata = argv[i];
		}
		else
		{
			return false;
		}
	}
	return !input.empty() && !output.empty() && !metadata.empty();
}

}

int main( int argc, char** argv )
{
	std::string inputPath;
	std::string outputPath;
	std::string metadataPath;
	bool summary = false;
	if( !ParseArgs( argc, argv, inputPath, outputPath, metadataPath, summary ) )
	{
		std::cerr << "Usage: TrinityDdsToRgba --input texture.dds --output texture.rgba --metadata texture.txt [--summary]\n";
		return 2;
	}

	std::vector<uint8_t> bytes;
	if( !LoadFile( inputPath, bytes ) )
	{
		std::cerr << "Failed to read DDS file: " << inputPath << "\n";
		return 1;
	}

	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> rgba;
	size_t bcDataOffset = 128;
	const BcFormat bcFormat = GetBcFormat( bytes, bcDataOffset );
	if( bcFormat != BcFormat::None )
	{
		if( !DecodeBcDds( bytes, bcFormat, bcDataOffset, width, height, rgba ) )
		{
			std::cerr << "BC DDS decode failed\n";
			return 1;
		}
	}
	else
	{
		MemoryReadStream stream( std::move( bytes ) );
		ImageIO::HostBitmap bitmap;
		const ImageIO::LoadParameters parameters( L"SharedCache.dds", 0, 1 );
		const ImageIO::Result result = ImageIO::Dds::ReadImage( stream, parameters, bitmap, nullptr );
		if( !result )
		{
			std::cerr << "DDS decode failed: " << result.GetErrorMessage() << "\n";
			return 1;
		}
		if( !bitmap.ConvertFormat( ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM ) )
		{
			std::cerr << "DDS conversion to RGBA8 failed\n";
			return 1;
		}
		width = bitmap.GetWidth();
		height = bitmap.GetHeight();
		rgba.resize( static_cast<size_t>( width ) * height * 4 );
		const size_t rowBytes = static_cast<size_t>( width ) * 4;
		for( uint32_t y = 0; y < height; ++y )
		{
			std::memcpy( rgba.data() + static_cast<size_t>( y ) * rowBytes, bitmap.GetMipRawData( 0 ) + static_cast<size_t>( y ) * bitmap.GetPitch(), rowBytes );
		}
	}

	std::ofstream output( outputPath, std::ios::binary );
	if( !output )
	{
		std::cerr << "Failed to open output file: " << outputPath << "\n";
		return 1;
	}
	output.write( reinterpret_cast<const char*>( rgba.data() ), static_cast<std::streamsize>( rgba.size() ) );
	if( !output )
	{
		std::cerr << "Failed to write RGBA output: " << outputPath << "\n";
		return 1;
	}

	std::ofstream metadata( metadataPath );
	metadata << width << ' ' << height << '\n';
	if( !metadata )
	{
		std::cerr << "Failed to write metadata output: " << metadataPath << "\n";
		return 1;
	}
	if( summary )
	{
		std::cout << "Wrote " << outputPath << "\n";
		std::cout << "Dimensions: " << width << 'x' << height << "\n";
	}
	return 0;
}
