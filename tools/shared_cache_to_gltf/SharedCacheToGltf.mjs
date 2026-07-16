// Copyright (c) 2026 CCP Games

import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { fileURLToPath } from "node:url";

import CjsFormatGr2 from "@carbonenginejs/format-gr2";

function usage()
{
    console.error(
        "Usage: SharedCacheToGltf.mjs --cache-root PATH [--model RES_PATH --output MODEL.gltf]\n" +
        "       [--base-color RES_PATH --base-color-output TEXTURE.dds] [--lod N]\n" +
        "       [--include-groups 0,1,...] [--preserve-skin] [--instance-data]\n" +
        "       [--copy-resource RES_PATH OUTPUT]\n" +
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
        if( arg === "--instance-data" )
        {
            options.instanceData = true;
            continue;
        }
        if( arg === "--preserve-skin" )
        {
            options.preserveSkin = true;
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
    if( options.preserveSkin && !options.model )
    {
        throw new Error( "--preserve-skin requires a model conversion" );
    }
    if( options.preserveSkin && options.instanceData )
    {
        throw new Error( "--preserve-skin and --instance-data are mutually exclusive" );
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
    const normalComponents = mesh.vertex.normal.length / vertexCount;
    if( normalComponents !== 0 && normalComponents !== 3 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has an incomplete authored normal stream` );
    }
    const tangentComponents = mesh.vertex.tangent.length / vertexCount;
    const binormalComponents = mesh.vertex.binormal.length / vertexCount;
    const hasTangents = tangentComponents !== 0 || binormalComponents !== 0;
    if( hasTangents && ( tangentComponents !== binormalComponents || ( tangentComponents !== 3 && tangentComponents !== 4 ) ) )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has an incomplete authored tangent frame` );
    }
    const texcoordComponents = mesh.vertex.texcoord0.length / vertexCount;
    if( texcoordComponents !== 0 && texcoordComponents !== 2 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has an incomplete authored TEXCOORD_0 stream` );
    }
    return { vertexCount, normalComponents, tangentComponents, texcoordComponents };
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

function finiteArray( values, count, label )
{
    if( !Array.isArray( values ) || values.length !== count || values.some( value => !Number.isFinite( value ) ) )
    {
        throw new Error( `${label} must contain exactly ${count} finite values` );
    }
    return values;
}

function normalizeQuaternion( values, label )
{
    const quaternion = finiteArray( values, 4, label ).slice();
    const length = Math.hypot( ...quaternion );
    if( length < 1.0e-8 )
    {
        throw new Error( `${label} is a zero quaternion` );
    }
    for( let component = 0; component < 4; ++component )
    {
        quaternion[component] /= length;
    }
    return quaternion;
}

function buildSkinData( root, rawRoot, mesh, meshIndex )
{
    const vertexCount = mesh.vertex.position.length / 3;
    const sourceJointIndices = mesh.vertex.blendIndice || [];
    const sourceWeights = mesh.vertex.blendWeight || [];
    const boneBindings = mesh.boneBindings || [];
    const hasSkinData = sourceJointIndices.length !== 0 || sourceWeights.length !== 0 || boneBindings.length !== 0;
    if( !hasSkinData )
    {
        return null;
    }
    if( boneBindings.length === 0 || sourceJointIndices.length !== vertexCount * 4 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has incomplete authored skin data` );
    }
    if( sourceWeights.length !== 0 && sourceWeights.length !== vertexCount * 4 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has an incomplete authored bone-weight stream` );
    }

    const modelIndex = ( root.models || [] ).findIndex( model => ( model.meshBindings || [] ).includes( meshIndex ) );
    if( modelIndex < 0 )
    {
        throw new Error( `GR2 mesh '${mesh.name}' has bone bindings but no owning model` );
    }
    const model = root.models[modelIndex];
    const skeleton = model.skeleton;
    if( !skeleton || !Array.isArray( skeleton.bones ) || skeleton.bones.length === 0 )
    {
        throw new Error( `GR2 model '${model.name}' has no authored skeleton` );
    }
    if( skeleton.bones.length > 254 )
    {
        throw new Error( `GR2 skeleton '${skeleton.name}' exceeds the CMF v1 254-bone limit` );
    }

    const rawModel = rawRoot?.fileInfo?.Models?.[modelIndex];
    const rawSkeleton = rawModel?.Skeleton;
    if( !rawSkeleton || !Array.isArray( rawSkeleton.Bones ) || rawSkeleton.Bones.length !== skeleton.bones.length )
    {
        throw new Error( `GR2 model '${model.name}' has no matching raw skeleton for inverse bind matrices` );
    }

    const skeletonIndexByName = new Map();
    for( let boneIndex = 0; boneIndex < skeleton.bones.length; ++boneIndex )
    {
        const bone = skeleton.bones[boneIndex];
        if( !bone.name || skeletonIndexByName.has( bone.name ) )
        {
            throw new Error( `GR2 skeleton '${skeleton.name}' has an empty or duplicate bone name` );
        }
        skeletonIndexByName.set( bone.name, boneIndex );
        if( bone.parentIndex < -1 || bone.parentIndex >= skeleton.bones.length || bone.parentIndex === boneIndex )
        {
            throw new Error( `GR2 skeleton '${skeleton.name}' bone '${bone.name}' has an invalid parent` );
        }
    }

    const bindingToSkeleton = boneBindings.map( binding => {
        const index = skeletonIndexByName.get( binding.name );
        if( index === undefined )
        {
            throw new Error( `GR2 mesh '${mesh.name}' bone binding '${binding.name}' is absent from its skeleton` );
        }
        return index;
    } );
    const joints = new Uint8Array( vertexCount * 4 );
    const weights = new Float32Array( vertexCount * 4 );
    for( let vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
    {
        const offset = vertexIndex * 4;
        for( let influence = 0; influence < 4; ++influence )
        {
            const bindingIndex = sourceJointIndices[offset + influence];
            if( !Number.isInteger( bindingIndex ) || bindingIndex < 0 || bindingIndex >= bindingToSkeleton.length )
            {
                throw new Error( `GR2 mesh '${mesh.name}' vertex ${vertexIndex} has an invalid bone-binding index` );
            }
            joints[offset + influence] = bindingToSkeleton[bindingIndex];
        }

        if( sourceWeights.length === 0 )
        {
            // Granny's rigid skin stream repeats one bone index in all four
            // lanes and omits weights. Express that exact contract in glTF.
            if( joints[offset] !== joints[offset + 1] || joints[offset] !== joints[offset + 2] || joints[offset] !== joints[offset + 3] )
            {
                throw new Error( `GR2 mesh '${mesh.name}' vertex ${vertexIndex} omits weights for a non-rigid bone tuple` );
            }
            weights[offset] = 1.0;
        }
        else
        {
            let sum = 0.0;
            for( let influence = 0; influence < 4; ++influence )
            {
                const value = sourceWeights[offset + influence];
                if( !Number.isFinite( value ) || value < 0.0 )
                {
                    throw new Error( `GR2 mesh '${mesh.name}' vertex ${vertexIndex} has an invalid bone weight` );
                }
                sum += value;
            }
            if( sum <= 0.0 )
            {
                throw new Error( `GR2 mesh '${mesh.name}' vertex ${vertexIndex} has zero total bone weight` );
            }
            for( let influence = 0; influence < 4; ++influence )
            {
                weights[offset + influence] = sourceWeights[offset + influence] / sum;
            }
        }
    }

    const rawBoneByName = new Map( rawSkeleton.Bones.map( bone => [ bone.Name, bone ] ) );
    const inverseBindMatrices = new Float32Array( skeleton.bones.length * 16 );
    const nodes = skeleton.bones.map( ( bone, boneIndex ) => {
        const rawBone = rawBoneByName.get( bone.name );
        const inverseWorld = finiteArray( rawBone?.InverseWorldTransform, 16, `GR2 bone '${bone.name}' inverse world transform` );
        inverseBindMatrices.set( inverseWorld, boneIndex * 16 );

        const scaleShear = bone.scaleShear || [ 1, 0, 0, 0, 1, 0, 0, 0, 1 ];
        finiteArray( scaleShear, 9, `GR2 bone '${bone.name}' scale/shear` );
        const shear = [ scaleShear[1], scaleShear[2], scaleShear[3], scaleShear[5], scaleShear[6], scaleShear[7] ];
        if( shear.some( value => Math.abs( value ) > 1.0e-5 ) )
        {
            throw new Error( `GR2 bone '${bone.name}' has authored shear that glTF TRS cannot preserve` );
        }
        return {
            name: bone.name,
            translation: finiteArray( bone.position || [ 0, 0, 0 ], 3, `GR2 bone '${bone.name}' position` ).slice(),
            rotation: normalizeQuaternion( bone.orientation || [ 0, 0, 0, 1 ], `GR2 bone '${bone.name}' orientation` ),
            scale: [ scaleShear[0], scaleShear[4], scaleShear[8] ],
        };
    } );
    for( let boneIndex = 0; boneIndex < skeleton.bones.length; ++boneIndex )
    {
        const parentIndex = skeleton.bones[boneIndex].parentIndex;
        if( parentIndex >= 0 )
        {
            const children = nodes[parentIndex].children || [];
            children.push( boneIndex );
            nodes[parentIndex].children = children;
        }
    }

    return {
        model,
        skeleton,
        nodes,
        roots: skeleton.bones.map( ( bone, index ) => bone.parentIndex < 0 ? index : -1 ).filter( index => index >= 0 ),
        joints,
        weights,
        inverseBindMatrices,
    };
}

function animationSampleTimes( animation )
{
    if( !Number.isFinite( animation.duration ) || animation.duration < 0.0 )
    {
        throw new Error( `GR2 animation '${animation.name}' has an invalid duration` );
    }
    if( animation.duration === 0.0 )
    {
        return new Float32Array( [ 0.0 ] );
    }
    if( !Number.isFinite( animation.timeStep ) || animation.timeStep <= 0.0 )
    {
        throw new Error( `GR2 animation '${animation.name}' has no valid authored timestep` );
    }
    const samples = [];
    for( let time = 0.0; time < animation.duration; time += animation.timeStep )
    {
        samples.push( time );
    }
    if( samples.length === 0 || Math.abs( samples[samples.length - 1] - animation.duration ) > 1.0e-7 )
    {
        samples.push( animation.duration );
    }
    return new Float32Array( samples );
}

function sampleTransformCurve( curve, dimension, times, duration, label )
{
    if( !curve || curve.dimension !== dimension || !Array.isArray( curve.knots ) || !Array.isArray( curve.controls ) )
    {
        throw new Error( `${label} was not decompressed into an authored ${dimension}-component curve` );
    }
    const values = new Float32Array( times.length * dimension );
    const sample = new Array( dimension ).fill( 0.0 );
    let previousQuaternion = null;
    for( let sampleIndex = 0; sampleIndex < times.length; ++sampleIndex )
    {
        CjsFormatGr2.curves.sample( sample, curve, times[sampleIndex], false, duration );
        if( sample.some( value => !Number.isFinite( value ) ) )
        {
            throw new Error( `${label} produced a nonfinite sample` );
        }
        if( dimension === 4 )
        {
            const quaternion = normalizeQuaternion( sample, label );
            if( previousQuaternion && quaternion.reduce( ( dot, value, index ) => dot + value * previousQuaternion[index], 0.0 ) < 0.0 )
            {
                for( let component = 0; component < 4; ++component )
                {
                    quaternion[component] = -quaternion[component];
                }
            }
            values.set( quaternion, sampleIndex * dimension );
            previousQuaternion = quaternion;
        }
        else
        {
            values.set( sample, sampleIndex * dimension );
        }
    }
    return values;
}

function buildGltf( root, rawRoot, mesh, meshIndex, sourcePath, outputPath, includeGroups, preserveSkin )
{
    const { vertexCount, normalComponents, tangentComponents, texcoordComponents } = validateMesh( mesh );
    const hasTangents = tangentComponents !== 0;
    const hasTexcoords = texcoordComponents !== 0;
    const positions = new Float32Array( mesh.vertex.position );
    const normals = normalComponents === 3 ? new Float32Array( mesh.vertex.normal ) : new Float32Array( vertexCount * 3 );
    if( normalComponents === 0 )
    {
        for( let vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
        {
            normals[vertexIndex * 3 + 2] = 1.0;
        }
        console.warn( `GR2 mesh '${mesh.name}' has no authored normals; using deterministic +Z billboard normals` );
    }
    else
    {
        let replacedZeroNormal = false;
        for( let vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
        {
            const offset = vertexIndex * 3;
            const length = Math.hypot( normals[offset + 0], normals[offset + 1], normals[offset + 2] );
            if( length < 1.0e-8 )
            {
                normals[offset + 0] = 0.0;
                normals[offset + 1] = 0.0;
                normals[offset + 2] = 1.0;
                replacedZeroNormal = true;
            }
            else
            {
                normals[offset + 0] /= length;
                normals[offset + 1] /= length;
                normals[offset + 2] /= length;
            }
        }
        if( replacedZeroNormal )
        {
            console.warn( `GR2 mesh '${mesh.name}' has zero authored normals; using deterministic +Z billboard normals for those vertices` );
        }
    }
    const tangents = hasTangents ? new Float32Array( vertexCount * 4 ) : null;
    for( let vertexIndex = 0; hasTangents && vertexIndex < vertexCount; ++vertexIndex )
    {
        const normalOffset = vertexIndex * 3;
        const tangentOffset = vertexIndex * tangentComponents;
        const nx = normals[normalOffset + 0];
        const ny = normals[normalOffset + 1];
        const nz = normals[normalOffset + 2];
        const tx = mesh.vertex.tangent[tangentOffset + 0];
        const ty = mesh.vertex.tangent[tangentOffset + 1];
        const tz = mesh.vertex.tangent[tangentOffset + 2];
        const bx = mesh.vertex.binormal[tangentOffset + 0];
        const by = mesh.vertex.binormal[tangentOffset + 1];
        const bz = mesh.vertex.binormal[tangentOffset + 2];
        const tangentLength = Math.hypot( tx, ty, tz );
        if( tangentLength < 1.0e-8 )
        {
            const fallbackX = Math.abs( nz ) > 0.9 ? 1.0 : -ny;
            const fallbackY = Math.abs( nz ) > 0.9 ? 0.0 : nx;
            const fallbackZ = 0.0;
            const fallbackLength = Math.hypot( fallbackX, fallbackY, fallbackZ );
            tangents[vertexIndex * 4 + 0] = fallbackX / fallbackLength;
            tangents[vertexIndex * 4 + 1] = fallbackY / fallbackLength;
            tangents[vertexIndex * 4 + 2] = fallbackZ;
            tangents[vertexIndex * 4 + 3] = 1.0;
            console.warn( `GR2 mesh '${mesh.name}' has a zero authored tangent at vertex ${vertexIndex}; using a deterministic orthogonal tangent` );
            continue;
        }
        tangents[vertexIndex * 4 + 0] = tx / tangentLength;
        tangents[vertexIndex * 4 + 1] = ty / tangentLength;
        tangents[vertexIndex * 4 + 2] = tz / tangentLength;
        const crossX = ny * tz - nz * ty;
        const crossY = nz * tx - nx * tz;
        const crossZ = nx * ty - ny * tx;
        tangents[vertexIndex * 4 + 3] = crossX * bx + crossY * by + crossZ * bz < 0 ? -1 : 1;
    }
    const texcoords = hasTexcoords ? new Float32Array( mesh.vertex.texcoord0 ) : null;
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
    const tangentView = tangents ? append( tangents, 34962 ) : null;
    const texcoordView = texcoords ? append( texcoords, 34962 ) : null;
    const skin = preserveSkin ? buildSkinData( root, rawRoot, mesh, meshIndex ) : null;
    const jointView = skin ? append( skin.joints, 34962 ) : null;
    const weightView = skin ? append( skin.weights, 34962 ) : null;
    const inverseBindView = skin ? append( skin.inverseBindMatrices, undefined ) : null;

    const bounds = positionBounds( positions );
    const accessors = [];
    const attributes = {};
    attributes.POSITION = accessors.length;
    accessors.push( { bufferView: positionView, componentType: 5126, count: vertexCount, type: "VEC3", min: bounds.minimum, max: bounds.maximum } );
    attributes.NORMAL = accessors.length;
    accessors.push( { bufferView: normalView, componentType: 5126, count: vertexCount, type: "VEC3" } );
    if( tangents )
    {
        attributes.TANGENT = accessors.length;
        accessors.push( { bufferView: tangentView, componentType: 5126, count: vertexCount, type: "VEC4" } );
    }
    if( texcoords )
    {
        attributes.TEXCOORD_0 = accessors.length;
        accessors.push( { bufferView: texcoordView, componentType: 5126, count: vertexCount, type: "VEC2" } );
    }
    let inverseBindAccessor = null;
    if( skin )
    {
        attributes.JOINTS_0 = accessors.length;
        accessors.push( { bufferView: jointView, componentType: 5121, count: vertexCount, type: "VEC4" } );
        attributes.WEIGHTS_0 = accessors.length;
        accessors.push( { bufferView: weightView, componentType: 5126, count: vertexCount, type: "VEC4" } );
        inverseBindAccessor = accessors.length;
        accessors.push( { bufferView: inverseBindView, componentType: 5126, count: skin.nodes.length, type: "MAT4" } );
    }

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
            indexAccessor: -1,
        };
    } );
    for( const group of groupData )
    {
        group.indexAccessor = accessors.length;
        accessors.push( {
            bufferView: group.indexView,
            componentType: group.indices.BYTES_PER_ELEMENT === 2 ? 5123 : 5125,
            count: group.indices.length,
            type: "SCALAR",
        } );
    }

    const nodes = groupData.map( ( group, groupIndex ) => ( {
        name: `Astero_group_${group.sourceGroup}`,
        mesh: groupIndex,
        ...( skin ? { skin: 0 } : {} ),
    } ) );
    const sceneNodes = groupData.map( ( _, groupIndex ) => groupIndex );
    let skins;
    let animations;
    if( skin )
    {
        const boneNodeOffset = nodes.length;
        for( const sourceNode of skin.nodes )
        {
            nodes.push( {
                ...sourceNode,
                ...( sourceNode.children ? { children: sourceNode.children.map( child => child + boneNodeOffset ) } : {} ),
            } );
        }
        sceneNodes.push( ...skin.roots.map( rootIndex => rootIndex + boneNodeOffset ) );
        skins = [ {
            name: skin.skeleton.name || skin.model.name || "GR2_Skeleton",
            inverseBindMatrices: inverseBindAccessor,
            joints: skin.nodes.map( ( _, boneIndex ) => boneIndex + boneNodeOffset ),
            skeleton: skin.roots[0] + boneNodeOffset,
        } ];

        animations = [];
        for( const sourceAnimation of root.animations || [] )
        {
            const tracksByName = new Map();
            for( const trackGroup of sourceAnimation.trackGroups || [] )
            {
                for( const track of trackGroup.transformTracks || [] )
                {
                    if( tracksByName.has( track.name ) )
                    {
                        throw new Error( `GR2 animation '${sourceAnimation.name}' has duplicate track '${track.name}'` );
                    }
                    tracksByName.set( track.name, track );
                }
            }
            const times = animationSampleTimes( sourceAnimation );
            const timeView = append( times, undefined );
            const timeAccessor = accessors.length;
            accessors.push( {
                bufferView: timeView,
                componentType: 5126,
                count: times.length,
                type: "SCALAR",
                min: [ times[0] ],
                max: [ times[times.length - 1] ],
            } );
            const animation = { name: sourceAnimation.name || `animation_${animations.length}`, samplers: [], channels: [] };
            for( let boneIndex = 0; boneIndex < skin.skeleton.bones.length; ++boneIndex )
            {
                const bone = skin.skeleton.bones[boneIndex];
                const track = tracksByName.get( bone.name );
                if( !track )
                {
                    continue;
                }
                const channelData = [
                    { path: "translation", dimension: 3, values: sampleTransformCurve( track.position, 3, times, sourceAnimation.duration, `GR2 animation '${sourceAnimation.name}' track '${bone.name}' position` ) },
                    { path: "rotation", dimension: 4, values: sampleTransformCurve( track.orientation, 4, times, sourceAnimation.duration, `GR2 animation '${sourceAnimation.name}' track '${bone.name}' orientation` ) },
                ];
                const scaleShear = sampleTransformCurve( track.scaleShear, 9, times, sourceAnimation.duration, `GR2 animation '${sourceAnimation.name}' track '${bone.name}' scale/shear` );
                const scaleValues = new Float32Array( times.length * 3 );
                for( let sampleIndex = 0; sampleIndex < times.length; ++sampleIndex )
                {
                    const offset = sampleIndex * 9;
                    const shear = [
                        scaleShear[offset + 1], scaleShear[offset + 2], scaleShear[offset + 3],
                        scaleShear[offset + 5], scaleShear[offset + 6], scaleShear[offset + 7],
                    ];
                    if( shear.some( value => Math.abs( value ) > 1.0e-5 ) )
                    {
                        throw new Error( `GR2 animation '${sourceAnimation.name}' track '${bone.name}' has shear that glTF cannot preserve` );
                    }
                    scaleValues.set( [ scaleShear[offset], scaleShear[offset + 4], scaleShear[offset + 8] ], sampleIndex * 3 );
                }
                channelData.push( { path: "scale", dimension: 3, values: scaleValues } );

                for( const channel of channelData )
                {
                    const outputView = append( channel.values, undefined );
                    const outputAccessor = accessors.length;
                    accessors.push( {
                        bufferView: outputView,
                        componentType: 5126,
                        count: times.length,
                        type: channel.dimension === 4 ? "VEC4" : "VEC3",
                    } );
                    const samplerIndex = animation.samplers.length;
                    animation.samplers.push( { input: timeAccessor, output: outputAccessor, interpolation: "LINEAR" } );
                    animation.channels.push( {
                        sampler: samplerIndex,
                        target: { node: boneNodeOffset + boneIndex, path: channel.path },
                    } );
                }
            }
            if( animation.channels.length !== 0 )
            {
                animations.push( animation );
            }
        }
    }

    const binary = Buffer.concat( chunks );
    const binaryName = `${path.basename( outputPath, path.extname( outputPath ) )}.bin`;
    const gltf = {
        asset: {
            version: "2.0",
            generator: "Trinity SharedCache GR2 bridge",
            extras: {
                source: sourcePath,
                sourceGroups: selectedGroups,
                ...( skin ? {
                    skinned: true,
                    boneCount: skin.nodes.length,
                    animationCount: animations?.length || 0,
                } : {} ),
            },
        },
        buffers: [ { uri: binaryName, byteLength: binary.length } ],
        bufferViews,
        accessors,
        meshes: groupData.map( ( group, groupIndex ) => ( {
            name: `${mesh.name}_group_${group.sourceGroup}`,
            extras: { sourceGroup: group.sourceGroup },
            primitives: [ {
                attributes,
                indices: group.indexAccessor,
                mode: 4,
                extras: { sourceGroup: group.sourceGroup },
            } ],
        } ) ),
        nodes,
        ...( skins ? { skins } : {} ),
        ...( animations?.length ? { animations } : {} ),
        scenes: [ { name: "Astero", nodes: sceneNodes } ],
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
        boneCount: skin?.nodes.length || 0,
        animationCount: animations?.length || 0,
        binaryPath: path.join( path.dirname( outputPath ), binaryName ),
    };
}

function buildInstanceDataGltf( mesh, rawMesh, sourcePath, outputPath )
{
    const { vertexCount, normalComponents, tangentComponents, texcoordComponents } = validateMesh( mesh );
    if( normalComponents !== 0 || tangentComponents !== 0 || texcoordComponents !== 2 )
    {
        throw new Error( `GR2 instance mesh '${mesh.name}' must contain exactly POSITION and TEXCOORD_0` );
    }
    if( mesh.indices.length !== 0 )
    {
        throw new Error( `GR2 instance mesh '${mesh.name}' unexpectedly contains triangle groups` );
    }
    const rawVertices = rawMesh?.PrimaryVertexData?.Vertices;
    if( !Array.isArray( rawVertices ) || rawVertices.length !== vertexCount )
    {
        throw new Error( `GR2 instance mesh '${mesh.name}' has no matching raw vertex stream` );
    }
    const positions = new Float32Array( mesh.vertex.position );
    const texcoords = new Float32Array( mesh.vertex.texcoord0 );
    const extras = new Float32Array( vertexCount * 3 );
    for( let vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex )
    {
        const rawPosition = rawVertices[vertexIndex].Position;
        const rawMisc = rawVertices[vertexIndex].TextureCoordinates0;
        if( !Array.isArray( rawPosition ) || rawPosition.length !== 4 ||
            !Array.isArray( rawMisc ) || rawMisc.length !== 4 )
        {
            throw new Error( `GR2 instance mesh '${mesh.name}' must retain authored float4 POSITION and TEXCOORD_0 streams` );
        }
        for( let component = 0; component < 3; ++component )
        {
            if( rawPosition[component] !== positions[vertexIndex * 3 + component] )
            {
                throw new Error( `GR2 instance mesh '${mesh.name}' raw POSITION disagrees with the normalized stream` );
            }
        }
        for( let component = 0; component < 2; ++component )
        {
            if( rawMisc[component] !== texcoords[vertexIndex * 2 + component] )
            {
                throw new Error( `GR2 instance mesh '${mesh.name}' raw TEXCOORD_0 disagrees with the normalized stream` );
            }
        }
        extras[vertexIndex * 3] = rawPosition[3];
        extras[vertexIndex * 3 + 1] = rawMisc[2];
        extras[vertexIndex * 3 + 2] = rawMisc[3];
    }
    const positionBytes = Buffer.from( positions.buffer, positions.byteOffset, positions.byteLength );
    const texcoordOffset = align4( positionBytes.length );
    const texcoordBytes = Buffer.from( texcoords.buffer, texcoords.byteOffset, texcoords.byteLength );
    const extrasOffset = align4( texcoordOffset + texcoordBytes.length );
    const extrasBytes = Buffer.from( extras.buffer, extras.byteOffset, extras.byteLength );
    const binary = Buffer.concat( [
        positionBytes,
        Buffer.alloc( texcoordOffset - positionBytes.length ),
        texcoordBytes,
        Buffer.alloc( extrasOffset - texcoordOffset - texcoordBytes.length ),
        extrasBytes,
    ] );
    const binaryName = `${path.basename( outputPath, path.extname( outputPath ) )}.bin`;
    const bounds = positionBounds( positions );
    const gltf = {
        asset: {
            version: "2.0",
            generator: "Trinity SharedCache GR2 instance-data bridge",
            extras: { source: sourcePath, instanceData: true },
        },
        buffers: [ { uri: binaryName, byteLength: binary.length } ],
        bufferViews: [
            { buffer: 0, byteOffset: 0, byteLength: positionBytes.length, target: 34962 },
            { buffer: 0, byteOffset: texcoordOffset, byteLength: texcoordBytes.length, target: 34962 },
            { buffer: 0, byteOffset: extrasOffset, byteLength: extrasBytes.length, target: 34962 },
        ],
        accessors: [
            { bufferView: 0, componentType: 5126, count: vertexCount, type: "VEC3", min: bounds.minimum, max: bounds.maximum },
            { bufferView: 1, componentType: 5126, count: vertexCount, type: "VEC2" },
            { bufferView: 2, componentType: 5126, count: vertexCount, type: "VEC3" },
        ],
        meshes: [ {
            name: mesh.name || "instance_data",
            primitives: [ { attributes: { POSITION: 0, TEXCOORD_0: 1, _INSTANCE_EXTRAS: 2 }, mode: 0 } ],
        } ],
        nodes: [ { name: mesh.name || "instance_data", mesh: 0 } ],
        scenes: [ { name: "InstanceData", nodes: [ 0 ] } ],
        scene: 0,
    };
    fs.mkdirSync( path.dirname( outputPath ), { recursive: true } );
    fs.writeFileSync( outputPath, `${JSON.stringify( gltf, null, 2 )}\n` );
    fs.writeFileSync( path.join( path.dirname( outputPath ), binaryName ), binary );
    return { vertexCount, indexCount: 0, selectedGroups: [], groupData: [], binaryPath: path.join( path.dirname( outputPath ), binaryName ) };
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
        const sourceBytes = fs.readFileSync( resolved.get( options.model ).physicalPath );
        const reader = new CjsFormatGr2( { emit: "json", decompressCurves: Boolean( options.preserveSkin ), unpackTangents: true } );
        const root = reader.Read( sourceBytes );
        if( options.lod >= root.meshes.length )
        {
            throw new Error( `Requested LOD ${options.lod}, but the GR2 contains ${root.meshes.length} meshes` );
        }
        mesh = root.meshes[options.lod];
        const rawRoot = options.instanceData || options.preserveSkin ?
            new CjsFormatGr2( { emit: "raw" } ).Read( sourceBytes ) : null;
        const rawMesh = rawRoot?.fileInfo?.Meshes?.[options.lod];
        result = options.instanceData ?
            buildInstanceDataGltf( mesh, rawMesh, options.model, path.resolve( options.output ) ) :
            buildGltf( root, rawRoot, mesh, options.lod, options.model, path.resolve( options.output ), options.includeGroups, options.preserveSkin );
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
                sourceIndexPath: path.resolve( source.indexPath ),
                hashedSource: path.relative( cacheRoot, source.physicalPath ),
                sourceFilePath: path.resolve( source.physicalPath ),
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
            console.log( `Bones: ${result.boneCount || 0}` );
            console.log( `Animations: ${result.animationCount || 0}` );
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

export { buildGltf, buildSkinData, parseArgs, sampleTransformCurve };

if( process.argv[1] && path.resolve( process.argv[1] ) === fileURLToPath( import.meta.url ) )
{
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
}
