class Symbol(object):
    def __init__(self, name, offset=0, is_relative=False):
        self.name = name
        self.offset = offset
        self.is_relative = is_relative

    def __repr__(self):
        return 'Symbol(%s)' % ', '.join(
            '%s=%r' % (attr, getattr(self, attr))
            for attr in ('name', 'offset', 'is_relative')
        )

    def resolve(self, find, current):
        addr = find(self.name) + self.offset
        if addr is None:
            raise ValueError('Can not resolve symbol %s' % self.name)

        if self.is_relative:
            return addr - 4 - current
        else:
            return addr
