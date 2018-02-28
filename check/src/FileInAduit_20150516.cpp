/*************************************************************************
Copyright (c) 2011-2012, GUANGDONG ESHORE TECHNOLOGY CO., LTD.
All rights reserved.

Created:		 2013-07-22
File:			 FileInAduit.cpp
Description:	 �ļ��˶����ģ��
History:
<table>
revision	author            date                description
--------    ------            ----                -----------
</table>
**************************************************************************/
#include <dirent.h>
#include <string>
//#include <vector>
#include <sys/types.h>
#include <sys/stat.h>  //stat()��������ѯ�ļ���Ϣ
#include<unistd.h>     //��ȡ��ǰ��������Ŀ¼
//#include<iostream>
#include<fstream>
#include "FileInAduit.h"

CLog theJSLog;

SGW_RTInfo	rtinfo;

int day_or_month = 1;		//�պ˶Ի����º˶�
bool mFullPathCdr = false;  //2014-01-11�����Ľ���·����ȫ·���������·��

FileInAduit::FileInAduit()
{  
	memset(m_szFileName,0,sizeof(m_szFileName));
	
	memset(receive_path,0,sizeof(receive_path));	
	memset(input_path,0,sizeof(input_path));		
	memset(output_path,0,sizeof(output_path));
	memset(bak_path1,0,sizeof(bak_path1));
	memset(bak_path2,0,sizeof(bak_path2));
	memset(month_input_path,0,sizeof(month_input_path));

	memset(currTime,0,sizeof(currTime));
	memset(erro_msg,0,sizeof(erro_msg));
	memset(sql,0,sizeof(sql));
}

FileInAduit::~FileInAduit()
{
	mdrDeal.dr_ReleaseDR();
}

//ģ���ʼ������
bool FileInAduit::init(int argc,char** argv)
{ 
    if(!PS_Process::init(argc,argv))
    {
      return false;
    }
	
	mConfParam.iflowID = getFlowID();
	mConfParam.iModuleId = getModuleID();

	try
	{
		if(!(dbConnect(conn)))
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"init() �������ݿ�ʧ�� connect error");
			theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//�������ݿ�ʧ��
			return  false;
		}
		
		string sql;
		Statement stmt = conn.createStatement();
		sql = "select ext_param from TP_PROCESS where billing_line_id = :1 and module_id = :2";
		stmt.setSQLString(sql);
		stmt<<mConfParam.iflowID<<mConfParam.iModuleId;
		stmt.execute();
		if(!(stmt>>mConfParam.szService))
		{
			cout<<"����tp_process���ֶ�ext_param������ģ��["<<mConfParam.iModuleId<<"]service"<<endl;
			return false ;
		}
		
		theJSLog.setLog(szLogPath,szLogLevel,mConfParam.szService, "GJJS", 001);
		
		theJSLog<<"szLogPath="<<szLogPath<<"	szLogLevel="<<szLogLevel<<endi;
		theJSLog<<"flowID="<<mConfParam.iflowID<<"	ModuleId="<<mConfParam.iModuleId<<"	szService="<<mConfParam.szService<<endi;
		
		if(LoadSourceCfg() == -1)	    //��������Դ������Ϣ 2013-08-07
		{
			return false;
		}

		char fee[256];
		memset(fee,0,sizeof(fee));
		Check_Sum_Conf fmt;

		theJSLog<<"�����»����ļ�������Ϣ..."<<endi;
		sql = "select check_type,sum_table,cdr_count,cdr_duration,cdr_fee,rate_cycle,source_id,busi_type from c_check_file_config ";
		stmt = conn.createStatement();
		stmt.setSQLString(sql);
		stmt.execute();
		while(stmt>>fmt.check_type>>fmt.sum_table>>fmt.cdr_count>>fmt.cdr_duration>>fee>>fmt.rate_cycle>>fmt.source_id>>fmt.busi_type)
		{
			theJSLog<<"check_type="<<fmt.check_type<<"  sum_table="<<fmt.sum_table<<"   cdr_duration="<<fmt.cdr_duration<<" fee="<<fee
					<<"  rate_cycle="<<fmt.rate_cycle<<" source_id="<<fmt.source_id<<"	busi_type="<<fmt.busi_type<<endi;
			
			splitString(fee,",",fmt.cdr_fee,"true");
			monthSumMap.insert( map< string,Check_Sum_Conf >::value_type(fmt.check_type,fmt));

			memset(fee,0,sizeof(fee));
			//fmt.cdr_fee.clear();
			fmt.clear();
		}
		
		stmt.close();

		conn.close();
	}
	catch(SQLException  e)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"init ��ʼ��ʱ���ݿ��ѯ�쳣:%s [%s]",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣
		return false ;
	}	

//#######################################################################
		char sParamName[256],bak_path[512];
		CString sKeyVal;
	 
		sprintf(sParamName, "file.check.receive_path");			//Դ�ļ�����Ŀ¼
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(receive_path,0,sizeof(receive_path));
			strcpy(receive_path,(const char*)sKeyVal);
			completeDir(receive_path);

			if(receive_path[0] == '/')
			{
				mFullPathCdr = true;
			}
		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ���������Դ�ļ�����·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ���������Դ�ļ�����·��[file.check.receive_path]"<<endw;
			return false ;
		}	 
	 
		sprintf(sParamName, "file.check.output_path");			  //Դ�ļ����Ŀ¼
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(output_path,0,sizeof(output_path));
			strcpy(output_path,(const char*)sKeyVal);
			completeDir(output_path);
		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ���������Դ�ļ������·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ���������Դ�ļ������·��[file.check.output_path]"<<endw;
			return false ;
		}	

		sprintf(sParamName, "file.check.input_path");			//�˶��ļ�����Ŀ¼
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(input_path,0,sizeof(input_path));
			strcpy(input_path,(const char*)sKeyVal);

		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ��������ú˶��ļ�������·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ��������ú˶��ļ�������·��[file.check.input_path]"<<endw;
			return false ;
		}	 

		sprintf(sParamName, "file.check.month_input_path");			//�»����ļ�����Ŀ¼
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(month_input_path,0,sizeof(month_input_path));
			strcpy(month_input_path,(const char*)sKeyVal);
		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ����������º˶��ļ�������·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ����������º˶��ļ�������·��[file.check.month_input_path]"<<endw;
			return false ;
		}	 
	
		sprintf(sParamName, "file.check.bak_path");			
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(bak_path,0,sizeof(bak_path));
			strcpy(bak_path,(const char*)sKeyVal);

		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ��������ú˶�ģ��ı���·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ��������ú˶�ģ��ı���·��[file.check.bak_path]"<<endw;
			return false ;
		}	 	
	
		sprintf(sParamName, "file.check.fail_path");			//�˶��ļ�ʧ��Ŀ¼
		if(param_cfg.bGetMem(sParamName, sKeyVal))
		{
			memset(fail_path,0,sizeof(fail_path));
			strcpy(fail_path,(const char*)sKeyVal);
		}
		else
		{	
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"���ں��Ĳ��������ú˶��ļ��ĺ˶�ʧ�ܴ��·��,������:%s",sParamName);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);

			//theJSLog<<"���ں��Ĳ��������ú˶��ļ��ĺ˶�ʧ�ܴ��·��[file.check.fail_path]"<<endw;
			return false ;
		}	 

		//�ж�Ŀ¼�Ƿ����
		DIR *dirptr = NULL; 
		
		if(mFullPathCdr)
		{
			if((dirptr=opendir(receive_path)) == NULL)
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"ԭʼ�����ļ�����Ŀ¼[%s]��ʧ��",receive_path);
				theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

	 			return false ;

			}else closedir(dirptr);
		}

		if((dirptr=opendir(input_path)) == NULL)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�˶��ļ�����Ŀ¼[%s]��ʧ��",input_path);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

	 		//theJSLog<<"�˶��ļ�����Ŀ¼["<<input_path<<"]��ʧ��"<<endw;
	 		return false ;

		}else closedir(dirptr);
		completeDir(input_path);
	
		if((dirptr=opendir(month_input_path)) == NULL)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�»����ļ�����Ŀ¼[%s]��ʧ��",month_input_path);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

	 		//theJSLog<<"�»����ļ�����Ŀ¼["<<month_input_path<<"]��ʧ��"<<endw;
	 		return false ;

		}else closedir(dirptr);
		completeDir(month_input_path);

		if((dirptr=opendir(bak_path)) == NULL)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�˶��ļ�����Ŀ¼[%s]��ʧ��",bak_path);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

	 		//theJSLog<<"����Ŀ¼["<<bak_path<<"]��ʧ��"<<endw;
	 		return false ;

		}else closedir(dirptr);
		completeDir(bak_path);

		if((dirptr=opendir(fail_path)) == NULL)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�˶�ʧ��Ŀ¼[%s]��ʧ��",fail_path);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

	 		//theJSLog<<"�˶�ʧ��Ŀ¼["<<fail_path<<"]��ʧ��"<<endw;
	 		return false ;

		}else closedir(dirptr);
		completeDir(fail_path);
	 
		//����Ŀ¼����Ӧ�½�����Ŀ¼�����ֺ˶��ļ����»����ļ�
		strcpy(bak_path1,bak_path);
		strcat(bak_path1,"AUD");
		if(chkDir(bak_path1))
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�պ˶Ա���Ŀ¼[%s]��ʧ��",bak_path1);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

			//theJSLog<<"����Ŀ¼["<<bak_path1<<"]��ʧ��"<<endw;
			return false;
		}
		completeDir(bak_path1);

		strcpy(bak_path2,bak_path);
		strcat(bak_path2,"SUM");
		if(chkDir(bak_path2))
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�º˶Ա���Ŀ¼[%s]��ʧ��",bak_path2);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

			//theJSLog<<"����Ŀ¼["<<bak_path2<<"]��ʧ��"<<endw;
			return false ;
		}
		completeDir(bak_path2);


		//��ʼ���ڴ���־�ӿ�
		bool bb = initializeLog(argc,argv,false);  //�Ƿ����ģʽ
		if(!bb)
		{
			theJSLog.writeLog(0,"��ʼ���ڴ���־�ӿ�ʧ��");
			return false;
		}
	
		//theJSLog.setLog(szLogPath, szLogLevel,"CHECK","GJJS", 001);	//�ļ���־�ӿڣ��������ڴ���־�ӿ�
		//theJSLog<<"	��־·��:"<<szLogPath<<" ��־����:"<<"�˶��ļ�����Ŀ¼:"<<input_path<<"�»����ļ�����Ŀ¼:"<<month_input_path
		//	<<" Դ�ļ�����Ŀ¼:"<<receive_path<<" Դ�ļ����Ŀ¼:"<<output_path <<"  ����Ŀ¼:"<<bak_path<<" �˶�ʧ���ļ����Ŀ¼"<<fail_path<<" sql�ļ�Ŀ¼:"<<sql_path<<endi;
		theJSLog<<"AUD_path="<<input_path<<"  SUM_path="<<month_input_path<<endi;
		theJSLog<<"Fail_path="<<fail_path<<"  AUD_bakPath="<<bak_path1<<"  SUM_bakPath="<<bak_path2<<endi;
		theJSLog<<"receive_path="<<receive_path<<"  output_path="<<output_path<<endi;	
		   	
		//�ж����������·���Ƿ����
		char input_dir[512],output_dir[512];
		//int rett = -1;
		for(map<string,SOURCECFG>::const_iterator iter = m_SourceCfg.begin();iter !=m_SourceCfg.end(); ++iter)
		{	
			if(!mFullPathCdr)
			{
				memset(input_dir,0,sizeof(input_dir));
				strcpy(input_dir,iter->second.szSourcePath);
				strcat(input_dir,receive_path);
				if((dirptr=opendir(input_dir)) == NULL)
				{
						//memset(erro_msg,0,sizeof(erro_msg));
						//sprintf(erro_msg,"����Դ[%s]�Ľ����ļ�·��[%s]������",iter->first,input_dir);
						//theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

						theJSLog<<"����Դ��"<<iter->first<<"���Ľ����ļ�·��: "<<input_dir<<"�����ڣ����д���"<<endw;
						//rett = mkdir(input_dir,0755);
						if(chkAllDir(input_dir))
						{
							memset(erro_msg,0,sizeof(erro_msg));
							sprintf(erro_msg,"����Դ[%s]�Ľ����ļ��ļ�·��[%s]�����ڣ����д���ʧ��",iter->first,input_dir);
							theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

							return false;
						}

				}else closedir(dirptr);
			}

			memset(output_dir,0,sizeof(output_dir));
			strcpy(output_dir,iter->second.szSourcePath);
			strcat(output_dir,output_path);
			if((dirptr=opendir(output_dir)) == NULL)
			{
					//memset(erro_msg,0,sizeof(erro_msg));
					//sprintf(erro_msg,"����Դ[%s]������ļ�·��[%s]������",iter->first,output_dir);
					//theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����
					
					theJSLog<<"����Դ��"<<iter->first<<"��������ļ�·��: "<<output_dir<<"�����ڣ����д���"<<endw;
					//rett = mkdir(output_dir,0755);
					if(chkAllDir(output_dir))
					{
						memset(erro_msg,0,sizeof(erro_msg));
						sprintf(erro_msg,"����Դ[%s]������ļ��ļ�·��[%s]�����ڣ����д���ʧ��",iter->first,output_dir);
						theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����

						return false;
					}

			}else closedir(dirptr);
		}

		
		bool flag = true;
		for(int i=1;i<argc;i++)
		{
			if(strcmp(argv[i],"-k") == 0)
			{
				theJSLog<<"ģ��["<<module_name<<"]����������..."<<endi;
				flag = false;
				mdrDeal.mdrParam.m_enable = false;
				break;
			}	
		}
	
		if(flag)
		{
			char tmp[12];
			memset(tmp,0,sizeof(tmp));
			sprintf(tmp,"%ld",getPrcID());

			if(!mdrDeal.drInit(module_name,tmp))  return false;
		}
	
		if(!(rtinfo.connect()))
		{
			theJSLog.writeLog(0,"��������ʱ��Ϣʧ��");
			return false;
		}
		rtinfo.getDBSysMode(petri_status);
		petri_status_tmp = petri_status;
		theJSLog<<"petri status:"<<petri_status<<endi;

   theJSLog<<"��ʼ�����...\n"<<endi;

   return true ;
}

//��������Դ������Ϣ��ȡȫ������Դ�������Դ��Ϣ
int FileInAduit::LoadSourceCfg()
{
	char m_szSrcGrpID[8];
	int iSourceCount=0;
	string sql ;
	try
	{		
		Statement stmt = conn.createStatement();
		Statement stmt2 = conn.createStatement();
		sql = "select source_group from c_source_group_define ";
		stmt2.setSQLString(sql);
		stmt2.execute();
		memset(m_szSrcGrpID,0,sizeof(m_szSrcGrpID));

		while(stmt2>>m_szSrcGrpID)
		{			
			sql = "select count(*) from C_SOURCE_GROUP_CONFIG  where SOURCE_GROUP =:1";
			stmt.setSQLString(sql);
			stmt<<m_szSrcGrpID;
			stmt.execute();
			if(!(stmt>>iSourceCount))
			{
				continue ;
			}

			theJSLog<<"����Դ�飺"<<m_szSrcGrpID<<"  iSourceCount="<<iSourceCount<<endi;
		
			sql = "select a.source_id,b.source_path from C_SOURCE_GROUP_CONFIG a,I_SOURCE_DEFINE b where SOURCE_GROUP=:1 and a.source_id=b.source_id";
			stmt.setSQLString(sql);
			stmt<<m_szSrcGrpID;
			if(stmt.execute())
			{
				for (int i=0; i<iSourceCount; i++)
				{
					SOURCECFG SourceCfg;
					string strSourceId;

					stmt>>SourceCfg.szSourceId>>SourceCfg.szSourcePath;      
					strSourceId=SourceCfg.szSourceId;			    
					completeDir(SourceCfg.szSourcePath);

					if(getSourceFilter(SourceCfg.szSourceId,SourceCfg.filterRule,SourceCfg.file_begin,SourceCfg.file_length))
					{
							return -1;
					}
					
					theJSLog<<"szSourceId="<<SourceCfg.szSourceId<<" szSourcePath="<<SourceCfg.szSourcePath
							<<"  filterRule="<<SourceCfg.filterRule<<"  file_begin="<<SourceCfg.file_begin<<" file_length="<<SourceCfg.file_length<<endi;
					
					m_SourceCfg[strSourceId]=SourceCfg;
				}
			}
			
			memset(m_szSrcGrpID,0,sizeof(m_szSrcGrpID));
		}
		
		stmt2.close();
		stmt.close();

	}catch (SQLException e)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"LoadSourceCfg err��%s (%s)",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣

		return -1;
	}
	catch (jsexcp::CException &e) 
	{	
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"LoadSourceCfg() err %s ",e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);
		return -1;
	}

	return 0;
}

/******��������Դ��ȡ���˹��� 0û�в鵽����1�鵽������ ���ӻ�ȡ�ļ�����ʱ�����ʼλ��,�ͳ���*********************/
int FileInAduit::getSourceFilter(char* source,char* filter,int &index,int &length)
{	
	try
	{	
		string file_time;
		
		Statement stmt = conn.createStatement();
		string sql = "select file_filter,file_time_index_len from C_FILE_RECEIVE_ENV where source_id = :1 ";		
		stmt.setSQLString(sql);
		stmt << source;
		stmt.execute();
		if(!(stmt>>filter>>file_time))
		{
				stmt.close();
				sprintf(erro_msg,"����Դ[%s]û�����ù��˹�������ļ���ʱ���ȡ����",source);
				theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);
				return -1;
		}
		
		char tmp[6];
		memset(tmp,0,sizeof(tmp));
		strcpy(tmp,file_time.c_str());
		
		vector<string> fileTime;
		//cout<<"tmp="<<tmp<<endl;
		splitString(tmp,",",fileTime,false);	//���������ظ����ַ���: 8,8 �����ͻ�������(ֻ������8)
		if(fileTime.size() != 2) 
		{
			stmt.close();
			sprintf(erro_msg,"����Դ[%s]�ļ���ʱ���ȡ�������ù������:%s  [��3,8]",source,file_time);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);
			return -1;
		}
		
		index = atoi(fileTime[0].c_str());
		length = atoi(fileTime[1].c_str());
		
		//cout<<"source = "<<source<<"  tmp = "<<tmp<<" index = "<<index<<"  length = "<<length<<" file_time = "<<file_time<<endl;
		if(index < 1 || length == 0)
		{
			stmt.close();
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"����Դ[%s]�ļ���ʱ���ȡ�������ù������:%s  [��3,8]",source,file_time);
			theJSLog.writeLog(LOG_CODE_ENV_MISSING,erro_msg);
			return -1;
		}

		index--;

		stmt.close();

	}
	catch(SQLException e)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"getSourceFilter err: %s(%s)",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);
		return -1 ;
	}
	catch(jsexcp::CException e)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"getSourceFilter err��%s",e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);
		return -1;
	}
	
	return 0;
}

//���������ļ����Ƿ����ļ�����·�������ҵ�,��ͨ���ļ����ҵ���������Դ 0�����ڣ�1 ����
int FileInAduit::check_file_exist(char* file)
{			
	int ret = 0,flag = 0;
	CF_CFscan scan2;
	char tmp[512],tmp2[512], *p,receive_dir[512];  //out_path[1024],receive_dir[1024],filter[256];
	string source_id;

	memset(file_time,0,sizeof(file_time));

	//��ѯ�ļ���������ԴID,�ٵ�Ŀ¼����ȥ����ԭʼ�ļ�
	for(map<string,SOURCECFG>::const_iterator it = m_SourceCfg.begin();it != m_SourceCfg.end();++it)
	{
			if(checkFormat(file,it->second.filterRule))		//HDC.2013---    HD*
			{		
					//theJSLog<<"�ļ�["<<file<<"]��������Դ:"<<it->first<<endi;
					flag = 1;
					source_id = it->first;
					//memset(out_path,0,sizeof(out_path));
					//strcpy(out_path,it->second.szSourcePath);
					memset(m_szSourceID,0,sizeof(m_szSourceID));
					strcpy(m_szSourceID,source_id.c_str());

					memset(receive_dir,0,sizeof(receive_dir));				
					strcpy(receive_dir,it->second.szSourcePath);
					strncpy(file_time,file+it->second.file_begin,it->second.file_length);
					file_time[8] = '\0';
					break;
			}
	}
	
	if(flag == 0)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"�Ҳ����ļ�[%s]��������Դ",file);
		theJSLog.writeLog(LOG_CODE_SOURCE_NOT_FIND,erro_msg);
		return 0;
	}
	
	if(mFullPathCdr)
	{
		memset(receive_dir,0,sizeof(receive_dir));
		strcpy(receive_dir,receive_path);
	}
	else
	{
		strcat(receive_dir,receive_path);
	}

	flag = scan2.openDir(receive_dir);		//��ѯ�ļ�����·�� flag = 0
	if(flag)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"���ļ�����Ŀ¼[%s]ʧ��",receive_dir);
		theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg);		//��Ŀ¼����
		return 0;
	}
	//cout<<"ɨ�����Ŀ¼"<<receive_dir<<endl;
	flag = 1;  
	memset(tmp,0,sizeof(tmp));		
	while(true)
	{
		ret = scan2.getFile("*",tmp);			//���ļ�	
		if(ret == 0)
		{
			p = strrchr(tmp,'/');
			memset(tmp2,0,sizeof(tmp2));
			strcpy(tmp2,p+1);

			//cout<<"ɨ���ļ�["<<tmp2<<"]  �Ա��ļ�["<<file<<"]  :"<<ret<<endl;

			if(strcmp(tmp2,file) == 0)
			{							
				mapFileSource.insert(map<string,string>::value_type(file,source_id));

				break ;
			}
		}
		else if(ret == 100)			//�ļ�ɨ����
		{	
			 flag = 0;
			 break ;	
		}
		else if(ret == -1) 			//��ȡ�ļ���Ϣʧ��
		{
			 flag = 0;
			 break;
		}
	}
				
	scan2.closeDir();	
	
	return flag;
}

void FileInAduit::execute()
{
	int ret = 0,event_sn, event_type;
	long param1, param2, src_id;

	while(1)
	{
		if(gbExitSig)
		{
			theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
			prcExit();
			return;
		}
		
		ret=getCmd(event_sn, event_type, param1, param2, src_id);
		if(ret == 1)
		{
			if(event_type == EVT_CMD_STOP)
			{
				theJSLog<<"***********���յ��˳�����**********************\n"<<endw;
				prcExit();
			}
		}

		theJSLog.reSetLog();
		
		//2014-10-15���ӶԱ�ϵͳ���ж�(�պ˶Ժ��º˶�)
		if(mdrDeal.mdrParam.drStatus == 1 )  //��ϵͳ
		{
			short db_status = 0;
			rtinfo.getDBSysMode(db_status);

		   if(db_status != petri_status_tmp)
			{
				theJSLog<<"���ݿ�״̬�л�... "<<petri_status_tmp<<"->"<<db_status<<endw;
				int cmd_sn = 0;
				if( !putEvt(cmd_sn, EVT_RPT_DBSTATUS, 0, db_status, DSPCH_PRC_ID) )
				{
					theJSLog<<"�������ݿ����״̬ʧ�ܣ�\n"<<endw;
					continue;
				}
				petri_status_tmp = db_status;
			}

			//���trigger�����ļ��Ƿ����
			if(!mdrDeal.CheckTriggerFile(m_triggerFile))
			{
				sleep(1);
				continue ;
			}
			
			//��ȡͬ������
			memset(mdrDeal.m_SerialString,0,sizeof(mdrDeal.m_SerialString));
			ret = mdrDeal.drVarGetSet(mdrDeal.m_SerialString);
			if(ret)
			{
				theJSLog<<"ͬ��ʧ��..."<<endw;
				continue ;
			}
	
			//��ȡͬ������
			vector<string> data;		
			splitString(mdrDeal.m_SerialString,";",data,false,false);  //���͵��ַ����ļ���,���ݿ�״̬,�ļ�����
			
			memset(fileName,0,sizeof(fileName));
			memset(m_szFileName,0,sizeof(m_szFileName));
			strcpy(m_szFileName,data[0].c_str());

			setSQLFileName(data[0].c_str());		//����sqlFile�ļ���
			
			if(petri_status != atoi(data[1].c_str()))
			{
				theJSLog<<"��ϵͳ�����ݿ�״̬�������л�..."<<endw;
			}
			petri_status = atoi(data[1].c_str());		//��ϵͳ��״̬������ϵͳ����
			
			//��Ҫ�ж����պ˶��ļ������º˶��ļ�
			if(strcmp("AUD",data[2].c_str()) == 0)
			{
				strcpy(fileName,input_path);
				strcat(fileName,data[0].c_str());

				run();		//�պ˶��ļ�
			}
			else if(strcmp("SUM",data[2].c_str()) == 0)
			{
				strcpy(fileName,month_input_path);
				strcat(fileName,data[0].c_str());

				run2();	    //�º˶��ļ�
			}
		}
		else
		{
			run();		//�պ˶��ļ�
			run2();	    //�º˶��ļ�
		}

		sleep(10);
	}

}

//ɨ�������ļ�·�����������η������ĺ˶��ļ�����ѯ���е��ļ����Ƿ��ڵ��ȱ����еǼǣ�����¼�˶���Ϣ
void FileInAduit::run(int flag)
{
	int ret = -1,day_or_month = 1;

	try
	{	
		short db_status = 0;
		rtinfo.getDBSysMode(db_status);

		if(mdrDeal.mdrParam.drStatus == 1 )  //��ϵͳ
		{
/*			if(db_status != petri_status_tmp)
			{
				theJSLog<<"���ݿ�״̬�л�... "<<petri_status_tmp<<"->"<<db_status<<endw;
				int cmd_sn = 0;
				if( !putEvt(cmd_sn, EVT_RPT_DBSTATUS, 0, db_status, DSPCH_PRC_ID) )
				{
					theJSLog<<"�������ݿ����״̬ʧ�ܣ�\n"<<endw;
					return;
				}
				petri_status_tmp = db_status;
			}

			//���trigger�����ļ��Ƿ����
			if(!mdrDeal.CheckTriggerFile(m_triggerFile))
			{
				sleep(1);
				return ;
			}

			//��ȡͬ������
			memset(mdrDeal.m_SerialString,0,sizeof(mdrDeal.m_SerialString));
			ret = mdrDeal.drVarGetSet(mdrDeal.m_SerialString);
			if(ret)
			{
				theJSLog<<"ͬ��ʧ��..."<<endw;
				return ;
			}
	
			//��ȡͬ������
			vector<string> data;		
			splitString(mdrDeal.m_SerialString,";",data,false,false);  //���͵��ַ����ļ���,���ݿ�״̬
			
			memset(fileName,0,sizeof(fileName));
			memset(m_szFileName,0,sizeof(m_szFileName));
			strcpy(fileName,input_path);
			strcat(fileName,data[0].c_str());
			strcpy(m_szFileName,data[0].c_str());

			setSQLFileName(data[0].c_str());		//����sqlFile�ļ���
			
			if(petri_status != atoi(data[1].c_str()))
			{
				theJSLog<<"��ϵͳ�����ݿ�״̬�������л�..."<<endw;
			}
			petri_status = atoi(data[1].c_str());		//��ϵͳ��״̬������ϵͳ����
*/
			//int dr_GetAuditMode()1��ʾͬ����2��ʾ����, ����Ϊʧ�ܣ�-1�����ô���-2�����ļ���ȡ��������
			//int iStatus = dr_GetAuditMode(module_name);
			int iStatus = mdrDeal.mdrParam.aduitMode;

			if(iStatus == 1)		//ͬ��ģʽ,�ȴ�����ϵͳҪô�ɹ�,Ҫôʧ��,Ҫô��ʱ
			{	
				bool flag = false;
				int times = 1;
				while(times < 31)
				{
					if(access(fileName,F_OK|R_OK))
					{
						theJSLog<<"������"<<times<<"���ļ�"<<endi;
						times++;
						sleep(10);
					}
					else
					{
						flag = true;
						break;
					}
				}
				if(!flag)
				{
					//dr_AbortIDX();
					mdrDeal.dr_abort();

					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�պ˶��ļ�[%s]������,dr_AbortIDX()",fileName);
					theJSLog.writeLog(LOG_CODE_FILE_MISSING,erro_msg);
					return ;
				}	
				
			}
			else if(iStatus == 2)		//����ģʽ,���ȣ���ϵͳֱ���ٲóɹ�,����ģʽ����Ϊ��ģʽ
			{
				while(1)
				{
					//�����ж�
					if(gbExitSig)
					{
						//dr_AbortIDX();
						mdrDeal.dr_abort();

						theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
						PS_Process::prcExit();
						return;
					}

					if(access(fileName,F_OK|R_OK))
					{
						sleep(10);
					}
				}
			}
				
			theJSLog<<"######## start deal file "<<fileName<<" ########"<<endi;

			if(iStatus == 1)		//ͬ��ģʽ,�ȴ�����ϵͳҪô�ɹ�,Ҫôʧ��,Ҫô��ʱ
			{	
				bool flag = false;
				int times = 1;
				while(times < 10)
				{
					if(checkFile())
					{					
						//theJSLog<<"������"<<times<<"���ļ�"<<endi;
						times++;
						sleep(10);
					}
					else
					{
						flag = true;
						break;
					}
				}
				if(!flag)
				{
					//dr_AbortIDX();
					mdrDeal.dr_abort();

					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�˶��ļ�[%s]���ļ�������ȫ����,���κ˶�ʧ��,dr_AbortIDX() ",fileName);
					theJSLog.writeLog(LOG_CODE_FILE_MISSING,erro_msg);
					
					//theJSLog<<"�ļ���ʱ�����ļ���ʽ����,���ļ�Ų��ʧ��Ŀ¼"<<endi;
					char tmp[512];
					memset(tmp,0,sizeof(tmp));
					strcpy(tmp,fail_path);
					strcat(tmp,m_szFileName);
					
					if(rename(fileName,tmp))
					{
						memset(erro_msg,0,sizeof(erro_msg));
						sprintf(erro_msg,"�ƶ��ļ�[%s]������Ŀ¼ʧ��: %s",m_szFileName,strerror(errno));
						theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
					}

					theJSLog<<"######## end deal file ########\n"<<endi;

					return ;
				}	
				
			}
			else if(iStatus == 2)		//����ģʽ,���ȣ���ϵͳֱ���ٲóɹ�
			{
				while(1)
				{
					//�����ж�
					if(gbExitSig)
					{
						//dr_AbortIDX();
						mdrDeal.dr_abort();

						theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
						PS_Process::prcExit();
						return;
					}

					ret = checkFile();
					if(ret == 0)  break;
				}			
			}
			else
			{
				char tmp[50] = {0};
				snprintf(tmp, sizeof(tmp), "����ƽ̨dr_GetAuditMode�������ô��󣬷���ֵ[%d]", iStatus);
				theJSLog<<tmp<<endw;
				return ;
			}
	}
	else
	{
			if(db_status != petri_status)
			{
				theJSLog<<"���ݿ�״̬�л�... "<<petri_status<<"->"<<db_status<<endw;
				int cmd_sn = 0;
				if( !putEvt(cmd_sn, EVT_RPT_DBSTATUS, 0, db_status, DSPCH_PRC_ID) )
				{
					theJSLog<<"�������ݿ����״̬ʧ�ܣ�\n"<<endw;
					return;
				}
				petri_status = db_status;
				petri_status_tmp = db_status;
			}

			//��ϵͳ,������ϵͳ
			if(scan.openDir(input_path))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"���ļ�Ŀ¼[%s]ʧ��",input_path);
				theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����
				return ;		
			}
			
			char tmp[512];
			int rett = 0;
			while(1)		
			{
				memset(fileName,0,sizeof(fileName));
			
				rett = scan.getFile("*.AUD",fileName); 
			
				if(rett == 100)
				{		
						scan.closeDir();
						return ;
				}
				else if(rett == -1)
				{	
						scan.closeDir();
						return ;			//��ʾ��ȡ�ļ���Ϣʧ��
				}
				
				/*�����ļ�*.tmp,*.TMP,~* */			
				char* p = strrchr(fileName,'/');
				memset(tmp,0,sizeof(tmp));
				strcpy(tmp,p+1);
			
				if(tmp[0] == '~' )	continue ;
				//if(strlen(tmp) <= 3) continue;

				if(strlen(tmp) > 3)						//�����ɿ���������ǰ��ɨ���ļ��ѹ���
				{		
						int pos = strlen(tmp)-4;
						if((strcmp(tmp+pos,".tmp") && strcmp(tmp+pos,".TMP")) == 0) 
						{
							//cout<<"ɨ�赽��ʱ�ļ�������"<<fileName<<endl;
							continue;
						}
				}
				
				theJSLog<<"######## start deal file "<<fileName<<" ########"<<endi;	

				strcpy(m_szFileName,p+1);		
				setSQLFileName(m_szFileName);

				break;
			}
		
		 scan.closeDir();

		 int cnt = 0;
		//�Ƚ��ļ����ļ����ҳ���,��ȫ�����ҳɹ�����,�ŷ�ͬ����Ϣ����ϵͳ
		while(cnt < 10)
		{	
				if(gbExitSig)
				{
						theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
						prcExit();
						return;
				}

				cnt++;
				ret = checkFile();
				if(ret == 0)
				{
					break;
				}
				else if(ret == -1)
				{
					break;
				}
								
				theJSLog<<"�˶��ļ�ʧ�ܸ���:"<<ret<<" �˶Դ���:"<<cnt<<endw;

				sleep(90);
		}

		if(cnt == 10 ||ret == -1)		//�ļ���ʱ�����ļ���ʽ�����
		{
			//theJSLog<<"�ļ���ʱ�����ļ���ʽ����,���ļ�Ų��ʧ��Ŀ¼:"<<ret<<endw;
			if(ret > 0)
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"���κ˶��ļ�[%s]�˶�ʧ��,ʧ�ܸ���:(%d)",m_szFileName,ret);
				theJSLog.writeLog(LOG_CODE_FILE_ERR_CHECK,erro_msg);
			}

			char tmp[512];
			memset(tmp,0,sizeof(tmp));
			strcpy(tmp,fail_path);
			strcat(tmp,m_szFileName);
			
			theJSLog<<"���ļ��Ƶ�����Ŀ¼:"<<fail_path<<endi;
			if(rename(fileName,tmp))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"�ƶ��ļ�[%s]������Ŀ¼ʧ��: %s",m_szFileName,strerror(errno));
				theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
			}
			
			theJSLog<<"######## end deal file ########\n"<<endi;

			return;  
		}

		memset(mdrDeal.m_SerialString,0,sizeof(mdrDeal.m_SerialString));		//�ļ�ȫ��������
		sprintf(mdrDeal.m_SerialString,"%s;%d;AUD",m_szFileName,petri_status);
		ret = mdrDeal.drVarGetSet(mdrDeal.m_SerialString);
		if(ret)
		{
			theJSLog<<"ͬ��ʧ��...."<<endw;

			//rollBackSQL();
			m_vsql.clear();
			fileList.clear();
			mapFileSource.clear();

			return ;
		}
	}
		
		if(petri_status == DB_STATUS_ONLINE)
		{
			if(!(dbConnect(conn)))
			{
				//rollBackSQL();
				m_vsql.clear();
				fileList.clear();
				mapFileSource.clear();

				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"run() �������ݿ�ʧ�� connect error");
				theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//�������ݿ�ʧ��
	
				//dr_AbortIDX();
				mdrDeal.dr_abort();
				return ;
			}		
		}

		memset(mdrDeal.m_AuditMsg,0,sizeof(mdrDeal.m_AuditMsg));
		//for(int i = 0;i<fileList.size();i++)
		//{
		//	sprintf(mdrParam.m_AuditMsg,"%s%s|",mdrParam.m_AuditMsg,fileList[i].fileName);
		//}
		sprintf(mdrDeal.m_AuditMsg,"%d",fileList.size());	//2013-10-20

		fileList.clear();		//����ļ��б�
		
		ret = mdrDeal.IsAuditSuccess(mdrDeal.m_AuditMsg);
		if(ret)										
		{
			//rollBackSQL();							//�ع����ݿ�,���ļ��Ƶ�����Ŀ¼
			m_vsql.clear();

			if(petri_status == DB_STATUS_ONLINE)
			{
				conn.close();
			}

			if(ret != 3)						//2013-11-07 �ٲó�ʱ���ƶ��ļ�
			{
				theJSLog<<"���ļ�Ų��ʧ��Ŀ¼:"<<fail_path<<endi;
				char tmp[512];
				memset(tmp,0,sizeof(tmp));
				strcpy(tmp,fail_path);
				strcat(tmp,m_szFileName);
				if(rename(fileName,tmp))
				{
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�ƶ��ļ�[%s]ʧ��: %s",m_szFileName,strerror(errno));
					theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
				}	
			}
			
			mapFileSource.clear();
			
			theJSLog<<"######## end deal file ########\n"<<endi;

			return;
		}
				
		//�ٲóɹ�,�ύ���ݿ�,���ɹ����ļ��Ƶ����Ŀ¼,��ʽ��Ŀ¼
		theJSLog<<"�ύ�պ˶�sql..."<<endi;	
		if(petri_status == DB_STATUS_ONLINE)
		{
			//m_vsql = getvSQL();
			updateDB();
			conn.close();
		}
		else
		{
			theJSLog<<"���ݿ�״̬Ϊֻ��̬,дsql�ļ�"<<endi;
			for(int i = 0;i<m_vsql.size();i++)
			{
					writeSQL(m_vsql[i].c_str());
			}
			commitSQLFile();
			
			m_vsql.clear();
 		}		

		char out_dir[512],receive_dir[512];
		map<string,SOURCECFG>::const_iterator it;

		theJSLog<<"�������ļ��ӽ���Ŀ¼["<<receive_path<<"]�Ƶ���ʽ��Ŀ¼"<<output_path<<endi;
		for(map<string,string>::const_iterator iter = mapFileSource.begin();iter != mapFileSource.end();++iter)
		{
				memset(out_dir,0,sizeof(out_dir));
				memset(receive_dir,0,sizeof(receive_dir));

				it = m_SourceCfg.find(iter->second);
				
				strcpy(out_dir,it->second.szSourcePath);

				if(mFullPathCdr)
				{
					strcpy(receive_dir,receive_path);
				}
				else
				{
					strcpy(receive_dir,out_dir);
					strcat(receive_dir,receive_path);
				}

				strcat(out_dir,output_path);
				
				strcat(receive_dir,iter->first.c_str());
				strcat(out_dir,iter->first.c_str());
				if(rename(receive_dir,out_dir))
				{
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�ƶ��ļ�[%s]ʧ��: %s",iter->first,strerror(errno));
					theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
				}
		}
		
		//��պ˶Գɹ����ļ�
		mapFileSource.clear();
		
		//���ļ����� ��������������
		char bak_dir[512];
		memset(bak_dir,0,sizeof(bak_dir));
		strcpy(bak_dir,bak_path1);

		strncat(bak_dir,currTime,6);
		completeDir(bak_dir);
		strncat(bak_dir,currTime+6,2);
		completeDir(bak_dir);

		theJSLog<<"�պ˶��ļ�"<<m_szFileName<<"���ݵ�"<<bak_dir<<endi;

		if(chkAllDir(bak_dir) == 0)			//����ʧ��,������д��Ϊ��ʱ�ļ�,���ڵ�ǰĿ¼,��Ҫ�ֹ���Ԥ
		{
			strcat(bak_dir,m_szFileName);
			if(rename(fileName,bak_dir))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"�ļ�[%s]�ƶ�[%s]ʧ��: %s",m_szFileName,bak_dir,strerror(errno));
				theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);		//�ƶ��ļ�ʧ��

				theJSLog<<"�޷��ƶ�������Ŀ¼,��ʱ��Ϊ��ʱ�ļ�"<<endw;
				char tmp[512];
				memset(tmp,0,sizeof(tmp));
				strcpy(tmp,fileName);
				strcat(tmp,".tmp");
				rename(fileName,tmp);		
			}
		}
		else
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�˶��ļ�����·��[%s]�����ڣ����޷�����",bak_dir);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg);		//��Ŀ¼����

			theJSLog<<"�޷��ƶ�������Ŀ¼,��ʱ��Ϊ��ʱ�ļ�"<<endw;
			char tmp[512];
			memset(tmp,0,sizeof(tmp));
			strcpy(tmp,fileName);
			strcat(tmp,".tmp");
			rename(fileName,tmp);
			//rename(fileName,strcat(fileName,".tmp"));
		}
		
	}catch (jsexcp::CException &e) 
	{	
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"[%s]run() err %s",m_szFileName,e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);	
		
		conn.close();
		//rollBackSQL();
		m_vsql.clear();
		fileList.clear();
		mapFileSource.clear();
	}
	
	theJSLog<<"######## end deal file ########\n"<<endi;
}

//���˶��ļ�������ĸ�ʽ�Ƿ����ҵ� 0��ʾ�����ҵ�,>0��ʾδ�ҵ��ļ�¼����,-1��ʾ�˶����ݼ�¼��ʽ����
int FileInAduit::checkFile()
{	
		char szBuff[1024];
		ifstream in(fileName,ios::in) ;
		if(!in)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"checkFile �ļ�[%s]�򿪳���",fileName);
			theJSLog.writeLog(LOG_CODE_FILE_OPEN_ERR,erro_msg);		//���ļ�ʧ��
			return -1;
		}
		
		memset(szBuff,0,sizeof(szBuff));
		vector<string> vf ;
		int count = 0 ,total = 0,month = 0,cnt = 0,err_cnt = 0,suc_cnt = 0;
		while(in.getline(szBuff,sizeof(szBuff)))		//����ʵ�ּ�¼����У��???
		{	
			if(count == 0)
			{
				theJSLog<<"�ļ�ͷ:"<<szBuff<<endi;					//���Զ��ļ�ͷ���н������ж������ļ����ĸ���
				vf.clear();
				splitString(szBuff,"|",vf,false,false);
				total = atoi(vf[3].c_str());					//�����ļ�ͷ
				if(strcmp("10",vf[0].c_str()))
				{
					theJSLog.writeLog(LOG_CODE_FILE_HEAD_TAIL_VALID,"�պ˶��ļ� ͷ��¼��ʽ����ȷ");
					return -1;
				}

				total++;
				count++;
				continue;
			}
			
			if(count == total)		
			{
				theJSLog<<"�ļ�β��"<<szBuff<<endi;
				if(strcmp("90",szBuff))
				{
					in.close();
					fileList.clear();
					theJSLog.writeLog(LOG_CODE_FILE_HEAD_TAIL_VALID,"�պ˶��ļ� β��¼��ʽ����ȷ");

					return -1;
				}
				break ;
			}

			vf.clear();
			splitString(szBuff,"|",vf,false,false);
			if(vf.size() != 3)
			{
				in.close();
				fileList.clear();

				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"�պ˶��ļ���¼��ʽ����ȷ,��¼�к�(%d)",count);
				theJSLog.writeLog(LOG_CODE_FILE_ERR_RCD,erro_msg);

				return -1;
			}

			Check_Rec_Fmt rc ;					//�����ļ���¼�У��ļ��������ڱ�־������
			rc.fileName = vf[0];
			rc.rate_flag = vf[1];
			rc.month = vf[2];
			fileList.push_back(rc);
			
			count++;
			memset(szBuff,0,sizeof(szBuff));
			
		}
		in.close();
		
		total--;
		theJSLog<<"��¼������"<<total<<endi;
		char check_flag = 'Y';
		for(int i = 0;i<fileList.size();i++)
		{		
				cnt = 1;	
				check_flag = 'Y';
				cnt = check_file_exist(fileList[i].fileName.c_str());
			
				if(cnt == 0)
				{
						theJSLog<<"�ļ�["<<fileList[i].fileName<<"]�˶�ʧ��"<<endw;
						check_flag = 'N';
						err_cnt++;
				}
				else
				{		theJSLog<<"�ļ�["<<fileList[i].fileName<<"]�˶Գɹ�"<<endi;
						suc_cnt++;
				}
				
				//ÿ��ѯһ���ļ���Ҫ�Ǽ�
				getCurTime(currTime);    //��ȡ��ǰ�ļ�ʱ��
				memset(sql,0,sizeof(sql));
				sprintf(sql,"insert into d_check_file_detail(check_file,content,deal_time,check_flag,check_type,rate_cycle,file_time,source_id,cycle_flag) values('%s','%s','%s','%c','AUD','%s','%s','%s','%s')",m_szFileName,fileList[i].fileName,currTime,check_flag,fileList[i].month,file_time,m_szSourceID,fileList[i].rate_flag);		
				//writeSQL(sql);
				m_vsql.push_back(sql);
				
				//2013-10-24 ���� C_RATE_CYCLE
				if(atoi(fileList[i].rate_flag.c_str()) == 1)
				{
					theJSLog<<"�ļ�"<<fileList[i].fileName<<"Ϊ���ʱ�־�ļ�,��������:"<<fileList[i].month<<endi;
					
					char tmp_date[8+1];
					memset(tmp_date,0,sizeof(tmp_date));
					strncpy(tmp_date,currTime,8);
					tmp_date[8] = '\0';

					memset(sql,0,sizeof(sql));
					sprintf(sql,"update C_RATE_CYCLE set CYCLE_FLAG='Y',UPDATE_TIME='%s' where source_id = '%s' and rate_cycle = '%s' ",tmp_date,m_szSourceID,fileList[i].month);
					//writeSQL(sql);
					m_vsql.push_back(sql);

					//�Զ������¸�����
					char tmp_month[6+1];
					int month,cnt;
					
					memset(tmp_month,0,sizeof(tmp_month));
					strcpy(tmp_month,fileList[i].month.c_str());
					if(strncmp("12",tmp_month+4,2) == 0)
					{
						month = (atoi(tmp_month)/100)+1;
						month = month*100+1;
					}
					else   //2014-05-19 ���� else
					{
						month = (atoi(tmp_month)+1);
					}
					
					if(dbConnect(conn))
					{
						memset(sql,0,sizeof(sql));
						sprintf(sql,"select count(*) from C_RATE_CYCLE where source_id = '%s' and rate_cycle = '%d' ",m_szSourceID,month);
						Statement stmt = conn.createStatement();
						stmt.setSQLString(sql);
						stmt.execute();
						stmt>>cnt;
						if(cnt == 0)	//��ʾҪ�Զ������¸�������Դ���ڼ�¼
						{
							//char tmp_date[8+1];
							//memset(tmp_date,0,sizeof(tmp_date));
							//strncpy(tmp_date,currTime,8);
							//currTime[8] = '\0';

							memset(sql,0,sizeof(sql));
							sprintf(sql,"insert into C_RATE_CYCLE (source_id,rate_cycle,update_time) values('%s','%d','%s')",m_szSourceID,month,tmp_date);
							//writeSQL(sql);
							m_vsql.push_back(sql);
						}
						
						conn.close();
					}
					else
					{
						theJSLog<<"�������ݿ�ʧ��"<<endw;
					}
					
				}		
		}
	
	 //���Ǽ��ܵ����
	 memset(sql,0,sizeof(sql));
	 sprintf(sql,"insert into d_check_file_result(check_file,total_cnt,suc_cnt,err_cnt,deal_time,check_type) values('%s',%d,%d,%d,'%s','AUD')",m_szFileName,total,suc_cnt,err_cnt,currTime);
	 //writeSQL(sql);
	 m_vsql.push_back(sql);
		
	if(err_cnt)			//ʧ�����������,�����´β�ѯ�������ϴ�
	{
		//rollBackSQL();
		m_vsql.clear();
		fileList.clear();
		mapFileSource.clear();
	}

	return err_cnt;  //����0��ʾ����ʧ�ܵ��ļ�����
}

//�»���
void FileInAduit::run2()
{
	int ret = -1 ;

	if(gbExitSig)
	{
		theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
		prcExit();
		return;
	}

	day_or_month = 0;

	try
	{	
		short db_status = 0;
		rtinfo.getDBSysMode(db_status);

		//����Ǳ�ϵͳ
		if( mdrDeal.mdrParam.drStatus == 1 ) 
		{
/*			if(db_status != petri_status_tmp)
			{
				theJSLog<<"���ݿ�״̬�л�... "<<petri_status_tmp<<"->"<<db_status<<endw;
				int cmd_sn = 0;
				if( !putEvt(cmd_sn, EVT_RPT_DBSTATUS, 0, db_status, DSPCH_PRC_ID) )
				{
					theJSLog<<"�������ݿ����״̬ʧ�ܣ�\n"<<endw;
					return;
				}
				petri_status_tmp = db_status;
			}

			//���trigger�����ļ��Ƿ����
			if(!mdrDeal.CheckTriggerFile(m_triggerFile))
			{
				sleep(1);
				return ;
			}

			//��ȡͬ������
			memset(mdrDeal.m_SerialString,0,sizeof(mdrDeal.m_SerialString));
			ret = mdrDeal.drVarGetSet(mdrDeal.m_SerialString);
			if(ret)
			{
				theJSLog<<"ͬ��ʧ��..."<<endw;
				return ;
			}
	
			//��ȡͬ������
			vector<string> data;		
			splitString(mdrDeal.m_SerialString,";",data,false,false);  //���͵��ַ���:�ļ���,���ݿ�״̬
			
			memset(m_szFileName,0,sizeof(m_szFileName));
			strcpy(m_szFileName,data[0].c_str());

			memset(fileName,0,sizeof(fileName));		
			strcpy(fileName,month_input_path);		
			strcat(fileName,data[0].c_str());
			
			setSQLFileName(data[0].c_str());		//����sqlFile�ļ���
			
			if(petri_status != atoi(data[1].c_str()))
			{
				theJSLog<<"��ϵͳ�����ݿ�״̬�������л�..."<<endw;
			}
			petri_status = atoi(data[1].c_str());		//��ϵͳ��״̬������ϵͳ����	
*/
			//check_file check_path ��ʵ���Ա�֤������뵽��һ���Ѿ���ʾ�����ҵ��º˶��ļ�

			//int dr_GetAuditMode()1��ʾͬ����2��ʾ����, ����Ϊʧ�ܣ�-1�����ô���-2�����ļ���ȡ��������
			//int iStatus = dr_GetAuditMode(module_name);
			int iStatus = mdrDeal.mdrParam.aduitMode;

			if(iStatus == 1)		//ͬ��ģʽ,�ȴ�����ϵͳҪô�ɹ�,Ҫôʧ��,Ҫô��ʱ
			{	
				bool flag = false;
				int times = 1;
				while(times < 31)
				{
					if(access(fileName,F_OK|R_OK))
					{
						theJSLog<<"������"<<times<<"���ļ�"<<endi;
						times++;
						sleep(10);
					}
					else
					{
						flag = true;
						break;
					}
				}
				if(!flag)
				{
					//dr_AbortIDX();
					mdrDeal.dr_abort();

					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�º˶��ļ�[%s]���ļ���������",fileName);
					theJSLog.writeLog(LOG_CODE_FILE_MISSING,erro_msg);
					return ;
				}	
				
			}
			else if(iStatus == 2)		//����ģʽ,���ȣ���ϵͳֱ���ٲóɹ�
			{
				while(1)
				{
					//�����ж�

					if(gbExitSig)
					{
						//dr_AbortIDX();
						mdrDeal.dr_abort();

						theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
						prcExit();
						return;
					}

					if(access(fileName,F_OK|R_OK))
					{
						//theJSLog<<"������"<<times<<"���ļ�"<<endi;
						sleep(10);
					}
				}
			}
			else
			{
				char tmp[50] = {0};
				snprintf(tmp, sizeof(tmp), "����ƽ̨dr_GetAuditMode�������ô��󣬷���ֵ[%d]", iStatus);
				theJSLog<<tmp<<endi;
				return ;
			}
			
			theJSLog<<"######## start deal file "<<fileName<<" ########"<<endi;	
	}
	else
	{		
			if(db_status != petri_status)
			{
				theJSLog<<"���ݿ�״̬�л�... "<<petri_status<<"->"<<db_status<<endw;
				int cmd_sn = 0;
				if( !putEvt(cmd_sn, EVT_RPT_DBSTATUS, 0, db_status, DSPCH_PRC_ID) )
				{
					theJSLog<<"�������ݿ����״̬ʧ�ܣ�\n"<<endw;
					return;
				}
				petri_status = db_status;
				petri_status_tmp = db_status;
			}

			//��ϵͳ,������ϵͳ	
			if(scan.openDir(month_input_path))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"���ļ�Ŀ¼[%s]ʧ��",month_input_path);
				theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����
				return ;		
			}
			
			char tmp[512];
			int rett = 0;
			while(1)		
			{
				memset(fileName,0,sizeof(fileName));		
				rett = scan.getFile("*.SUM",fileName); 		
				if(rett == 100)
				{		
						scan.closeDir();
						return ;
				}
				else if(rett == -1)
				{	
						scan.closeDir();
						return ;			//��ʾ��ȡ�ļ���Ϣʧ��
				}
				
				/*�����ļ�*.tmp,*.TMP,~* */			
				char* p = strrchr(fileName,'/');
				memset(tmp,0,sizeof(tmp));
				strcpy(tmp,p+1);
			
				if(tmp[0] == '~' )	continue ;
				//if(strlen(tmp) <= 3) continue;
				if(strlen(tmp) > 3)						//�����ɿ���������ǰ��ɨ���ļ��ѹ���
				{		
						int pos = strlen(tmp)-4;
						//cout<<"��׺��Ϊ��"<<tmp+pos<<endl;
						if((strcmp(tmp+pos,".tmp") && strcmp(tmp+pos,".TMP")) == 0) 
						{
							//cout<<"ɨ�赽��ʱ�ļ�������"<<fileName<<endl;
							continue;
						}
				}
				
				theJSLog<<"######## start deal file "<<fileName<<" ########"<<endi;

				strcpy(m_szFileName,p+1);		
				setSQLFileName(m_szFileName);

				break;
			}
		
		 scan.closeDir();	 
	}
	
	if(!(dbConnect(conn)))
	{
		if(mdrDeal.mdrParam.drStatus  == 1)			//��ϵͳ
		{
			//dr_AbortIDX();
			mdrDeal.dr_abort();
		}

		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"run2() �������ݿ�ʧ��");
		theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);
		
		theJSLog<<"######## end deal file ########\n"<<endi;
		return ;
	}

	//�Ƚ����º˶�,����ϵͳ
	ret = checkMonthFile();

	if(ret)
	{
		conn.close();

		if(ret > 0 )
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�ļ�[%s]�º˶�ʧ��",m_szFileName);
			theJSLog.writeLog(LOG_CODE_FILE_ERR_CHECK,erro_msg);
		}
		
		//rollBackSQL();
		m_vsql.clear();
		fileList2.clear();

		if(mdrDeal.mdrParam.drStatus  == 1)				//��ϵͳ
		{
			//dr_AbortIDX();
			mdrDeal.dr_abort();
		}
		
		if(ret == -2)					//�º˶�����������
		{	
			sleep(60);
			return ;
		}

		char tmp[512];
		memset(tmp,0,sizeof(tmp));
		strcpy(tmp,fail_path);
		strcat(tmp,m_szFileName);
		if(rename(fileName,tmp))
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�ƶ��ļ�[%s]ʧ��: %s",m_szFileName,strerror(errno));
			theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
		}
		
		theJSLog<<"######## end deal file ########\n"<<endi;

		return ;
	}

	if(mdrDeal.mdrParam.drStatus != 1)				//��ϵͳ����ͬ����
	{
		//����ͬ����Ϣ
		memset(mdrDeal.m_SerialString,0,sizeof(mdrDeal.m_SerialString));
		sprintf(mdrDeal.m_SerialString,"%s;%d;SUM",m_szFileName,petri_status);
		ret = mdrDeal.drVarGetSet(mdrDeal.m_SerialString);
		if(ret)
		{
			conn.close();
			//rollBackSQL();
			m_vsql.clear();
			fileList2.clear();

			theJSLog<<"ͬ��ʧ��...."<<endw;
			return ;
		}
	}
	
	//ƴ���ٲ��ַ���
	memset(mdrDeal.m_AuditMsg,0,sizeof(mdrDeal.m_AuditMsg));
	for(int i = 0;i<fileList2.size();i++)
	{
		sprintf(mdrDeal.m_AuditMsg,"%s%s,%ld,%ld,%.2f|",mdrDeal.m_AuditMsg,fileList2[i].file_type,fileList2[i].cdr_count,fileList2[i].cdr_duration,fileList2[i].cdr_fee);
	}
	
	fileList2.clear();
	
	char tmp[512];

	//�ٲ�......................................
	ret = mdrDeal.IsAuditSuccess(mdrDeal.m_AuditMsg);
	if(ret)									//�ع����ݿ�,���ļ��Ƶ�����Ŀ¼
	{
		conn.close();
		//rollBackSQL();	
		m_vsql.clear();

		if(ret != 3)						//2013-11-07 �ٲó�ʱ���ƶ��ļ�
		{
			theJSLog<<"���ļ�Ų��ʧ��Ŀ¼:"<<fail_path<<endi;
			memset(tmp,0,sizeof(tmp));
			strcpy(tmp,fail_path);
			strcat(tmp,m_szFileName);
			//strcat(fail_path,m_szFileName);
			if(rename(fileName,tmp))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"�ƶ��ļ�[%s]ʧ��: %s",fileName,strerror(errno));
				theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
			}
		}

		theJSLog<<"######## end deal file ########\n"<<endi;
		return ;
	}

	//�ٲóɹ�,д���ݿ�,�����ļ�����
	theJSLog<<"�ύ�º˶���Ϣ�����ݿ�..."<<endi;
	if(petri_status == DB_STATUS_ONLINE)
	{
		//m_vsql = getvSQL();
		updateDB();
	}
	else
	{
		theJSLog<<"���ݿ�״̬Ϊֻ��̬,дsql�ļ�"<<endi;
		for(int i = 0;i<m_vsql.size();i++)
		{
			writeSQL(m_vsql[i].c_str());
		}
		commitSQLFile();

		m_vsql.clear();
 	}
	
	conn.close();

	theJSLog<<"�º˶��ļ�"<<m_szFileName<<"���ݵ�"<<bak_path2<<endi;
	memset(tmp,0,sizeof(tmp));
	strcpy(tmp,bak_path2);
	strcat(tmp,m_szFileName);
	if(rename(fileName,tmp))
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"�ļ�[%s]�ƶ�[%s]ʧ��: %s",fileName,bak_path2,strerror(errno));
		theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);		//�ƶ��ļ�ʧ��
	}

	}catch (jsexcp::CException &e) 
	{	
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"[%s]run() err %s",m_szFileName,e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);	

		conn.close();
		m_vsql.clear();		
	}

	theJSLog<<"######## end deal file ########\n"<<endi;
}

//�����»����ļ�  �ļ���ͷβ��û��У�飬���Կ��ǣ���������������������
int FileInAduit::checkMonthFile()
{
	int ret = 0 ;
	try
	{	
		ifstream in(fileName,ios::in) ;
		if(!in)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"dealMonthFile �ļ�%s�򿪳���",fileName);
			theJSLog.writeLog(LOG_CODE_FILE_OPEN_ERR,erro_msg);		//���ļ�ʧ��
			return -1;
		}

		int count = 0 ,total = 0,err_cnt = 0,suc_cnt = 0;
		long cdr_cnt = 0,cdr_duration = 0;
		long totalFee = 0,feeTmp = 0;
		char check_flag ,month[6+1];
		
		char szBuff[1024];
		memset(szBuff,0,sizeof(szBuff));
		vector<string> vf ;
		map< string,Check_Sum_Conf >::const_iterator iter;

		while(in.getline(szBuff,sizeof(szBuff)))   
		{	
			if(count == 0)
			{
				theJSLog<<"�ļ�ͷ:"<<szBuff<<endi;  
				vf.clear();
				splitString(szBuff,"|",vf,false,false);
				total = atoi(vf[3].c_str());				//�����ļ�ͷ����¼���ͣ��ļ��������ڣ��˶����ڣ���¼����
				
				memset(month,0,sizeof(month));
				strncpy(month,vf[2].c_str(),6);
				month[6] = '\0';
				//month = atoi(vf[2].c_str())/100;

				if(strcmp("10",vf[0].c_str()))
				{
					in.close();
					memset(erro_msg,0,sizeof(erro_msg));
					theJSLog.writeLog(LOG_CODE_FILE_HEAD_TAIL_VALID,"�º˶��ļ� ͷ��¼��ʽ����ȷ");
					return -1;
				}

				total++;
				count++;
				continue;
			}
			
			if(count == total)		
			{
				theJSLog<<"�ļ�β��"<<szBuff<<endi;
				if(strcmp("90",szBuff))
				{
					in.close();
					memset(erro_msg,0,sizeof(erro_msg));
					theJSLog.writeLog(LOG_CODE_FILE_HEAD_TAIL_VALID,"�º˶��ļ� β��¼��ʽ����ȷ");
					return -1;
				}
				break ;

			}

			vf.clear();
			splitString(szBuff,"|",vf,false,false);
			if((vf.size() != 4) && (vf.size() != 5))
			{
				in.close();
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"checkMonthFile �º˶��ļ���¼��ʽ����ȷ,�к�(%d)",count);
				theJSLog.writeLog(LOG_CODE_FILE_ERR_RCD,erro_msg);

				//err_cnt++;					//��ʽ����ȷ ��ʱ���Ǽǵ����ݿ�
				//continue ;
				return -1;
			}

			Check_Sum_Rec_Fmt rc ;				//�����ļ���¼��,�ļ����ͣ�����������ͨ��ʱ�����ܷ���
			rc.file_type = vf[0];
			
			iter = monthSumMap.find(rc.file_type);	
			if(iter == monthSumMap.end())
			{
				in.close();
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"checkMonthFile ���ñ���û���ҵ��»����ļ��ļ�����Ϊ[%s]����Ϣ",rc.file_type);
				theJSLog.writeLog(LOG_CODE_FILE_ERR_RCD,erro_msg);
				return -1;
			}

			//�����ƶ�,����; �����ƶ�
			if((iter->second.busi_type == 1) || (iter->second.busi_type == 2) || (iter->second.busi_type == 3))
			{
				rc.cdr_count = atol(vf[1].c_str());
				rc.cdr_duration = atol(vf[2].c_str());
				rc.cdr_fee = atof(vf[3].c_str());
			}
			//���ڹ���
			else if(iter->second.busi_type == 4)
			{
				//strcpy(month,vf[0].c_str());
				rc.cdr_count = atol(vf[2].c_str());
				rc.cdr_duration = atol(vf[3].c_str());
				rc.cdr_fee = atof(vf[4].c_str());
			}

			fileList2.push_back(rc);
			
			count++;
			memset(szBuff,0,sizeof(szBuff));
			
		}
		in.close();
		
		total--; 
		theJSLog<<"�»����ļ���¼������"<<total<<endi;

		getCurTime(currTime);    //��ȡ��ǰ�ļ�ʱ��

		//���ڿ��ܴ����ļ���ʽ����ȷ�������ܵļ�¼����һ��=ƥ�䵽�ļ�¼��
		for(int i = 0;i<fileList2.size();i++)
		{		
				check_flag = 'Y';

				Check_Sum_Rec_Fmt ff = fileList2[i];
				iter = monthSumMap.find(ff.file_type);

		/* ǰ���Ѿ��ж���
				if(iter == monthSumMap.end())
				{					
						theJSLog<<"checkMonthFile ���ñ���û���ҵ��»����ļ��ļ�����Ϊ["<<ff.file_type<<"]����Ϣ"<<endw;
						check_flag = 'N';
						err_cnt++;
						memset(sql,0,sizeof(sql));
						sprintf(sql,"insert into d_check_file_detail(check_file,content,deal_time,check_flag,check_type,rate_cycle) values('%s','%s','%s','%c','SUM','%s')",m_szFileName,ff.file_type,currTime,check_flag,month);
						//writeSQL(sql);
						m_vsql.push_back(sql);
						continue;
				}
		*/		
				//2013-11-12,�º˶��������»�������һֱ,�������ջ����Ƿ����

				//��ѯ�����Ƿ����
				Statement stmt = conn.createStatement();

/* 2014-10-13 �º˶��ļ�Ӧ���ڷ���֮ǰ������,����Ҫ����

				memset(sql,0,sizeof(sql));
				sprintf(sql,"select count(*) from D_CHECK_FILE_DETAIL where source_id = '%s' and check_type = 'AUD' and rate_cycle = '%s' and cycle_flag = '1'",iter->second.source_id,month);
				stmt.setSQLString(sql);
				stmt.execute();
				stmt>>ret;
				if(ret == 0)
				{
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"checkMonthFile ����Դ[%s]��������[%s]��û����...",iter->second.source_id,month);
					theJSLog<<erro_msg<<endw;
					return -2;	
				}
*/

/* 2014-08-04
				//�����µķ��ʺ��ʱ���,���ջ��ܺ��ʱ��Ƚ�
				vector<string> vft;
				string ft;
				memset(sql,0,sizeof(sql));
				sprintf(sql,"select distinct(file_time) from D_CHECK_FILE_DETAIL where source_id = '%s' and check_type = 'AUD' and rate_cycle = '%s'  order by  file_time desc ",iter->second.source_id,month);
				stmt.setSQLString(sql);
				stmt.execute();
				while(stmt>>ft)
				{
					vft.push_back(ft);
				}
	
				memset(sql,0,sizeof(sql));
				sprintf(sql,"select count(*) from D_SUMMARY_RESULT where sourceid = '%s' and sumtype in('D','RD') and sumdate = :1 ",ff.source_id);
				for(int  i = 0;i<vft.size();i++)
				{
					stmt.setSQLString(sql);
					stmt<<vft[i];
					stmt.execute();
					stmt>>ret;
					if(ret == 0)
					{		
						theJSLog<<"����Դ["<<ff.file_type<<"]������["<<vft[i]<<"]��û������ջ���..."<<endi;
						vft.clear();
						return -2;
					}
				}
	
				vft.clear();
*/
				
				//��ѯ���������Ӧ���е�������˶Լ�¼���кϼ� ���ÿ����ж���
				memset(sql,0,sizeof(sql));

				//2014-08-11��;ҵ������ڱ����⴦��
				if(strcmp("TOLL",ff.file_type.c_str()) == 0)
				{
					char next_month[6+1];
					memset(next_month,0,sizeof(next_month));
					if(strncmp(month+4,"12",2) == 0)
					{
						sprintf(next_month,"%d",(atoi(month)/100+1)*100+1);					
					}
					else
					{
						sprintf(next_month,"%d",atoi(month)+1);
					}

					theJSLog<<"next_month="<<next_month<<endi;
					
					//if((ret == 1) && (strncmp(next_month,currTime,6) == 0))
					if(strncmp(next_month,currTime,6) == 0)			//���ǵ�ǰ����
					{
						sprintf(sql,"select sum(%s),sum(%s)",iter->second.cdr_count,iter->second.cdr_duration);
						vector<string> vfee = iter->second.cdr_fee;
						for(int i = 0;i<vfee.size();i++)
						{
							sprintf(sql,"%s,sum(%s)",sql,vfee[i]);
						}
						sprintf(sql,"%s from %s_%s where %s = '%s' ",sql,iter->second.sum_table,month,iter->second.rate_cycle,month);
						
						cout<<"���� sql = "<<sql<<endl;
						
						stmt.setSQLString(sql);
						stmt.execute();

						stmt>>cdr_cnt>>cdr_duration;
						totalFee = 0;
						for(int i = 0;i<vfee.size();i++)
						{	
							stmt>>feeTmp;
							totalFee += feeTmp;			
						}
					}
					else					//�ϸ�����,����Ҫ���
					{
						// �������ڱ������� 
						sprintf(sql,"select sum(%s),sum(%s)",iter->second.cdr_count,iter->second.cdr_duration);
						vector<string> vfee = iter->second.cdr_fee;
						for(int i = 0;i<vfee.size();i++)
						{
							sprintf(sql,"%s,sum(%s)",sql,vfee[i]);
						}
						sprintf(sql,"%s from %s_%s where %s = '%s' ",sql,iter->second.sum_table,month,iter->second.rate_cycle,month);
						
						cout<<"���� sqlA = "<<sql<<endl;
						
						stmt.setSQLString(sql);
						stmt.execute();

						stmt>>cdr_cnt>>cdr_duration;
						totalFee = 0;
						for(int i = 0;i<vfee.size();i++)
						{	
							stmt>>feeTmp;
							totalFee += feeTmp;			
						}

						//�������� ���µĵ�������

						long cdr_cnt_tmp = 0,cdr_duration_tmp = 0,totalFee_tmp;

						sprintf(sql,"select sum(%s),sum(%s)",iter->second.cdr_count,iter->second.cdr_duration);
						//vector<string> vfee = iter->second.cdr_fee;
						for(int i = 0;i<vfee.size();i++)
						{
							sprintf(sql,"%s,sum(%s)",sql,vfee[i]);
						}
						sprintf(sql,"%s from %s_%s where %s = '%s' ",sql,iter->second.sum_table,next_month,iter->second.rate_cycle,month);
						
						cout<<"���� sqlB = "<<sql<<endl;
						
						stmt.setSQLString(sql);
						stmt.execute();

						stmt>>cdr_cnt_tmp>>cdr_duration_tmp;
						totalFee_tmp = 0;
						for(int i = 0;i<vfee.size();i++)
						{	
							stmt>>feeTmp;
							totalFee_tmp += feeTmp;			
						}
						
						cdr_cnt +=  cdr_cnt_tmp;
						cdr_duration += cdr_duration_tmp;
						totalFee += totalFee_tmp;
					}

				}
				else
				{
					sprintf(sql,"select sum(%s),sum(%s)",iter->second.cdr_count,iter->second.cdr_duration);
					vector<string> vfee = iter->second.cdr_fee;
					for(int i = 0;i<vfee.size();i++)
					{
							sprintf(sql,"%s,sum(%s)",sql,vfee[i]);
					}
					sprintf(sql,"%s from %s_%s ",sql,iter->second.sum_table,month);

					cout<<"���� sql = "<<sql<<endl;

					//Statement stmt = conn.createStatement();
					stmt.setSQLString(sql);
					stmt.execute();

					stmt>>cdr_cnt>>cdr_duration;
					totalFee = 0;
					for(int i = 0;i<vfee.size();i++)
					{	
						stmt>>feeTmp;
						totalFee += feeTmp;			
					}
				}
				
				stmt.close();

				if(cdr_cnt != fileList2[i].cdr_count)
				{
					//theJSLog<<"�ļ����ͣ�["<<ff.file_type<<"] ͨ�������˶�ʧ�� "<<cdr_cnt<<" != "<<fileList2[i].cdr_count<<endw;
					
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�ļ����ͣ�[%s] ͨ�������˶�ʧ�� %ld != %ld",ff.file_type,cdr_cnt,fileList2[i].cdr_count);
					theJSLog.writeLog(LOG_CODE_FILE_ERR_CHECK,erro_msg);
					
					check_flag = 'N';
				}
				if(cdr_duration != fileList2[i].cdr_duration)
				{
					//theJSLog<<"�ļ����ͣ�["<<ff.file_type<<"] ͨ��ʱ���˶�ʧ�� "<<cdr_duration<<" != "<<fileList2[i].cdr_duration<<endw;
					
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�ļ����ͣ�[%s] ͨ��ʱ���˶�ʧ�� %ld != %ld",ff.file_type,cdr_duration,fileList2[i].cdr_duration);
					theJSLog.writeLog(LOG_CODE_FILE_ERR_CHECK,erro_msg);

					check_flag = 'N';
				}
				
				if(totalFee != fileList2[i].cdr_fee)
				{
					//theJSLog<<"�ļ����ͣ�["<<ff.file_type<<"] ���ú˶�ʧ�� "<<totalFee<<" != "<<fileList2[i].cdr_fee<<endw;
					
					memset(erro_msg,0,sizeof(erro_msg));
					sprintf(erro_msg,"�ļ����ͣ�[%s] ���ú˶�ʧ�� %ld != %ld",ff.file_type,totalFee,fileList2[i].cdr_fee);
					theJSLog.writeLog(LOG_CODE_FILE_ERR_CHECK,erro_msg);

					check_flag = 'N';
				}


				if(check_flag == 'Y')
				{
						theJSLog<<"�ļ����ͣ�["<<ff.file_type<<"]�˶Գɹ�"<<endi;
						suc_cnt++;
				}
				else
				{		
						//theJSLog<<"�ļ����ͣ�["<<ff.file_type<<"]�˶�ʧ��"<<endi;
						err_cnt++;
				}

				//ÿ��ѯһ���ļ����Ͷ�Ҫ�Ǽ�
				memset(sql,0,sizeof(sql));
				sprintf(sql,"insert into d_check_file_detail(check_file,content,deal_time,check_flag,check_type,rate_cycle) values('%s','%s','%s','%c','SUM','%s')",m_szFileName,ff.file_type,currTime,check_flag,month);
				//writeSQL(sql);
				m_vsql.push_back(sql);
				
				//2013-09-19 д�˶Խ�����ݵ�����ǰ̨��ʾ
				memset(sql,0,sizeof(sql));
				sprintf(sql,"insert into D_MONTH_CHECK_RESULT(RATE_CYCLE,FILE_TYPE,CALL_COUNTS_I,DURATION_I,CDR_FEE_I,CALL_COUNTS_O,DURATION_O,CDR_FEE_O,source_id,deal_time) values('%s','%s',%ld,%ld,%ld,%ld,%ld,%ld,'%s','%s')",month,ff.file_type,fileList2[i].cdr_count,fileList2[i].cdr_duration,fileList2[i].cdr_fee,cdr_cnt,cdr_duration,totalFee,iter->second.source_id,currTime);
				//writeSQL(sql);
				m_vsql.push_back(sql);
				cout<<"�º˶Խ��:"<<sql<<endl;			
		}

		theJSLog<<"�Ǽ��»����ļ��ܵĺ˶����"<<endi;
		//���Ǽ��ܵ����
		memset(sql,0,sizeof(sql));
		sprintf(sql,"insert into d_check_file_result(check_file,total_cnt,suc_cnt,err_cnt,deal_time,check_type) values('%s',%d,%d,%d,'%s','SUM')",m_szFileName,total,suc_cnt,err_cnt,currTime);
		//writeSQL(sql);
		m_vsql.push_back(sql);

		ret = err_cnt;

	}catch (util_1_0::db::SQLException e)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"checkMonthFile() ���ݿ�����쳣%s(%s)",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣
		return -1;
	}	
	catch (jsexcp::CException &e) 
	{	
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"checkMonthFile() %s",e.GetErrMessage());
		theJSLog.writeLog(LOG_CODE_FIELD_CONVERT_ERR,erro_msg);	
		return -1;
	}
		
	return ret;
}


//2013-10-23 �����ύsql,��֤һ������������
int FileInAduit::updateDB()
{	
	int ret = 0;
	Statement stmt;
	char ssql[1024];
    try
    {	
		stmt = conn.createStatement();

		for(vector<string>::iterator iter = m_vsql.begin();iter != m_vsql.end();++iter)
		{	
			memset(ssql,0,sizeof(ssql));
			strcpy(ssql,(*iter).c_str());
			stmt.setSQLString(ssql);
			ret = stmt.execute();
		}

		stmt.close();
	}
	catch(util_1_0::db::SQLException e)
	{ 
		stmt.rollback();
		stmt.close();

		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"updateDB ���ݿ����%s (%s)",e.what(),ssql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣

		ret = -1;
	}

	m_vsql.clear();
	
	return ret ;
}

//2013-11-02 �����˳�����
void FileInAduit::prcExit()
{
	mdrDeal.dr_ReleaseDR();

	PS_Process::prcExit();
}

/*
//���ֳ�ʼ��
bool FileInAduit::drInit()
{
		//����ģ������ʵ��ID
		char tmp[32];
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%ld",getPrcID());

		theJSLog << "��ʼ������ƽ̨,ģ����:"<< module_name<<" ʵ����:"<<tmp<<endi;

		int ret = dr_InitPlatform(module_name,tmp);
		if(ret != 0)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"����ƽ̨��ʼ��ʧ��,����ֵ=%d",ret);
			theJSLog.writeLog(LOG_CODE_DR_INIT_ERR,erro_msg);

			return false;
		}
		else
		{
			theJSLog<<"dr_InitPlatform ok."<<endi;
		}

		mdrParam.m_enable = true;

		mdrParam.drStatus = _dr_GetSystemState();	//��ȡ����ϵͳ״̬
		if(mdrParam.drStatus < 0)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"��ȡ����ƽ̨״̬����,����ֵ=%d",mdrParam.drStatus);
			theJSLog.writeLog(LOG_CODE_DR_GETSTATE_ERR,erro_msg);

			return false;
		}
		
		if(mdrParam.drStatus == 0)		theJSLog<<"��ǰϵͳ����Ϊ��ϵͳ"<<endi;
		else if(mdrParam.drStatus == 1)	theJSLog<<"��ǰϵͳ����Ϊ��ϵͳ"<<endi;
		else if(mdrParam.drStatus == 2)	theJSLog<<"��ǰϵͳ���÷�����ϵͳ"<<endi;

		return true;
}

//��ϵͳ����ͬ������,��ϵͳ��ȡͬ������
int FileInAduit::drVarGetSet(char* serialString)
{
		int ret  = 0;
		char tmpVar[5000] = {0};
		
		if(!mdrParam.m_enable)
		{
			return ret;
		}

		//�������ƽ̨���л���
		ret = dr_CheckSwitchLock();   
		if(ret != 0)  
		{  
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"��������л���ʧ��,����ֵ=%d",ret);
			theJSLog.writeLog(LOG_CODE_DR_CHECK_LOCK_ERR,erro_msg);

			return -1;  
		} 
		//��ʼ��index  
		ret = dr_InitIDX();  
		if(ret != 0)  
		{  
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"��ʼ��indexʧ��,����ֵ=%d",ret);
			theJSLog.writeLog(LOG_CODE_DR_INIT_IDX_ERR,erro_msg);

			//dr_AbortIDX();
			return -1;  
		}
		
		//��ϵͳ�����ļ�����·�����ļ��� ֻ������ƽ̨���Ը�֪,��ϵͳ�޷�ʶ��
		if(mdrParam.drStatus != 1)
		{
			if(day_or_month)
			{
				snprintf(tmpVar, sizeof(tmpVar), "%s", input_path);
			}
			else
			{
				snprintf(tmpVar, sizeof(tmpVar), "%s", month_input_path);
			}

			ret = dr_SyncIdxVar("@@CHECKPATH", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"�����ļ�����·��ʧ��,�ļ�·��["<<tmpVar<<"]"<<endi;
				dr_AbortIDX();
				return -1;
			}
			
			snprintf(tmpVar, sizeof(tmpVar), "%s", m_szFileName);
			ret = dr_SyncIdxVar("@@CHECKFILE", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"�����ļ�ʧ��,�ļ���["<<m_szFileName<<"]"<<endw;
				dr_AbortIDX();
				return -1;
			}

			cout<<"�����ļ�·��:"<<tmpVar<<" �ļ���:"<<m_szFileName<<endl;
		}


		snprintf(tmpVar, sizeof(tmpVar), "%s", serialString);
		//��ϵͳ��Ҫͬ����index ����ֵ�ԡ�д������ƽ̨ά����index �ļ���
		//��ϵͳ���øú����Ľ���ǣ�var��ú���ϵͳһ�������������ֵ��	SYNC_SINGLE��ʾע�ᵥһ���������
		ret = dr_SyncIdxVar("serialString", tmpVar, SYNC_SINGLE);		
		if (ret != 0)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�����д�ʱʧ�ܣ�������:%s",serialString);
			theJSLog.writeLog(LOG_CODE_DR_SYSC_IDXVAR_ERR,erro_msg);

			dr_AbortIDX();
			return -1;
		}
		if(mdrParam.drStatus == 1)
		{
			//serialString = tmpVar;			//ͬ�������ַ���,��ϵͳ�Ǹ�ֵ����ϵͳ��ȡֵ
			strcpy(serialString,tmpVar);
			//mdrParam.m_AuditMsg = tmpVar;			//Ҫ�ٲõ��ַ���
		}

		theJSLog<<"���ε�ͬ����serialString:"<<serialString<<endi;//for test

		// <5> ����ʵ����  ������ϵͳע���IDX��ģ����ò�����
		//��ϵͳ��index manager���IDX��������󣬰�ʹ�øú���ע������������Ϊģ��ĵ��ò���trigger��Ӧ�Ľ���
		snprintf(tmpVar, sizeof(tmpVar), "%d", getPrcID());
		ret = dr_SyncIdxVar("@@ARG", tmpVar,SYNC_SINGLE);  
		if(ret !=0)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"����ʵ����ʧ��:%s",tmpVar);
			theJSLog.writeLog(LOG_CODE_DR_SYSC_IDXVAR_ERR,erro_msg);

			dr_AbortIDX();  
			return -1;
		}
		
		
		// <6> Ԥ�ύindex  �˹ؼ������ڽ�ƽ̨��ǰ�ڴ��е��������д�����
		ret = dr_SyncIdxVar("@@FLUSH","SUCCESS",SYNC_SINGLE);  
		if (ret != 0 )
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"Ԥ�ύindexʧ��");
			theJSLog.writeLog(LOG_CODE_DR_SYSC_IDXVAR_ERR,erro_msg);

			dr_AbortIDX();
			return -1;
		}
		
		
		// <7> �ύindex  	�ύIndex����index�ļ���������ɱ�־
		ret = dr_CommitIDX();  
		if(ret != 0)  
		{  
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�ύindexʧ��,����ֵ=%d",ret);
			theJSLog.writeLog(LOG_CODE_DR_COMMIT_IDX_ERR,erro_msg);

			dr_AbortIDX();  
			return -1;  
		}

		//��ϵͳ����Ŀ¼
		//if(!m_syncDr.isMaster())thelog<<"��ϵͳSerialString��"<<mdrParam.m_SerialString<<endi;

		return ret;

}

//�ٲ��ַ���
 int FileInAduit::IsAuditSuccess(const char* dealresult)
 {
		int auitStatus = 0, ret = 0;

		if(!mdrParam.m_enable)
		{
			return ret;
		}
		
		theJSLog<<"wait dr_audit() ..."<<endi;
		ret = dr_Audit(dealresult);
		if(2 == ret )
		{
			//theJSLog << "�����ٲ�ʧ��,���:" << ret <<"���ˣ�"<<dealresult<< endw;
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�����ٲ�ʧ��,���:%d,����:%s",ret,dealresult);
			theJSLog.writeLog(LOG_CODE_DR_AUDIT_ERR,erro_msg);

			dr_AbortIDX();
		}
		else if (3 == ret)
		{
			theJSLog<<"�����ٲó�ʱ..."<<endw;
			dr_AbortIDX();
		}
		else if(4 == ret)
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�Զ�idx�쳣��ֹ");
			theJSLog.writeLog(LOG_CODE_DR_IDX_STOP_ERR,erro_msg);

			dr_AbortIDX();
		}
		else if(1 == ret)
		{
			ret = dr_CommitSuccess();
			if(ret != 0)
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"ҵ��ȫ���ύʧ��(����ƽ̨),����ֵ=%d",ret);
				theJSLog.writeLog(LOG_CODE_DR_COMMIT_SUCCESS_ERR,erro_msg);

				//dr_AbortIDX();
			}
			theJSLog<<"ret = "<<ret<<"�ٲóɹ�...\n�ٲ����ݣ�"<<dealresult<<endi;
		}
		else
		{
			theJSLog<<"δ֪ret="<<ret<<"	�ٲ����ݣ�"<<dealresult<<endw;
			dr_AbortIDX();
		}
	
	return ret;
 }

bool FileInAduit::CheckTriggerFile()
{
	int ret = 0;
	if(access(m_triggerFile.c_str(),F_OK) != 0)	return false;

	theJSLog<< "��鵽trigger�ļ�����ɾ��"<< m_triggerFile <<endi;

	ret = remove(m_triggerFile.c_str());	
	if(ret)
	{
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"ɾ��trigger�ļ�[%s]ʧ��: %s",m_triggerFile,strerror(errno));
		theJSLog.writeLog(LOG_CODE_FILE_DELETE_ERR,erro_msg);
		return false;
	}
	
	return true;
}
*/

/**************************************************
*	Function Name:	checkFormat
*	Description:	�Ƚ������ַ����Ƿ�ƥ�䣨��ȣ�
*	Input Param:
*		cmpString -------> ���Ƚϵ��ַ���
*		format	   -------> ƥ����ַ�����֧��*,?,[]��ͨ���
*	Returns:
*		��������ַ���ƥ�䣬����SUC
*		��������ַ�����ƥ�䣬����FAIL
*	complete:	2001/12/13
******************************************************/
bool FileInAduit::checkFormat(const char *cmpString,const char *format)
{
	while(1)
	{
		switch(*format)
	  	{
	  		case '\0':
					if (*cmpString == '\0')
					{
						return true;
					}
					else
					{
						return false;
					}
			case '!':
					if (checkFormat(cmpString,format + 1) == true)
					{
						return false;
					}
					else
					{
						return true;
					}
			case '?' :
					if(*cmpString == '\0')
					{
						return false;
					}
					return checkFormat(cmpString + 1,format + 1);
			case '*' :
					if(*(format+1) == '\0')
					{
						return true;
					}
					do
					{
						if(checkFormat(cmpString,format+1)==true)
						{
							return true;
						}
					}while(*(cmpString++));
					return false;
			case '[' :
					format++;
					do
					{
						
						if(*format == *cmpString)
						{
							while(*format != '\0' && *format != ']')
							{
								format++;
							}
							if(*format == ']')
							{
								format++;
							}
							return checkFormat(cmpString+1,format);			
						}
						format++;
						if((*format == ':') && (*(format+1) != ']'))
						{
							if((*cmpString >= *(format - 1)) && (*cmpString <= *(format + 1)))
							{
								while(*format != '\0' && *format != ']')
								{
									format++;
								}
								if(*format == ']')
								{
									format++;
								}
								return checkFormat(cmpString+1,format);
							}
							format++;
							format++;

						}
					}while(*format != '\0' && *format != ']');

					return false;
			default  :
					if(*cmpString == *format)
					{
						return checkFormat(cmpString+1,format+1);
					}
					else
					{
						return false;
					}
		}//switch
	}//while(1)
}

int main(int argc,char** argv)
{
	cout<<"********************************************** "<<endl;
	cout<<"*    China Telecom. Telephone Network          "<<endl;
	cout<<"*    InterNational Account Settle System       "<<endl;
	cout<<"*                                              "<<endl;
	cout<<"*            jsfileCheck                       "<<endl;
	cout<<"*            sys.GJZW.Version 1.0	          "<<endl;
	cout<<"*     created time :     2013-07-01 by  hed 	  "<<endl;
	cout<<"*     last update time : 2014-10-15 by  hed	  "<<endl;
	cout<<"********************************************** "<<endl;

	FileInAduit fm ;

	if( !fm.init( argc, argv ) )
	{
		 return -1;
	}
    	
	//while(1)
	//{		
	//		theJSLog.reSetLog();

	//		fm.run();		//�պ˶��ļ�
	//		fm.run2();
	//		sleep(30);
	//}
	
	fm.execute();

   return 0;
}

