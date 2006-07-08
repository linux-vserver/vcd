======================================
 VServer Control Daemon Specification
======================================

:Version: 0
:Created: 23 Mar 2006
:Last-Modified: 06 Jul 2006 11:20 UTC
:Author: Benedikt Boehm
:Status: Draft

**THIS IS A DRAFT, NOT A PUBLISHED VERSION OF THE SPECIFICATION**

Abstract
========

In order to ease management of Virtual Private Servers a central instance
is needed to provide an open and well-known Application Programming
Interface (API). This specification introduces the VServer Control Daemon
(VCD) -- a daemon running in the host context that provides the
aforementioned API via XMLRPC [1]_. It also takes care of collecting
time-series data usable for RRDtool [2]_ analyzes.


Motivation
==========

There is an emerging need for a well-known and easy to use API accessible
from a broad range of clients regardless of language or location of the
caller. Additionally collecting time-series data is needed for extensive
configurations in data centers.


Rationale
=========

The current user-space implementations of the VServer kernel API [3]_ [4]_
suffer a mechanism to call any of the management commands regardless of the
language or location of the caller. Such callers include non-C lanuages
like Python, PHP or Ruby, remote GUIs for KDE, Gnome or even Windows as
well as web control panels for service providers.

Therefore this specification defines an API accessible by any caller
capable of both the HTTP and the XMLRPC protocol -- two open standards
implemented in most common languages.


Specification
=============

The daemon consists of three major parts:

1. XMLRPC Server
2. Data Collector
3. VPS Killer


XMLRPC Server
-------------

The server implements the XMLRPC standard for Remote Procedure Calls
(RPC). Based on XML nearly any frontend is capabale of sending remote
method requests. Additionally plain-text open-standard protocols are very
easy to trace and debug.

The server defines a global registry of methods accessible by its clients.
All methods should be registered with documentation and methods for an inline
help system should be povided. For a list of methods see below.

The server should implement a form of Transport Layer Security
such as SSL3 or TLS. Actually the server should never listen on untrusted
LANs or WANs without encryption. However, the administrator is responsible for
setting up secure remote connections.

The client connection is at no time persistent or kept alive. After a
request the output or fault notification is sent to the client and the
connection is closed regardless of the return code. Therefore
authentication information has to be resubmitted with every method call.

Authentication is based on the cryptographic hash function SHA-1. At no point
the clear-text password is stored in the servers database. For a fine-grained
access control the server implements its own set of capabilities:

  ====== ==========================================================
   ID    Description
  ====== ==========================================================
  AUTH   User may call methods for authentication
  DLIM   User may call methods for disk limits
  INIT   User may call methods for start/stop and init methods
  MOUNT  User may call methods for mount points
  NET    User may call methods for network interfaces
  BCAP   User may call methods for system capabilities
  CCAP   User may call methods for context capabilities
  CFLAG  User may call methods for context flags
  RLIM   User may call methods for resource limits
  SCHED  User may call methods for context schedulers
  UNAME  User may call methods for utsname/virtual host information
  CREATE User may call methods for creation and destruction of VPSs
  HELPER User may call helper methods
  ====== ==========================================================

The client submits the username and the password as first value in the request
array using a struct with the following signature: ``{s:s,s:s}``. The members
of this struct must be named ``username`` and ``password``.

The second value in the request array is a variable-length struct of parameters
specific to the requested method.

The server may return one of the following error codes:

  ====== ======================================================
   ID     Description
  ====== ======================================================
  ====== ======================================================

Note that the underlying XMLRPC library may sent different internal error codes.
Read the implementation documentation for your server.

For each of the above error codes the server may override the default error
message with a custom one to make fault notifications more expressive.


Data Collector
--------------

The Data Collector works mainly in background and polls different
statistical resources such as entries in ``/proc/virtual/`` to collect
time-series data for analysis with RRDtool or one of its frontends such as
Cacti [5]_.

The following data sources should be collected at least:

  ============ ======== ==================================
   DS           DST      Description
  ============ ======== ==================================
  mem_VM       GAUGE    Virtual memory pages
  mem_VML      GAUGE    Pages locked into memory
  mem_RSS      GAUGE    Resident Set Size
  mem_ANON     GAUGE    Anonymous memory pages
  mem_SHM      GAUGE    Shared memory pages
  file_FILES   GAUGE    File handles
  file_OFD     GAUGE    Open file descriptors
  file_LOCKS   GAUGE    Filesystem locks
  file_SOCK    GAUGE    Number of sockets
  ipc_MSGQ     GAUGE    Message queue size
  ipc_SEMA     GAUGE    ??
  ipc_SEMS     GAUGE    ??
  sys_PROC     GAUGE    Number of processes
  sys_LOADAVG  GAUGE    Load average
  thread_TOTAL GAUGE    Number of threads
  thread_RUN   GAUGE    Number of running threads
  thread_NOINT GAUGE    Number of uninteruptable processes
  thread_HOLD  GAUGE    Number of processes on hold
  net_UNIX     COUNTER  Unix socket traffic
  net_INET     COUNTER  IPv4 socket traffic
  net_INET6    COUNTER  IPv6 socket traffic
  net_OTHER    COUNTER  Other socket traffic
  ============ ======== ==================================

The collector should store data for the following resolutions at least:
  - 30 minutes
  - 6 hours
  - 1 day
  - 1 month
  - 1 year

The following round-robin archives should be stored for each data source:

  ======== =====================
     CF     Description
  ======== =====================
  AVERAGE  Average amount/sec
  MIN      Minimum value reached
  MAX      Maximum value reached
  ======== =====================

The Data Collector rarely requires user interaction. In special cases the
administrator may use the ``rrd.reset`` and ``rrd.dump`` methods provided
by the XMLRPC Server mentioned above.


VPS Killer
----------

The VPS Killer is used to handle the disposal of VPSs. During a stop,
restart or reboot request the procedure is as follows:

  1. Spawn a thread waiting for the VPS to exit using a timeout
  2. If the timeout is reached kill all processes in the VPS and go back to
     1
  3. Run shutdown sequence for the host (network interfaces, scripts)
  4. If a reboot request was recorded start VPS again

Using the definition above stop, restart and reboot requests are handled as
follows:

stop
  1. Start VPS Killer
  2. Run VPS init/rc shutdown command
  3. Wait for VPS Killer

restart
  1. Record reboot request
  2. Start VPS Killer
  3. Run VPS init/rc shutdown command
  4. Wait for VPS Killer

reboot
  0. VPS calls sys_reboot
  1. Kernel calls VCD using vshelper
  2. Record reboot request
  3. Start VPS Killer
  4. Run VPS init/rc shutdown command for init-less VPSs
  5. Return from 1


Valid Methods
=============

  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | Method Name              | Request                                   | Response                                 | Capability | Ownercheck |
  +==========================+===========================================+==========================================+============+============+
  | vx.create                | ``{s:s,s:s,s:i}``                         | ``[ ]``                                  | CREATE     | no         |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``template``                           |                                          |            |            |
  |                          |  - ``rebuild``                            |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.killer                | ``{s:s,s:i,s:i}``                         | ``[ ]``                                  | INIT       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``wait``                               |                                          |            |            |
  |                          |  - ``reboot``                             |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.remove                | ``{s:s}``                                 | ``[ ]``                                  | CREATE     | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.rename                | ``{s:s,s:s}``                             | ``[ ]``                                  | CREATE     | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``newname``                            |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.start                 | ``{s:s}``                                 | ``[ ]``                                  | INIT       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.status                | ``{s:s}``                                 | ``{s:i}``                                | INIT       | yes        |
  |                          |  - ``name``                               |  - ``running``                           |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vx.stop                  | ``{s:s,s:i,s:i}``                         | ``[ ]``                                  | INIT       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``wait``                               |                                          |            |            |
  |                          |  - ``reboot``                             |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.dx.limit.get        | ``{s:s,[s:s]}``                           | ``({s:s,s:i,s:i,s:i}*)``                 | DLIM       | yes        |
  |                          |  - ``name``                               |  - ``path``                              |            |            |
  |                          |  - ``path``                               |  - ``space``                             |            |            |
  |                          |                                           |  - ``inodes``                            |            |            |
  |                          |                                           |  - ``reserved``                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.dx.limit.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | DLIM       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``path``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.dx.limit.set        | ``{s:s,s:s,s:i,s:i,s:i}``                 | ``[ ]``                                  | DLIM       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``path``                               |                                          |            |            |
  |                          |  - ``space``                              |                                          |            |            |
  |                          |  - ``inodes``                             |                                          |            |            |
  |                          |  - ``reserved``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.init.method.get     | ``{s:s}``                                 | ``{s:s,s:s,s:s,s:i}``                    | INIT       | yes        |
  |                          |  - ``name``                               |  - ``method``                            |            |            |
  |                          |                                           |  - ``start``                             |            |            |
  |                          |                                           |  - ``stop``                              |            |            |
  |                          |                                           |  - ``timeout``                           |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.init.method.set     | ``{s:s,s:s,[s:s],[s:s],s:i}``             | ``[ ]``                                  | INIT       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``method``                             |                                          |            |            |
  |                          |  - ``start``                              |                                          |            |            |
  |                          |  - ``stop``                               |                                          |            |            |
  |                          |  - ``timeout``                            |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.list                | ``[ ]``                                   | ``(s*)``                                 |            | yes        |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.mount.get           | ``{s:s,[s:s]}``                           | ``({s:s,s:s,s:s,s:s}*)``                 | MOUNT      | yes        |
  |                          |  - ``name``                               |  - ``path``                              |            |            |
  |                          |  - ``path``                               |  - ``spec``                              |            |            |
  |                          |                                           |  - ``mntops``                            |            |            |
  |                          |                                           |  - ``vfstype``                           |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.mount.remove        | ``{s:s,[s:s]}``                           | ``[ ]``                                  | MOUNT      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``path``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.mount.set           | ``{s:s,s:s,[s:s],[s:s],[s:s]}``           | ``[ ]``                                  | MOUNT      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``path``                               |                                          |            |            |
  |                          |  - ``spec``                               |                                          |            |            |
  |                          |  - ``mntops``                             |                                          |            |            |
  |                          |  - ``vfstype``                            |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.name.get            | ``{s:i}``                                 | ``s``                                    | HELPER     | no         |
  |                          |  - ``xid``                                |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.addr.get         | ``{s:s,[s:s]}``                           | ``({s:s,s:s,s:s}*)``                     | NET        | yes        |
  |                          |  - ``name``                               |  - ``addr``                              |            |            |
  |                          |  - ``addr``                               |  - ``netmask``                           |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.addr.remove      | ``{s:s,[s:s]}``                           | ``[ ]``                                  | NET        | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``addr``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.addr.set         | ``{s:s,s:s,[s:s]}``                       | ``[ ]``                                  | NET        | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``addr``                               |                                          |            |            |
  |                          |  - ``netmask``                            |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.broadcast.get    | ``{s:s}``                                 | ``s``                                    | NET        | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.broadcast.remove | ``{s:s}``                                 | ``[ ]``                                  | NET        | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.nx.broadcast.set    | ``{s:s,s:s}``                             | ``[ ]``                                  | NET        | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``broadcast``                          |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.owner.add           | ``{s:s,s:s}``                             | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``username``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.owner.get           | ``{s:s}``                                 | ``(s*)``                                 | AUTH       | no         |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.owner.remove        | ``{s:s,s:s}``                             | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``username``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.caps.add       | ``{s:s,s:s}``                             | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``username``                           |                                          |            |            |
  |                          |  - ``cap``                                |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.caps.get       | ``{[s:s]}``                               | ``(s*)``                                 | AUTH       | no         |
  |                          |  - ``username``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.caps.remove    | ``{s:s,[s:s]}``                           | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``username``                           |                                          |            |            |
  |                          |  - ``cap``                                |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.get            | ``{[s:s]}``                               | ``({s:s,s:i,s:i}*)``                     | AUTH       | no         |
  |                          |  - ``username``                           |  - ``username``                          |            |            |
  |                          |                                           |  - ``uid``                               |            |            |
  |                          |                                           |  - ``admin``                             |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.remove         | ``{s:s}``                                 | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``username``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.user.set            | ``{s:s,[s:s],s:i}``                       | ``[ ]``                                  | AUTH       | no         |
  |                          |  - ``username``                           |                                          |            |            |
  |                          |  - ``password``                           |                                          |            |            |
  |                          |  - ``admin``                              |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vdir.get            | ``{s:s}``                                 | ``s``                                    | HELPER     | no         |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.bcaps.add        | ``{s:s,s:s}``                             | ``[ ]``                                  | BCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``bcap``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.bcaps.get        | ``{[s:s]}``                               | ``(s*)``                                 | BCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.bcaps.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | BCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``bcap``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.ccaps.add        | ``{s:s,s:s}``                             | ``[ ]``                                  | CCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``ccap``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.ccaps.get        | ``{[s:s]}``                               | ``(s*)``                                 | CCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.ccaps.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | CCAPS      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``ccap``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.flags.add        | ``{s:s,s:s}``                             | ``[ ]``                                  | CFLAGS     | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``flag``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.flags.get        | ``{[s:s]}``                               | ``(s*)``                                 | CFLAGS     | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.flags.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | CFLAGS     | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``flag``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.limit.get        | ``{[s:s],[s:s]}``                         | ``({s:s,s:i,s:i}*)``                     | RLIM       | yes        |
  |                          |  - ``name``                               |  - ``limit``                             |            |            |
  |                          |  - ``limit``                              |  - ``soft``                              |            |            |
  |                          |                                           |  - ``max``                               |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.limit.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | RLIM       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``limit``                              |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.limit.set        | ``{s:s,s:s,s:i,s:i}``                     | ``[ ]``                                  | RLIM       | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``limit``                              |                                          |            |            |
  |                          |  - ``soft``                               |                                          |            |            |
  |                          |  - ``max``                                |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.sched.get        | ``{s:s,s:i}``                             | ``({s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}*)`` | SCHED      | yes        |
  |                          |  - ``name``                               |  - ``interval``                          |            |            |
  |                          |  - ``cpuid``                              |  - ``fillrate``                          |            |            |
  |                          |                                           |  - ``interval2``                         |            |            |
  |                          |                                           |  - ``fillrate2``                         |            |            |
  |                          |                                           |  - ``tokensmin``                         |            |            |
  |                          |                                           |  - ``tokensmax``                         |            |            |
  |                          |                                           |  - ``priobias``                          |            |            |
  |                          |                                           |  - ``cpuid``                             |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.sched.remove     | ``{s:s,s:i}``                             | ``[ ]``                                  | SCHED      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``cpuid``                              |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.sched.set        | ``{s:s,s:i,s:i,s:i,s:i,s:i,s:i,s:i,s:i}`` | ``[ ]``                                  | SCHED      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``cpuid``                              |                                          |            |            |
  |                          |  - ``interval``                           |                                          |            |            |
  |                          |  - ``fillrate``                           |                                          |            |            |
  |                          |  - ``interval2``                          |                                          |            |            |
  |                          |  - ``fillrate2``                          |                                          |            |            |
  |                          |  - ``tokensmin``                          |                                          |            |            |
  |                          |  - ``tokensmax``                          |                                          |            |            |
  |                          |  - ``priobias``                           |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.uname.get        | ``{[s:s],[s:s]}``                         | ``({s:s,s:s}*)``                         | UNAME      | yes        |
  |                          |  - ``name``                               |  - ``uname``                             |            |            |
  |                          |  - ``uname``                              |  - ``value``                             |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.uname.remove     | ``{s:s,[s:s]}``                           | ``[ ]``                                  | UNAME      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``uname``                              |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.vx.uname.set        | ``{s:s,s:s,s:s}``                         | ``[ ]``                                  | UNAME      | yes        |
  |                          |  - ``name``                               |                                          |            |            |
  |                          |  - ``uname``                              |                                          |            |            |
  |                          |  - ``value``                              |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+
  | vxdb.xid.get             | ``{s:s}``                                 | ``i``                                    | HELPER     | no         |
  |                          |  - ``name``                               |                                          |            |            |
  +--------------------------+-------------------------------------------+------------------------------------------+------------+------------+



References
==========

.. [1] XMLRPC specification; Tue, Jun 15, 1999; Dave Winer
       (http://www.xmlrpc.com/spec)

.. [2] Round Robin Database tool; Tobi Oetiker
       (http://people.ee.ethz.ch/~oetiker/webtools/rrdtool/)

.. [3] util-vserver; Enrico Scholz
       (http://savannah.nongnu.org/projects/util-vserver)

.. [4] vserver-utils; Benedikt Boehm
       (http://dev.croup.de/vserver-utils)

.. [5] Cacti - network graphing solution; Cacti Group
       (http://www.cacti.net)


Copyright
=========

This document has been placed in public domain.
