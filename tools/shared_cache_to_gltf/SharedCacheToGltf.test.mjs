// Copyright (c) 2026 CCP Games

import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import test from "node:test";

import { buildGltf } from "./SharedCacheToGltf.mjs";

const IDENTITY = [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1,
];
const CHILD_INVERSE_BIND = [
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    -2, -3, -4, 1,
];
const IDENTITY_SCALE_SHEAR = [ 1, 0, 0, 0, 1, 0, 0, 0, 1 ];

function curve( dimension, first, second )
{
    return { degree: 1, dimension, knots: [ 0, 1 ], controls: [ ...first, ...second ] };
}

function makeFixture()
{
    const skeleton = {
        name: "SyntheticSkeleton",
        bones: [
            {
                name: "root",
                parentIndex: -1,
                position: [ 0, 0, 0 ],
                orientation: [ 0, 0, 0, 1 ],
                scaleShear: IDENTITY_SCALE_SHEAR.slice(),
            },
            {
                name: "child",
                parentIndex: 0,
                position: [ 2, 3, 4 ],
                orientation: [ 0, 0, 0, 1 ],
                scaleShear: IDENTITY_SCALE_SHEAR.slice(),
            },
        ],
    };
    const mesh = {
        name: "SyntheticSkinnedTriangle",
        boneBindings: [ { name: "child" }, { name: "root" } ],
        vertex: {
            position: [ 0, 0, 0, 1, 0, 0, 0, 1, 0 ],
            normal: [ 0, 0, 1, 0, 0, 1, 0, 0, 1 ],
            tangent: [ 1, 0, 0, 1, 0, 0, 1, 0, 0 ],
            binormal: [ 0, 1, 0, 0, 1, 0, 0, 1, 0 ],
            texcoord0: [ 0, 0, 1, 0, 0, 1 ],
            blendIndice: [
                0, 0, 0, 0,
                1, 1, 1, 1,
                0, 0, 0, 0,
            ],
            blendWeight: [],
        },
        indices: [ { faces: [ 0, 1, 2 ] } ],
    };
    const scaleCurve = curve( 9, IDENTITY_SCALE_SHEAR, IDENTITY_SCALE_SHEAR );
    return {
        root: {
            models: [ { name: "SyntheticModel", meshBindings: [ 0 ], skeleton } ],
            animations: [ {
                name: "SyntheticMove",
                duration: 1,
                timeStep: 0.5,
                trackGroups: [ {
                    name: "SyntheticModel",
                    transformTracks: [ {
                        name: "child",
                        position: curve( 3, [ 0, 0, 0 ], [ 2, 4, 6 ] ),
                        orientation: curve( 4, [ 0, 0, 0, 1 ], [ 0, 0, 1, 0 ] ),
                        scaleShear: scaleCurve,
                    } ],
                } ],
            } ],
        },
        rawRoot: {
            fileInfo: {
                Models: [ {
                    Skeleton: {
                        Bones: [
                            { Name: "root", InverseWorldTransform: IDENTITY.slice() },
                            { Name: "child", InverseWorldTransform: CHILD_INVERSE_BIND.slice() },
                        ],
                    },
                } ],
            },
        },
        mesh,
    };
}

function readAccessor( gltf, binary, accessorIndex )
{
    const accessor = gltf.accessors[accessorIndex];
    const view = gltf.bufferViews[accessor.bufferView];
    const componentCount = { SCALAR: 1, VEC2: 2, VEC3: 3, VEC4: 4, MAT4: 16 }[accessor.type];
    const component = {
        5121: { size: 1, read: ( offset ) => binary.readUInt8( offset ) },
        5126: { size: 4, read: ( offset ) => binary.readFloatLE( offset ) },
    }[accessor.componentType];
    assert.ok( componentCount, `unsupported accessor type ${accessor.type}` );
    assert.ok( component, `unsupported component type ${accessor.componentType}` );
    const elementSize = componentCount * component.size;
    const stride = view.byteStride || elementSize;
    const base = ( view.byteOffset || 0 ) + ( accessor.byteOffset || 0 );
    const values = [];
    for( let element = 0; element < accessor.count; ++element )
    {
        for( let item = 0; item < componentCount; ++item )
        {
            values.push( component.read( base + element * stride + item * component.size ) );
        }
    }
    return values;
}

function convertFixture( directory, fixture )
{
    const output = path.join( directory, "Synthetic.gltf" );
    const result = buildGltf(
        fixture.root,
        fixture.rawRoot,
        fixture.mesh,
        0,
        "res:/synthetic/skinned-triangle.gr2",
        output,
        undefined,
        true );
    return {
        result,
        gltfBytes: fs.readFileSync( output ),
        binaryBytes: fs.readFileSync( path.join( directory, "Synthetic.bin" ) ),
        gltf: JSON.parse( fs.readFileSync( output, "utf8" ) ),
    };
}

function makeTemporaryDirectory( t )
{
    const directory = fs.mkdtempSync( path.join( os.tmpdir(), "trinity-shared-cache-to-gltf-" ) );
    t.after( () => fs.rmSync( directory, { recursive: true, force: true } ) );
    return directory;
}

test( "preserve-skin emits deterministic joints, hierarchy, inverse binds, and animation samples", t => {
    const fixture = makeFixture();
    const first = convertFixture( makeTemporaryDirectory( t ), fixture );
    const second = convertFixture( makeTemporaryDirectory( t ), makeFixture() );
    assert.deepEqual( first.gltfBytes, second.gltfBytes );
    assert.deepEqual( first.binaryBytes, second.binaryBytes );
    assert.equal( first.result.boneCount, 2 );
    assert.equal( first.result.animationCount, 1 );

    const { gltf, binaryBytes } = first;
    const primitive = gltf.meshes[0].primitives[0];
    assert.deepEqual( readAccessor( gltf, binaryBytes, primitive.attributes.JOINTS_0 ), [
        1, 1, 1, 1,
        0, 0, 0, 0,
        1, 1, 1, 1,
    ] );
    assert.deepEqual( readAccessor( gltf, binaryBytes, primitive.attributes.WEIGHTS_0 ), [
        1, 0, 0, 0,
        1, 0, 0, 0,
        1, 0, 0, 0,
    ] );
    assert.deepEqual( gltf.skins[0].joints, [ 1, 2 ] );
    assert.equal( gltf.skins[0].skeleton, 1 );
    assert.deepEqual( gltf.nodes[1].children, [ 2 ] );
    assert.equal( gltf.nodes[2].name, "child" );
    assert.deepEqual(
        readAccessor( gltf, binaryBytes, gltf.skins[0].inverseBindMatrices ),
        [ ...IDENTITY, ...CHILD_INVERSE_BIND ] );

    const animation = gltf.animations[0];
    const positionChannel = animation.channels.find( channel => channel.target.path === "translation" );
    const rotationChannel = animation.channels.find( channel => channel.target.path === "rotation" );
    const scaleChannel = animation.channels.find( channel => channel.target.path === "scale" );
    assert.equal( positionChannel.target.node, 2 );
    assert.equal( rotationChannel.target.node, 2 );
    assert.equal( scaleChannel.target.node, 2 );
    assert.deepEqual(
        readAccessor( gltf, binaryBytes, animation.samplers[positionChannel.sampler].input ),
        [ 0, 0.5, 1 ] );
    assert.deepEqual(
        readAccessor( gltf, binaryBytes, animation.samplers[positionChannel.sampler].output ),
        [ 0, 0, 0, 1, 2, 3, 2, 4, 6 ] );
    const rotations = readAccessor( gltf, binaryBytes, animation.samplers[rotationChannel.sampler].output );
    assert.deepEqual( rotations.slice( 0, 4 ), [ 0, 0, 0, 1 ] );
    assert.ok( Math.abs( rotations[6] - Math.SQRT1_2 ) < 1.0e-6 );
    assert.ok( Math.abs( rotations[7] - Math.SQRT1_2 ) < 1.0e-6 );
    assert.deepEqual( rotations.slice( 8 ), [ 0, 0, 1, 0 ] );
    assert.deepEqual(
        readAccessor( gltf, binaryBytes, animation.samplers[scaleChannel.sampler].output ),
        [ 1, 1, 1, 1, 1, 1, 1, 1, 1 ] );
} );

test( "preserve-skin rejects incomplete authored skin streams", t => {
    const fixture = makeFixture();
    fixture.mesh.vertex.blendIndice.pop();
    assert.throws(
        () => convertFixture( makeTemporaryDirectory( t ), fixture ),
        /incomplete authored skin data/ );
} );

test( "preserve-skin rejects bind-pose and animation shear", async t => {
    await t.test( "bind-pose shear", () => {
        const fixture = makeFixture();
        fixture.root.models[0].skeleton.bones[1].scaleShear[1] = 0.25;
        assert.throws(
            () => convertFixture( makeTemporaryDirectory( t ), fixture ),
            /authored shear that glTF TRS cannot preserve/ );
    } );
    await t.test( "animation shear", () => {
        const fixture = makeFixture();
        fixture.root.animations[0].trackGroups[0].transformTracks[0].scaleShear.controls[1] = 0.25;
        assert.throws(
            () => convertFixture( makeTemporaryDirectory( t ), fixture ),
            /has shear that glTF cannot preserve/ );
    } );
} );
