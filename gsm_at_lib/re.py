#!/usr/bin/python
# -*- coding: utf-8 -*-
from __future__ import unicode_literals

import os

def replace(fpath, old_str, new_str):
    for path, subdirs, files in os.walk(fpath):
        for name in files:
            if(old_str.lower() in name.lower()):
                print(name)
                os.rename(os.path.join(path,name),os.path.join(path,name.lower().replace(old_str,new_str)))

replace('.',u'gsm_',u'esp_')