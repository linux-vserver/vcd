#!/bin/bash

echo "This script will setup vxdb and a default vcd configuration"
echo "To use default values just press ENTER"
echo

echo -n " * Checking for sqlite3 ... "
SQLITE3=$(which sqlite3 2>/dev/null)
if [[ $? -ne 0 ]]; then
	echo "NOT FOUND"
	exit 1
fi
echo ${SQLITE3}

echo -n " * Checking for md5sum ... "
MD5SUM=$(which md5sum 2>/dev/null)
if [[ $? -ne 0 ]]; then
	echo "NOT FOUND"
	exit 1
fi
echo ${MD5SUM}

echo
echo -n "Directory for log files? (/var/log/vcd): "
read log_dir
echo -n "Directory for vxdb (/var/lib/vcd): "
read vxdb_path
echo -n "Name of the admin user (admin): "
read admin_name
while [[ -z "${admin_password}" ]]; do
	echo -n "Password of the admin user: "
	stty -echo
	read admin_password; echo
	stty echo
done
while [[ -z "${admin_password2}" ]]; do
	echo -n "Repeat password of the admin user: "
	stty -echo
	read admin_password2; echo
	stty echo
done
if [[ "${admin_password}" != "${admin_password2}" ]]; then
	echo "Passwords are not equal!"
	exit 1
fi
echo -n "IP address to listen on (127.0.0.1): "
read listen_host
echo -n "Port to listen on (13386): "
read listen_port
echo -n "Location of config file (/etc/vcd.conf): "
read vcd_conf

: ${log_dir:=/var/log/vcd}
: ${vxdb_path:=/var/lib/vcd}
: ${admin_name:=admin}
: ${listen_host:=127.0.0.1}
: ${listen_port:=13386}
: ${vcd_conf:=/etc/vcd.conf}

admin_password=$(echo -n ${admin_password} | md5sum | cut -d\  -f1)

echo " * Creating log directory"
mkdir -p ${log_dir}
echo " * Creating vxdb directory"
mkdir -p ${vxdb_path}

echo " * Initialize vxdb"
if [[ -e "${vxdb_path}/vxdb" ]]; then
	echo -n "Database already exists. Overwrite? (y/N): "
	read overwrite
	[[ "${overwrite}" == "y" || "${overwrite}" == "Y" ]] && rm "${vxdb_path}/vxdb"
fi

if [[ ! -e "${vxdb_path}/vxdb" ]]; then
	cat <<SQLEOF | sqlite3 "${vxdb_path}/vxdb"
CREATE TABLE dx_limit (
  xid SMALLINT NOT NULL,
  path TEXT NOT NULL,
  space INT NOT NULL,
  inodes INT NOT NULL,
  reserved TINYINT
);
CREATE TABLE init_method (
  xid SMALLINT NOT NULL UNIQUE,
  method TEXT NOT NULL,
  start TEXT,
  stop TEXT,
  timeout TINYINT
);
CREATE TABLE init_mount (
  xid SMALLINT NOT NULL,
  spec TEXT NOT NULL,
  file TEXT NOT NULL,
  vfstype TEXT,
  mntops TEXT
);
CREATE TABLE nx_addr (
  xid SMALLINT NOT NULL,
  addr TEXT NOT NULL,
  netmask TEXT NOT NULL,
  broadcast TEXT NOT NULL
);
CREATE TABLE user (
  uid SMALLINT NOT NULL UNIQUE,
  name TEXT NOT NULL UNIQUE,
  password TEXT NOT NULL,
  admin TINYINT NOT NULL
);
INSERT INTO user VALUES(1, '${admin_name}', '${admin_password}', 1);
CREATE TABLE vx_bcaps (
  xid SMALLINT NOT NULL,
  bcap TEXT NOT NULL
);
CREATE TABLE vx_ccaps (
  xid SMALLINT NOT NULL,
  ccap TEXT NOT NULL
);
CREATE TABLE vx_flags (
  xid SMALLINT NOT NULL,
  flag TEXT NOT NULL
);
CREATE TABLE vx_limit (
  xid SMALLINT NOT NULL,
  type TEXT NOT NULL,
  min BIGINT,
  soft BIGINT,
  max BIGINT
);
CREATE TABLE vx_pflags (
  xid SMALLINT NOT NULL,
  pflag TEXT NOT NULL
);
CREATE TABLE vx_sched (
  xid SMALLINT NOT NULL UNIQUE,
  fill_rate INT NOT NULL,
  fill_rate2 INT NOT NULL,
  interval INT NOT NULL,
  interval2 INT NOT NULL,
  prio_bias INT NOT NULL,
  tokens_min INT NOT NULL,
  tokens_max INT NOT NULL
);
CREATE TABLE vx_uname (
  xid SMALLINT NOT NULL UNIQUE,
  domainname TEXT,
  machine TEXT,
  nodename TEXT,
  release TEXT,
  sysname TEXT,
  version TEXT
);
CREATE TABLE xid_name_map (
  xid SMALLINT NOT NULL UNIQUE,
  name TEXT NOT NULL UNIQUE
);
CREATE TABLE xid_uid_map (
  xid SMALLINT NOT NULL,
  uid INT NOT NULL
);
SQLEOF
	
	chmod 600 "${vxdb_path}/vxdb"
fi

echo " * Writing default configuration to ${vcd_conf}"
if [[ -e "${vcd_conf}" ]]; then
	echo -n "Configuration already exists. Overwrite? (y/N): "
	read overwrite
	[[ "${overwrite}" == "y" || "${overwrite}" == "Y" ]] && rm "${vcd_conf}"
fi

if [[ ! -e "${vcd_conf}" ]]; then
	cat <<VCDCONFEOF > "${vcd_conf}"
/* listen on the specified IP/PORT */
listen-host = ${listen_host}
listen-port = ${listen_port}

/* maximum number of clients connected */
#client-max = 20

/* request timeout */
#client-timeout = 30

/* log directory */
log-dir = "${log_dir}"

/* log level (1 = err, 2 = warn, 3 = info, 4 = debug) */
#log-level = 3

/* tls mode (0 = disabled, 1 = anonymous, 2 = X.509) */
#tls-mode = 0

/* for X.509 auth */
#tls-server-key = /etc/vcd/server.key
#tls-server-crt = /etc/vcd/server.crt
#tls-server-crl = /etc/vcd/server.crl
#tls-ca-crt     = /etc/vcd/ca.crt

/* vxdb configuration */
vxdb-path = "${vxdb_path}"
VCDCONFEOF
	
	chmod 600 "${vcd_conf}"
fi

echo " * vcd configured successfully"
