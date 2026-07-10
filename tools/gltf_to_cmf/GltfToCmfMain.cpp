// Copyright (c) 2026 CCP Games

#include "GltfToCmf.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

namespace
{

void PrintUsage( const char* executable )
{
	std::cerr << "Usage: " << executable << " --input <asset.glb|asset.gltf> --output <asset.cmf> [--summary]\n";
}

bool WriteFile( const std::string& path, const std::vector<uint8_t>& bytes, std::string& error )
{
	std::ofstream out( path, std::ios::binary );
	if( !out )
	{
		error = "Failed to open output file: " + path;
		return false;
	}
	out.write( reinterpret_cast<const char*>( bytes.data() ), static_cast<std::streamsize>( bytes.size() ) );
	if( !out )
	{
		error = "Failed to write output file: " + path;
		return false;
	}
	return true;
}

bool WriteTextFile( const std::string& path, const std::string& text, std::string& error )
{
	std::ofstream out( path, std::ios::binary );
	if( !out )
	{
		error = "Failed to open output file: " + path;
		return false;
	}
	out << text;
	if( !out )
	{
		error = "Failed to write output file: " + path;
		return false;
	}
	return true;
}

}

int main( int argc, char** argv )
{
	GltfToCmfOptions options;
	std::string outputPath;
	bool summary = false;

	for( int i = 1; i < argc; ++i )
	{
		if( std::strcmp( argv[i], "--input" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				PrintUsage( argv[0] );
				return 2;
			}
			options.inputPath = argv[++i];
		}
		else if( std::strcmp( argv[i], "--output" ) == 0 )
		{
			if( i + 1 >= argc )
			{
				PrintUsage( argv[0] );
				return 2;
			}
			outputPath = argv[++i];
		}
		else if( std::strcmp( argv[i], "--summary" ) == 0 )
		{
			summary = true;
			options.verbose = true;
		}
		else if( std::strcmp( argv[i], "--help" ) == 0 || std::strcmp( argv[i], "-h" ) == 0 )
		{
			PrintUsage( argv[0] );
			return 0;
		}
		else
		{
			PrintUsage( argv[0] );
			return 2;
		}
	}

	if( options.inputPath.empty() || outputPath.empty() )
	{
		PrintUsage( argv[0] );
		return 2;
	}

	GltfToCmfResult result;
	std::string error;
	if( !BuildCmfFromGltf( options, result, error ) )
	{
		std::cerr << error << "\n";
		return 1;
	}

	if( !WriteFile( outputPath, result.cmfBytes, error ) )
	{
		std::cerr << error << "\n";
		return 1;
	}
	if( !result.baseColorTextures.empty() )
	{
		const GltfToCmfTexture& texture = result.baseColorTextures.front();
		const std::string rgbaPath = outputPath + ".basecolor.rgba";
		const std::string metadataPath = outputPath + ".basecolor.txt";
		if( !WriteFile( rgbaPath, texture.rgba, error ) )
		{
			std::cerr << error << "\n";
			return 1;
		}
		const std::string metadata =
			std::to_string( texture.width ) + " " + std::to_string( texture.height ) + "\n";
		if( !WriteTextFile( metadataPath, metadata, error ) )
		{
			std::cerr << error << "\n";
			return 1;
		}
	}

	if( summary )
	{
		std::cout << "Wrote " << outputPath << "\n";
		std::cout << "Meshes: " << result.meshCount << "\n";
		std::cout << "Skeletons: " << result.skeletonCount << "\n";
		std::cout << "Animations:";
		for( const auto& animationName : result.animationNames )
		{
			std::cout << " " << animationName;
		}
		std::cout << "\n";
		for( const auto& warning : result.warnings )
		{
			std::cerr << "warning: " << warning << "\n";
		}
		if( !result.baseColorTextures.empty() )
		{
			const GltfToCmfTexture& texture = result.baseColorTextures.front();
			std::cout << "BaseColorTexture: " << texture.width << "x" << texture.height << "\n";
		}
	}

	return 0;
}
