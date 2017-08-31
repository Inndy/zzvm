import codecs
import collections
import io
import os
import re
import struct

from .instruction import Instruction
from .opcode import Opcodes
from .registers import Registers
from .section import Section
from .symbol import Symbol

def p32(v):
    return struct.pack('<I', v)

def unescape_str_to_bytes(x):
    return codecs.escape_decode(x.encode('utf8'))[0]

class QueueReader(object):
    def __init__(self, *files):
        self.fq = list(files)

    def add_file(self, f):
        self.fq.append(f)

    def insert_file(self, f, idx=0):
        self.fq.insert(idx, f)

    def readline(self):
        while len(self.fq) > 0:
            r = self.fq[0].readline()
            if not r:
                self.fq.pop(0)
                continue

            return r

        return ''

class Parser(object):
    def __init__(self, fin):
        self.sections = None
        self.section_bodies = {}
        self.entry = None

        if type(fin) is str:
            fin = io.StringIO(fin)
        self.reader = QueueReader(fin)

        self.parse()

    def parse(self):
        sections = collections.OrderedDict()
        current_section = None
        lineno = 0

        while True:
            lineno += 1
            raw = self.reader.readline()
            if not raw:
                break
            line = raw.split(';')[0].strip()
            if not line:
                continue
            elif line.startswith('.sect'):
                args = line.split(maxsplit=1)[1].split(' ')
                name = args[0].upper()

                if len(args) > 1:
                    addr = int(args[1], 16)
                else:
                    if name == 'TEXT':
                        addr = 0x4000
                    else:
                        addr = 0x6000

                new_sect = Section(addr)
                sections[name] = new_sect
                current_section = new_sect
            elif line.startswith('.include'):
                filename = line.split(maxsplit=1)[1].strip()
                if filename.startswith('zstdlib/'):
                    filename = os.path.join(os.path.dirname(__file__), '../../..', filename)
                self.reader.insert_file(open(filename))
            elif line.startswith('.entry'):
                entry = line.split()[1]
                return self.try_parse_imm(entry)
            elif line.startswith('.align'):
                current_section.align(self._parse_int(line.split()[1]))
            elif line.startswith('.db'):
                data = line[3:].split(',')
                bytes_data = bytes(int(i.strip(), 16) for i in data)
                current_section.write(bytes_data)
            elif line.startswith('.zero'):
                data = line[5:].strip()
                if data.startswith('0x'):
                    n = int(data, 16)
                else:
                    n = int(data)
                current_section.write(b'\0' * n)
            elif line.startswith('.str'):
                data = line[4:].strip()
                bytes_data = unescape_str_to_bytes(data[1:-1])
                current_section.write(bytes_data + b'\0\0')
            elif line[-1] == ':':
                label_name = line[:-1]
                current_section.label(label_name)
            else:
                for ins in self.parse_instruction(line):
                    current_section.write(ins)

        self.sections = sections

    def resolve_label(self, name):
        for section in self.sections.values():
            addr = section.labels.get(name, None)
            if addr:
                return addr

    def get_entry(self):
        if type(self.entry) is Symbol:
            return self.entry.resolve(self.resolve_label)
        elif self.entry is not None:
            return self.entry
        elif self.resolve_label('start'):
            return self.resolve_label('start')
        else:
            return 0x4000

    def build(self):
        sections = []
        bodies = []

        for name, section in self.sections.items():
            buff = io.BytesIO()

            ip = section.addr
            for data in section.container:
                if type(data) is Instruction:
                    ins = data
                    if type(ins.imm) is Symbol:
                        sym = ins.imm
                        buff.write(ins.compose(sym.resolve(self.resolve_label, ip)))
                    else:
                        buff.write(ins.compose())
                    ip += 4
                elif type(data) is Symbol:
                    val = data.resolve(self.resolve_label, ip)
                    buff.write(p32(val))
                    ip += 4
                elif type(data) is bytes:
                    buff.write(data)
                    ip += len(data)

            body = buff.getvalue()
            self.section_bodies[name] = body

            bodies.append(body)
            sections.append(struct.pack('<HH',
                section.addr, # section_addr
                len(body),    # section_size
            ))

        header = struct.pack('<ccHHH',
            b'Z', b'z',       # magic
            0,                # file_ver
            self.get_entry(), # entry
            len(bodies),      # section_count
        )

        return header + b''.join(sections) + b''.join(bodies)

    def parse_instruction(self, line):
        try:
            ins_name, args = line.split(maxsplit=1)
            args = [ i.strip() for i in args.split(',') ]
        except:
            ins_name = line
            args = []

        if ins_name.upper() == 'JMP':
            is_jmp = True
            ins_name = 'ADDI'
            args = ['IP', 'IP', args[0]]
        else:
            is_jmp = False

        if len(args) > 0:
            if ins_name[0].upper() == 'J' or ins_name.upper() == 'CALL' or is_jmp:
                rel = True
            else:
                rel = False

            imm = self.try_parse_imm(args[-1], rel=rel)
            if imm is None:
                if rel:
                    raise ValueError('jump instruction must have target\nline: %r' % line)
                regs = args
            else:
                regs = args[:-1]
            yield Instruction(ins_name, *regs, imm=imm)
        else:
            yield Instruction(ins_name, *args)

    def try_parse_imm(self, val, rel=False):
        if val[0] == '$':
            if '+' in val:
                name, offset = val[1:].split('+')
                offset = self._parse_int(offset)
                return Symbol(name, offset, is_relative=rel)
            else:
                return Symbol(val[1:], is_relative=rel)

        try:
            return self._parse_int(val)
        except:
            pass

    def _parse_int(self, s):
        s = s.strip()
        if s[:2] == '0x':
            return int(s, 16)
        elif s[0] == '#':
            return int(s[1:], 10)
        else:
            return int (s)
