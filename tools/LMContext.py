#!/usr/bin/python

idapath = None
lmhome = None
dotpath = None
mntpath_prefix = None
host = None
user = None
passwd = None
db = None
cid = None
expt = None
expt_id = None
mode = None
type = None
how = None
filecount = 0
numfilesqueued = 0
workcount = 0
shortlist = 0
experiment = None
vuln = None
vulnfile = None
vulnclient = None
timeout_ms = None
QCOMPLETE = False
IDAWorkQueue = None
IDAWorkQueueLock = None
ClRdyEvt = None
BigFileLock = None
bigThresh = None
semaCount = None
missingdots = None
ngram = 0
idbext = None
bits = None


ERR_TIMEOUT_GENIDB = -1
ERR_TIMEOUT_FUNCSTATS = -2
ERR_TIMEOUT_DIST = -3
ERR_IDB_MISSING = -4
