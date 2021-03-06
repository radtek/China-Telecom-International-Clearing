
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
	//获取文件格式
	//CBindSQL ds(DBConn);
	char szFileType[30];
	
	//DBConnection conn;
	DBConnection conn;//数据库连接
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
	//公式编辑器绑定变量
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

	//获取数据源配置
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

		//获取时间配置
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
		
		//采用公式编辑器获取字串
		if(SourceEnv.key[0].szKeyType == 'I')
		{
			char conText[200];
			sprintf(conText, "\t\t%s", SourceEnv.key[0].szPickKeyFormat);
			sprintf(SourceEnv.key[0].szPickKeyFormat, "%s", conText);
			char result[500] ;
			memset(result, 0, sizeof(result));
			int ierrorno = 0;
			//预编译
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
		//获取其他去重关键字
		bool isHaveT = false;	//是否已经有T的类型出现
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
				//预编译
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
		//校验去重关键字个数
		if(i==1 || i>2&&isHaveT)
		{
			conn.close();
			char szLogStr[500];
			sprintf(szLogStr, "缺少关键字配置或者类型有为T且超过1个关键字,组ID=%s=",SourceEnv.szPickConfigId);
	  		throw CException(1, szLogStr, __FILE__, __LINE__);
		}

		SourceEnv.lPickLen= 0;
		for (i = 1; i<SourceEnv.ikeyCount; i++)
		{
			SourceEnv.lPickLen+= SourceEnv.key[i].iLen;
		}
		
		//设置压缩后的长度
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
	
	//获取数据源配置
	
}

void C_CFilterPlugin::dealNegativeExecute(CFmt_Change & org_rcd)
{
	if(source.ikeyCount <=0)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "尚未执行文件开始函数");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	//获取去重时间
	char szIndexTime[20];
	//此处获取整行话单,以提供给公式编辑器
	//inrcd = org_rcd;    CF_CFmtchange的=操作是重新生成新的地址，Copy_Record是复制记录到原地址上去
	inrcd.Copy_Record(org_rcd);
	
	if(source.key[0].szKeyType=='I')
	{
		//表示用公式编辑器得到字符串
		char result[500];
		memset(result, 0, sizeof(result));
		int errorno = 0;
		interpreter.Operation( result,sizeof(result)-1, &errorno, source.key[0].szPickKeyFormat);
		if ( errorno != 0)
		{
			char szLogStr[500];
  	  		sprintf(szLogStr, "获取时间字串失败 =%s=",source.key[0].szPickKeyFormat);    
		  	throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		sprintf(szIndexTime, result);
	}
	else
	{
		inrcd.Get_Field(source.key[0].szColName, szIndexTime);
	}

	//获取除时间以外的去重关键字
	char szPickIndex[3000];
	memset(szPickIndex, 0, sizeof(szPickIndex));
	//是否有已压缩字段
	bool isHaveT = false;
	for(int i=1; i<source.ikeyCount; i++)
	{
		char szTemp[256];
		memset(szTemp, 0, sizeof(szTemp));
		if(source.key[i].szKeyType=='I')
		{
			//表示用公式编辑器得到字符串
			int errorno = 0;
			interpreter.Operation( szTemp,sizeof(szTemp)-1, &errorno, source.key[i].szPickKeyFormat);
			if ( errorno != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "获取非时间字串失败 =%s=",source.key[i].szPickKeyFormat);    
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
	  	  		sprintf(szLogStr, "解析去重字串失败 %s=%s=", source.key[i].szColName, szInStr);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			int iTempResult = TransHToD(szInStr, szTemp, 18);
			if(iTempResult != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "解析去重字串失败 %s=%s=", source.key[i].szColName, szTemp);    
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
	//有已压缩字段
	if(isHaveT)
	{
		int iHashLen = hash.getHashLen(source.lPickLen);
		memcpy(szRealIndex, szPickIndex, FILTER_VALUESIZE);
		szRealIndex[FILTER_VALUESIZE]=0;
	}
	//没有已压缩字段,则对串进行压缩
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
		//theJSLog<< "本单无删除索引,szIndexTime == " << szIndexTime << " , szPickIndex ==" << szPickIndex << endi;
	}
	else
	{
		//theJSLog<< "本单已删除索引（内存）,szIndexTime == " << szIndexTime << " , szPickIndex ==" << szPickIndex << endi;
	}
}


void C_CFilterPlugin::execute(PacketParser& ps,ResParser& retValue)
{
//	int err___ = filter.IsMemError();
//	  if (err___)
//		{
//			char szMsg[FILTER_ERRMSG_LEN];
//			sprintf(szMsg, "共享内存异常，文件需要重处理:jiangjz-execute1:code=%d", err___);
//			throw CException(FILTER_ERR_OUT_OF_RANGE, szMsg, __FILE__, __LINE__);
//			return;
//		}

	//theJSLog <<"C_CFilterPlugin::execute" <<endd;
	//cout << "begin execute!" << endl;
	if(source.ikeyCount <=0)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "尚未执行文件开始函数");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	if(ps.getItem_num() != 1)
	{
		char szLogStr[500];
  	  	sprintf(szLogStr, "参数个数必须为1");
		throw CException(1, szLogStr, __FILE__, __LINE__);
	}

	char szClassifyId[RECORD_LENGTH+1];
	memset(szClassifyId, 0, sizeof(szClassifyId));
	ps.getFieldValue(1, szClassifyId);

	
	//获取去重时间
	char szIndexTime[20];
	//此处获取整行话单,以提供给公式编辑器
	//inrcd = ps.m_inRcd;          CF_CFmtchange的=操作是重新生成新的地址，Copy_Record是复制记录到原地址上去
	inrcd.Copy_Record(ps.m_inRcd);
	//cout << " ps.m_inRcd = "  << endl;
	if(source.key[0].szKeyType=='I')
	{
		//表示用公式编辑器得到字符串
		char result[500];
		memset(result, 0, sizeof(result));
		int errorno = 0;
		interpreter.Operation( result,sizeof(result)-1, &errorno, source.key[0].szPickKeyFormat);
		if ( errorno != 0)
		{
			char szLogStr[500];
  	  		sprintf(szLogStr, "获取时间字串失败 =%s=",source.key[0].szPickKeyFormat);    
		  	throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		sprintf(szIndexTime, result);
	}
	else
	{
		ps.getField(source.key[0].szColName, szIndexTime);
	}

	//获取除时间以外的去重关键字
	char szPickIndex[3000];
	memset(szPickIndex, 0, sizeof(szPickIndex));
	//是否有已压缩字段
	bool isHaveT = false;
	for(int i=1; i<source.ikeyCount; i++)
	{
		char szTemp[256];
		memset(szTemp, 0, sizeof(szTemp));
		if(source.key[i].szKeyType=='I')
		{
			//表示用公式编辑器得到字符串
			//memset(szTemp, 0, sizeof(szTemp));
			int errorno = 0;
			interpreter.Operation( szTemp,sizeof(szTemp)-1, &errorno, source.key[i].szPickKeyFormat);
			if ( errorno != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "获取非时间字串失败 =%s=",source.key[i].szPickKeyFormat);    
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
	  	  		sprintf(szLogStr, "解析去重字串失败 %s=%s=", source.key[i].szColName, szInStr);    
			  	throw CException(1, szLogStr, __FILE__, __LINE__);
			}
			int iTempResult = TransHToD(szInStr, szTemp, 18);
			if(iTempResult != 0)
			{
				char szLogStr[500];
	  	  		sprintf(szLogStr, "解析去重字串失败 %s=%s=", source.key[i].szColName, szTemp);    
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
	//有已压缩字段
	if(isHaveT)
	{
		int iHashLen = hash.getHashLen(source.lPickLen);
		memcpy(szRealIndex, szPickIndex, FILTER_VALUESIZE);
		szRealIndex[FILTER_VALUESIZE]=0;
		//strncpy(szRealIndex, szPickIndex, iHashLen);
	}
	//没有已压缩字段,则对串进行压缩
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
	//插不进表示重单
	if(!isInsert)
	{
		//theJSLog<< "C_CFilterPlugin::execute() 重单" <<endi;
		//debug
		  
		//end debug
		char result[10] = "";
		retValue.setAnaResult(eRepeat, szClassifyId, result);
	}
	else
	{
		//theJSLog<< "C_CFilterPlugin::execute() 非重单" <<endi;
	}
	
//	err___ = filter.IsMemError();
//		  if (err___)
//			{
//				char szMsg[FILTER_ERRMSG_LEN];
//				sprintf(szMsg, "共享内存异常，文件需要重处理:jiangjz-execute2:code=%d", err___);
//				throw CException(FILTER_ERR_OUT_OF_RANGE, szMsg, __FILE__, __LINE__);
//				return;
//			}
}

void C_CFilterPlugin::message(MessageParser&  pMessage)
{
	int dealType = pMessage.getMessageType();
	//处理批次
	if(dealType == MESSAGE_NEW_BATCH)
	{
		DEBUG_LOG <<"MESSAGE_NEW_BATCH" <<endd;
		DEBUG_LOG << "SOURCE_ID1=" << pMessage.getSourceId() << endd;
		sprintf(m_szSourceId, pMessage.getSourceId());
		DEBUG_LOG << "SOURCE_ID2=" << m_szSourceId << endd;
		
		theJSLog << "开始处理批次,m_szSourceId == " << m_szSourceId << endi;
		
		filter.DealBatch(m_szSourceId);
	}
	//处理数据源文件
	else if(dealType == MESSAGE_NEW_FILE)
	{
		DEBUG_LOG <<"MESSAGE_NEW_FILE" <<endd;
		sprintf(m_szSourceId, pMessage.getSourceId());
		sprintf(m_szFileName, pMessage.getFileName());
		
		theJSLog << "准备处理文件,数据源：" << m_szSourceId << ",文件：" << m_szFileName << endi;
		
		map<string, SSourceEnv>::iterator ite = mapSourceKey.find(m_szSourceId);
		if(ite != mapSourceKey.end())
		{
			source = ite->second;
		}
		else
		{
			char szLogStr[500];
			sprintf(szLogStr, "无法获取数据源配置=%s=", m_szSourceId);
			throw CException(1, szLogStr, __FILE__, __LINE__);
		}
		filter.DealFile(pMessage.getSourceId(), pMessage.getFileName());
	}
	//提交批次数据库
	else if(dealType == MESSAGE_END_BATCH_END_DATA)
	{
		DEBUG_LOG <<"MESSAGE_END_BATCH_END_DATA" <<endd;
		theJSLog << "批次处理结束" << endi;
		//do nothing here
	}
	//提交批次文件
	else if(dealType == MESSAGE_END_BATCH_END_FILES)
	{
		DEBUG_LOG <<"MESSAGE_END_BATCH_END_FILES" <<endd;
		theJSLog << "批次处理结束" << endi;
		//do nothing here
		source.ikeyCount = 0;
	}
	//提交数据源文件
	else if(dealType == MESSAGE_END_FILE)
	{
		DEBUG_LOG <<"MESSAGE_END_FILE" <<endd;
		filter.CommitFile();
	}
	else if(dealType == MESSAGE_BREAK_BATCH)
	{
		DEBUG_LOG <<"MESSAGE_BREAK_BATCH" <<endd;
		theJSLog << "批次中断" << endi;
		//filter.HandleLastError();
		//do nothing here
		//init can figure it
	}
	//无法识别的命令
	else
	{
		DEBUG_LOG <<"插件无实际操作 :" <<dealType <<endd;
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
*	Description	:H码制两个字节字符转化为10进制一个字节
*	Input param	: 
*		instr     入口字符串,要求双数字节数
*       outstr    出口字符串,要求字符串容量大于1/2入口字符串长度
*       outstrlen  出口字符串容量
*	Returns		: 
*		0:成功;
*		-1:入口字符串长度为单数
*		-2:出口字符串容量小于或等于1/2入口字符串长度
*		-3:入口字符串包含非H码字符
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


