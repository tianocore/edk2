# @file NmakeSubdirs.py
# This script support parallel build for nmake in windows environment.
# It supports Python2.x and Python3.x both.
#
#  Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
#

#
# Import Modules
#

from __future__ import print_function
import argparse
import threading
import time
import os
import subprocess
import multiprocessing
import copy
import sys
__prog__        = 'NmakeSubdirs'
__version__     = '%s Version %s' % (__prog__, '0.10 ')
__copyright__   = 'Copyright (c) 2018, Intel Corporation. All rights reserved.'
__description__ = 'Replace for NmakeSubdirs.bat in windows ,support parallel build for nmake.\n'

cpu_count = multiprocessing.cpu_count()
output_lock = threading.Lock()
def RunCommand(WorkDir=None, *Args, **kwargs):
    if WorkDir is None:
        WorkDir = os.curdir
    if "stderr" not in kwargs:
        kwargs["stderr"] = subprocess.STDOUT
    if "stdout" not in kwargs:
        kwargs["stdout"] = subprocess.PIPE
    p = subprocess.Popen(Args, cwd=WorkDir, stderr=kwargs["stderr"], stdout=kwargs["stdout"])
    stdout, stderr = p.communicate()
    message = ""
    if stdout is not None:
        message = stdout.decode(errors='ignore') #for compatibility in python 2 and 3

    if p.returncode != 0:
        raise RuntimeError("Error while execute command \'{0}\' in direcotry {1}\n{2}".format(" ".join(Args), WorkDir, message))

    output_lock.acquire(True)
    print("execute command \"{0}\" in directory {1}".format(" ".join(Args), WorkDir))
    try:
        print(message)
    except:
        pass
    output_lock.release()

    return p.returncode, stdout

class TaskUnit(object):
    def __init__(self, func, args, kwargs):
        self.func = func
        self.args = args
        self.kwargs = kwargs

    def __eq__(self, other):
        return id(self).__eq__(id(other))

    def run(self):
        return self.func(*self.args, **self.kwargs)

    def __str__(self):
        para = list(self.args)
        para.extend("{0}={1}".format(k, v)for k, v in self.kwargs.items())

        return "{0}({1})".format(self.func.__name__, ",".join(para))

class ThreadControl(object):

    def __init__(self, maxthread):
        self._processNum = maxthread
        self.pending = []
        self.running = []
        self.pendingLock = threading.Lock()
        self.runningLock = threading.Lock()
        self.error = False
        self.errorLock = threading.Lock()
        self.errorMsg = "errorMsg"

    def addTask(self, func, *args, **kwargs):
        self.pending.append(TaskUnit(func, args, kwargs))

    def waitComplete(self):
        self._schedule.join()

    def startSchedule(self):
        self._schedule = threading.Thread(target=self.Schedule)
        self._schedule.start()

    def Schedule(self):
        for i in range(self._processNum):
            task = threading.Thread(target=self.startTask)
            task.daemon = False
            self.running.append(task)

        self.runningLock.acquire(True)
        for thread in self.running:
            thread.start()
        self.runningLock.release()

        while len(self.running) > 0:
            time.sleep(0.1)
        if self.error:
            print("subprocess not exit successfully")
            print(self.errorMsg)

    def startTask(self):
        while True:
            if self.error:
                break
            self.pendingLock.acquire(True)
            if len(self.pending) == 0:
                self.pendingLock.release()
                break
            task = self.pending.pop(0)
            self.pendingLock.release()
            try:
                task.run()
            except RuntimeError as e:
                if self.error: break
                self.errorLock.acquire(True)
                self.error = True
                self.errorMsg = str(e)
                time.sleep(0.1)
                self.errorLock.release()
                break

        self.runningLock.acquire(True)
        self.running.remove(threading.current_thread())
        self.runningLock.release()

def Run():
    curdir = os.path.abspath(os.curdir)
    if len(args.subdirs) == 1:
        args.jobs = 1
    if args.jobs == 1:
        try:
            for dir in args.subdirs:
                RunCommand(os.path.join(curdir, dir), "nmake", args.target, stdout=sys.stdout, stderr=subprocess.STDOUT)
        except RuntimeError:
            exit(1)
    else:
        controller = ThreadControl(args.jobs)
        for dir in args.subdirs:
            controller.addTask(RunCommand, os.path.join(curdir, dir), "nmake", args.target)
        controller.startSchedule()
        controller.waitComplete()
        if controller.error:
            exit(1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog=__prog__, description=__description__ + __copyright__, conflict_handler='resolve')

    parser.add_argument("target", help="the target for nmake")
    parser.add_argument("subdirs", nargs="+", help="the relative dir path of makefile")
    parser.add_argument("--jobs", type=int, dest="jobs", default=cpu_count, help="thread number")
    parser.add_argument('--version', action='version', version=__version__)
    args = parser.parse_args()
    Run()

