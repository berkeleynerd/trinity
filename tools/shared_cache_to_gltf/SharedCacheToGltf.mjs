// Copyright (c) 2026 CCP Games

import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";

import CjsFormatGr2 from "@carbonenginejs/format-gr2";

function usage()
{
    console.error(
        "Usage: SharedCacheToGltf.mjs --cache-root PATH [--model RES_PATH --output MODEL.gltf]\n" +
        "       [--base-color RES_PATH --base-color-output TEXTURE.dds] [--lod N]\n" +
        "       [--include-groups 0,1,...] [--copy-resource RES_PATH OUTPUT]\n" +
        "       [--manifest OUTPUT.json] [--summary]" );
}

function parseArgs( argv )
{
    const options = { lod: 0, summary: false, copiedResources: [] };
    for( let i = 2; i < argv.length; ++i )
    {
        const arg = argv[i];
        if( arg === "--summary" )
        {
            options.summary = true;
            continue;
        }
        if( arg === "--copy-resource" )
        {
            if( i + 2 >= argv.length )
            {
                throw new Error( "--copy-resource requires a logical resource path and output path" );
            }
            options.copiedResources.push( { logical: argv[++i], output: argv[++i] } );
            continue;
        }
        if( ++i >= argv.length )
        {
            throw new Error( `Missing value for ${arg}` );
        }
        const value = argv[i];
        switch( arg )
        {
        case "--cache-root":
            options.cacheRoot = value;
            break;
        case "--model":
            options.model = value;
            break;
        case "--output":
            options.output = value;
            break;
        case "--base-color":
            options.baseColor = value;
            break;
        case "--base-color-output":
            options.baseColorOutput = value;
            break;
        case "--lod":
            options.lod = Number.parseInt( value, 10 );
            if( !Number.isInteger( options.lod ) || options.lod < 0 )
            {
                throw new Error( `Invalid LOD index: ${value}` );
            }
            break;
        case "--include-groups":
            options.includeGroups = value.split( "," ).map( item => Number.parseInt( item, 10 ) );
            if( options.includeGroups.length === 0 || options.includeGroups.some( item => !Number.isInteger( item ) || item < 0 ) )
            {
                throw new Error( `Invalid group list: ${value}` );
            }
            break;
        case "--manifest":
            options.manifest = value;
            break;
        default:
            throw new Error( `Unknown argument: ${arg}` );
        }
    }

    if( !options.cacheRoot )
    {
        throw new Error( "--cache-root is required" );
    }
    if( Boolean( options.model ) !== Boolean( options.output ) )
    {
        throw new Error( "--model and --output must be provided together" );
    }
    if( !options.model && options.copiedResources.length === 0 )
    {
        throw new Error( "Provide a model conversion or at least one --copy-resource" );
    }
    if( Boolean( options.baseColor ) !== Boolean( options.baseColorOutput ) )
    {
        throw new Error( "--base-color and --base-color-output must be provided together" );
    }
    if( options.baseColor && !options.model )
    {
        throw new Error( "--base-color requires a model conversion" );
    }
    return options;
}

function findIndexes( cacheRoot )
{
    const candidates = [
        path.join( cacheRoot, "tq/EVE.app/Contents/Resources/build/resfileindex.txt" ),
        path.join( cacheRoot, "tq/EVE.app/Contents/Resources/build/resfileindex_macOS.txt" ),
        path.join( cacheRoot, "tq/resfileindex.txt" ),
    ];
    const result = [ ...new Set( candidates.filter( candidate => fs.existsSync( candidate ) ) ) ];
    if( result.length === 0 )
    {
        throw new Error( `Could not find any Tranquility resource indexes under ${cacheRoot}` );
    }
    return result;
}

function resolveResources( cacheRoot, logicalPaths )
{
    const wanted = new Map( logicalPaths.map( value => [ value.toLowerCase(), value ] ) );
    const resolved = new Map();
    for( const indexPath of findIndexes( cacheRoot ) )
    {
        for( const line of fs.readFileSync( indexPath, "utf8" ).split( /\r?\n/ ) )
        {
            const firstComma = line.indexOf( "," );
            if( firstComma < 0 )
            {
                continue;
            }
            const logical = line.slice( 0, firstComma );
            const requested = wanted.get( logical.toLowerCase() );
            if( !requested || resolved.has( requested ) )
            {
                continue;
            }
            const secondComma = line.indexOf( ",", firstComma + 1 );
            const hashedPath = line.slice( firstComma + 1, secondComma );
            const physicalPath = path.join( cacheRoot, "ResFiles", hashedPath );
            if( !fs.existsSync( physicalPath ) )
            {
                throw new Error( `Resource is indexed but not downloaded: ${requested} (${physicalPath})` );
            }
            resolved.set( requested, { physicalPath, indexPath } );
        }
    }

    for( const logical of logicalPaths )
    {
        if( !resolved.has( logical ) )
        {
            throw new Error( `Resource was not found in the index: ${logical}` );
        }
    }
    return resolved;
}

function validateMesh( mesh )
{
    const vertexCount = mesh.vertex.position.length / 3;
    if( !Number.isInteger( vertexCount ) || vertexCount === 0 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has no valid positions` );
    }
    if( mesh.vertex.normal.length !== vertexCount * 3 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' does not have one unpacked normal per vertex` );
    }
    if( mesh.vertex.texcoord0.length !== vertexCount * 2 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' does not have one TEXCOORD_0 per vertex` );
    }
    return vertexCount;
}

function positionBounds( positions )
{
    const minimum = [ Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY ];
    const maximum = [ Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY, Number.NEGATIVE_INFINITY ];
    for( let i = 0; i < positions.length; i += 3 )
    {
        for( let component = 0; component < 3; ++component )
        {
            minimum[component] = Math.min( minimum[component], positions[i + component] );
            maximum[component] = Math.max( maximum[component], positions[i + component] );
        }
    }
    return { minimum, maximum };
}

function align4( value )
{
    return ( value + 3 ) & ~3;
}

function buildGltf( mesh, sourcePath, outputPath, includeGroups )
{
    const vertexCount = validateMesh( mesh );
    const positions = new Float32Array( mesh.vertex.position );
    const normals = new Float32Array( mesh.vertex.normal );
    const texcoords = new Float32Array( mesh.vertex.texcoord0 );
    const selectedGroups = includeGroups ?? mesh.indices.map( ( _, index ) => index );
    if( selectedGroups.some( index => index < 0 || index >= mesh.indices.length ) )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has ${mesh.indices.length} index groups, but group ${Math.max( ...selectedGroups )} was requested` );
    }

    const chunks = [];
    const bufferViews = [];
    let byteOffset = 0;
    const append = ( typedArray, target ) => {
        const alignedOffset = align4( byteOffset );
        if( alignedOffset > byteOffset )
        {
            chunks.push( Buffer.alloc( alignedOffset - byteOffset ) );
        }
        const bytes = Buffer.from( typedArray.buffer, typedArray.byteOffset, typedArray.byteLength );
        chunks.push( bytes );
        bufferViews.push( { buffer: 0, byteOffset: alignedOffset, byteLength: bytes.length, target } );
        byteOffset = alignedOffset + bytes.length;
        return bufferViews.length - 1;
    };

    const positionView = append( positions, 34962 );
    const normalView = append( normals, 34962 );
    const texcoordView = append( texcoords, 34962 );
    const groupData = selectedGroups.map( sourceGroup => {
        const faces = mesh.indices[sourceGroup].faces;
        if( faces.length === 0 || faces.length % 3 !== 0 )
        {
            throw new Error( `GR2 mesh '${mesh.name}' group ${sourceGroup} does not contain triangle indices` );
        }
        const maximumIndex = Math.max( ...faces );
        if( maximumIndex >= vertexCount )
        {
            throw new Error( `GR2 mesh '${mesh.name}' group ${sourceGroup} contains an out-of-range index` );
        }
        const IndexArray = maximumIndex <= 0xffff ? Uint16Array : Uint32Array;
        const indices = new IndexArray( faces );
        return {
            sourceGroup,
            indices,
            indexView: append( indices, 34963 ),
        };
    } );
    const binary = Buffer.concat( chunks );
    const bounds = positionBounds( positions );
    const binaryName = `${path.basename( outputPath, path.extname( outputPath ) )}.bin`;

    const gltf = {
        asset: {
            version: "2.0",
            generator: "Trinity SharedCache GR2 bridge",
            extras: { source: sourcePath, sourceGroups: selectedGroups },
        },
        buffers: [ { uri: binaryName, byteLength: binary.length } ],
        bufferViews,
        accessors: [
            { bufferView: positionView, componentType: 5126, count: vertexCount, type: "VEC3", min: bounds.minimum, max: bounds.maximum },
            { bufferView: normalView, componentType: 5126, count: vertexCount, type: "VEC3" },
            { bufferView: texcoordView, componentType: 5126, count: vertexCount, type: "VEC2" },
            ...groupData.map( group => ( {
                bufferView: group.indexView,
                componentType: group.indices.BYTES_PER_ELEMENT === 2 ? 5123 : 5125,
                count: group.indices.length,
                type: "SCALAR",
            } ) ),
        ],
        meshes: groupData.map( ( group, groupIndex ) => ( {
            name: `${mesh.name}_group_${group.sourceGroup}`,
            extras: { sourceGroup: group.sourceGroup },
            primitives: [ {
                attributes: { POSITION: 0, NORMAL: 1, TEXCOORD_0: 2 },
                indices: 3 + groupIndex,
                mode: 4,
                extras: { sourceGroup: group.sourceGroup },
            } ],
        } ) ),
        nodes: groupData.map( ( group, groupIndex ) => ( {
            name: `Astero_group_${group.sourceGroup}`,
            mesh: groupIndex,
        } ) ),
        scenes: [ { name: "Astero", nodes: groupData.map( ( _, groupIndex ) => groupIndex ) } ],
        scene: 0,
    };

    fs.mkdirSync( path.dirname( outputPath ), { recursive: true } );
    fs.writeFileSync( outputPath, `${JSON.stringify( gltf, null, 2 )}\n` );
    fs.writeFileSync( path.join( path.dirname( outputPath ), binaryName ), binary );
    return {
        vertexCount,
        indexCount: groupData.reduce( ( total, group ) => total + group.indices.length, 0 ),
        selectedGroups,
        groupData,
        binaryPath: path.join( path.dirname( outputPath ), binaryName ),
    };
}

function main()
{
    const options = parseArgs( process.argv );
    const requested = [];
    if( options.model )
    {
        requested.push( options.model );
    }
    if( options.baseColor )
    {
        requested.push( options.baseColor );
    }
    for( const resource of options.copiedResources )
    {
        requested.push( resource.logical );
    }
    const resolved = resolveResources( path.resolve( options.cacheRoot ), requested );
    let mesh;
    let result;
    if( options.model )
    {
        const reader = new CjsFormatGr2( { emit: "json", unpackTangents: true } );
        const root = reader.Read( fs.readFileSync( resolved.get( options.model ).physicalPath ) );
        if( options.lod >= root.meshes.length )
        {
            throw new Error( `Requested LOD ${options.lod}, but the GR2 contains ${root.meshes.length} meshes` );
        }
        mesh = root.meshes[options.lod];
        result = buildGltf( mesh, options.model, path.resolve( options.output ), options.includeGroups );
    }

    if( options.baseColor )
    {
        fs.mkdirSync( path.dirname( path.resolve( options.baseColorOutput ) ), { recursive: true } );
        fs.copyFileSync( resolved.get( options.baseColor ).physicalPath, path.resolve( options.baseColorOutput ) );
    }
    for( const resource of options.copiedResources )
    {
        const output = path.resolve( resource.output );
        fs.mkdirSync( path.dirname( output ), { recursive: true } );
        fs.copyFileSync( resolved.get( resource.logical ).physicalPath, output );
    }
    if( options.manifest )
    {
        const destinations = new Map();
        if( options.model )
        {
            destinations.set( options.model, path.resolve( options.output ) );
        }
        if( options.baseColor )
        {
            destinations.set( options.baseColor, path.resolve( options.baseColorOutput ) );
        }
        for( const resource of options.copiedResources )
        {
            destinations.set( resource.logical, path.resolve( resource.output ) );
        }
        const cacheRoot = path.resolve( options.cacheRoot );
        const resources = requested.map( logicalPath => {
            const source = resolved.get( logicalPath );
            const bytes = fs.readFileSync( source.physicalPath );
            return {
                logicalPath,
                sourceIndex: path.relative( cacheRoot, source.indexPath ),
                hashedSource: path.relative( cacheRoot, source.physicalPath ),
                byteSize: bytes.length,
                sha256: crypto.createHash( "sha256" ).update( bytes ).digest( "hex" ),
                stagedDestination: destinations.get( logicalPath ),
            };
        } );
        const manifestPath = path.resolve( options.manifest );
        fs.mkdirSync( path.dirname( manifestPath ), { recursive: true } );
        fs.writeFileSync( manifestPath, `${JSON.stringify( { resources }, null, 2 )}\n` );
    }
    if( options.summary )
    {
        if( options.model )
        {
            console.log( `GR2: ${options.model}` );
            console.log( `Mesh: ${mesh.name}` );
            console.log( `Vertices: ${result.vertexCount}` );
            console.log( `Groups: ${result.selectedGroups.join( "," )}` );
            for( const group of result.groupData )
            {
                console.log( `Group ${group.sourceGroup}: ${group.indices.length / 3} triangles` );
            }
            console.log( `Triangles: ${result.indexCount / 3}` );
            console.log( `glTF: ${path.resolve( options.output )}` );
        }
        if( options.baseColor )
        {
            console.log( `BaseColorDDS: ${path.resolve( options.baseColorOutput )}` );
        }
        for( const resource of options.copiedResources )
        {
            console.log( `Resource: ${resource.logical} -> ${path.resolve( resource.output )}` );
        }
        if( options.manifest )
        {
            console.log( `Manifest: ${path.resolve( options.manifest )}` );
        }
    }
}

try
{
    main();
}
catch( error )
{
    usage();
    console.error( error instanceof Error ? error.message : String( error ) );
    process.exit( 1 );
}
