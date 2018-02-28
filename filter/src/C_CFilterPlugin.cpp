
#include "C_CFilterPlugin.h"
using namespace std;
using namespace tpss;

C_CFilterPlugin* C_CFilterPlugin::pfilterplugin = NULL;
int C_CFilterPlugin::icout = 0;

C_CFilterPlugin::C_CFilterPlugin()
{
	memset(m_szComFile, 0, sizeof(m_szComFile));	
	memset(m_szFileName, 0, sizeof(m_szFileName));
	memset(m_szSourceGroup, 0, sizeof(m_szSourceGroup));
	memset(m_szSourceId, 0, sizeof(m_szSourceId));
	mapSourceKey.clear();
}

void C_CFilterPlugin::InitPickKey(const char *szSourceGroupID, const char *szServiceID)
{
	//��ȡ�ļ���ʽ
	//CBindSQL ds(DBConn);
	char szFileType[30];
	
	//DBConnection conn;
	DBConnection conn;//���ݿ�����
	if (!dbConnect(conn)){
		char szLogStr[500];	 	
		sprintf(szLogStr, "Connect DB err");
		throw CException(1, szLogStr, __FILE__, __LINE__);		
	}
	
	Statement stmt = conn.createStatement();
	Statement stmt1 = conn.createStatement();
	string sql = "select filetype_id from C_SOURCE_GROUP_DEFINE where SOURCE_GROUP=:szGroupId";
	stmt.setSQLString(sql);
	stmt << szSourceGroupID;	
	//ds.Open("select filetype_id from C_SOURCE_GROUP_DEFINE where SOURCE_GROUP=:szGroupId ", SELECT_QUERY );
	//ds<<szSourceGroupID;
	//if (!(ds>>szFileType))
	stmt.execute();
	if(!(stmt>>szFileType))
	{
		conn.close();
	 	char szLogStr[500];	 	
		sprintf(szLogStr, "select filetype_id from C_SOURCE_GROUP_DEFINE where SOURCE_GROUP='%s'  \
			and SERVICE='%s' ERR ", szSourceGroupID,  szServiceID);
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}
	
	//ds.Close();

	inrcd.Init(szFileType);
	theJSLog <<"filetype_id = " << szFileType <<endi;
	//��ʽ�༭���󶨱���
	int iColIndex;
	char szFieldName[256];
	sql = "select col_index,colname from C_TXTFILE_FMT where filetype_id = :input_filetype_id";
	stmt.setSQLString(sql);
	stmt<<szFileType;
	stmt.execute();
	/*if(!stmt.execute()){
		conn.close();
	 	char szLogStr[500];	 	
		sprintf(szLogStr, "select col_index,colname from C_TXTFILE_FMT  \
		where filetype_id = %s  ERR",szFileType);
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}*/
	//ds.Open("select col_index,colname from C_TXTFILE_FMT where filetype_id = :input_filetype_id");
	//ds<<szFileType;
	//while((ds>>iColIndex>>szFieldName))
	while((stmt>>iColIndex>>szFieldName))
	{
		interpreter.DefineVariable(szFieldName, inrcd.Get_Field(iColIndex));
	}
	//ds.Close();
	interpreter.DefineVariable("FILENAME", m_szFileName);
	interpreter.DefineVariable("SOURCEGROUP", m_szSourceGroup);
	interpreter.DefineVariable("SOURCE", m_szSourceId);

	//��ȡ����Դ����
	mapSourceKey.clear();
	//ds.Open("select SOURCE_ID from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP=:szSourcegroup", SELECT_QUERY);
	char szSourceId[10];
	//ds<<m_szSourceGroup;
	sql = "select SOURCE_ID from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP=:szSourcegroup";
	stmt.setSQLString(sql);
	stmt<<m_szSourceGroup;
	stmt.execute();
	//cout << "m_szSourceGroup = " <<m_szSourceGroup<< endl;
	//theJSLog << "sql = " << sql<<endl;
	/*if(!stmt.execute()){
		conn.close();
	 	char szLogStr[500];	 	
		sprintf(szLogStr, "select SOURCE_ID from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP= %s ERR",m_szSourceGroup);
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}*/
	//while(ds>>szSourceId)
	while(stmt>>szSourceId)
	{
		theJSLog << "loading source="<<szSourceId<<"= config..." << endi;
		SSourceEnv SourceEnv ;
		selectFromSourceEnv(szSourceId, szServiceID, "FILTER_CONFIG_ID", SourceEnv.szPickConfigId);

		//��ȡʱ������
		//CBindSQL dsTemp(DBConn);
		sql = "select a.colname,pick_time_flag,pick_key_type,pick_key_format, a.filetype_id,col_len from C_PICK_KEY a,C_TXTFILE_FMT b where pick_grp_id = :pick_config_id \
	     and pick_time_flag = 'Y' and a.filetype_id = b.filetype_id and a.colname = b.colname";
		//dsTemp.Open("select a.colname,pick_time_flag,pick_key_type,pick_key_format, a.filetype_id,col_len from C_PICK_KEY a,C_TXTFILE_FMT b where pick_grp_id = :pick_config_id \
	   //  and pick_time_flag = 'Y' and a.filetype_id = b.filetype_id and a.colname = b.colname ", SELECT_QUERY);
    stmt1.setSQLString(sql);
    stmt1<<SourceEnv.szPickConfigId;
		char szFiletypeId[20];
		//dsTemp<<SourceEnv.szPickConfigId;
		stmt1.execute();
		/*if(!stmt1.execute()){
			conn.close();
			char szLogStr[500];
	  		sprintf(szLogStr, "in pick_key where pick_grp_id = %s and pick_time_flag = 'Y' Find Info ERR",SourceEnv.szPickConfigId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);
		}*/
		//if (!(dsTemp>>SourceEnv.key[0].szColName>>SourceEnv.key[0].szTimeFlag>>SourceEnv.key[0].szKeyType>>SourceEnv.key[0].szPickKeyFormat>>szFiletypeId>>SourceEnv.key[0].iLen))
		stmt1>>SourceEnv.key[0].szColName>>SourceEnv.key[0].szTimeFlag>>SourceEnv.key[0].szKeyType>>SourceEnv.key[0].szPickKeyFormat>>szFiletypeId>>SourceEnv.key[0].iLen;
		/*if (!(stmt1>>SourceEnv.key[0].szColName>>SourceEnv.key[0].szTimeFlag>>SourceEnv.key[0].szKeyType>>SourceEnv.key[0].szPickKeyFormat>>szFiletypeId>>SourceEnv.key[0].iLen) )
		{
		    conn.close();
			char szLogStr[500];
	  		sprintf(szLogStr, "in pick_key where pick_grp_id = %s and pick_time_flag = 'Y' Find Info ERR",SourceEnv.szPickConfigId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);
		}*/
		
		//dsTemp.Close();
		SourceEnv.ikeyCount=1;

		if(strcmp(szFiletypeId, szFileType) != 0)
		{
			conn.close();
			char szLogStr[500];
	  		sprintf(szLogStr, "No matching filetypeid in C_SOURCE_GROUP_DEFINE[%s] and C_PICK_KEY[%s]", szFileType, szFiletypeId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		
		//���ù�ʽ�༭����ȡ�ִ�
		if(SourceEnv.key[0].szKeyType == 'I')
		{
			char conText[200];
			sprintf(conText, "\t\t%s", SourceEnv.key[0].szPickKeyFormat);
			sprintf(SourceEnv.key[0].szPickKeyFormat, "%s", conText);
			char result[500] ;
			memset(result, 0, sizeof(result));
			int ierrorno = 0;
			//Ԥ����
			interpreter.Operation( result,sizeof(result)-1, &ierrorno, SourceEnv.key[0].szPickKeyFormat);
			if ( ierrorno )
			{
				conn.close();
				char szLogStr[ERROR_MSG_LEN+1];
				sprintf(szLogStr,"excute a compiler =%s=%d err",SourceEnv.key[0].szPickKeyFormat, ierrorno);
				throw CException(1, szLogStr, __FILE__, __LINE__);
			}
		}
		else
		{
			SourceEnv.key[0].szKeyType= 'F';
		}

		//add by linyb 20100823
		//CBindSQL dsCheck(DBConn);
		sql = "select filetype_id, colname from C_PICK_KEY where pick_grp_id = :pick_config_id \
		 and (filetype_id, colname) not in (select filetype_id, colname from C_TXTFILE_FMT)";
		stmt1.setSQLString(sql);
		stmt1<<SourceEnv.szPickConfigId;
		//cout << "pick_grp_id = " << SourceEnv.szPickConfigId;
		//dsCheck.Open("select filetype_id, colname from C_PICK_KEY where pick_grp_id = :pick_config_id \
		// and (filetype_id, colname) not in (select filetype_id, colname from C_TXTFILE_FMT) ", SELECT_QUERY);

		//dsCheck <<SourceEnv.szPickConfigId;
		char szChFiletypeid[30];
		char szChColName[40];
		stmt1.execute();
		stmt1 >> szChFiletypeid >> szChColName;
		/*if(!(stmt1 >> szChFiletypeid >> szChColName)){
			conn.close();
			char szLogStr[500];
	  		sprintf(szLogStr, "Find filetypeid=%s= colname=%s= in C_PICK_KEY[pick_group_id = %s] ERR", szChFiletypeid, szChColName, SourceEnv.szPickConfigId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);	  		
		}*/
		//cout << "   szChFiletypeid >> szChColName " << szChFiletypeid << szChColName << endl;
		//if(dsCheck >> szChFiletypeid >> szChColName)
		//stmt1 >> szChFiletypeid >> szChColName;
		//dsCheck.Close();
		//end of add by linyb 20100823
		//��ȡ����ȥ�عؼ���
		bool isHaveT = false;	//�Ƿ��Ѿ���T�����ͳ���
		int i = 1;
		char szSql[1024];
		sprintf(szSql, "select a.colname,pick_time_flag,pick_key_type,pick_key_format, a.filetype_id,col_len from C_PICK_KEY a,C_TXTFILE_FMT b where pick_grp_id = :pick_config_id \
	     and pick_time_flag <> 'Y' and a.filetype_id = b.filetype_id and a.colname = b.colname  order by pick_key_no");

		//dsTemp.Open(szSql, SELECT_QUERY);
		stmt1.setSQLString(szSql);
		stmt1<<SourceEnv.szPickConfigId;
		if(!stmt1.execute()){
			conn.close();
			char szLogStr[500];
	  			sprintf(szLogStr, "Find filetypeid in C_SOURCE_GROUP_DEFINE[%s] and C_PICK_KEY[%s] ERR", szFileType, szFiletypeId);
	  			throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		//dsTemp<<SourceEnv.szPickConfigId;
		//while(dsTemp>>SourceEnv.key[i].szColName>>SourceEnv.key[i].szTimeFlag>>SourceEnv.key[i].szKeyType>>SourceEnv.key[i].szPickKeyFormat>>szFiletypeId>>SourceEnv.key[i].iLen)
	  while(stmt1>>SourceEnv.key[i].szColName>>SourceEnv.key[i].szTimeFlag>>SourceEnv.key[i].szKeyType>>SourceEnv.key[i].szPickKeyFormat>>szFiletypeId>>SourceEnv.key[i].iLen)
		{
			if(strcmp(szFiletypeId, szFileType) != 0)
			{
				conn.close();
				char szLogStr[500];
	  			sprintf(szLogStr, "No matching filetypeid in C_SOURCE_GROUP_DEFINE[%s] and C_PICK_KEY[%s]", szFileType, szFiletypeId);
	  			throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			
			if(SourceEnv.key[i].szKeyType == 'I')
			{
				char conText[200];
				sprintf(conText, "\t\t%s", SourceEnv.key[i].szPickKeyFormat);
				sprintf(SourceEnv.key[i].szPickKeyFormat, "%s", conText);
				char result[500] ;
				memset(result, 0, sizeof(result));
				int ierrorno = 0;
				//Ԥ����
				interpreter.Operation( result,sizeof(result)-1, &ierrorno, SourceEnv.key[i].szPickKeyFormat);
				if ( ierrorno )
				{
					conn.close();
					char szLogStr[ERROR_MSG_LEN+1];
					sprintf(szLogStr,"excute a compiler =%s=%d err",SourceEnv.key[i].szPickKeyFormat, ierrorno);
					throw CException(1, szLogStr, __FILE__, __LINE__);
				}
			}
			else if(SourceEnv.key[i].szKeyType == 'T')
			{
				//do nothing here
				isHaveT = true;
			}
			else
			{
				SourceEnv.key[i].szKeyType= 'F';
			}
			i++;
			SourceEnv.ikeyCount++;
			if(SourceEnv.ikeyCount >= FILTER_MAX_KEYNUM)
			{
				break;
			}
		}
	//	dsTemp.Close();
		//У��ȥ�عؼ��ָ���
		if(i==1 || i>2&&isHaveT)
		{
			conn.close();
			char szLogStr[500];
			sprintf(szLogStr, "ȱ�ٹؼ������û���������ΪT�ҳ���1���ؼ���,��ID=%s=",SourceEnv.szPickConfigId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);
		}

		SourceEnv.lPickLen= 0;
		for (i = 1; i<SourceEnv.ikeyCount; i++)
		{
			SourceEnv.lPickLen+= SourceEnv.key[i].iLen;
		}
		
		//����ѹ����ĳ���
		long lLen = hash.getHashLen(SourceEnv.lPickLen);
		SourceEnv.lPickLen = lLen;
		mapSourceKey.insert(map<string, SSourceEnv>::value_type(szSourceId, SourceEnv));
		//mapSourceKey[string(szSourceId)] = SourceEnv;
	}
	conn.close();
}

void C_CFilterPlugin::init(char *szSourceGroupID, char *szServiceID, int index)
{
	strcpy(m_szSourceGroup, szSourceGroupID);

	selectFromProcessEnv(szSourceGroupID, szServiceID, "FILTER_MEM_FILE", m_szComFile);

	char szPath[FILTER_FILESIZE];
	selectFromProcessEnv(szSourceGroupID, szServiceID, "FILTER_INDEX_PATH", szPath);
	completeDir(szPath);
	
	/*get source count from C_source_group_config*/
	SIZE_TYPE maxSource;
	//CBindSQL ds(DBConn);
	DBConnection conn;
	if(!dbConnect(conn)){
		char szLogStr[500];
		sprintf(szLogStr, "Connect DB err");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}
	Statement stmt = conn.createStatement();
	string sql = "select count(*)+3 from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP=:szGroupId ";
	stmt.setSQLString(sql);
	stmt<<szSourceGroupID;
	stmt.execute();
				
	//ds.Open("select count(*)+3 from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP=:szGroupId ", SELECT_QUERY );
	//ds<<szSourceGroupID;
	stmt>>maxSource;
	conn.close();
	
	//if (!(ds>>maxSource))
	//{
	//	ds.Close();  	
	//	char szLogStr[500];
	//	sprintf(szLogStr, "select count(*)+3 from C_SOURCE_GROUP_CONFIG where SOURCE_GROUP=%s error", szSourceGroupID);
	//	throw CException(1, szLogStr, __FILE__, __LINE__);
	//}
	//ds.Close();
	
	theJSLog << "init filter" <<endi;
	theJSLog << "FILTER_MEM_FILE=" << m_szComFile << endi;
	theJSLog << "FILTER_INDEX_PATH=" << szPath << endi;
	theJSLog << "MAXSOURCE=" << maxSource << endi;
	filter.Init(szPath, m_szComFile, maxSource);
	theJSLog << "init pickkey" <<endi;
	InitPickKey(szSourceGroupID, szServiceID);
	
	//��ȡ����Դ����
	
}

void C_CFilterPlugin::dealNegativeExecute(CFmt_Change & org_rcd)
{
	if(source.ikeyCount <=0)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "��δִ���ļ���ʼ����");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	//��ȡȥ��ʱ��
	char szIndexTime[20];
	//�˴���ȡ���л���,���ṩ����ʽ�༭��
	//inrcd = org_rcd;    CF_CFmtchange��=���������������µĵ�ַ��Copy_Record�Ǹ��Ƽ�¼��ԭ��ַ��ȥ
	inrcd.Copy_Record(org_rcd);
	
	if(source.key[0].szKeyType=='I')
	{
		//��ʾ�ù�ʽ�༭���õ��ַ���
		char result[500];
		memset(result, 0, sizeof(result));
		int errorno = 0;
		interpreter.Operation( result,sizeof(result)-1, &errorno, source.key[0].szPickKeyFormat);
		if ( errorno != 0)
		{
			char szLogStr[500];
  	  		sprintf(szLogStr, "��ȡʱ���ִ�ʧ�� =%s=",source.key[0].szPickKeyFormat);    
		  	throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		sprintf(szIndexTime, result);
	}
	else
	{
		inrcd.Get_Field(source.key[0].szColName, szIndexTime);
	}

	//��ȡ��ʱ�������ȥ�عؼ���
	char szPickIndex[3000];
	memset(szPickIndex, 0, sizeof(szPickIndex));
	//�Ƿ�����ѹ���ֶ�
	bool isHaveT = false;
	for(int i=1; i<source.ikeyCount; i++)
	{
		char szTemp[256];
		memset(szTemp, 0, sizeof(szTemp));
		if(source.key[i].szKeyType=='I')
		{
			//��ʾ�ù�ʽ�༭���õ��ַ���
			int errorno = 0;
			interpreter.Operation( szTemp,sizeof(szTemp)-1, &errorno, source.key[i].szPickKeyFormat);
			if ( errorno != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "��ȡ��ʱ���ִ�ʧ�� =%s=",source.key[i].szPickKeyFormat);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
		}
		else if(source.key[i].szKeyType=='T')
		{
			char szInStr[256];
			inrcd.Get_Field(source.key[i].szColName, szInStr);
			if(strlen(szInStr) != 32)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "����ȥ���ִ�ʧ�� %s=%s=", source.key[i].szColName, szInStr);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			int iTempResult = TransHToD(szInStr, szTemp, 18);
			if(iTempResult != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "����ȥ���ִ�ʧ�� %s=%s=", source.key[i].szColName, szTemp);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			isHaveT = true;
		}
		else
		{
			inrcd.Get_Field(source.key[i].szColName, szTemp);
		}
		szTemp[strlen(szTemp)] = 0;
		strcat(szPickIndex, szTemp);
	}
	
	char szRealIndex[FILTER_VALUESIZE+1];
	memset(szRealIndex, 0, sizeof(szRealIndex));
	//����ѹ���ֶ�
	if(isHaveT)
	{
		int iHashLen = hash.getHashLen(source.lPickLen);
		memcpy(szRealIndex, szPickIndex, FILTER_VALUESIZE);
		szRealIndex[FILTER_VALUESIZE]=0;
	}
	//û����ѹ���ֶ�,��Դ�����ѹ��
	else
	{
		unsigned char szTempIndex[100];
		memset(szTempIndex, 0, sizeof(szTempIndex));
		//theJSLog << "szPickIndex,strlen(szPickIndex)=="<< szPickIndex << "," << strlen(szPickIndex) << ";" << endi;
    hash.getHashStr(szTempIndex, szPickIndex, strlen(szPickIndex));
    memcpy(szRealIndex, szTempIndex, FILTER_VALUESIZE);
    szRealIndex[FILTER_VALUESIZE]=0;
	}
  //theJSLog << "filter.ReMoveIndex,szIndexTime==" << szIndexTime << ",szRealIndex==" << szRealIndex << ";" << endi;
	bool isRemove = filter.ReMoveIndex(szIndexTime , szRealIndex);
	if(!isRemove)
	{
		//theJSLog<< "������ɾ������,szIndexTime == " << szIndexTime << " , szPickIndex ==" << szPickIndex << endi;
	}
	else
	{
		//theJSLog<< "������ɾ���������ڴ棩,szIndexTime == " << szIndexTime << " , szPickIndex ==" << szPickIndex << endi;
	}
}


void C_CFilterPlugin::execute(PacketParser& ps,ResParser& retValue)
{
//	int err___ = filter.IsMemError();
//	  if (err___)
//		{
//			char szMsg[FILTER_ERRMSG_LEN];
//			sprintf(szMsg, "�����ڴ��쳣���ļ���Ҫ�ش���:jiangjz-execute1:code=%d", err___);
//			throw CException(FILTER_ERR_OUT_OF_RANGE, szMsg, __FILE__, __LINE__);
//			return;
//		}

	//theJSLog <<"C_CFilterPlugin::execute" <<endd;
	//cout << "begin execute!" << endl;
	if(source.ikeyCount <=0)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "��δִ���ļ���ʼ����");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	if(ps.getItem_num() != 1)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "������������Ϊ1");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	char szClassifyId[RECORD_LENGTH+1];
	memset(szClassifyId, 0, sizeof(szClassifyId));
	ps.getFieldValue(1, szClassifyId);

	
	//��ȡȥ��ʱ��
	char szIndexTime[20];
	//�˴���ȡ���л���,���ṩ����ʽ�༭��
	//inrcd = ps.m_inRcd;          CF_CFmtchange��=���������������µĵ�ַ��Copy_Record�Ǹ��Ƽ�¼��ԭ��ַ��ȥ
	inrcd.Copy_Record(ps.m_inRcd);
	//cout << " ps.m_inRcd = "  << endl;
	if(source.key[0].szKeyType=='I')
	{
		//��ʾ�ù�ʽ�༭���õ��ַ���
		char result[500];
		memset(result, 0, sizeof(result));
		int errorno = 0;
		interpreter.Operation( result,sizeof(result)-1, &errorno, source.key[0].szPickKeyFormat);
		if ( errorno != 0)
		{
			char szLogStr[500];
  	  		sprintf(szLogStr, "��ȡʱ���ִ�ʧ�� =%s=",source.key[0].szPickKeyFormat);    
		  	throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		sprintf(szIndexTime, result);
	}
	else
	{
		ps.getField(source.key[0].szColName, szIndexTime);
	}

	//��ȡ��ʱ�������ȥ�عؼ���
	char szPickIndex[3000];
	memset(szPickIndex, 0, sizeof(szPickIndex));
	//�Ƿ�����ѹ���ֶ�
	bool isHaveT = false;
	for(int i=1; i<source.ikeyCount; i++)
	{
		char szTemp[256];
		memset(szTemp, 0, sizeof(szTemp));
		if(source.key[i].szKeyType=='I')
		{
			//��ʾ�ù�ʽ�༭���õ��ַ���
			//memset(szTemp, 0, sizeof(szTemp));
			int errorno = 0;
			interpreter.Operation( szTemp,sizeof(szTemp)-1, &errorno, source.key[i].szPickKeyFormat);
			if ( errorno != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "��ȡ��ʱ���ִ�ʧ�� =%s=",source.key[i].szPickKeyFormat);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
		}
		else if(source.key[i].szKeyType=='T')
		{
			char szInStr[256];
			ps.getField(source.key[i].szColName, szInStr);
			if(strlen(szInStr) != 32)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "����ȥ���ִ�ʧ�� %s=%s=", source.key[i].szColName, szInStr);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			int iTempResult = TransHToD(szInStr, szTemp, 18);
			if(iTempResult != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "����ȥ���ִ�ʧ�� %s=%s=", source.key[i].szColName, szTemp);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			isHaveT = true;
		}
		else
		{
			ps.getField(source.key[i].szColName, szTemp);
		}
		szTemp[strlen(szTemp)] = 0;
		strcat(szPickIndex, szTemp);
	}

	char szRealIndex[FILTER_VALUESIZE+1];
	memset(szRealIndex, 0, sizeof(szRealIndex));
	//����ѹ���ֶ�
	if(isHaveT)
	{
		int iHashLen = hash.getHashLen(source.lPickLen);
		memcpy(szRealIndex, szPickIndex, FILTER_VALUESIZE);
		szRealIndex[FILTER_VALUESIZE]=0;
		//strncpy(szRealIndex, szPickIndex, iHashLen);
	}
	//û����ѹ���ֶ�,��Դ�����ѹ��
	else
	{
		unsigned char szTempIndex[100];
		memset(szTempIndex, 0, sizeof(szTempIndex));
		//theJSLog << "szPickIndex,strlen(szPickIndex)=="<< szPickIndex << "," << strlen(szPickIndex) << ";" << endi;
    hash.getHashStr(szTempIndex, szPickIndex, strlen(szPickIndex));
    //int iHashLen = hash.getHashLen(strlen(szPickIndex));
    memcpy(szRealIndex, szTempIndex, FILTER_VALUESIZE);
    szRealIndex[FILTER_VALUESIZE]=0;
    //strncpy(szRealIndex, szTempIndex, iHashLen);
	}
  //theJSLog << "filter.AddIndex,szIndexTime==" << szIndexTime << ",szRealIndex==" << szRealIndex << ";" << endi;
	bool isInsert = filter.AddIndex(szIndexTime , szRealIndex);
	//�岻����ʾ�ص�
	if(!isInsert)
	{
		//theJSLog<< "C_CFilterPlugin::execute() �ص�" <<endi;
		//debug
		  
		//end debug
		char result[10] = "";
		retValue.setAnaResult(eRepeat, szClassifyId, result);
	}
	else
	{
		//theJSLog<< "C_CFilterPlugin::execute() ���ص�" <<endi;
	}
	
//	err___ = filter.IsMemError();
//		  if (err___)
//			{
//				char szMsg[FILTER_ERRMSG_LEN];
//				sprintf(szMsg, "�����ڴ��쳣���ļ���Ҫ�ش���:jiangjz-execute2:code=%d", err___);
//				throw CException(FILTER_ERR_OUT_OF_RANGE, szMsg, __FILE__, __LINE__);
//				return;
//			}
}

void C_CFilterPlugin::message(MessageParser&  pMessage)
{
	int dealType = pMessage.getMessageType();
	//��������
	if(dealType == MESSAGE_NEW_BATCH)
	{
		DEBUG_LOG <<"MESSAGE_NEW_BATCH" <<endd;
		DEBUG_LOG << "SOURCE_ID1=" << pMessage.getSourceId() << endd;
		sprintf(m_szSourceId, pMessage.getSourceId());
		DEBUG_LOG << "SOURCE_ID2=" << m_szSourceId << endd;
		
		theJSLog << "��ʼ��������,m_szSourceId == " << m_szSourceId << endi;
		
		filter.DealBatch(m_szSourceId);
	}
	//��������Դ�ļ�
	else if(dealType == MESSAGE_NEW_FILE)
	{
		DEBUG_LOG <<"MESSAGE_NEW_FILE" <<endd;
		sprintf(m_szSourceId, pMessage.getSourceId());
		sprintf(m_szFileName, pMessage.getFileName());
		
		theJSLog << "׼�������ļ�,����Դ��" << m_szSourceId << ",�ļ���" << m_szFileName << endi;
		
		map<string, SSourceEnv>::iterator ite = mapSourceKey.find(m_szSourceId);
		if(ite != mapSourceKey.end())
		{
			source = ite->second;
		}
		else
		{
			char szLogStr[500];
			sprintf(szLogStr, "�޷���ȡ����Դ����=%s=", m_szSourceId);
			throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		filter.DealFile(pMessage.getSourceId(), pMessage.getFileName());
	}
	//�ύ�������ݿ�
	else if(dealType == MESSAGE_END_BATCH_END_DATA)
	{
		DEBUG_LOG <<"MESSAGE_END_BATCH_END_DATA" <<endd;
		theJSLog << "���δ�������" << endi;
		//do nothing here
	}
	//�ύ�����ļ�
	else if(dealType == MESSAGE_END_BATCH_END_FILES)
	{
		DEBUG_LOG <<"MESSAGE_END_BATCH_END_FILES" <<endd;
		theJSLog << "���δ�������" << endi;
		//do nothing here
		source.ikeyCount = 0;
	}
	//�ύ����Դ�ļ�
	else if(dealType == MESSAGE_END_FILE)
	{
		DEBUG_LOG <<"MESSAGE_END_FILE" <<endd;
		filter.CommitFile();
	}
	else if(dealType == MESSAGE_BREAK_BATCH)
	{
		DEBUG_LOG <<"MESSAGE_BREAK_BATCH" <<endd;
		theJSLog << "�����ж�" << endi;
		//filter.HandleLastError();
		//do nothing here
		//init can figure it
	}
	//�޷�ʶ�������
	else
	{
		DEBUG_LOG <<"�����ʵ�ʲ��� :" <<dealType <<endd;
		//do nothing or special
	}
}

C_CFilterPlugin::~C_CFilterPlugin()
{
	map<string, SSourceEnv>::iterator ite = mapSourceKey.begin();
	DEBUG_LOG << "in ~C_CFilterPlugin()" <<endd;
	while(ite != mapSourceKey.end())
	{
		DEBUG_LOG <<'\t'<<ite->first << endd;
		DEBUG_LOG <<'\t'<< (ite->second).szPickConfigId<<endd;
		DEBUG_LOG <<'\t'<< (ite->second).ikeyCount<<endd;
		DEBUG_LOG <<'\t'<< (ite->second).lPickLen<<endd;
		ite++ ;
	}
	mapSourceKey.clear();
}


void selectFromProcessEnv(const char *szGroupId, const char *szServiceId, const char *szEnvName, char *szEnvValue)
{
	//CBindSQL ds(DBConn);
	//ds.Open("select var_value from C_PROCESS_ENV where SOURCE_GROUP=:szGroupId \
	//	and SERVICE=:szservice and varname= :varname", SELECT_QUERY );
  DBConnection conn;
  if (!dbConnect(conn)){
  	char szLogStr[500];
		sprintf(szLogStr, "Connect DB err");
		throw CException(1, szLogStr, __FILE__, __LINE__);
  }
  Statement stmt = conn.createStatement();
	string sql = "select var_value from C_PROCESS_ENV where SOURCE_GROUP=:szGroupId \
		and SERVICE=:szservice and varname= :varname";
	stmt.setSQLString(sql);
	stmt<<szGroupId<<szServiceId<<szEnvName;
	if (!stmt.execute()){
		conn.close();
		char szLogStr[500];
		sprintf(szLogStr, "select var_value from C_PROCESS_ENV where SOURCE_GROUP='%s'  \
			and SERVICE='%s' and varname= '%s' is NULL", szGroupId,  szServiceId, szEnvName);
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}
	//ds<<szGroupId<<szServiceId<<szEnvName;
	stmt>>szEnvValue;
	//if (!(ds>>szEnvValue))
	
	//ds.Close();
	conn.close(); 	
}

void selectFromSourceEnv(const char *szSourceId, const char *szServiceId, const char *szEnvName, char *szEnvValue)
{
	//CBindSQL ds(DBConn);
	DBConnection conn;
  if (!dbConnect(conn)){
  	char szLogStr[500];
		sprintf(szLogStr, "Connect DB err");
		throw CException(1, szLogStr, __FILE__, __LINE__);
  }
  
	//ds.Open("select var_value from C_SOURCE_ENV where SOURCE_ID=:szSourceId \
	//	and SERVICE=:szservice and varname= :varname", SELECT_QUERY );

	//ds<<szSourceId<<szServiceId<<szEnvName;
	Statement stmt = conn.createStatement();
	string sql = "select var_value from C_SOURCE_ENV where SOURCE_ID=:szSourceId \
		and SERVICE=:szservice and varname= :varname";
	stmt.setSQLString(sql);
	stmt<<szSourceId<<szServiceId<<szEnvName;
	if (!stmt.execute()){
		conn.close();
		char szLogStr[500];
		sprintf(szLogStr, "select var_value from C_SOURCE_ENV where SOURCE_ID='%s'  \
			and SERVICE='%s' and varname= '%s' is NULL", szSourceId,  szServiceId, szEnvName);
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}
	stmt>>szEnvValue;
	
	conn.close();
	//ds.Close();
}


/****************************************************************
*	Function Name	: TransHToD
*	Description	:H���������ֽ��ַ�ת��Ϊ10����һ���ֽ�
*	Input param	: 
*		instr     ����ַ���,Ҫ��˫���ֽ���
*       outstr    �����ַ���,Ҫ���ַ�����������1/2����ַ�������
*       outstrlen  �����ַ�������
*	Returns		: 
*		0:�ɹ�;
*		-1:����ַ�������Ϊ����
*		-2:�����ַ�������С�ڻ����1/2����ַ�������
*		-3:����ַ���������H���ַ�
*	complete	:2005/10/18
*****************************************************************/
int TransHToD(char *instr,char *outstr,int outstrlen)
{
  int instrlen = strlen(instr);
  if((instrlen%2) !=0) return -1;
  if(outstrlen<=instrlen/2) return -2;
  
  for(int i =0;i<instrlen/2;i++)
  {
    int ascii_char=0;
    if((*(instr+i*2))<58&&(*(instr+i*2))>47) ascii_char = (*(instr+i*2) - 48) *16;
     else if((*(instr+i*2))<71&&(*(instr+i*2))>64) ascii_char = (*(instr+i*2) - 55) *16;
       else if((*(instr+i*2))<103&&(*(instr+i*2))>96) ascii_char = (*(instr+i*2) - 87) *16;
         else return -3;
    if((*(instr+i*2+1))<58&&(*(instr+i*2+1))>47) ascii_char += (*(instr+i*2+1) - 48) ;
     else if((*(instr+i*2+1))<71&&(*(instr+i*2+1))>64) ascii_char += (*(instr+i*2+1) - 55) ;
       else if((*(instr+i*2+1))<103&&(*(instr+i*2+1))>96) ascii_char += (*(instr+i*2+1) - 87) ;
         else return -3;
       
    outstr[i] = ascii_char; 
  }

  outstr[instrlen/2] = '\0';
  return 0;
}

