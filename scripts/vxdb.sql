BEGIN TRANSACTION;

CREATE TABLE IF NOT EXISTS dx_limit (
  xid SMALLINT NOT NULL,
  path TEXT NOT NULL,
  space INT NOT NULL,
  inodes INT NOT NULL,
  reserved TINYINT,
  UNIQUE(xid, path)
);

CREATE TABLE IF NOT EXISTS init (
  xid SMALLINT NOT NULL,
  init TEXT NOT NULL,
  halt TEXT,
  reboot TEXT,
  timeout INT,
  UNIQUE(xid)
);

CREATE TABLE IF NOT EXISTS mount (
  xid SMALLINT NOT NULL,
  src TEXT NOT NULL,
  dst TEXT NOT NULL,
  type TEXT,
  opts TEXT,
  UNIQUE(xid, dst)
);

CREATE TABLE IF NOT EXISTS nx_addr (
  xid SMALLINT NOT NULL,
  addr TEXT NOT NULL,
  netmask TEXT NOT NULL,
  UNIQUE(xid, addr)
);

CREATE TABLE IF NOT EXISTS nx_broadcast (
  xid SMALLINT NOT NULL,
  broadcast TEXT NOT NULL,
  UNIQUE(xid)
);

CREATE TABLE IF NOT EXISTS restart (
  xid SMALLINT NOT NULL,
  UNIQUE(xid)
);

CREATE TABLE IF NOT EXISTS user (
  uid SMALLINT NOT NULL,
  name TEXT NOT NULL,
  password TEXT NOT NULL,
  admin TINYINT NOT NULL,
  UNIQUE(uid),
  UNIQUE(name)
);

INSERT INTO user VALUES (1, 'admin', '*', 1);
INSERT INTO user VALUES (2, 'vshelper', '*', 1);

CREATE TABLE IF NOT EXISTS user_caps (
  uid SMALLINT NOT NULL,
  cap TEXT NOT NULL,
  UNIQUE(uid, cap)
);

INSERT INTO user_caps VALUES(1, 'AUTH');
INSERT INTO user_caps VALUES(1, 'DLIM');
INSERT INTO user_caps VALUES(1, 'INIT');
INSERT INTO user_caps VALUES(1, 'MOUNT');
INSERT INTO user_caps VALUES(1, 'NET');
INSERT INTO user_caps VALUES(1, 'BCAP');
INSERT INTO user_caps VALUES(1, 'CCAP');
INSERT INTO user_caps VALUES(1, 'CFLAG');
INSERT INTO user_caps VALUES(1, 'RLIM');
INSERT INTO user_caps VALUES(1, 'SCHED');
INSERT INTO user_caps VALUES(1, 'UNAME');
INSERT INTO user_caps VALUES(1, 'CREATE');
INSERT INTO user_caps VALUES(1, 'EXEC');
INSERT INTO user_caps VALUES(1, 'INFO');
INSERT INTO user_caps VALUES(2, 'HELPER');

CREATE TABLE IF NOT EXISTS vdir (
  xid SMALLINT NOT NULL,
  vdir TEXT,
  UNIQUE(xid),
  UNIQUE(vdir)
);

CREATE TABLE IF NOT EXISTS vx_bcaps (
  xid SMALLINT NOT NULL,
  bcap TEXT NOT NULL,
  UNIQUE(xid, bcap)
);

CREATE TABLE IF NOT EXISTS vx_ccaps (
  xid SMALLINT NOT NULL,
  ccap TEXT NOT NULL,
  UNIQUE(xid, ccap)
);

CREATE TABLE IF NOT EXISTS vx_flags (
  xid SMALLINT NOT NULL,
  flag TEXT NOT NULL,
  UNIQUE(xid, flag)
);

CREATE TABLE IF NOT EXISTS vx_limit (
  xid SMALLINT NOT NULL,
  type TEXT NOT NULL,
  soft TEXT NOT NULL,
  max TEXT NOT NULL,
  UNIQUE(xid, type)
);

CREATE TABLE IF NOT EXISTS vx_sched (
  xid SMALLINT NOT NULL,
  cpuid SMALLINT NOT NULL,
  fillrate INT NOT NULL,
  fillrate2 INT NOT NULL,
  interval INT NOT NULL,
  interval2 INT NOT NULL,
  tokensmin INT NOT NULL,
  tokensmax INT NOT NULL,
  UNIQUE(xid, cpuid)
);

CREATE TABLE IF NOT EXISTS vx_uname (
  xid SMALLINT NOT NULL,
  uname TEXT NOT NULL,
  value TEXT NOT NULL,
  UNIQUE(xid, uname)
);

CREATE TABLE IF NOT EXISTS xid_name_map (
  xid SMALLINT NOT NULL,
  name TEXT NOT NULL,
  UNIQUE(xid),
  UNIQUE(name)
);

CREATE TABLE IF NOT EXISTS xid_uid_map (
  xid SMALLINT NOT NULL,
  uid INT NOT NULL,
  UNIQUE(xid, uid)
);

COMMIT TRANSACTION;
