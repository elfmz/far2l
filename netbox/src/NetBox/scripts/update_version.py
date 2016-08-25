#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, string
import re, time

plugin_version_h = \
"""\
//------------------------------------------------------------------------------
// plugin_version.hpp
//
//------------------------------------------------------------------------------
#pragma once

#include <string>

#define NETBOX_VERSION_MAJOR         %(version_major)s
#define NETBOX_VERSION_MINOR         %(version_minor)s
#define NETBOX_VERSION_PATCH         %(version_patch)s
#define NETBOX_VERSION_BUILD         %(build)s

static const std::wstring NETBOX_VERSION_NUMBER(L"%(version_major)s.%(version_minor)s.%(version_patch)s");

"""

PLUGIN_VERSION_FILE = 'plugin_version.hpp'

def write_header(version_major, version_minor, version_patch, build, git_revision):
    f = open(PLUGIN_VERSION_FILE, "w")
    compile_time = time.strftime('%d.%m.%Y %H:%M:%S')
    plugin_version_h_content = plugin_version_h % locals()
    f.write(plugin_version_h_content)
    f.close()
    return

###############################################################################
# transform_template_file: generate file from .template file
#
###############################################################################

def transform_template_file(template_file_name, file_name,
    major_version, minor_version, version_patch, build, git_revision):
    # print 'transform_template_file: major_version = %s' % major_version
    # print '%s: %s.%s.%s.%s' % (rc_file, major_version, minor_version, version_patch, git_revision)
    contents = []
    f = open(template_file_name)
    for line in f.readlines():
        # print 'line = %s' % line.rstrip()
        s = line.replace('${file.ver.major}', major_version)
        s = s.replace('${file.ver.minor}', minor_version)
        s = s.replace('${file.ver.version_patch}', version_patch)
        s = s.replace('${file.ver.build}', build)
        # s = s.replace('${file.ver.revision}', revision)
        s = s.replace('${file.ver.gitrevision}', git_revision)
        contents.append(s)
    f.close()

    f = open(file_name, 'w')
    for line in contents:
        f.write(line)
    f.close()
    return
    #--transform_template_file

def transform_template_files(version_major, version_minor, version_patch, build, git_revision):
    template_files = ['resource.h.template', 'NetBox.rc.template']
    for template_file_name in template_files :
        file_name = template_file_name[0:-len('.template')] # + '.tmp'
        transform_template_file(template_file_name, file_name,
            version_major, version_minor, version_patch, build, git_revision)
    return

def getstatusoutput(cmdstr):
    """ replacement for command.getstatusoutput """
    pipe = os.popen(cmdstr)
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    return sts, text
    #--getstatusoutput

def get_git_revision():
    git_revision = ''
    git_re = re.compile(r'^.+g(\w+)$')
    git_re2 = re.compile(r'^(\w+)$')
    try:
        status, output = getstatusoutput('git describe --always')
        # print('output = %s' % output)
        # print(status)
        # print(output)
        context = git_re.match(output)
        # print(context)
        if context:
            git_revision = context.groups(1)[0]
            # print(git_revision)
            git_revision = git_revision
        else:
            context = git_re2.match(output)
            if context:
                git_revision = context.groups(1)[0]
                git_revision = git_revision
    except Exception, info:
        git_revision = ''
        print(info)
        pass
    return git_revision

def get_revision():
    git_revision = get_git_revision()
    if git_revision is None:
        git_revision = ''
    return git_revision

def get_build_number():
    """ read build number from plugin_version.hpp """
    build_number = "0"
    build_re = re.compile(r'^\#define NETBOX_VERSION_BUILD\s+(\d+).*$')
    try :
        f = open(PLUGIN_VERSION_FILE)
        for line in f.readlines():
            context = build_re.match(line)
            if context:
                build_number = context.groups(1)[0]
                build_number = str(int(build_number) + 1)
                # print('build_number = %s' % build_number)
                break
    except Exception :
        pass
    return build_number

def main():
    ver_re = re.compile(r'^NetBox\s(\d+)\.(\d+)\.(\d+).*$')
    f = open('../../ChangeLog')

    for line in f.readlines():
        context = ver_re.match(line)
        if context:
            version_major = context.groups(1)[0]
            version_minor = context.groups(1)[1]
            version_patch = context.groups(1)[2]
            build = get_build_number()
            git_revision = get_revision()
            write_header(version_major, version_minor, version_patch, build, git_revision)
            transform_template_files(version_major, version_minor, version_patch, build, git_revision)
            break
    return

if __name__ == '__main__':
    main()
