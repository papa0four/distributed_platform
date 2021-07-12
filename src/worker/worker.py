#!/usr/bin/env python3

import os
import socket

if __name__ == "__main__" and __package__ is None:
    from sys import path
    from os.path import dirname as dir

    path.append(dir(path[0]))
    __package__ = "worker"

# import modules.broadcast as bc
import modules.connect_to_scheduler as cts

cts.connect_to_scheduler()