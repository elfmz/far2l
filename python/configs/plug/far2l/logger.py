import os
import sys

class Logger:
    def __init__(self):
        self.stdout = sys.stdout
        self.stderr = sys.stderr
        self.fp = open('/tmp/far2.py.log-py','at')
    def write(self, msg):
        self.fp.write(msg)
        self.fp.flush()
    def flush(self):
        self.fp.flush()
    def close(self):
        self.fp.close()
        sys.stdout = self.stdout
        sys.stderr = self.stderr

sys.stderr = sys.stdout = Logger()

print('%s start' % ('*'*20))
print('sys.path=', sys.path)
print('cwd=', os.getcwd())
