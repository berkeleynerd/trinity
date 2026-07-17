// Copyright (c) 2026 CCP Games

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <CCPMemory.h>
#include <ICcpStream.h>
#include <HostBitmap.h>
#include <Tr2PngHandler.h>

#define STB_DXT_IMPLEMENTATION
#include <stb_dxt.h>

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

struct Image
{
	uint32_t width = 0;
	uint32_t height = 0;
	std::vector<uint8_t> rgba;
};

bool LoadPng( const std::string& path, Image& image )
{
	std::vector<uint8_t> bytes;
	if( !LoadFile( path, bytes ) )
	{
		std::cerr << "Failed to read PNG file: " << path << "\n";
		return false;
	}
	MemoryReadStream stream( std::move( bytes ) );
	ImageIO::HostBitmap bitmap;
	const ImageIO::LoadParameters parameters( L"Modernized.png", 0, 1 );
	const ImageIO::Result result = ImageIO::Png::ReadImage( stream, parameters, bitmap, nullptr );
	if( !result )
	{
		std::cerr << "PNG decode failed: " << result.GetErrorMessage() << "\n";
		return false;
	}
	if( !bitmap.ConvertFormat( ImageIO::PIXEL_FORMAT_R8G8B8A8_UNORM ) )
	{
		std::cerr << "PNG conversion to RGBA8 failed\n";
		return false;
	}
	image.width = bitmap.GetWidth();
	image.height = bitmap.GetHeight();
	image.rgba.resize( static_cast<size_t>( image.width ) * image.height * 4 );
	const size_t rowBytes = static_cast<size_t>( image.width ) * 4;
	for( uint32_t y = 0; y < image.height; ++y )
	{
		std::memcpy( image.rgba.data() + static_cast<size_t>( y ) * rowBytes,
					 bitmap.GetMipRawData( 0 ) + static_cast<size_t>( y ) * bitmap.GetPitch(),
					 rowBytes );
	}
	return true;
}

bool LoadRawRgba( const std::string& rgbaPath, const std::string& metadataPath, Image& image )
{
	std::ifstream metadata( metadataPath );
	if( !( metadata >> image.width >> image.height ) || image.width == 0 || image.height == 0 )
	{
		std::cerr << "Failed to parse metadata (width height): " << metadataPath << "\n";
		return false;
	}
	if( !LoadFile( rgbaPath, image.rgba ) )
	{
		std::cerr << "Failed to read RGBA file: " << rgbaPath << "\n";
		return false;
	}
	const size_t expected = static_cast<size_t>( image.width ) * image.height * 4;
	if( image.rgba.size() != expected )
	{
		std::cerr << "RGBA payload is " << image.rgba.size() << " bytes; metadata implies " << expected << "\n";
		return false;
	}
	return true;
}

// Successive 2x2 box halving down to 1x1. The average is byte-space (not
// gamma-correct); acceptable for the A/B rig and flagged in the docs.
Image HalveImage( const Image& source )
{
	Image next;
	next.width = std::max( 1u, source.width / 2 );
	next.height = std::max( 1u, source.height / 2 );
	next.rgba.resize( static_cast<size_t>( next.width ) * next.height * 4 );
	for( uint32_t y = 0; y < next.height; ++y )
	{
		for( uint32_t x = 0; x < next.width; ++x )
		{
			const uint32_t x0 = std::min( x * 2, source.width - 1 );
			const uint32_t x1 = std::min( x * 2 + 1, source.width - 1 );
			const uint32_t y0 = std::min( y * 2, source.height - 1 );
			const uint32_t y1 = std::min( y * 2 + 1, source.height - 1 );
			for( uint32_t channel = 0; channel < 4; ++channel )
			{
				const uint32_t sum =
					source.rgba[( static_cast<size_t>( y0 ) * source.width + x0 ) * 4 + channel] +
					source.rgba[( static_cast<size_t>( y0 ) * source.width + x1 ) * 4 + channel] +
					source.rgba[( static_cast<size_t>( y1 ) * source.width + x0 ) * 4 + channel] +
					source.rgba[( static_cast<size_t>( y1 ) * source.width + x1 ) * 4 + channel];
				next.rgba[( static_cast<size_t>( y ) * next.width + x ) * 4 + channel] =
					static_cast<uint8_t>( ( sum + 2 ) / 4 );
			}
		}
	}
	return next;
}

bool HasTranslucency( const Image& image )
{
	for( size_t pixel = 3; pixel < image.rgba.size(); pixel += 4 )
	{
		if( image.rgba[pixel] != 255 )
		{
			return true;
		}
	}
	return false;
}

void ExtractBlock( const Image& image, uint32_t blockX, uint32_t blockY, uint8_t block[16 * 4] )
{
	for( uint32_t pixelY = 0; pixelY < 4; ++pixelY )
	{
		for( uint32_t pixelX = 0; pixelX < 4; ++pixelX )
		{
			const uint32_t sourceX = std::min( blockX * 4 + pixelX, image.width - 1 );
			const uint32_t sourceY = std::min( blockY * 4 + pixelY, image.height - 1 );
			std::memcpy( block + ( pixelY * 4 + pixelX ) * 4,
						 image.rgba.data() + ( static_cast<size_t>( sourceY ) * image.width + sourceX ) * 4,
						 4 );
		}
	}
}

void CompressLevel( const Image& image, bool withAlpha, std::vector<uint8_t>& out )
{
	const uint32_t blocksWide = ( image.width + 3 ) / 4;
	const uint32_t blocksHigh = ( image.height + 3 ) / 4;
	const size_t blockSize = withAlpha ? 16 : 8;
	const size_t base = out.size();
	out.resize( base + static_cast<size_t>( blocksWide ) * blocksHigh * blockSize );
	uint8_t block[16 * 4];
	for( uint32_t blockY = 0; blockY < blocksHigh; ++blockY )
	{
		for( uint32_t blockX = 0; blockX < blocksWide; ++blockX )
		{
			ExtractBlock( image, blockX, blockY, block );
			uint8_t* destination = out.data() + base +
				( static_cast<size_t>( blockY ) * blocksWide + blockX ) * blockSize;
			stb_compress_dxt_block( destination, block, withAlpha ? 1 : 0, STB_DXT_HIGHQUAL );
		}
	}
}

void AppendUint32( std::vector<uint8_t>& out, uint32_t value )
{
	const size_t base = out.size();
	out.resize( base + 4 );
	std::memcpy( out.data() + base, &value, 4 );
}

// Legacy DDS header (no DX10 extension), matching the client's original
// DXT1/DXT5 ship textures so the probe's loader treats both sets alike.
void WriteHeader( std::vector<uint8_t>& out, const Image& top, uint32_t mipCount, bool compressed, bool withAlpha )
{
	constexpr uint32_t DDSD_CAPS = 0x1;
	constexpr uint32_t DDSD_HEIGHT = 0x2;
	constexpr uint32_t DDSD_WIDTH = 0x4;
	constexpr uint32_t DDSD_PITCH = 0x8;
	constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
	constexpr uint32_t DDSD_MIPMAPCOUNT = 0x20000;
	constexpr uint32_t DDSD_LINEARSIZE = 0x80000;
	constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
	constexpr uint32_t DDPF_FOURCC = 0x4;
	constexpr uint32_t DDPF_RGB = 0x40;
	constexpr uint32_t DDSCAPS_COMPLEX = 0x8;
	constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;
	constexpr uint32_t DDSCAPS_MIPMAP = 0x400000;
	constexpr uint32_t FOURCC_DXT1 = 0x31545844;
	constexpr uint32_t FOURCC_DXT5 = 0x35545844;

	uint32_t flags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
	uint32_t pitchOrLinearSize = 0;
	if( compressed )
	{
		flags |= DDSD_LINEARSIZE;
		pitchOrLinearSize = ( ( top.width + 3 ) / 4 ) * ( ( top.height + 3 ) / 4 ) * ( withAlpha ? 16u : 8u );
	}
	else
	{
		flags |= DDSD_PITCH;
		pitchOrLinearSize = top.width * 4;
	}
	uint32_t caps = DDSCAPS_TEXTURE;
	if( mipCount > 1 )
	{
		flags |= DDSD_MIPMAPCOUNT;
		caps |= DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
	}

	AppendUint32( out, 0x20534444 );
	AppendUint32( out, 124 );
	AppendUint32( out, flags );
	AppendUint32( out, top.height );
	AppendUint32( out, top.width );
	AppendUint32( out, pitchOrLinearSize );
	AppendUint32( out, 0 );
	AppendUint32( out, mipCount > 1 ? mipCount : 0 );
	for( int reserved = 0; reserved < 11; ++reserved )
	{
		AppendUint32( out, 0 );
	}
	AppendUint32( out, 32 );
	if( compressed )
	{
		AppendUint32( out, DDPF_FOURCC );
		AppendUint32( out, withAlpha ? FOURCC_DXT5 : FOURCC_DXT1 );
		AppendUint32( out, 0 );
		AppendUint32( out, 0 );
		AppendUint32( out, 0 );
		AppendUint32( out, 0 );
		AppendUint32( out, 0 );
	}
	else
	{
		AppendUint32( out, DDPF_RGB | DDPF_ALPHAPIXELS );
		AppendUint32( out, 0 );
		AppendUint32( out, 32 );
		AppendUint32( out, 0x000000ff );
		AppendUint32( out, 0x0000ff00 );
		AppendUint32( out, 0x00ff0000 );
		AppendUint32( out, 0xff000000 );
	}
	AppendUint32( out, caps );
	AppendUint32( out, 0 );
	AppendUint32( out, 0 );
	AppendUint32( out, 0 );
	AppendUint32( out, 0 );
}

struct Options
{
	std::string inputPng;
	std::string inputRgba;
	std::string metadata;
	std::string output;
	std::string format = "auto";
	bool uncompressed = false;
	bool noMips = false;
	bool summary = false;
};

bool ParseArgs( int argc, char** argv, Options& options )
{
	for( int i = 1; i < argc; ++i )
	{
		const std::string arg = argv[i];
		if( arg == "--summary" )
		{
			options.summary = true;
		}
		else if( arg == "--uncompressed" )
		{
			options.uncompressed = true;
		}
		else if( arg == "--no-mips" )
		{
			options.noMips = true;
		}
		else if( arg == "--input" && ++i < argc )
		{
			options.inputPng = argv[i];
		}
		else if( arg == "--input-rgba" && ++i < argc )
		{
			options.inputRgba = argv[i];
		}
		else if( arg == "--metadata" && ++i < argc )
		{
			options.metadata = argv[i];
		}
		else if( arg == "--output" && ++i < argc )
		{
			options.output = argv[i];
		}
		else if( arg == "--format" && ++i < argc )
		{
			options.format = argv[i];
			if( options.format != "auto" && options.format != "bc1" && options.format != "bc3" )
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	const bool pngInput = !options.inputPng.empty() && options.inputRgba.empty() && options.metadata.empty();
	const bool rawInput = options.inputPng.empty() && !options.inputRgba.empty() && !options.metadata.empty();
	return ( pngInput || rawInput ) && !options.output.empty();
}

}

int main( int argc, char** argv )
{
	Options options;
	if( !ParseArgs( argc, argv, options ) )
	{
		std::cerr << "Usage: TrinityRgbaToDds ( --input texture.png | --input-rgba texture.rgba --metadata texture.txt )\n"
					 "                        --output texture.dds [--format auto|bc1|bc3] [--uncompressed] [--no-mips] [--summary]\n";
		return 2;
	}

	Image top;
	if( !options.inputPng.empty() )
	{
		if( !LoadPng( options.inputPng, top ) )
		{
			return 1;
		}
	}
	else if( !LoadRawRgba( options.inputRgba, options.metadata, top ) )
	{
		return 1;
	}

	const bool withAlpha = options.format == "bc3" || ( options.format == "auto" && HasTranslucency( top ) );

	std::vector<Image> levels;
	levels.push_back( std::move( top ) );
	if( !options.noMips )
	{
		while( levels.back().width > 1 || levels.back().height > 1 )
		{
			levels.push_back( HalveImage( levels.back() ) );
		}
	}

	std::vector<uint8_t> out;
	WriteHeader( out, levels.front(), static_cast<uint32_t>( levels.size() ), !options.uncompressed, withAlpha );
	for( const Image& level : levels )
	{
		if( options.uncompressed )
		{
			out.insert( out.end(), level.rgba.begin(), level.rgba.end() );
		}
		else
		{
			CompressLevel( level, withAlpha, out );
		}
	}

	std::ofstream output( options.output, std::ios::binary );
	if( !output )
	{
		std::cerr << "Failed to open output file: " << options.output << "\n";
		return 1;
	}
	output.write( reinterpret_cast<const char*>( out.data() ), static_cast<std::streamsize>( out.size() ) );
	if( !output )
	{
		std::cerr << "Failed to write DDS output: " << options.output << "\n";
		return 1;
	}
	if( options.summary )
	{
		std::cout << "Wrote " << options.output << "\n";
		std::cout << "Dimensions: " << levels.front().width << 'x' << levels.front().height
				  << " mips: " << levels.size()
				  << " format: " << ( options.uncompressed ? "rgba8" : ( withAlpha ? "bc3" : "bc1" ) ) << "\n";
	}
	return 0;
}
