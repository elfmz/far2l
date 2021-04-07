import os
import sys
import configparser
import logging
import logging.config

logging.basicConfig(level=logging.INFO)

def setup():
    fname = __file__.replace('.py','.ini')
    with open(fname, "rt") as fp:
        ini = configparser.ConfigParser()
        ini.read_file(fp)
        logging.config.fileConfig(ini)

setup()

log = logging.getLogger(__name__)
log.debug('%s start' % ('*'*20))
log.debug('sys.path={0}'.format(sys.path))
log.debug('cwd={0}'.format(os.getcwd()))

def handle_exception(exc_type, exc_value, exc_traceback):
    if issubclass(exc_type, KeyboardInterrupt):
        sys.__excepthook__(exc_type, exc_value, exc_traceback)
        return

    log.error("Uncaught exception", exc_info=(exc_type, exc_value, exc_traceback))

sys.excepthook = handle_exception
