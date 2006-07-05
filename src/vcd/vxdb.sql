CREATE TABLE IF NOT EXISTS dx_limit (
  xid SMALLINT NOT NULL,
  path TEXT NOT NULL,
  space INT NOT NULL,
  inodes INT NOT NULL,
  reserved TINYINT,
  UNIQUE(xid, path)
);
CREATE TABLE IF NOT EXISTS init_method (
  xid SMALLINT NOT NULL,
  method TEXT NOT NULL,
  start TEXT,
  stop TEXT,
  timeout TINYINT,
  UNIQUE(xid)
);
CREATE TABLE IF NOT EXISTS mount (
  xid SMALLINT NOT NULL,
  spec TEXT NOT NULL,
  path TEXT NOT NULL,
  vfstype TEXT,
  mntops TEXT,
  UNIQUE(xid, path)
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
CREATE TABLE IF NOT EXISTS user (
  uid SMALLINT NOT NULL,
  name TEXT NOT NULL,
  password TEXT NOT NULL,
  admin TINYINT NOT NULL,
  UNIQUE(uid),
  UNIQUE(name)
);
CREATE TABLE IF NOT EXISTS user_caps (
  uid SMALLINT NOT NULL,
  cap TEXT NOT NULL,
  UNIQUE(uid, cap)
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
  soft BIGINT,
  max BIGINT,
  UNIQUE(xid, type)
);
CREATE TABLE IF NOT EXISTS vx_sched (
  xid SMALLINT NOT NULL,
  cpuid SMALLINT NOT NULL,
  fillrate INT NOT NULL,
  fillrate2 INT NOT NULL,
  interval INT NOT NULL,
  interval2 INT NOT NULL,
  priobias INT NOT NULL,
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
