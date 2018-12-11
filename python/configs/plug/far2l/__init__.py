import sys
import io
from . import logger
if 0:
    from . import rpdb
    rpdb.RemotePdb(host='127.0.0.1', port=4444).set_trace()


try:
    from .plugin import Plugin
    pluginmanager = Plugin()
except:
    fp = io.StringIO()
    import traceback
    traceback.print_exc(file=fp)
    sys.stderr.write(fp.getvalue())
