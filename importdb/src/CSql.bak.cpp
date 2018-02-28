/*************************************************************************
Copyright (c) 2011-2012, GUANGDONG ESHORE TECHNOLOGY CO., LTD.
All rights reserved.

Created:		 2013-08-16
File:			 CSql.cpp
Description:	 ʵʱSQL���ģ��
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
			snprintf(tmp, sizeof(tmp), "�ͷ�����ƽ̨ʧ��,����ֵ=%d", ret);
			theJSLog<<tmp<<endi;
		}
	}
}

	//ģ���ʼ������
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

	bool bb = initializeLog(argc,argv,false);  //�Ƿ����ģʽ
	if(!bb)
	{
			cout<<"��ʼ���ڴ���־�ӿ�ʧ��"<<endl;
			return false;
	}

	theJSLog.setLog(szLogPath, szLogLevel,"CSql", "jsextSQL", 001);
	
	theJSLog<<"��־·����"<<szLogPath<<" ��־����"<<szLogLevel<<endi;

	string sql;
  	try
	{
		if (dbConnect(conn))
	 	{
			Statement stmt = conn.createStatement();
			//��ȡ�ļ���ȡĿ¼
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//ִ��sql���
			stmt >> input_path;//��ȡ���
			theJSLog <<"��ȡ�ļ�·��Ϊ"<<input_path<<endi;
			completeDir(input_path);

			//��ȡ�ļ�ִ�гɹ��ı���Ŀ¼
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_BAK_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//ִ��sql���
			stmt >> output_path;//��ȡ���
			theJSLog <<"����·��Ϊ"<<output_path<<endi;
			completeDir(output_path);

			//��ȡ�ļ�ִ��ʧ�ܵĴ���Ŀ¼
			sql = "select VARVALUE from C_GLOBAL_ENV where VARNAME = 'SQL_ERR_PATH'";
			stmt.setSQLString(sql);
			stmt.execute();//ִ��sql���
			stmt >> erro_path;//��ȡ���
			theJSLog <<"���ݴ����ļ�·��Ϊ"<<erro_path<<endi;
			completeDir(erro_path);

			stmt.close();

		//	sql = "create table D_SQL_FILEREG( FILE_NAME  varchar2(256) not NULL,DEAL_DATE  date,DEAL_FLAG  varchar2(1) ) ";
		//	stmt.setSQLString(sql);
		//	stmt.execute();//ִ��sql���
		//	cout<<"������D_SQL_FILEREG�ɹ�"<<endl;
			
		 }else{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"init() �������ݿ�ʧ�� connect error");
			theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//�������ݿ�ʧ��
			return  false;
		 }
	 	 conn.close();
  	}catch( SQLException e ) {
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"�����ļ�·��ʱʧ��:%s,sql���Ϊ:%s",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣
		conn.close();
		return false;
  	}
   
   theJSLog<<"��ʼ�����"<<endi;

   return true ;
}

void CSql::run()
{
	getFiles();
}

bool CSql::getFiles()//ɨ��Ŀ¼����ȡ�ļ���
{
	int ret=0;
	if(gbExitSig)
	{
		if(gbExitSig) theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
		PS_Process::prcExit();
		return;
	}
	if(!(rtinfo.connect()))		//�����ڴ���
	{
		return false;
	}
	short petri_status ;
	rtinfo.getDBSysMode(petri_status);		//��ȡ״̬
//	cout<<"petri status:"<< petri_status <<endl;
	while(1)
	{
		if(petri_status==DB_STATUS_ONLINE)
		{
		//	cout<<"���ݿ�Ϊ����̬"<<endl;
			break;
		}
		else  if(petri_status==DB_STATUS_OFFLINE)
		{
			theJSLog<<"���ݿ�Ϊֻ��,�ȴ�..."<<endi;
			sleep(5);
		}
		else
		{
			theJSLog<<"���ݿ�״̬����"<<endi;
			return false;
		}
	}	
	try{
		if(drStatus==1)  //��ϵͳ
			{
				//���trigger�����ļ��Ƿ����
				if(!CheckTriggerFile())
				{
					sleep(1);
					return 0;
				}

				//��ȡͬ������
				memset(m_SerialString,0,sizeof(m_SerialString));
				ret=drVarGetSet(m_SerialString);
				if(ret)
				{
					theJSLog<<"ͬ��ʧ��..."<<endl;
					return 0;
				}
				
				memset(filenames,0,sizeof(filenames));
				strcpy(filenames,input_path);
				strcat(filenames,m_SerialString);
				memset(m_Filename,0,sizeof(m_Filename));
				strcpy(m_Filename,m_SerialString);

				//int dr_GetAuditMode()1��ʾͬ����2��ʾ����, ����Ϊʧ�ܣ�-1�����ô���-2�����ļ���ȡ��������
				int iStatus = dr_GetAuditMode(module_name);
				if(iStatus == 1)		//ͬ��ģʽ,	��ϵͳ�ȴ�ָ��ʱ�� 
				{
					bool flag=false;
					int times=1;
					while(times<31)
					{
						if(access(filenames,F_OK|R_OK))
						{
							theJSLog<<"������"<<times<<"���ļ�"<<endi;
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
						sprintf(erro_msg,"��ϵͳ���������ļ�[%s]������",m_Filename);
						theJSLog.writeLog(LOG_CODE_FILE_MISSING,erro_msg);

						dr_AbortIDX();
						return 0;
					}
				}
				else if(iStatus==2) //����ģʽ����ϵͳ
				{
					while(1)
					{
						//�����ж�
						if(gbExitSig)
						{
							dr_AbortIDX();
							
							if(gbExitSig) theJSLog.writeLog(LOG_CODE_APP_SEM_EXIT_ERR, "Ӧ�ó����յ��˳��ź�");
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
					snprintf(tmp,sizeof(tmp),"����ƽ̨dr_GetAuditMode�������ô��󣬷���ֵ[%d]",iStatus);
					theJSLog<<tmp<<endi;
					return 0;
				}
				
				//�����ļ�
				ret = doAllSQL();
		}
		else      //��ϵͳ,������ϵͳ    
		{
			if(scan.openDir(input_path))
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"���ļ�Ŀ¼[%s]ʧ��",input_path);
				theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg); //��Ŀ¼����
				return false;	
			}	
			
		//	theJSLog <<"ɨ�赽�ļ�·��"<<input_path<<endi;
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
						break ;			//��ʾ��ȡ�ļ���Ϣʧ��
				}
				/*���ļ�����·��ȥ��*/
				char* p = strrchr(filenames,'/');
				if(p)
					strcpy(m_Filename,p+1);
				else
					strcpy(m_Filename,filenames);

				theJSLog<<"ɨ�赽�ļ���"<<filenames<<endi;

				memset(m_SerialString,0,sizeof(m_SerialString));
				sprintf(m_SerialString,"%s",m_Filename);
				ret = drVarGetSet(m_SerialString);
				if(ret)
				{
					theJSLog<<"ͬ��ʧ��...."<<endi;
					break;
				}
			    
				//�����ļ�
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


bool CSql::doAllSQL()   //���ĳ���ļ�����ȡ����SQL��䣬��ִ��
{
	char stat='F';   //�ļ���sqlִ�гɹ���־
	bool flag=true;  //�ٲóɹ���־
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
				sprintf(erro_msg,"�ļ�%s�򿪳���",filenames);
				theJSLog.writeLog(LOG_CODE_FILE_OPEN_ERR,erro_msg);		//���ļ�ʧ��
				return false;
			}

			memset(szBuff,0,sizeof(szBuff));
			while(in.getline(szBuff,sizeof(szBuff)))   
			{
			 	cout<<"sql���Ϊ"<<szBuff<<endl;
				stmt.setSQLString(szBuff);	
				stmt.execute();  //ִ��sql���
				memset(szBuff,0,sizeof(szBuff));
			}
			in.close();
			cout<<"�������ļ�"<<endl;

			//ִ���ٲ�
			memset(m_AuditMsg,0,sizeof(m_AuditMsg));
			sprintf(m_AuditMsg,"%s|%c",m_Filename,stat);
			theJSLog<<"��ʼ�ٲ�"<<endi;
			if(!IsAuditSuccess(m_AuditMsg))   //�ٲ�ʧ��
			{
				theJSLog<<"�ٲ�ʧ��"<<endi;
				flag=false;
				stmt.rollback();

				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
				stmt.close();
				return false;
			}
		
			stmt.close();

			 //ÿ����һ���ļ�����¼��D_SQL_FILEREG����
			saveLog(stat);


			if(stat=='E')           
			{
				flag=false;
				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
				return false;
			}
			if(stat=='F')
				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
			return true;
		 }else{
	 		memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"init() �������ݿ�ʧ�� connect error");
			theJSLog.writeLog(LOG_CODE_DB_CONNECT_ERR,erro_msg);		//�������ݿ�ʧ��
			return  false;
		 }
	 } catch( SQLException e ) {
		stat='E';
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"�����ļ�[%s]��sql���ʧ��:%s,sql���Ϊ��%s",filenames,e.what(),szBuff);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣

			//ִ���ٲ�
			memset(m_AuditMsg,0,sizeof(m_AuditMsg));
			sprintf(m_AuditMsg,"%s|%c",m_Filename,stat);
			cout<<"��ʼ�ٲ�"<<endl;
			if(!IsAuditSuccess(m_AuditMsg))   //�ٲ�ʧ��
			{
				cout<<"�ٲ�ʧ��"<<endl;
				flag=false;
				stmt.rollback();

				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
				stmt.close();
				return false;
			}
		
			stmt.close();

			 //ÿ����һ���ļ�����¼��D_SQL_FILEREG����
			saveLog(stat);


			if(stat=='E')           
			{
				flag=false;
				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
				return false;
			}
			if(stat=='F')
				moveFiles(flag);		//�ļ��Ƶ�����Ŀ¼
	//	throw jsexcp::CException(0, "�����ļ���sql���ʧ��", __FILE__, __LINE__);
		return false;
      } 	
}


void CSql::saveLog(char stat)  //ÿ����һ���ļ������浽D_SQL_FILEREG����
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
			stmt.execute();//ִ��sql���
			stmt.close();
		//	cout<<"D_SQL_FILEREG���������ݳɹ�"<<endl;
			}else{
	 		cout<<"connect error."<<endl;
	 		return ;
		 }
	 	conn.close();
		conn.commit();
	}catch( SQLException e ) {
		memset(erro_msg,0,sizeof(erro_msg));
		sprintf(erro_msg,"�����ļ�ʱ���浽D_SQL_FILEREG��ʱsql���ʧ��:%s,sql���Ϊ��%s",e.what(),sql);
		theJSLog.writeLog(LOG_CODE_DB_EXECUTE_ERR,erro_msg);		//����sqlִ���쳣
		conn.close();
		return ;
    } 			
}


bool CSql::moveFiles(bool flag)//���Ѿ���������ļ��ƶ���ָ������Ŀ¼	
{
	char bak_dir[512];
	char tmp[1024];
	if(!flag)  //�ٲ�ʧ�ܣ�����ִ���ļ��е�sql���ʧ�ܣ����浽ʧ��Ŀ¼
	{
		
		theJSLog<<"�����ļ�"<<m_Filename<<" ������Ŀ¼ "<<erro_path<<endi;
			
		memset(tmp,0,sizeof(tmp));
		strcpy(tmp,erro_path);
		strcat(tmp,m_Filename);
	//	theJSLog <<"�����ļ�ȫ·����Ϊ:"<<tmp<<endi;
		if(rename(filenames,tmp))		
		{
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"�ƶ��ļ�[%s]������Ŀ¼ʧ��: %s",filenames,strerror(errno));
			theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
			return false;
		}	
	}
	else     //�ٲóɹ������浽ָ��Ŀ¼
	{ 
		getCurTime(currTime);
		memset(bak_dir,0,sizeof(bak_dir));
		strcpy(bak_dir,output_path);
		strncat(bak_dir,currTime,6);
		completeDir(bak_dir);
		strncat(bak_dir,currTime+6,2);
		completeDir(bak_dir);
		theJSLog<<"�����ļ�"<<m_Filename<<" ��Ŀ¼ "<<bak_dir<<endi;

		if(chkAllDir(bak_dir) == 0)
		{	
			memset(tmp,0,sizeof(tmp));
			strcpy(tmp,bak_dir);
			strcat(tmp,m_Filename);
		//	theJSLog <<"�����ļ�ȫ·����Ϊ:"<<tmp<<endi;
		
			if(rename(filenames,tmp))		
			{
				memset(erro_msg,0,sizeof(erro_msg));
				sprintf(erro_msg,"�ƶ��ļ�[%s]������Ŀ¼ʧ��: %s",filenames,strerror(errno));
				theJSLog.writeLog(LOG_CODE_FILE_MOVE_ERR,erro_msg);
				return false;
			}	
		 }
		 else
		 {
			memset(erro_msg,0,sizeof(erro_msg));
			sprintf(erro_msg,"����·��[%s]�����ڣ����޷�����",bak_dir);
			theJSLog.writeLog(LOG_CODE_DIR_OPEN_ERR,erro_msg);		//��Ŀ¼����
		 }
	}

	return true;
}


//���ֳ�ʼ��
bool CSql::drInit()
{
		//����ģ������ʵ��ID
		char tmp[32];
		memset(tmp,0,sizeof(tmp));
		sprintf(tmp,"%ld",getPrcID());

		theJSLog << "��ʼ������ƽ̨,ģ����:"<< module_name<<" ʵ����:"<<tmp<<endi;

		int ret = dr_InitPlatform(module_name,tmp);
		if(ret != 0)
		{
			theJSLog << "����ƽ̨��ʼ��ʧ��,����ֵ=" << ret<<endi;
			return false;
		}
		else
		{
			theJSLog<<"dr_InitPlatform ok."<<endi;
		}

		m_enable = true;

		drStatus = _dr_GetSystemState();	//��ȡ����ϵͳ״̬
		if(drStatus < 0)
		{
			theJSLog<<"��ȡ����ƽ̨״̬����: ����ֵ="<<drStatus<<endi;
			return false;
		}
		
		if(drStatus == 0)		theJSLog<<"��ǰϵͳ����Ϊ��ϵͳ"<<endi;
		else if(drStatus == 1)	theJSLog<<"��ǰϵͳ����Ϊ��ϵͳ"<<endi;
		else if(drStatus == 2)	theJSLog<<"��ǰϵͳ���÷�����ϵͳ"<<endi;

		return true;
}

//��ϵͳ����ͬ������,��ϵͳ��ȡͬ������
int CSql::drVarGetSet(char* serialString)
{
		int ret ;
		char tmpVar[5000] = {0};

		//�������ƽ̨���л���
		ret = dr_CheckSwitchLock();   
		if(ret != 0)  
		{  
			theJSLog<<"��������л���ʧ��,����ֵ="<<ret<<endi;
			return -1;  
		} 
		//��ʼ��index  
		ret = dr_InitIDX();  
		if(ret != 0)  
		{  
			theJSLog<<"��ʼ��indexʧ��,����ֵ=" <<ret<<endi;
			dr_AbortIDX();
			return -1;  
		}
/*		
		//��ϵͳ�����ļ�����·�����ļ��� ֻ������ƽ̨���Ը�֪,��ϵͳ�޷�ʶ��
		if(drStatus != 1)
		{
			snprintf(tmpVar, sizeof(tmpVar), "%s",input_path);
			ret = dr_SyncIdxVar("@@CHECKPATH", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"�����ļ�����·��ʧ��,�ļ�·��["<<input_path<<"]"<<endi;
				dr_AbortIDX();
				return -1;
			}
			
			snprintf(tmpVar, sizeof(tmpVar), "%s", m_Filename);
			ret = dr_SyncIdxVar("@@CHECKFILE", tmpVar,SYNC_SINGLE);  
			if(ret != 0)
			{
				theJSLog<<"�����ļ�ʧ��,�ļ���["<<m_Filename<<"]"<<endi;
				dr_AbortIDX();
				return -1;
			}

			cout<<"�����ļ�·��:"<<input_path<<" �ļ���:"<<m_Filename<<endl;
		}
*/

		snprintf(tmpVar, sizeof(tmpVar), "%s", serialString);
		//��ϵͳ��Ҫͬ����index ����ֵ�ԡ�д������ƽ̨ά����index �ļ���
		//��ϵͳ���øú����Ľ���ǣ�var��ú���ϵͳһ�������������ֵ��	SYNC_SINGLE��ʾע�ᵥһ���������
		ret = dr_SyncIdxVar("serialString", tmpVar, SYNC_SINGLE);		
		if (ret != 0)
		{
			theJSLog<<"�����д�ʱʧ��,������:["<<serialString<<"]"<<endi;
			dr_AbortIDX();
			return -1;
		}
		//serialString = tmpVar;			//ͬ�������ַ���,��ϵͳ�Ǹ�ֵ����ϵͳ��ȡֵ
		strcpy(serialString,tmpVar);
		//m_AuditMsg = tmpVar;			//Ҫ�ٲõ��ַ���
		

		// <5> ����ʵ����  ������ϵͳע���IDX��ģ����ò�����
		//��ϵͳ��index manager���IDX��������󣬰�ʹ�øú���ע������������Ϊģ��ĵ��ò���trigger��Ӧ�Ľ���
		snprintf(tmpVar, sizeof(tmpVar), "%d", getPrcID());
		ret = dr_SyncIdxVar("@@ARG", tmpVar,SYNC_SINGLE);  
		if(ret !=0)
		{
			theJSLog<<"����ʵ����ʧ�ܣ�"<<tmpVar<<endi;
			dr_AbortIDX();  
			return -1;
		}
		
		
		// <6> Ԥ�ύindex  �˹ؼ������ڽ�ƽ̨��ǰ�ڴ��е��������д�����
		ret = dr_SyncIdxVar("@@FLUSH","SUCCESS",SYNC_SINGLE);  
		if (ret != 0 )
		{
			theJSLog<<"Ԥ�ύindexʧ��"<<endi;
			dr_AbortIDX();
			return -1;
		}
		
		
		// <7> �ύindex  	�ύIndex����index�ļ���������ɱ�־
		ret = dr_CommitIDX();  
		if(ret != 0)  
		{  
			theJSLog<<"�ύindexʧ��,����ֵ="<<ret<<endi;
			dr_AbortIDX();  
			return -1;  
		}

		//��ϵͳ����Ŀ¼
		//if(!m_syncDr.isMaster())thelog<<"��ϵͳSerialString��"<<m_SerialString<<endi;

		theJSLog<<"���ε�ͬ����serialString:"<<serialString<<endi;//for test

		return ret;

}

//�ٲ��ַ���
 bool CSql::IsAuditSuccess(const char* dealresult)
 {
		int auitStatus = 0, ret = 0;
//cout<<"��ʼ�ٲ�"<<endl;
		ret = dr_Audit(dealresult);
//cout<<"�Ѿ��ٲ�"<<endl;
		if(2 == ret )
		{
			theJSLog << "�����ٲ�ʧ��,���:" << ret <<"���ˣ�"<<dealresult<< endi;
			dr_AbortIDX();
			return false;
		}
		else if (3 == ret)
		{
			theJSLog<<"�����ٲó�ʱ..."<<endi;
			dr_AbortIDX();
			return false;
		}
		else if(4 == ret)
		{
			theJSLog<<"�Զ�idx�쳣��ֹ..."<<endi;
			dr_AbortIDX();
			return false;
		}
		else if(1 == ret)
		{
			ret = dr_CommitSuccess();
			if(ret != 0)
			{
				theJSLog << "ҵ��ȫ���ύʧ��(����ƽ̨)" << endi;
				dr_AbortIDX();
				return false;
			}
			theJSLog<<"ret = "<<ret<<"�ٲóɹ�...\n�ٲ����ݣ�"<<dealresult<<endi;
			return true;
		}
		else
		{
			theJSLog<<"δ֪ret="<<ret<<"	�ٲ����ݣ�"<<dealresult<<endi;
			dr_AbortIDX();
			return false;
		}
	
	return true;
 }

bool CSql::CheckTriggerFile()
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

















