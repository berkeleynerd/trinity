#!/usr/bin/env python3
"""Read EVE FSD binary statics (embedded-schema .static files).

Independent Python 3 implementation of the FSD binary layout: a uint32
schema size, a pickled schema, then binary records. Objects store a uint64
optional-attribute bitfield at endOfFixedSizeData followed by a uint32
offset table for present variable attributes; dicts store a uint32 body
size, bodies, and a key footer whose size occupies the final 4 body bytes.

Reads local SharedCache payloads only; never copies client bytes anywhere.

Usage:
  fsd_read.py <file.static> --keys
  fsd_read.py <file.static> --key 30005286 [--offset BYTEOFFSET]
  fsd_read.py <file.static> --all
"""
import argparse
import json
import pickle
import struct


def load_schema(data):
    n = struct.unpack_from('<I', data, 0)[0]
    return pickle.loads(data[4:4 + n], encoding='latin1'), 4 + n


def _clean(k):
    return k.decode('latin1') if isinstance(k, bytes) else k


def _get(schema, key, default=None):
    for k, v in schema.items():
        if _clean(k) == key:
            return v
    return default


def read_node(data, offset, schema):
    kind = _clean(_get(schema, 'type'))
    if kind == 'object':
        return read_object(data, offset, schema)
    if kind == 'dict':
        return read_dict(data, offset, schema)
    if kind == 'list':
        return read_list(data, offset, schema)
    if kind in ('int', 'typeID', 'localizationID', 'fsdReference', 'npcTag',
                'deploymentType', 'factionID', 'raceID', 'marketGroupID',
                'dungeonID', 'typeListID', 'metaGroupID'):
        minimum = _get(schema, 'min')
        exclusive = _get(schema, 'exclusiveMin')
        unsigned = (minimum is not None and minimum >= 0) or \
            (exclusive is not None and exclusive >= -1)
        return struct.unpack_from('<I' if unsigned else '<i', data, offset)[0]
    if kind == 'float':
        double = _clean(_get(schema, 'precision', 'single')) == 'double'
        return struct.unpack_from('<d' if double else '<f', data, offset)[0]
    if kind == 'bool':
        return data[offset] == 255
    if kind in ('string', 'resPath', 'unicode'):
        n = struct.unpack_from('<I', data, offset)[0]
        return data[offset + 4:offset + 4 + n].decode('utf-8', 'replace')
    if kind in ('vector2', 'vector3', 'vector4'):
        count = {'vector2': 2, 'vector3': 3, 'vector4': 4}[kind]
        double = _clean(_get(schema, 'precision', 'single')) == 'double'
        return list(struct.unpack_from('<%d%s' % (count, 'd' if double else 'f'), data, offset))
    if kind == 'enum':
        largest = _get(schema, 'maxEnumValue')
        size = 1 if largest < 256 else (2 if largest < 65536 else 4)
        value = int.from_bytes(data[offset:offset + size], 'little')
        for name, enum_value in _get(schema, 'values').items():
            if enum_value == value:
                return _clean(name)
        return value
    if kind == 'union':
        index = struct.unpack_from('<I', data, offset)[0]
        return read_node(data, offset + 4, _get(schema, 'optionTypes')[index])
    raise NotImplementedError(kind)


def read_object(data, offset, schema):
    out = {}
    attributes = _get(schema, 'attributes')
    constant = {_clean(k): v for k, v in _get(schema, 'constantAttributeOffsets', {}).items()}
    variable = [_clean(v) for v in _get(schema, 'attributesWithVariableOffsets', [])]
    optional = {_clean(k): v for k, v in _get(schema, 'optionalValueLookups', {}).items()}
    for key, sub in attributes.items():
        name = _clean(key)
        if name in constant:
            out[name] = read_node(data, offset + constant[name], sub)
    if variable and _get(schema, 'size') is None:
        end_fixed = _get(schema, 'endOfFixedSizeData', 0)
        if optional:
            field = struct.unpack_from('<Q', data, offset + end_fixed)[0]
            present = [n for n in variable if n not in optional or (field & optional[n])]
        else:
            present = variable[:]
        table_at = offset + end_fixed + 8
        base = table_at + 4 * len(present)
        offsets = struct.unpack_from('<%dI' % len(present), data, table_at)
        for name, relative in zip(present, offsets):
            out[name] = read_node(data, base + relative, _get(attributes, name))
    return out


def read_list(data, offset, schema):
    items = _get(schema, 'itemTypes')
    fixed = _get(schema, 'fixedItemsSize') or _get(items, 'size')
    count = struct.unpack_from('<I', data, offset)[0]
    out = []
    if fixed:
        for i in range(count):
            out.append(read_node(data, offset + 4 + fixed * i, items))
    else:
        offsets = struct.unpack_from('<%dI' % count, data, offset + 4)
        for i in range(count):
            out.append(read_node(data, offset + 4 + offsets[i], items))
    return out


def read_dict(data, offset, schema):
    body_size = struct.unpack_from('<I', data, offset)[0]
    footer_size = struct.unpack_from('<I', data, offset + body_size)[0]
    entries = read_list(data, offset + body_size - footer_size, _get(schema, 'keyFooter'))
    values = _get(schema, 'valueTypes')
    return {e['key']: read_node(data, offset + 4 + e['offset'], values) for e in entries}


def read_dict_keys(data, offset, schema):
    body_size = struct.unpack_from('<I', data, offset)[0]
    footer_size = struct.unpack_from('<I', data, offset + body_size)[0]
    return read_list(data, offset + body_size - footer_size, _get(schema, 'keyFooter'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('path', help='FSD binary static file')
    parser.add_argument('--keys', action='store_true', help='list top-level dict keys')
    parser.add_argument('--key', type=int, help='dump one top-level record by key')
    parser.add_argument('--offset', type=int, help='record byte offset for multiIndex files, '
                        'where the top-level footer is not a simple dict')
    parser.add_argument('--all', action='store_true', help='dump every record (small files only)')
    args = parser.parse_args()

    data = open(args.path, 'rb').read()
    schema, data_start = load_schema(data)
    values = _get(schema, 'valueTypes')
    if args.offset is not None:
        record = read_node(data, args.offset, values)
        print(json.dumps({args.key if args.key is not None else 'record': record}, indent=1, default=str))
        return
    if args.keys:
        for entry in read_dict_keys(data, data_start, schema):
            print(entry['key'], 'offset', entry['offset'], 'size', entry.get('size'))
        return
    if args.all:
        print(json.dumps(read_dict(data, data_start, schema), indent=1, default=str))
        return
    if args.key is not None:
        record = read_dict(data, data_start, schema).get(args.key)
        print(json.dumps(record, indent=1, default=str))
        return
    parser.print_help()


if __name__ == '__main__':
    main()
