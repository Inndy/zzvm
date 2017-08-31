import codecs
import collections
import io
import os
import re
import struct

from .instruction import Instruction
from .opcode import Opcodes
from .registers import Registers
from .section import Section, CodeSection
from .symbol import Symbol

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
                    addr = None

                if name == 'TEXT':
                    if addr is None:
                        addr = 0x4000
                    new_sect = CodeSection(addr)
                    sections['TEXT'] = new_sect
                else:
                    if addr is None:
                        addr = 0x6000
                    new_sect = Section(addr)
                    sections['DATA'] = new_sect

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
            elif line.startswith('jmp'):
                target = self.try_parse_imm(line.split()[1], rel=True)
                ins = Instruction('ADDI', 'IP', 'IP', imm=target)
                current_section.write(ins)
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

            if name == 'TEXT':
                ip = section.addr
                for ins in section.container:
                    if type(ins.imm) is Symbol:
                        sym = ins.imm
                        buff.write(ins.compose(sym.resolve(self.resolve_label, ip)))
                    else:
                        buff.write(ins.compose())
                    ip += 4
            else:
                buff.write(section.raw())

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
            if ins_name.upper() == 'JGEI':
                yield from self.parse_instruction('JEI ' + args)
                yield from self.parse_instruction('JGI ' + args)
                return
            args = [ i.strip() for i in args.split(',') ]
        except:
            ins_name = line
            args = []

        if len(args) > 0:
            if ins_name[0].upper() == 'J' or ins_name.upper() == 'CALL':
                rel = True
            else:
                rel = False

            imm = self.try_parse_imm(args[-1], rel=rel)
            if rel and imm is None:
                raise ValueError('jump instruction must have target\nline: %r' % line)
        else:
            imm = None

        if imm is None:
            yield Instruction(ins_name, *args, imm=imm)
        else:
            regs = args[:-1]
            yield Instruction(ins_name, *regs, imm=imm)

    def try_parse_imm(self, val, rel=False):
        if val[0] == '$':
            if '+' in val:
                name, offset = val[1:].split('+')
                offset = int(offset, 16)
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
