import logging
import uuid
from far2l.plugin import PluginBase


log = logging.getLogger(__name__)


class Plugin(PluginBase):
    label = "Python uuuidgen"
    openFrom = ["PLUGINSMENU", "EDITOR"]

    @staticmethod
    def HandleCommandLine(line):
        return line in ("uuidgen",)

    def CommandLine(self, line):
        if line != "uuidgen":
            return
        self.uuidgen()

    def uuidgen(self):
        s = '{%s}'%uuid.uuid4()
        log.debug('uuidgen: {}'.format(s))
        self.info.FSF.CopyToClipboard(self.s2f(s))

    def OpenPlugin(self, OpenFrom):
        self.uuidgen()
        return -1
