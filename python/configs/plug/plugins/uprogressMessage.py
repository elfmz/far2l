import time
import logging
from far2l.plugin import PluginBase
from far2l.farprogress import ProgressMessage

log = logging.getLogger(__name__)

class Plugin(PluginBase):
    label = "Python Progress Message"
    openFrom = ["PLUGINSMENU"]

    def OpenPlugin(self, OpenFrom):
        t = ProgressMessage(self, "Progress demo", "Please wait ... working", 100)
        t.show()
        time.sleep(2)
        for i in range(0, 100, 20):
            if t.aborted():
                break
            t.update(i)
            time.sleep(2)
        t.close()
