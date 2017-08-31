import io

from .instruction import Instruction

class Section(object):
    def __init__(self, addr=0x4000):
        self.container = []
        self.labels = {}
        self.addr = addr
        self.ptr = addr
        self.alignment = 1

    def label(self, name):
        self.labels[name] = self.ptr

    def pos(self):
        return self.ptr

    def align(self, n=16):
        self.alignment = n
        pos = self.ptr
        if pos % self.alignment > 0:
            self.write(b'\0' * (self.alignment - pos % self.alignment))

    def write(self, data):
        if type(data) is bytes:
            self.ptr += len(data)
        elif type(data) is Instruction:
            self.ptr += 4

        self.container.append(data)
        self.align(self.alignment)
