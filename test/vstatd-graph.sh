#!/bin/bash

DATADIR="/var/lib/vstatd"

DB_LIM="${DB_LIM} file_DENT file_FILES file_LOCKS file_OFD file_SOCK"
DB_LIM="${DB_LIM} ipc_MSGQ ipc_SEMA ipc_SEMS ipc_SHM"
DB_LIM="${DB_LIM} mem_ANON mem_RSS mem_VML mem_VM"
DB_LIM="${DB_LIM} sys_PROC"

DB_NET="net_INET6 net_INET net_OTHER net_UNIX net_UNSPEC"

RESOLUTIONS="300s 10m 30m"

make_idx_html() {
	echo "
	<html><head>
	<title>VServer Statistics for '${2}'</title>
	</head>
	<body>
	<center>
	<h1>Choose a resolution:</h1>"

	for res in ${RESOLUTIONS}; do
		echo "<a href=\"${res}.html\">${res}</a><br>"
	done

	echo "</body></html>"
}

make_res_html() {
	echo "
	<html><head>
	<title>VServer Statistics for '${2}' - Resolution: ${1}</title>
	</head>
	<body>
	<center>
	<h1>VServer Statistics for '${2}' - Resolution: ${1}</h1>"

	for db in ${DB_LIM}; do
		echo "<img src=\"${1}-${db}.png\" alt=\"${db}\"><br>"
	done

	for db in ${DB_NET}; do
		echo "<img src=\"${1}-${db}P.png\" alt=\"${db}P\"><br>"
		echo "<img src=\"${1}-${db}B.png\" alt=\"${db}B\"><br>"
	done

	echo "<img src=\"${1}-LOADAVG.png\" alt=\"LOADAVG\"><br>"
	echo "<img src=\"${1}-THREADS.png\" alt=\"THREADS\"><br>"

	echo "</body></html>"
}

for dir in "${DATADIR}"/*; do
	[[ ! -d ${dir} ]] && continue

	name=$(basename ${dir})

	mkdir -p ./${name} || exit 1

	make_idx_html > ./${name}/index.html

	for res in ${RESOLUTIONS}; do
		make_res_html ${res} ${name} > ./${name}/${res}.html
		
		for db in ${DB_LIM}; do
			rrdtool graph \
				--end now \
				--start end-${res} \
			./${name}/${res}-${db}.png \
			DEF:min=${dir}/${db}.rrd:min:AVERAGE \
			DEF:cur=${dir}/${db}.rrd:cur:AVERAGE \
			DEF:max=${dir}/${db}.rrd:max:AVERAGE \
			LINE1:max#FF0000:"${db#*_} (Maximum)\n" \
			LINE1:cur#FF6600:"${db#*_} (Current)\n" \
			LINE1:min#00FF00:"${db#*_} (Minimum)\n"
		done
		
		for db in ${DB_NET}; do
			rrdtool graph \
				--end now \
				--start end-${res} \
			./${name}/${res}-${db}P.png \
			DEF:recvp=${dir}/${db}.rrd:recvp:MAX \
			DEF:sendp=${dir}/${db}.rrd:sendp:MAX \
			DEF:failp=${dir}/${db}.rrd:failp:MAX \
			LINE1:recvp#00FF00:"${db#*_} (Received)\n" \
			LINE1:sendp#FF6600:"${db#*_} (Sent)\n" \
			LINE1:failp#FF0000:"${db#*_} (Failed)\n"
			
			rrdtool graph \
				--end now \
				--start end-${res} \
			./${name}/${res}-${db}B.png \
			DEF:recvb=${dir}/${db}.rrd:recvb:MAX \
			DEF:sendb=${dir}/${db}.rrd:sendb:MAX \
			DEF:failb=${dir}/${db}.rrd:failb:MAX \
			LINE1:recvb#00FF00:"${db#*_} (Received)\n" \
			LINE1:sendb#FF6600:"${db#*_} (Sent)\n" \
			LINE1:failb#FF0000:"${db#*_} (Failed)\n"
		done
		
		rrdtool graph \
			--end now \
			--start end-${res} \
		./${name}/${res}-LOADAVG.png \
		DEF:load=${dir}/sys_LOADAVG.rrd:1MIN:AVERAGE \
		LINE1:load#000000:"Load Average\n"
		
		rrdtool graph \
			--end now \
			--start end-${res} \
		./${name}/${res}-THREADS.png \
		DEF:total=${dir}/thread_TOTAL.rrd:value:AVERAGE \
		DEF:onhold=${dir}/thread_ONHOLD.rrd:value:AVERAGE \
		DEF:unintr=${dir}/thread_UNINTR.rrd:value:AVERAGE \
		DEF:running=${dir}/thread_RUNNING.rrd:value:AVERAGE \
		LINE1:total#000000:"Total Threads\n" \
		LINE1:onhold#FF6600:"Paused Threads\n" \
		LINE1:unintr#FF0000:"Uninteruptable Threads\n" \
		LINE1:running#00FF00:"Running Threads\n"
	done
done
