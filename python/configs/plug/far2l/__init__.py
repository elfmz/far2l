import sys
import io
from . import logger


try:
    from .pluginmanager import PluginManager
    pluginmanager = PluginManager()
except:
    fp = io.StringIO()
    import traceback
    traceback.print_exc(file=fp)
    sys.stderr.write(fp.getvalue())
