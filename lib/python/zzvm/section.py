import io

class Section(object):
    def __init__(self, addr):
        self.container = io.BytesIO()
        self.labels = {}
        self.addr = addr
        self.alignment = 1

    def label(self, name):
        self.labels[name] = self.pos()

    def pos(self):
        return self.addr + self.container.tell()

    def align(self, n=16):
        self.alignment = n

        pos = self.container.tell()
        if pos % self.alignment > 0:
            self.container.write(b'\0' * (self.alignment - pos % self.alignment))

    def write(self, data):
        self.container.write(data)
        self.align(self.alignment)

        return self.addr + self.container.tell()

    def raw(self):
        return self.container.getvalue()

class CodeSection(Section):
    def __init__(self, addr=0x4000):
        self.container = []
        self.labels = {}
        self.addr = addr

    def label(self, name):
        self.labels[name] = self.pos()

    def pos(self):
        return self.addr + len(self.container) * 4

    def write(self, data):
        if type(data) is bytes:
            assert len(data) == 4
        self.container.append(data)
        return self.pos()
