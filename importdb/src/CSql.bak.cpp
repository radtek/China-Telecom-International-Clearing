/*************************************************************************
Copyright (c) 2011-2012, GUANGDONG ESHORE TECHNOLOGY CO., LTD.
All rights reserved.

Created:		 2013-08-16
File:			 CSql.cpp
Description:	 实时SQL入库模块
History:
<table>
revision	author            date                description
--------    ------            ----                -----------
</table>
**************************************************************************/
#include "CSql.h"
CLog theJSLog;
SGW_RTInfo	rtinfo;

CSql::CSql()
{
	memset(filenames,0,sizeof(filenames));
	memset(input_path,0,sizeof(input_path));
	memset(output_path,0,sizeof(output_path));
	memset(erro_path,0,sizeof(erro_path));
	memset(erro_msg,0,sizeof(erro_msg));
	memset(m_Filename,0,sizeof(m_Filename));	
}

CSql::~CSql()
{
	if(m_enable) 
	{
		int ret = dr_ReleasePlatform();
		if(ret != 0)
		{
			char tmp[100] = {0};
			snprintf(tmp, sizeof(tmp), "释放容灾平台失败,返回值=%d", ret);
			theJSLog<<tmp<<endi;
		}
	}
}

	//模块初始化动作
bool CSql::init(int argc,char** argv)
{

    if(!PS_Process::init(argc,argv))
    {
      return false;
    }

	if(!drInit())	return false;

	//PS_Process::setSignalPrc(); 
  
	//char sParamName[256],szLogPath[PATH_NAME_LEN+1],szLogLevel[10],sql_path[1024];
	//CString sKeyVal;

	bool bb = initializeLog(argc,argv,false);  //是否调试模式
	if(!bb)
	{
			cout<<"初始化内存日志接口失败"<<endl;
			return false;
	}

	theJSLog.setLog(szLogPath, szLogLevel,"CSql", "jsextSQL", 001);
	
	theJSLog<<"日志路径："<<szLogPath<<" 日志级别："<<szLogLevel<<endi;

	string sql;
  	try
	{
		if (dbConnect(conn))
	 	{
			Statement stmt = conn.createStatement();
			//获取文件读取目录
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//执行sql语句
			stmt >> input_path;//获取结果
			theJSLog <<"读取文件路径为"<<input_path<<endi;
			completeDir(input_path);

			//获取文件执行成功的备份目录
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_BAK_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//执行sql语句
			stmt >> output_path;//获取结果
			theJSLog <<"备份路径为"<<output_path<<endi;
			completeDir(output_path);

			//获取文件执行失败的错误目录
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_ERR_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//执行sql语句
			stmt >> erro_path;//获取结果
			theJSLog <<"备份错误文件路径为"<<erro_path<<endi;
			completeDir(erro_path);

			stmt.close();

		//	sql = "create table D_SQL_FILEREG( FILE_NAME  varchar2(256) not NULL,DEAL_DATE  date,DEAL_FLAG  varchar2(1) ) ";
		//	stmt.setSQLString(sql);
		//	stmt.execute();//执行sql语句
		//	cout<<"创建表D_SQL_FILEREG成功"<<endl;
			
		 }else{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"init() 连接数据库失败 connect error");
			theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//连接数据库失败
			return  false;
		 }
	 	 conn.close();
  	}catch( SQLException e ) {
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"查找文件路径时失败:%s,sql语句为:%s",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//发生sql执行异常
		conn.close();
		return false;
  	}
   
   theJSLog<<"初始化完毕"<<endi;

   return true ;
}

void CSql::run()
{
	getFiles();
}

bool CSql::getFiles()//扫描目录，获取文件名
{
	int ret=0;
	if(gbExitSig)
	{
		if(gbExitSig) theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "应用程序收到退出信号");
		PS_Process::prcExit();
		return;
	}
	if(!(rtinfo.connect()))		//连接内存区
	{
		return false;
	}
	short petri_status ;
	rtinfo.getDBSysMode(petri_status);		//获取状态
//	cout<<"petri status:"<< petri_status <<endl;
	while(1)
	{
		if(petri_status==DB_STATUS_ONLINE)
		{
		//	cout<<"数据库为正常态"<<endl;
			break;
		}
		else  if(petri_status==DB_STATUS_OFFLINE)
		{
			theJSLog<<"数据库为只读,等待..."<<endi;
			sleep(5);
		}
		else
		{
			theJSLog<<"数据库状态错误"<<endi;
			return false;
		}
	}	
	try{
		if(drStatus==1)  //备系统
			{
				//检查trigger触发文件是否存在
				if(!CheckTriggerFile())
				{
					sleep(1);
					return 0;
				}

				//获取同步变量
				memset(m_SerialString,0,sizeof(m_SerialString));
				ret=drVarGetSet(m_SerialString);
				if(ret)
				{
					theJSLog<<"同步失败..."<<endl;
					return 0;
				}
				
				memset(filenames,0,sizeof(filenames));
				strcpy(filenames,input_path);
				strcat(filenames,m_SerialString);
				memset(m_Filename,0,sizeof(m_Filename));
				strcpy(m_Filename,m_SerialString);

				//int dr_GetAuditMode()1表示同步，2表示跟随, 其它为失败，-1是配置错误，-2配置文件读取出现问题
				int iStatus = dr_GetAuditMode(module_name);
				if(iStatus == 1)		//同步模式,	主系统等待指定时间 
				{
					bool flag=false;
					int times=1;
					while(times<31)
					{
						if(access(filenames,F_OK|R_OK))
						{
							theJSLog<<"查找了"<<times<<"次文件"<<endi;
							times++;
							sleep(10);
						}
						else
						{
							flag=true;
							break;
						}
					}
					if(!flag)
					{
						memset(erro_msg,0,sizeof(erro_msg));
						sprintf(erro_msg,"主系统传过来的文件[%s]不存在",m_Filename);
						theJSLog.writeLog(LOG_CODE_FILE_MISSING,erro_msg);

						dr_AbortIDX();
						return 0;
					}
				}
				else if(iStatus==2) //跟随模式，备系统
				{
					while(1)
					{
						//设置中断
						if(gbExitSig)
						{
							dr_AbortIDX();
							
							if(gbExitSig) theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "应用程序收到退出信号");
							PS_Process::prcExit();
							return ;
						}

						if(access(filenames,F_OK|R_OK))
						{
							sleep(10);
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					char tmp[50]={0};
					snprintf(tmp,sizeof(tmp),"容灾平台dr_GetAuditMode函数配置错误，返回值[%d]",iStatus);
					theJSLog<<tmp<<endi;
					return 0;
				}
				
				//处理文件
				ret = doAllSQL();
		}
		else      //主系统,非容灾系统    
		{
			if(scan.openDir(input_path))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"打开文件目录[%s]失败",input_path);
				theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //打开目录出错
				return false;	
			}	
			
		//	theJSLog <<"扫描到文件路径"<<input_path<<endi;
			int rett = 0;
			while(1)		
			{
				memset(filenames,0,sizeof(filenames));
			    
				rett = scan.getFile("*.sql",filenames); 
				if(rett == 100)
				{		
						break ;
				}
				if(rett == -1)
				{	
						break ;			//表示获取文件信息失败
				}
				/*把文件名的路径去掉*/
				char* p = strrchr(filenames,'/');
				if(p)
					strcpy(m_Filename,p+1);
				else
					strcpy(m_Filename,filenames);

				theJSLog<<"扫描到文件："<<filenames<<endi;

				memset(m_SerialString,0,sizeof(m_SerialString));
				sprintf(m_SerialString,"%s",m_Filename);
				ret = drVarGetSet(m_SerialString);
				if(ret)
				{
					theJSLog<<"同步失败...."<<endi;
					break;
				}
			    
				//处理文件
				ret = doAllSQL();

			}
			scan.closeDir();
		}
	}catch (jsexcp::CException &e) 
	{	
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"getFiles() %s",e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);		
	}
}


bool CSql::doAllSQL()   //针对某个文件，获取里面SQL语句，并执行
{
	char stat='F';   //文件中sql执行成功标志
	bool flag=true;  //仲裁成功标志
	Statement stmt;
	char szBuff[1024];
	try{			
		if (dbConnect(conn))
		 {
			stmt = conn.createStatement();
			ifstream in(filenames,ios::in) ;
			if(!in)
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"文件%s打开出错",filenames);
				theJSLog.writeLog(LOG_CODE_FILE_OPEN_ERR,erro_msg);		//打开文件失败
				return false;
			}

			memset(szBuff,0,sizeof(szBuff));
			while(in.getline(szBuff,sizeof(szBuff)))   
			{
			 	cout<<"sql语句为"<<szBuff<<endl;
				stmt.setSQLString(szBuff);	
				stmt.execute();  //执行sql语句
				memset(szBuff,0,sizeof(szBuff));
			}
			in.close();
			cout<<"处理完文件"<<endl;

			//执行仲裁
			memset(m_AuditMsg,0,sizeof(m_AuditMsg));
			sprintf(m_AuditMsg,"%s|%c",m_Filename,stat);
			theJSLog<<"开始仲裁"<<endi;
			if(!IsAuditSuccess(m_AuditMsg))   //仲裁失败
			{
				theJSLog<<"仲裁失败"<<endi;
				flag=false;
				stmt.rollback();

				moveFiles(flag);		//文件移到错误目录
				stmt.close();
				return false;
			}
		
			stmt.close();

			 //每处理一个文件都记录到D_SQL_FILEREG表中
			saveLog(stat);


			if(stat=='E')           
			{
				flag=false;
				moveFiles(flag);		//文件移到错误目录
				return false;
			}
			if(stat=='F')
				moveFiles(flag);		//文件移到备份目录
			return true;
		 }else{
	 		memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"init() 连接数据库失败 connect error");
			theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//连接数据库失败
			return  false;
		 }
	 } catch( SQLException e ) {
		stat='E';
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"处理文件[%s]的sql语句失败:%s,sql语句为：%s",filenames,e.what(),szBuff);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//发生sql执行异常

			//执行仲裁
			memset(m_AuditMsg,0,sizeof(m_AuditMsg));
			sprintf(m_AuditMsg,"%s|%c",m_Filename,stat);
			cout<<"开始仲裁"<<endl;
			if(!IsAuditSuccess(m_AuditMsg))   //仲裁失败
			{
				cout<<"仲裁失败"<<endl;
				flag=false;
				stmt.rollback();

				moveFiles(flag);		//文件移到错误目录
				stmt.close();
				return false;
			}
		
			stmt.close();

			 //每处理一个文件都记录到D_SQL_FILEREG表中
			saveLog(stat);


			if(stat=='E')           
			{
				flag=false;
				moveFiles(flag);		//文件移到错误目录
				return false;
			}
			if(stat=='F')
				moveFiles(flag);		//文件移到备份目录
	//	throw jsexcp::CException(0, "处理文件的sql语句失败", __FILE__, __LINE__);
		return false;
      } 	
}


void CSql::saveLog(char stat)  //每处理一个文件都保存到D_SQL_FILEREG表中
{
	string sql;
	try{			
		if (dbConnect(conn))
		 {
			Statement stmt = conn.createStatement();

			char flag;
			if(stat=='F')
				flag='Y';
			if(stat=='E')
				flag='N';
			sql = "insert into D_SQL_FILEREG(FILE_NAME,DEAL_DATE,DEAL_FLAG) values(:v1,sysdate,:v2)";
			stmt.setSQLString(sql);
			stmt<<m_Filename<<flag;
			stmt.execute();//执行sql语句
			stmt.close();
		//	cout<<"D_SQL_FILEREG表插入数据成功"<<endl;
			}else{
	 		cout<<"connect error."<<endl;
	 		return ;
		 }
	 	conn.close();
		conn.commit();
	}catch( SQLException e ) {
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"处理文件时保存到D_SQL_FILEREG表时sql语句失败:%s,sql语句为：%s",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//发生sql执行异常
		conn.close();
		return ;
    } 			
}


bool CSql::moveFiles(bool flag)//将已经处理后的文件移动到指定备份目录	
{
	char bak_dir[512];
	char tmp[1024];
	if(!flag)  //仲裁失败，或者执行文件中的sql语句失败，保存到失败目录
	{
		
		theJSLog<<"备份文件"<<m_Filename<<" 到错误目录 "<<erro_path<<endi;
			
		memset(tmp,0,sizeof(tmp));
		strcpy(tmp,erro_path);
		strcat(tmp,m_Filename);
	//	theJSLog <<"备份文件全路径名为:"<<tmp<<endi;
		if(rename(filenames,tmp))		
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"移动文件[%s]到错误目录失败: %s",filenames,strerror(errno));
			theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
			return false;
		}	
	}
	else     //仲裁成功，保存到指定目录
	{ 
		getCurTime(currTime);
		memset(bak_dir,0,sizeof(bak_dir));
		strcpy(bak_dir,output_path);
		strncat(bak_dir,currTime,6);
		completeDir(bak_dir);
		strncat(bak_dir,currTime+6,2);
		completeDir(bak_dir);
		theJSLog<<"备份文件"<<m_Filename<<" 到目录 "<<bak_dir<<endi;

		if(chkAllDir(bak_dir) == 0)
		{	
			memset(tmp,0,sizeof(tmp));
			strcpy(tmp,bak_dir);
			strcat(tmp,m_Filename);
		//	theJSLog <<"备份文件全路径名为:"<<tmp<<endi;
		
			if(rename(filenames,tmp))		
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"移动文件[%s]到备份目录失败: %s",filenames,strerror(errno));
				theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
				return false;
			}	
		 }
		 else
		 {
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"备份路径[%s]不存在，且无法创建",bak_dir);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg);		//打开目录出错
		 }
	}

	return true;
}


//容灾初始化
bool CSql::drInit()
{
		//传入模块名和实例ID
		char tmp[32];
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%ld",getPrcID());

		theJSLog << "初始化容灾平台,模块名:"<< module_name<<" 实例名:"<<tmp<<endi;

		int ret = dr_InitPlatform(module_name,tmp);
		if(ret != 0)
		{
			theJSLog << "容灾平台初始化失败,返回值=" << ret<<endi;
			return false;
		}
		else
		{
			theJSLog<<"dr_InitPlatform ok."<<endi;
		}

		m_enable = true;

		drStatus = _dr_GetSystemState();	//获取主备系统状态
		if(drStatus < 0)
		{
			theJSLog<<"获取容灾平台状态出错: 返回值="<<drStatus<<endi;
			return false;
		}
		
		if(drStatus == 0)		theJSLog<<"当前系统配置为主系统"<<endi;
		else if(drStatus == 1)	theJSLog<<"当前系统配置为备系统"<<endi;
		else if(drStatus == 2)	theJSLog<<"当前系统配置非容灾系统"<<endi;

		return true;
}

//主系统发送同步变量,备系统获取同步变量
int CSql::drVarGetSet(char* serialString)
{
		int ret ;
		char tmpVar[5000] = {0};

		//检查容灾平台的切换锁
		ret = dr_CheckSwitchLock();   
		if(ret != 0)  
		{  
			theJSLog<<"检查容灾切换锁失败,返回值="<<ret<<endi;
			return -1;  
		} 
		//初始化index  
		ret = dr_InitIDX();  
		if(ret != 0)  
		{  
			theJSLog<<"初始化index失败,返回值=" <<ret<<endi;
			dr_AbortIDX();
			return -1;  
		}
/*		
		//主系统传递文件所在路径和文件名 只有容灾平台可以感知,备系统无法识别
		if(drStatus != 1)
		{
			snprintf(tmpVar, sizeof(tmpVar), "%s",input_path);
			ret = dr_SyncIdxVar("@@CHECKPATH", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"传输文件所在路径失败,文件路径["<<input_path<<"]"<<endi;
				dr_AbortIDX();
				return -1;
			}
			
			snprintf(tmpVar, sizeof(tmpVar), "%s", m_Filename);
			ret = dr_SyncIdxVar("@@CHECKFILE", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"传输文件失败,文件名["<<m_Filename<<"]"<<endi;
				dr_AbortIDX();
				return -1;
			}

			cout<<"传输文件路径:"<<input_path<<" 文件名:"<<m_Filename<<endl;
		}
*/

		snprintf(tmpVar, sizeof(tmpVar), "%s", serialString);
		//主系统把要同步的index “键值对”写入容灾平台维护的index 文件中
		//备系统调用该函数的结果是，var获得和主系统一样的随机变量的值。	SYNC_SINGLE表示注册单一的随机变量
		ret = dr_SyncIdxVar("serialString", tmpVar, SYNC_SINGLE);		
		if (ret != 0)
		{
			theJSLog<<"传序列串时失败,序列名:["<<serialString<<"]"<<endi;
			dr_AbortIDX();
			return -1;
		}
		//serialString = tmpVar;			//同步索引字符串,主系统是赋值，备系统是取值
		strcpy(serialString,tmpVar);
		//m_AuditMsg = tmpVar;			//要仲裁的字符串
		

		// <5> 传输实例名  用于主系统注册此IDX的模块调用参数。
		//备系统的index manager检查IDX条件满足后，把使用该函数注册的随机变量作为模块的调用参数trigger相应的进程
		snprintf(tmpVar, sizeof(tmpVar), "%d", getPrcID());
		ret = dr_SyncIdxVar("@@ARG", tmpVar,SYNC_SINGLE);  
		if(ret !=0)
		{
			theJSLog<<"传输实例名失败："<<tmpVar<<endi;
			dr_AbortIDX();  
			return -1;
		}
		
		
		// <6> 预提交index  此关键字用于将平台当前内存中的随机变量写入磁盘
		ret = dr_SyncIdxVar("@@FLUSH","SUCCESS",SYNC_SINGLE);  
		if (ret != 0 )
		{
			theJSLog<<"预提交index失败"<<endi;
			dr_AbortIDX();
			return -1;
		}
		
		
		// <7> 提交index  	提交Index。在index文件中设置完成标志
		ret = dr_CommitIDX();  
		if(ret != 0)  
		{  
			theJSLog<<"提交index失败,返回值="<<ret<<endi;
			dr_AbortIDX();  
			return -1;  
		}

		//备系统搜索目录
		//if(!m_syncDr.isMaster())thelog<<"备系统SerialString："<<m_SerialString<<endi;

		theJSLog<<"本次的同步串serialString:"<<serialString<<endi;//for test

		return ret;

}

//仲裁字符串
 bool CSql::IsAuditSuccess(const char* dealresult)
 {
		int auitStatus = 0, ret = 0;
//cout<<"开始仲裁"<<endl;
		ret = dr_Audit(dealresult);
//cout<<"已经仲裁"<<endl;
		if(2 == ret )
		{
			theJSLog << "容灾仲裁失败,结果:" << ret <<"本端："<<dealresult<< endi;
			dr_AbortIDX();
			return false;
		}
		else if (3 == ret)
		{
			theJSLog<<"容灾仲裁超时..."<<endi;
			dr_AbortIDX();
			return false;
		}
		else if(4 == ret)
		{
			theJSLog<<"对端idx异常终止..."<<endi;
			dr_AbortIDX();
			return false;
		}
		else if(1 == ret)
		{
			ret = dr_CommitSuccess();
			if(ret != 0)
			{
				theJSLog << "业务全部提交失败(容灾平台)" << endi;
				dr_AbortIDX();
				return false;
			}
			theJSLog<<"ret = "<<ret<<"仲裁成功...\n仲裁内容："<<dealresult<<endi;
			return true;
		}
		else
		{
			theJSLog<<"未知ret="<<ret<<"	仲裁内容："<<dealresult<<endi;
			dr_AbortIDX();
			return false;
		}
	
	return true;
 }

bool CSql::CheckTriggerFile()
{
	int ret = 0;
	if(access(m_triggerFile.c_str(),F_OK) != 0)	return false;

	theJSLog<< "检查到trigger文件，并删除"<< m_triggerFile <<endi;

	ret = remove(m_triggerFile.c_str());	
	if(ret) 
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"删除trigger文件[%s]失败: %s",m_triggerFile,strerror(errno));
		theJSLog.writeLog(LOG_CODE_FILE_DELETE_ERR,erro_msg);
		return false;
	}

	return true;
}


int main(int argc,char** argv)
{
	cout<<"********************************************** "<<endl;
	cout<<"*    GuangDong Telecom. Telephone Network    * "<<endl;
	cout<<"*        Centralized Settlement System       * "<<endl;
	cout<<"*                                            * "<<endl;
	cout<<"*                    CSql                    * "<<endl;
	cout<<"*                  Version 3.0	            * "<<endl;
	cout<<"*     last update time : 2013-08-16 by  cwf	* "<<endl;
	cout<<"********************************************** "<<endl;


	CSql fm ;
   	if( !fm.init( argc, argv ) )
	{
		 return -1;
	}
	while(1)
	{
		theJSLog.reSetLog();
		fm.run();
		sleep(10);
	}

   return 0;
}


















