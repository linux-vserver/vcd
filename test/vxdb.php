<?
$error = NULL;

$HOST = "127.0.0.1";
$PORT = 13386;

$ADMINUSER = "admin";
$ADMINPASS = "MySecret";

function do_request($request)
{
	global $HOST;
	global $PORT;

	$url = "http://" . $HOST . ":" . $PORT . "/RPC2";
	$header = array(
		"Content-Type: text/xml; charset=UTF-8",
		"Content-Length: ".strlen($request)
	);

	$ch = curl_init();

	curl_setopt($ch, CURLOPT_URL, $url);
	curl_setopt($ch, CURLOPT_POST, 1);
	curl_setopt($ch, CURLOPT_POSTFIELDS, $request);
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
	curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, 0);
	curl_setopt($ch, CURLOPT_HTTPHEADER, $header);

	$data = curl_exec($ch);

	if (curl_errno($ch)) {
		$GLOBALS["error"] = curl_error($ch);
		$data  = NULL;
	}

	curl_close($ch);
	return $data;
}

function call($method, $params)
{
	global $ADMINUSER;
	global $ADMINPASS;

	printf("%s .. ", $method);

	$request = xmlrpc_encode_request($method,
		array(
			array("username"=>$ADMINUSER,"password"=>$ADMINPASS),
			$params
		),
		array(
			"encoding" => "UTF-8",
			"verbosity" => "newlines_only"
		));

	$response = do_request($request);

	if (!$response) {
		printf("ERR: " . $GLOBALS["error"] . "\n");
		die();
	}

	else {
		$data = xmlrpc_decode_request($response, $method);
		
		if (xmlrpc_is_fault($data)) {
			printf("ERR: " . $data["faultString"] . "\n");
			die();
		}
		
		else {
			printf("OK\n");
			
			if ($data != NULL)
				var_dump($data);
		}
	}
}

/* data */
$DUMMYUSER = "dummy";
$DUMMYPASS = "dummypass";

$NAME     = "testdummy";
$NEWNAME  = "testdummynew";
$TEMPLATE = "testdummy";
$REBUILD  = 1;

$DXPATH   = "/absolute/path";
$DXPATH2  = "/absolute/path2";
$DXSPACE  = 1024;
$DXINODES = 10240;
$DXRESERV = 10;

$INITMETHOD  = "gentoo";
$INITSTART   = "myrunlevel";
$INITSTOP    = "forcestop";
$INITTIMEOUT = 20;

$MOUNTPATH1 = "/proc";
$MOUNTSPEC1 = "none";
$MOUNTOPTS1 = "defaults";
$MOUNTTYPE1 = "proc";

$MOUNTPATH2 = "/dev/pts";
$MOUNTSPEC2 = "none";
$MOUNTOPTS2 = "defaults";
$MOUNTTYPE2 = "devpts";

$NXADDR1  = "192.168.0.123";
$NXMASK1  = "255.255.255.0";

$NXADDR2  = "192.168.0.124";
$NXMASK2  = "255.255.255.255";

$NXBCAST = "192.168.0.255";

$BCAP1 = "MKNOD";
$BCAP2 = "SYS_ADMIN";

$CCAP1 = "RAW_ICMP";
$CCAP2 = "BINARY_MOUNT";

$CFLAG1 = "HIDE_NETIF";
$CFLAG2 = "HIDE_MOUNT";

$VXLIM1     = "DENTRY";
$VXLIMSOFT1 = 100;
$VXLIMMAX1  = 200;

$VXLIM2     = "AS";
$VXLIMSOFT2 = 200;
$VXLIMMAX2  = 400;

$UNAME1  = "nodename";
$UVALUE1 = "foobar";

$UNAME2  = "domainname";
$UVALUE2 = "domain.tld";

$CPUID1     = 0;
$FILLRATE1  = 1;
$INTERVAL1  = 10;
$TOKENSMIN1 = 0;
$TOKENSMAX1 = 1000;

$CPUID2     = 1;
$FILLRATE2  = 2;
$INTERVAL2  = 15;
$TOKENSMIN2 = 20;
$TOKENSMAX2 = 300;


/* users */
call("vxdb.user.get",    array("username"=>NULL));
call("vxdb.user.set",    array("username"=>$DUMMYUSER,"password"=>$DUMMYPASS,"admin"=>1));
call("vxdb.user.get",    array("username"=>NULL));
call("vxdb.user.set",    array("username"=>$DUMMYUSER,"password"=>$DUMMYPASS,"admin"=>0));
call("vxdb.user.get",    array("username"=>NULL));
call("vxdb.user.remove", array("username"=>$DUMMYUSER));
call("vxdb.user.get",    array("username"=>NULL));


call("vxdb.list", array());
call("vx.create", array("name"=>$NAME,"template"=>$TEMPLATE,"rebuild"=>$REBUILD));
//call("vxdb.name.get", array("xid"=>$XID));
call("vxdb.xid.get", array("name"=>$NAME));
call("vxdb.list", array());

call("vx.status", array("name"=>$NAME));
call("vx.start", array("name"=>$NAME));
call("vx.status", array("name"=>$NAME));
call("vx.stop", array("name"=>$NAME,"wait"=>1,"reboot"=>0));

call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.set",    array("name"=>$NAME,"path"=>$DXPATH, "space"=>$DXSPACE,"inodes"=>$DXINODES,"reserved"=>$DXRESERV));
call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.set",    array("name"=>$NAME,"path"=>$DXPATH2,"space"=>$DXSPACE,"inodes"=>$DXINODES,"reserved"=>$DXRESERV));
call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.remove", array("name"=>$NAME,"path"=>$DXPATH));
call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.set",    array("name"=>$NAME,"path"=>$DXPATH, "space"=>$DXSPACE,"inodes"=>$DXINODES,"reserved"=>$DXRESERV));
call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.remove", array("name"=>$NAME,"path"=>NULL));
call("vxdb.dx.limit.get",    array("name"=>$NAME,"path"=>NULL));


call("vxdb.init.method.get", array("name"=>$NAME));
call("vxdb.init.method.set", array("name"=>$NAME,"method"=>$INITMETHOD,"start"=>$INITSTART,"stop"=>$INITSTOP,"timeout"=>$INITTIMEOUT));
call("vxdb.init.method.get", array("name"=>$NAME));


call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.set",    array("name"=>$NAME,"path"=>$MOUNTPATH1,"spec"=>$MOUNTSPEC1,"mntops"=>$MOUNTOPT1,"vfstype"=>$MOUNTTYPE1));
call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.set",    array("name"=>$NAME,"path"=>$MOUNTPATH2,"spec"=>$MOUNTSPEC2,"mntops"=>$MOUNTOPT2,"vfstype"=>$MOUNTTYPE2));
call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.remove", array("name"=>$NAME,"path"=>$MOUNTPATH1));
call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.set",    array("name"=>$NAME,"path"=>$MOUNTPATH1,"spec"=>$MOUNTSPEC1,"mntops"=>$MOUNTOPT1,"vfstype"=>$MOUNTTYPE1));
call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.remove", array("name"=>$NAME,"path"=>NULL));
call("vxdb.mount.get",    array("name"=>$NAME,"path"=>NULL));


call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.set",    array("name"=>$NAME,"addr"=>$NXADDR1,"netmask"=>$NXMASK1));
call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.set",    array("name"=>$NAME,"addr"=>$NXADDR2,"netmask"=>$NXMASK2));
call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.remove", array("name"=>$NAME,"addr"=>$NXADDR1));
call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.set",    array("name"=>$NAME,"addr"=>$NXADDR1,"netmask"=>$NXMASK1));
call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.remove", array("name"=>$NAME,"addr"=>NULL));
call("vxdb.nx.addr.get",    array("name"=>$NAME,"addr"=>NULL));


call("vxdb.nx.broadcast.get",    array("name"=>$NAME));
call("vxdb.nx.broadcast.set",    array("name"=>$NAME,"broadcast"=>$NXBCAST));
call("vxdb.nx.broadcast.get",    array("name"=>$NAME));
call("vxdb.nx.broadcast.remove", array("name"=>$NAME));
call("vxdb.nx.broadcast.get",    array("name"=>$NAME));


call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.add",    array("name"=>$NAME,"bcap"=>$BCAP1));
call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.add",    array("name"=>$NAME,"bcap"=>$BCAP2));
call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.remove", array("name"=>$NAME,"bcap"=>$BCAP1));
call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.add",    array("name"=>$NAME,"bcap"=>$BCAP1));
call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.remove", array("name"=>$NAME,"bcap"=>NULL));
call("vxdb.vx.bcaps.get",    array("name"=>$NAME));
call("vxdb.vx.bcaps.get",    array("name"=>NULL));


call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.add",    array("name"=>$NAME,"ccap"=>$CCAP1));
call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.add",    array("name"=>$NAME,"ccap"=>$CCAP2));
call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.remove", array("name"=>$NAME,"ccap"=>$CCAP1));
call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.add",    array("name"=>$NAME,"ccap"=>$CCAP1));
call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.remove", array("name"=>$NAME,"ccap"=>NULL));
call("vxdb.vx.ccaps.get",    array("name"=>$NAME));
call("vxdb.vx.ccaps.get",    array("name"=>NULL));


call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.add",    array("name"=>$NAME,"flag"=>$CFLAG1));
call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.add",    array("name"=>$NAME,"flag"=>$CFLAG2));
call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.remove", array("name"=>$NAME,"flag"=>$CFLAG1));
call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.add",    array("name"=>$NAME,"flag"=>$CFLAG1));
call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.remove", array("name"=>$NAME,"flag"=>NULL));
call("vxdb.vx.flags.get",    array("name"=>$NAME));
call("vxdb.vx.flags.get",    array("name"=>NULL));


call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.set",    array("name"=>$NAME,"limit"=>$VXLIM1,"soft"=>$VXLIMSOFT1,"max"=>$VXLIMMAX1));
call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.set",    array("name"=>$NAME,"limit"=>$VXLIM2,"soft"=>$VXLIMSOFT2,"max"=>$VXLIMMAX2));
call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.remove", array("name"=>$NAME,"limit"=>$VXLIM1));
call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.set",    array("name"=>$NAME,"limit"=>$VXLIM1,"soft"=>$VXLIMSOFT1,"max"=>$VXLIMMAX1));
call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.remove", array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.get",    array("name"=>$NAME,"limit"=>NULL));
call("vxdb.vx.limit.get",    array("name"=>NULL,"limit"=>NULL));


call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID1));
call("vxdb.vx.sched.set",    array("name"=>$NAME,"cpuid"=>$CPUID1,"fillrate"=>$FILLRATE1,"interval"=>$INTERVAL1,"fillrate2"=>$FILLRATE1,"interval2"=>$INTERVAL1,"tokensmin"=>$TOKENSMIN1,"tokensmax"=>$TOKENSMAX1,"priobias"=>0));
call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID1));
call("vxdb.vx.sched.remove", array("name"=>$NAME,"cpuid"=>$CPUID1));
call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID1));

/*
call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID2));
call("vxdb.vx.sched.set",    array("name"=>$NAME,"cpuid"=>$CPUID2,"fillrate"=>$FILLRATE2,"interval"=>$INTERVAL2,"fillrate2"=>$FILLRATE2,"interval2"=>$INTERVAL2,"tokensmin"=>$TOKENSMIN2,"tokensmax"=>$TOKENSMAX2,"priobias"=>0));
call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID2));
call("vxdb.vx.sched.remove", array("name"=>$NAME,"cpuid"=>$CPUID2));
call("vxdb.vx.sched.get",    array("name"=>$NAME,"cpuid"=>$CPUID2));
*/

call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.set",    array("name"=>$NAME,"uname"=>$UNAME1,"value"=>$UVALUE1));
call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.set",    array("name"=>$NAME,"uname"=>$UNAME2,"value"=>$UVALUE2));
call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.remove", array("name"=>$NAME,"uname"=>$UNAME1));
call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.set",    array("name"=>$NAME,"uname"=>$UNAME1,"value"=>$UVALUE1));
call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.remove", array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.get",    array("name"=>$NAME,"uname"=>NULL));
call("vxdb.vx.uname.get",    array("name"=>NULL,"uname"=>NULL));


call("vxdb.owner.get",    array("name"=>$NAME));
call("vxdb.owner.add",    array("name"=>$NAME,"username"=>$ADMINUSER));
call("vxdb.owner.get",    array("name"=>$NAME));
call("vxdb.owner.add",    array("name"=>$NAME,"username"=>$DUMMYUSER));
call("vxdb.owner.get",    array("name"=>$NAME));
call("vxdb.owner.remove", array("name"=>$NAME,"username"=>$ADMINUSER));
call("vxdb.owner.get",    array("name"=>$NAME));
call("vxdb.owner.add",    array("name"=>$NAME,"username"=>$ADMINUSER));
call("vxdb.owner.get",    array("name"=>$NAME));
call("vxdb.owner.remove", array("name"=>$NAME,"username"=>NULL));
call("vxdb.owner.get",    array("name"=>$NAME));

call("vx.rename", array("name"=>$NAME,"newname"=>$NEWNAME));
call("vx.remove", array("name"=>$NEWNAME));
?>
