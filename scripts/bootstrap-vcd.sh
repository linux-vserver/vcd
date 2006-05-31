#!/bin/bash

echo
echo "This script will setup vxdb and a default vcd configuration"
echo "Default values are shown in brackets"
echo
read -p "Press ENTER to continue .. "

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


# configure filesystem paths
echo
echo "=== Filesystem layout ==="
echo
echo "FHS compliant:"
echo "  config-dir:   /etc/vcd"
echo "  vxdb-dir:     /var/lib/vcd"
echo "  lock-dir:     /var/lock/vservers"
echo "  log-dir:      /var/log/vcd"
echo "  run-dir:      /var/run"
echo "  vserver-dir:  /srv/vservers"
echo "  template-dir: /var/cache/vcd/templates"
echo
echo "Using a prefix:"
echo "  config-dir:   <prefix>/conf"
echo "  vxdb-dir:     <prefix>/lib"
echo "  lock-dir:     <prefix>/lock"
echo "  log-dir:      <prefix>/log"
echo "  run-dir:      <prefix>/run"
echo "  vserver-dir:  <prefix>/vservers"
echo "  template-dir: <prefix>/templates"
echo
echo "You can choose between the following options:"
echo "  [1] FHS compliant"
echo "  [2] Using a prefix"
echo "  [3] Custom layout"
echo
while true; do
	read -p "Filesystem layout to use? (1) " fs_layout
	: ${fs_layout:=1}
	
	[[ ${fs_layout} -gt 0 && ${fs_layout} -lt 4 ]] && break;
	echo "Invalid choice. Try again!"
done

if [[ ${fs_layout} -eq 1 ]]; then
	config_dir="/etc/vcd"
	vxdb_dir="/var/lib/vcd"
	lock_dir="/var/lock/vservers"
	log_dir="/var/log/vcd"
	run_dir="/var/run"
	vserver_dir="/srv/vservers"
	template_dir="/var/cache/vcd/templates"
fi

if [[ ${fs_layout} -eq 2 ]]; then
	while [[ -z ${fs_prefix} ]]; do
		read -p "Enter prefix: " fs_prefix
	done
	
	config_dir="${fs_prefix}/conf"
	vxdb_dir="${fs_prefix}/lib"
	lock_dir="${fs_prefix}/lock"
	log_dir="${fs_prefix}/log"
	run_dir="${fs_prefix}/run"
	vserver_dir="${fs_prefix}/vservers"
	template_dir="${fs_prefix}/templates"
fi

fs_ask() {
	[[ -z $1 ]] && return
	
	while [[ -z $(eval echo \$$1) ]]; do
		read -p "Enter filesystem location for $1: " $1
	done
}

if [[ ${fs_layout} -eq 3 ]]; then
	fs_ask config_dir
	fs_ask vxdb_dir
	fs_ask lock_dir
	fs_ask log_dir
	fs_ask run_dir
	fs_ask vserver_dir
	fs_ask template_dir
fi

echo

fs_check() {
	[[ -z $1 ]] && return
	[[ -e $1 ]] || return
	
	local remove
	read -p "$1 does already exist! Remove? (y/N) " remove
	
	[[ ${remove} != "Y" && ${remove} != "y" ]] && return
	
	rm -rf $1
	echo
}

fs_check ${config_dir}
fs_check ${vxdb_dir}
fs_check ${lock_dir}
fs_check ${log_dir}
fs_check ${run_dir}
fs_check ${vserver_dir}
fs_check ${template_dir}


# configure TLS/SSL settings (not yet)


# configure listen socket
echo
echo "=== Listen socket ==="
echo

read -p "IP address to listen on (127.0.0.1): " listen_host
read -p "Port to listen on (13386): " listen_port

: ${listen_host:=127.0.0.1}
: ${listen_port:=13386}

echo

# configure admin user
echo
echo "=== Admin user ==="
echo

read -p "Name of the admin user (admin): " admin_user

: ${admin_user:=admin}

while true; do
	while [[ -z ${admin_pass} ]]; do
		read -s -p "Password of the admin user: " admin_pass
		echo
	done
	while [[ -z ${admin_pass2} ]]; do
		read -s -p "Repeat password of the admin user: " admin_pass2
		echo
	done
	
	[[ ${admin_pass} == ${admin_pass2} ]] && break;
	
	echo
	echo "Password are not equal! Try again!"
	echo
	
	admin_pass=
	admin_pass2=
done

admin_pass_md5=$(echo -n ${admin_pass} | ${MD5SUM} | cut -d\  -f1)

echo

# configure vshelper user
echo
echo "=== vshelper user ==="
echo

vshelper_user="vshelper"

while true; do
	while [[ -z ${vshelper_pass} ]]; do
		read -s -p "Password of the vshelper user: " vshelper_pass
		echo
	done
	while [[ -z ${vshelper_pass2} ]]; do
		read -s -p "Repeat password of the vshelper user: " vshelper_pass2
		echo
	done
	
	[[ ${vshelper_pass} == ${vshelper_pass2} ]] && break;
	
	echo
	echo "Password are not equal! Try again!"
	echo
	
	vshelper_pass=
	vshelper_pass2=
done

vshelper_pass_md5=$(echo -n ${vshelper_pass} | ${MD5SUM} | cut -d\  -f1)

echo
echo
read -p "Configuration finished. Press ENTER to write configuration .. "

echo
echo
echo "Creating filesystem layout:"

fs_create() {
	[[ -z $1 ]] && return
	
	echo "  $1 .. "
	mkdir -p $1 || exit 1
}

fs_create ${config_dir}
fs_create ${vxdb_dir}
fs_create ${lock_dir}
fs_create ${log_dir}
fs_create ${run_dir}
fs_create ${vserver_dir}
fs_create ${template_dir}

echo
echo "Creating initial vxdb structure:"

echo "  Database schema .. "

sql_schema=$(cat <<SQLEOF
CREATE TABLE dx_limit (
  xid SMALLINT NOT NULL,
  path TEXT NOT NULL,
  space INT NOT NULL,
  inodes INT NOT NULL,
  reserved TINYINT,
  UNIQUE(xid, path)
);
CREATE TABLE init_method (
  xid SMALLINT NOT NULL,
  method TEXT NOT NULL,
  start TEXT,
  stop TEXT,
  timeout TINYINT,
  UNIQUE(xid)
);
CREATE TABLE mount (
  xid SMALLINT NOT NULL,
  spec TEXT NOT NULL,
  file TEXT NOT NULL,
  vfstype TEXT,
  mntops TEXT,
  UNIQUE(xid, file)
);
CREATE TABLE nx_addr (
  xid SMALLINT NOT NULL,
  addr TEXT NOT NULL,
  netmask TEXT NOT NULL,
  broadcast TEXT NOT NULL,
  UNIQUE(xid, addr)
);
CREATE TABLE user (
  uid SMALLINT NOT NULL,
  name TEXT NOT NULL,
  password TEXT NOT NULL,
  admin TINYINT NOT NULL,
  UNIQUE(uid),
  UNIQUE(name)
);
CREATE TABLE vx_bcaps (
  xid SMALLINT NOT NULL,
  bcap TEXT NOT NULL,
  UNIQUE(xid, bcap)
);
CREATE TABLE vx_ccaps (
  xid SMALLINT NOT NULL,
  ccap TEXT NOT NULL,
  UNIQUE(xid, ccap)
);
CREATE TABLE vx_flags (
  xid SMALLINT NOT NULL,
  flag TEXT NOT NULL,
  UNIQUE(xid, flag)
);
CREATE TABLE vx_limit (
  xid SMALLINT NOT NULL,
  limit TEXT NOT NULL,
  soft BIGINT,
  max BIGINT,
  UNIQUE(xid, limit)
);
CREATE TABLE vx_pflags (
  xid SMALLINT NOT NULL,
  pflag TEXT NOT NULL,
  UNIQUE(xid, pflag)
);
CREATE TABLE vx_sched (
  xid SMALLINT NOT NULL,
  cpu_id SMALLINT NOT NULL,
  fill_rate INT NOT NULL,
  fill_rate2 INT NOT NULL,
  interval INT NOT NULL,
  interval2 INT NOT NULL,
  prio_bias INT NOT NULL,
  tokens_min INT NOT NULL,
  tokens_max INT NOT NULL,
  UNIQUE(xid, cpu_id)
);
CREATE TABLE vx_uname (
  xid SMALLINT NOT NULL,
  uname TEXT NOT NULL,
  value TEXT NOT NULL,
  UNIQUE(xid, uname)
);
CREATE TABLE xid_name_map (
  xid SMALLINT NOT NULL,
  name TEXT NOT NULL,
  UNIQUE(xid),
  UNIQUE(name)
);
CREATE TABLE xid_uid_map (
  xid SMALLINT NOT NULL,
  uid INT NOT NULL,
  UNIQUE(xid, uid)
);
SQLEOF
)

${SQLITE3} "${vxdb_dir}/vxdb" "${sql_schema}" || exit 1
chmod 600 "${vxdb_dir}/vxdb" || exit 1


echo "  User accounts .. "

sql_users=$(cat << SQLEOF
INSERT INTO user VALUES(1, '${admin_user}', '${admin_pass_md5}', 1);
INSERT INTO user VALUES(2, '${vshelper_user}', '${vshelper_pass_md5}', 1);
SQLEOF
)

${SQLITE3} "${vxdb_dir}/vxdb" "${sql_users}" || exit 1


echo
echo "Creating configuration files:"

echo "  ${config_dir}/vcd.conf .. "

cat <<EOF > "${config_dir}/vcd.conf"
/* listen on the specified IP/Port */
listen-host = ${listen_host}
listen-port = ${listen_port}

/* tls mode (none, anonymous, x509) */
tls-mode = none

/* for X.509 auth */
#tls-server-key = "${config_dir}/server.key"
#tls-server-crt = "${config_dir}/server.crt"
#tls-server-crl = "${config_dir}/server.crl"
#tls-ca-crt     = "${config_dir}/ca.crt"

/* maximum number of clients connected */
client-max = 20

/* request timeout */
client-timeout = 30

/* log level (error, warn, info, debug) */
log-level = info

/* filesystem layout */
vxdb-dir     = "${vxdb_dir}"
lock-dir     = "${lock_dir}"
log-dir      = "${log_dir}"
run-dir      = "${run_dir}"
vserver-dir  = "${vserver_dir}"
template-dir = "${template_dir}"
EOF

chmod 600 "${config_dir}/vcd.conf" || exit 1


echo "  /etc/vshelper.conf .. "

cat <<EOF > "/etc/vshelper.conf"
/* server configuration */
server-host = ${listen_host}
server-port = ${listen_port}
server-user = ${vshelper_user}
server-pass = ${vshelper_pass}

/* tls mode (none, anonymous, X.509) */
tls-mode = none

/* log directory */
log-dir      = "${log_dir}"

/* log level (error, warn, info, debug) */
log-level = info
EOF

chmod 600 "/etc/vshelper.conf" || exit 1

echo
echo "vcd configured successfully"
