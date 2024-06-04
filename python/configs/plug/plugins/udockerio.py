#!/usr/bin/env vpython3
import os
import stat
import time
from datetime import datetime
import io
import subprocess
import re
import logging

if __name__ == "__main__":
    import sys
    class Log:
        def debug(self, msg):
            sys.stdout.write(msg)
            sys.stdout.write('\n')
    log = Log()
else:
    log = logging.getLogger(__name__)


class Entry(object):
    def __init__(
        self,
        dirname=None,
        perms=None,
        links=0,
        uid=0,
        gid=0,
        size=None,
        devmaj=None,
        devmin=None,
        date=None,
        name=None,
        date_re=None,
        datezone=None,
    ):
        self.st_mode = 0
        self.st_perms = perms
        self.st_links = int(links)
        self.st_uid = int(uid)
        self.st_gid = int(gid)
        if devmin is not None:
            self.devmaj = int(devmaj)
            self.devmin = int(devmin)
            self.st_size = 0
        else:
            self.devmaj = None
            self.devmin = None
            self.st_size = int(size)
        self.st_mtime = datetime.strptime(date, date_re)
        self.st_name = name

        if (self.st_perms[0] if self.st_perms else None) == "l" and " -> " in self.st_name:
            try:
                name, target = self.st_name.split(" -> ")
            except ValueError:
                return
            self.st_name = name

        self.perms2mode()

    def perms2mode(self):
        perms = self.st_perms[1:]
        lperms = len(perms)
        mode = 0
        # Check if 'perms' has the right format
        if not [x for x in perms if x not in "-rwxXsStT"] and lperms == 9:
            pos = lperms - 1
            for c in perms:
                if c in "sStT":
                    # Special modes
                    mode += (1 << pos // 3) << 9

                mode += 1 << pos if c in "rwxst" else 0
                pos -= 1
        self.st_mode = (
            mode
            | {
                "-": 0,
                "b": stat.S_IFBLK,
                "c": stat.S_IFCHR,
                "d": stat.S_IFDIR,
                "l": stat.S_IFLNK,
                "p": 0,  # stat.S_IF???,
                "s": 0,  # stat.S_IF???,
            }[self.st_perms[0]]
        )

    def __str__(self):
        if not self.st_name:
            return ""

        template = (
            "{st_mode:5o} {st_perms} {st_links:>4} {st_uid:<8} {st_gid:<8} {st_size:>8} {st_mtime} {st_name}"
        )
        return template.format(**self.__dict__)


class Docker(object):
    def __init__(self):
        pass

    def run(self, *args, timeout=2):
        cmd = ["/usr/bin/docker"]
        cmd.extend(args)
        # log.debug('docker.run: {}'.format(cmd))
        res = subprocess.run(cmd, capture_output=True, timeout=timeout)
        log.debug("rc={} stderr={}".format(res.returncode, res.stderr.decode()))
        res.check_returncode()
        assert res.stderr == b""
        fp = io.BytesIO(res.stdout)
        lines = fp.readlines()
        return lines

    def list(self):
        lines = self.run("container", "ps", "-a")
        devices = []
        if len(lines) > 0 and lines[0].split()[0] == b"CONTAINER":
            line = lines[0].decode()
            cid = 0
            cimage = line.find("IMAGE", 0)
            ccommand = line.find("COMMAND", cimage)
            ccreated = line.find("CREATED", ccommand)
            cstatus = line.find("STATUS", ccreated)
            cports = line.find("PORTS", cstatus)
            cnames = line.find("NAMES", cports)
            for line in lines[1:]:
                line = line.decode()
                info = (line[cid:cimage].strip(), line[cnames:].strip(), line[cstatus:cports].strip().split()[0] == 'Up')
                devices.append(info)
        return devices

    ls = "/bin/ls -anL --full-time {}"
    date_re = "%Y-%m-%d %H:%M:%S"
    file_re1 = r"""
^
(?P<perms>[\-bcdlps][\-rwxsStT]{9})\s+
(?P<links>\d+)\s+
(?P<uid>\d+)\s+
(?P<gid>\d+)\s+
(?P<devmaj>\d+),\s+(?P<devmin>\d+)\s+
(?P<date>\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})
(?P<datezone>(\.\d+)?\s+\+\d{4})\s+
(?P<name>.*)
"""
    file_re2 = r'''
^
(?P<perms>[\-bcdlps][\-rwxsStT]{9})\s+
(?P<links>\d+)\s+
(?P<uid>\d+)\s+
(?P<gid>\d+)\s+
(?P<size>\d+)\s+
(?P<date>\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})
(?P<datezone>(\.\d+)?\s+\+\d{4})\s+
(?P<name>.*)
'''

    def ls(self, deviceid, top):
        file_re1 = re.compile(self.file_re1, re.VERBOSE)
        file_re2 = re.compile(self.file_re2, re.VERBOSE)
        lines = self.run(
            "exec", deviceid, "/bin/ls", "-anL", "--full-time", top
        )
        result = []
        for line in lines:
            line = line.strip()
            if not line:
                continue
            line = line.decode()
            rm = file_re1.match(line)
            if not rm:
                rm = file_re2.match(line)
            if not rm:
                if line.split()[0] != "total":
                    log.debug("ignored entry in ({}): {}".format(top, line))
                continue
            entry = Entry(**rm.groupdict(), date_re=self.date_re)
            result.append(entry)
        return result

    def pull(self, deviceid, sqname, dqname):
        lines = self.run("cp", "{}:{}".format(deviceid, sqname), dqname)
        for line in lines:
            log.debug("pull:".format(line))

    def push(self, deviceid, sqname, dqname):
        lines = self.run("cp", sqname, "{}:{}".format(deviceid, dqname))
        for line in lines:
            log.debug("push:".format(line))

    def mkdir(self, deviceid, dqname):
        self.run("exec", deviceid, "mkdir", dqname)

    def remove(self, deviceid, dqname):
        self.run("exec", deviceid, "rm", "-rf", dqname)


if __name__ == "__main__":
    cls = Docker()
    info = cls.list()
    print(info)
    result = cls.ls(info[0][0], "/")
    for e in result:
        print(e)
