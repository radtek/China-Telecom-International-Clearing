
★ 消息容灾环境说� 2012.11.13

1) API bug-fix: 旧版本备系统mdr_CmtResult()中有bug，予以修复。

2) 提供IBM-AIX、HP-UX的库: 做了HP-UX代码到IBM-AIX的移植，统一了代码。
   库版本备忘如下：
   $ what libmdrapi_node.ibm.so 
   libmdrapi_node.ibm.so:
       compiled at: [Nov 13 2012], [17:14:29] libmdrapi_node
   $ cksum libmdrapi_node.ibm.so 
   3421497478 5797275 libmdrapi_node.so
   $ what libmdrapi_node.hp.so 
   libmdrapi_node.hp.so:
       compiled at: [Nov 13 2012], [17:53:50] libmdrapi_node
   $ cksum libmdrapi_node.hp.so 
   3305416321 4437888 libmdrapi_node.hp.so

3) 提供了一个例子程序：ut-MdrNodeApi.cpp，供开发参考。

4) 为便于写日志及debug，MdrNodeApi.h中新增MdrAuditInfo::toStr(), MdrVarInfo::toStr()方法。

5) 其他：Java版本的mdrmgr无变化。


★ 消息容灾环境说� 2012.11.07

0) 确认
主机已安装jdk 1.6

1) 环境变量举例
export MDR_HOME=/BEA/tuxedo/DEV/MDRPT
export MDR_CONFIG=mdr_service_cfg.xml

export LIBPATH=$LIBPATH:$MDR_HOME/lib64		## IBM
LD_LIBRARY_PATH、SHLIB_PATH也是这个值		## HP-UX

* 主系统
export MDR_SERVERID=TPSS1

* 备系统
export MDR_SERVERID=TPSS2

2) 配置文件
请修改$MDR_HOME/$MDR_SERVERID/etc/mdrmgr_config.xml，其中的unixuser配置改为实际值。
其他配置项一般不用改。

3) 执行启动脚本
执行$MDR_HOME/$MDR_SERVERID/bin/runMdrMgr.sh start，观察$MDR_HOME/$MDR_SERVERID/var/log目录下的日志内容。

4) 积压的仲裁信息清理
某些情况下，需要清理环境，可以删除$MDR_HOME/$MDR_SERVERID/data/nodmq目录下的缓存文件，再重启应用。

