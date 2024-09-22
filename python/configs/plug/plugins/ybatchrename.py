
"""
Plugin to rename list of files all at once, using full power of 
FAR text editor.
"""

__author__ = 'Yaroslav Yanovsky'

import os
import tempfile

from yfar import FarPlugin


RENAME_INFO = b'''Rename list of files. File names to the right of slashes
are destination names. File names to the left are given for reference only,
you can modify them too - only the line order is important.
Full destination paths are supported too - either absolute or relative.

'''

class Plugin(FarPlugin):
    label = 'Batch Rename Files and Directories'
    openFrom = ['PLUGINSMENU', 'FILEPANEL']

    def OpenPlugin(self, _):
        panel = self.get_panel()

        dir = panel.directory
        names = []
        for sel in panel.selected:
            full_fn = sel.file_name
            if '/' in full_fn:
                if dir:
                    self.error('Panel directory {} and full panel name '
                               '{} conflict'.format(dir, full_fn))
                    return
            elif dir:
                full_fn = os.path.join(dir, full_fn)
            else:
                self.error('Panel name {} is local and panel directory not '
                           'specified')
                return
            names.append([full_fn])
        if not names:
            self.notice('No files selected')
            return

        max_len = max(len(os.path.basename(x[0])) for x in names)
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(RENAME_INFO)
            for fn, in names:
                fn = os.path.basename(fn)
                f.write(('{} / {}\n'.format(fn.ljust(max_len), fn)).encode())

        renamed = skipped = 0
        failed = []
        if self.editor(f.name, 'Rename list of files', 
                       line=RENAME_INFO.count(b'\n') + 1, column=max_len + 4):
            i = 0
            with open(f.name, 'rb') as f:
                used_names = set()
                for line in f:
                    if b'/' in line:
                        if i >= len(names):
                            self.error('Aborted: edited list has more '
                                       'names than required')
                            break
                        dst_fn = line.split(b'/', 1)[1].strip().decode()
                        if dst_fn.startswith('~'):
                            dst_fn = os.path.expanduser(dst_fn)
                        elif not dst_fn.startswith('/'):
                            dst_fn = os.path.abspath(os.path.join(
                                os.path.dirname(names[i][0]), dst_fn))
                        if dst_fn in used_names:
                            self.error('Aborted: multiple destination names '
                                       'point to the same file\n'
                                       '{}'.format(dst_fn))
                            break
                        names[i].append(dst_fn)
                        used_names.add(dst_fn)
                        i += 1
                else:
                    if i == len(names):
                        for src_fn, dst_fn in names:
                            if src_fn == dst_fn:
                                skipped += 1
                                continue
                            if os.path.exists(dst_fn):
                                failed.append(dst_fn + ': already exists')
                                continue
                            dst_dir, fn = os.path.split(dst_fn)
                            if not os.path.exists(dst_dir):
                                # create dst dir if not exist
                                try:
                                    os.makedirs(dst_dir)
                                except Exception as e:
                                    failed.append(fn + ' - mkdir: ' + str(e))
                                    continue
                            try:
                                os.rename(src_fn, dst_fn)
                            except Exception as e:
                                failed.append(fn + ': ' + str(e))
                            else:
                                renamed += 1
                    else:
                        self.error('Aborted: edited list has less names than '
                                   'required')
            panel.refresh()
            if renamed or skipped or failed:
                msg = '{:d} file(s) renamed'.format(renamed)
                if skipped:
                    msg += ', {:d} skipped'.format(skipped)
                if failed:
                    self.error(msg + ', {:d} failed:\n{}'.format(
                        len(failed), '\n'.join(failed)))
                else:
                    self.notice(msg)

        if os.path.exists(f.name):
            os.unlink(f.name)
                