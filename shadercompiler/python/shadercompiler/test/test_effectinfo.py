# Copyright (c) 2026 CCP ehf.

import struct
import unittest

from shadercompiler.effectinfo import ShaderLibrary, Stage, _StringTable, _StructStream


def _string_table(data):
    return _StringTable(_StructStream(struct.pack('<I', len(data)) + data))


class TestEffectInfoV15(unittest.TestCase):
    def test_stage_keeps_shader_blob_and_threadgroup_size(self):
        shader = b'MTLB'
        stage_data = b''.join((
            struct.pack('<BIIIII', 2, len(shader), 0, 8, 8, 1),
            struct.pack('<BBB', 0, 0, 0),
            struct.pack('<II', 0, 0),
            struct.pack('<I', 0),
            struct.pack('<BBBB', 0, 0, 0, 0),
        ))

        stage = Stage(_StructStream(stage_data), _string_table(shader), 15)

        self.assertEqual(shader, stage.shader_data)
        self.assertEqual(0, stage.shader_data_offset)
        self.assertEqual(len(shader), stage.shader_size)
        self.assertEqual((8, 8, 1), stage.thread_group_size)

    def test_shader_library_keeps_shader_blob(self):
        shader = b'LIBRARY'
        library_data = b''.join((
            struct.pack('<III', 16, len(shader), 0),
            struct.pack('<I', 0),
            struct.pack('<I', len(shader)),
            struct.pack('<BB', 0, 0),
            struct.pack('<III', 0, 0, 0),
            struct.pack('<BBBB', 0, 0, 0, 0),
            struct.pack('<BB', 0, 0),
            struct.pack('<III', 0, 0, 0),
            struct.pack('<BBBB', 0, 0, 0, 0),
        ))

        library = ShaderLibrary(_StructStream(library_data), _string_table(shader + b'\0'), 15)

        self.assertEqual(shader, library.shader_data)
        self.assertEqual(0, library.shader_data_offset)
        self.assertEqual(len(shader), library.shader_size)
