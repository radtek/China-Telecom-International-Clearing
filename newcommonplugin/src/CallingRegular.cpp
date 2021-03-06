/****************************************************************
 filename: CallingRegular.cpp
 module: 用户自定义插件头文件
 created by:	ouyh
 create date:	2010-07-04
 version: 3.0.0
 description: 
            用于整合业务长途计费业务计费阶段主叫号码规整，用于后续分析
            主要功能: 
				结合号码资料，对主叫、被叫、计费等号码统一规整为"区号+号码"的格式，被叫接入码输出到指定字段。
 update:
作主叫： 
   1）短信结算业务：a、对13800138000或013800138000号码规整成‘020+13800138000’； 
                   b、对0+区号+1380013800号码，取原区号作为138号码的归属区号，并进行号码规整； 
	   短信结算业务在做规整之前保证输入号码形式为区号+ 13800138000
   2）其他业务：13800138000、013800138000或0+区号+13800138000，均挂起处理。
 *****************************************************************/
#include "CallingRegular.h"

C_CallingRegular::C_CallingRegular()
{
	memset(m_szSourceGroupId,0,sizeof(m_szSourceGroupId));
	memset(m_szServCatId,0,sizeof(m_szServCatId));
	memset(m_szSourceId,0,sizeof(m_szSourceId));
	memset(m_szCallNbr,0,sizeof(m_szCallNbr));
	memset(m_szNbrType,0,sizeof(m_szNbrType));
	memset(m_szCdrBegin,0,sizeof(m_szCdrBegin));
	memset(m_szDealType,0,sizeof(m_szDealType));
	memset(m_szDefTollcode,0,sizeof(m_szDefTollcode));
	memset(m_szBefTollTel,0,sizeof(m_szBefTollTel));//接入码
	memset(m_szTollTel,0,sizeof(m_szTollTel));//带0区号+号码（固话或手机号码）
	memset(m_szAfterTollTel,0,sizeof(m_szAfterTollTel));	//号码（固话或手机号码）
	memset(m_szTollcode,0,sizeof(m_szTollcode));//带0区号
	memset(m_szCallNumber,0,sizeof(m_szCallNumber));//接入码+带0区号+号码（固话或手机号码）
	memset(m_sAbnReason,0,sizeof(m_sAbnReason));//无资料原因
	memset(m_szAbnField,0,sizeof(m_szAbnField));//无资料内容
	//m_iProcessId=0;
	m_iAbnReason=0;
	initFlag=0;
	table = NULL;
}

C_CallingRegular::~C_CallingRegular()
{
}

/*************************************************
* 描述    ：主叫号码规整
* 入口参数：StdChkCallingNbr(SOURCEGROUP_ID,PROCESS_ID,SERV_CAT_ID,SOURCE_ID,CALL_NBR,NBR_TYPE,CDR_BEGIN,DEAL_TYPE,DEF_TOLLCODE)
* 出口参数：
* 返回    ：
**************************************************/
void C_CallingRegular::execute(PacketParser& pps,ResParser& retValue)
{
	char ErrorMsg[MSG_LEN];
	
	//插件执行入口
	if ( pps.getItem_num() != 6 )
	{
		sprintf( ErrorMsg,"插件 StdChkCallingNbr 的输入参数不正确！");
		throw jsexcp::CException(ERR_LACK_PARAM,(char *)ErrorMsg,(char *)__FILE__,__LINE__);		
	}
			
	if( initFlag != 1 )
		Init( pps );
	
	//分析处理过程
	//主叫规整

	//得到输入参数
	getInputParams(pps);

	//传递到CallNumber 类
	setParam();

	//判断废号码头,若有则进行删除
	m_callnumber.DealRubTelHead();

	//判断是否是手机号
	if(m_callnumber.IsGsm())
	{	
		//查询之前去掉手机号之前的0
		m_callnumber.DealGsm();
		//从H码表获取区号
		m_iAbnReason=m_callnumber.queryGsm(m_szAbnField);
		if (m_iAbnReason)
		{
			//无资料
			pluginAnaResult result=eLackInfo;
			sprintf(m_sAbnReason,"%d",m_iAbnReason);
			retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
			//reOutputNum(pps,retValue);
			return;
		}

		//连接区号+手机号
		m_callnumber.ConnectTollGsm();
	}
	else
	{
		if (m_callnumber.IsZeroHead())
		{
			//以0开头
			m_iAbnReason=m_callnumber.queryTollcode(m_szAbnField);
			if (m_iAbnReason)
			{
				//无资料
				pluginAnaResult result=eLackInfo;
				sprintf(m_sAbnReason,"%d",m_iAbnReason);
				retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
				//reOutputNum(pps,retValue);
				return;
					
			}
			//区号+号码暂不进行处理
		}
		else
		{
			//add by weixy 0080128 修改分析流程
			m_iAbnReason=m_callnumber.Check10Len(m_szAbnField);
			if (m_iAbnReason==0)
			{
				//号码长度大于等于10位
				//号码补0，重新分析区号
				m_callnumber.AddZero();
				m_iAbnReason=m_callnumber.queryTollcode(m_szAbnField);
				if (m_iAbnReason)
				{
					//无资料
					//增加选项，当号码长度大于10，加0以后又找不到区号的，则增加区号
					if(m_szDealType[0]!='3')
					{
						pluginAnaResult result=eLackInfo;
						sprintf(m_sAbnReason,"%d",m_iAbnReason);
						retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
						//reOutputNum(pps,retValue);
						return;
					}
					m_callnumber.AddDefTollcode();	
				}
			}
			else
			{
				//判断本数据源是否能缺区号
				if(m_szDealType[0]=='1')
				//本数据源不能缺区号
				{
					//无资料长度小于10位，并且不能缺区号，直接挂起
					pluginAnaResult result=eLackInfo;
					sprintf(m_sAbnReason,"%d",m_iAbnReason);
					retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
					//reOutputNum(pps,retValue);
					return;
				}
				else
				{
					//本数据源可以缺区号
					
					//默认区号是否是省内
					m_iAbnReason=m_callnumber.IsDefTollProvince(m_szAbnField);
					if(m_iAbnReason==1)
					{
						//默认区号是省内
						
						//号码长度是否与默认区号归属本地网一致
						if (m_callnumber.IsTeleLengthRight())
						{
							//号码长度一致
							//是否以缺0默认区号开头
							if (m_callnumber.IsDefTollHead()==0)
							{
								//是否有局号资料
								if (!m_callnumber.queryTeleProperty(m_szAbnField))
								{
									//有局号资料，补默认区号
									m_callnumber.AddDefTollcode();
								}
								else
								{
									//没有局号资料
									//号码补0，
									m_callnumber.AddZero();
								}
							}
							else
							{
								//补默认区号
								m_callnumber.AddDefTollcode();
							}
						}
						else
						{
							//号码长度不一致
							//号码以缺0默认区号开头
							if (m_callnumber.IsDefTollHead()==0)
							{
								//号码补0，重新分析区号
								m_callnumber.AddZero();
							}
							else
							{
								//没有区号，补默认区号
								m_callnumber.AddDefTollcode();
							}
						}
					}//end of 默认区号是省内
					else if(m_iAbnReason ==0 )
					{
						//默认区号不是省内
						//号码补0，重新分析区号
						m_callnumber.AddZero();
						//没有区号，补默认区号
						if (m_callnumber.queryTollcode(m_szAbnField))
							m_callnumber.AddDefTollcode();
					}
					else
					{
						//无资料
						pluginAnaResult result=eLackInfo;
						sprintf(m_sAbnReason,"%d",m_iAbnReason);
						retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
						//reOutputNum(pps,retValue);
						return;
					}
				}//end of 本数据源能缺区号
			}
		}
	}//end of 固话
	
	//从CallNumber 类规整后的号码传递到本地
	m_callnumber.Get_Teleno(m_szCallNumber);
	
	//主叫校验	,输入形式为 区号+号码
	m_callnumber.Set_CallNbr(m_szCallNumber);
	m_callnumber.queryTollcode(m_szAbnField);
	m_callnumber.DealTollcode();
	
	//将AfterTollTel放在Teleno中
	m_callnumber.Get_AfterTollTel(m_szCallNumber);
	m_callnumber.Set_CallNbr(m_szCallNumber);
	
	//从此之后的工作就是针对AfterTollTel 进行
	if (m_szDealType[1]=='1')
	{
		//校验双区号	
		if (m_iAbnReason=m_callnumber.Check2Tollcode(m_szAbnField))
		{
			//无资料
			//双区号挂起原因填写入口单中的号码
			pluginAnaResult result=eLackInfo;
			sprintf(m_sAbnReason,"%d",m_iAbnReason);
			retValue.setAnaResult(result, m_sAbnReason, m_szCallNbr);
			//reOutputNum(pps,retValue);
			return;
		}
		
	}
	
	if (m_szDealType[2]!='0')
	{
		int checkLen;
		if (m_szDealType[2]=='Y') 
			checkLen=0;
		else 
			checkLen=m_szDealType[2]-48;
		//校验非法字符
		if (m_iAbnReason=m_callnumber.CheckValid(checkLen,m_szAbnField))
		{
			pluginAnaResult result=eLackInfo;
			sprintf(m_sAbnReason,"%d",m_iAbnReason);
			retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
			//reOutputNum(pps,retValue);
			return;
		}
	}
	
	if (m_callnumber.IsForeign())
	{
		if (m_szDealType[3]=='1')
		//校验区号后号码长度大于等于3位
		{
			if(m_iAbnReason=m_callnumber.CheckLen(m_szAbnField))
			{
				pluginAnaResult result=eLackInfo;
				sprintf(m_sAbnReason,"%d",m_iAbnReason);
				retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
				//reOutputNum(pps,retValue);
				return;
			}
		}

		if (m_szDealType[4]=='1')
		//校验省内2～8号码段号码长度的合法性
		{
			if(m_iAbnReason=m_callnumber.CheckGDLen(m_szAbnField))
			{
				pluginAnaResult result=eLackInfo;
				sprintf(m_sAbnReason,"%d",m_iAbnReason);
				retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
				//reOutputNum(pps,retValue);
				return;
			}
		}
	}

	else
	{
		//判断是否是手机号
		if(m_callnumber.IsGsm())
		{
			//手机号
			//增加校验方式2，3 分别表示短信结算，长途计费
			if (m_szDealType[5]!='0')
			{
				//先校验13800138000，然后在处理其他号码
				m_callnumber.Get_AfterTollTel(m_szCallNumber);
				if (strcmp(m_szCallNumber,"13800138000")==0)
				{
					if (m_szDealType[5]=='2')
					{
						//短信结算业务不挂起
						//直接往下流，保证输入是区号+13800138000，否则会出现0GD+13800138000的号码
					}
					else
					{
						//其他业务挂起
						pluginAnaResult result=eLackInfo;
						sprintf(m_sAbnReason,"%d",ABN_CALLING_138);
						retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
						//reOutputNum(pps,retValue);
						return;
					}
				}
				else
				{
					//校验手机号前区号和H码的一致性
					if(m_iAbnReason=m_callnumber.CheckGsm(m_szAbnField))
					{
						//告警正常输出
						getParam();
						sendOutputParams(retValue);
						
						//将原始单放在无资料原因字段
						pps.getFieldValue(2,m_szCallNbr);
						DeleteSpace( m_szCallNbr ); 
						pluginAnaResult result=eAbnormal;
						sprintf(m_sAbnReason,"%d",m_iAbnReason);
						retValue.setAnaResult(result, m_sAbnReason, m_szCallNbr);
						return;
					}
				}
			}
			if (m_szDealType[6]=='1')
			{
				//校验手机号码长度的合法性
				if (m_iAbnReason=m_callnumber.CheckGsmLen(m_szAbnField))
				{
					pluginAnaResult result=eLackInfo;
					sprintf(m_sAbnReason,"%d",m_iAbnReason);
					retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
					//reOutputNum(pps,retValue);
					return;
				}
			}
		}//end of 手机号
		else
		{	
			//固话
			if (m_szDealType[3]=='1')
			//校验区号后号码长度大于等于3位
			{
				if(m_iAbnReason=m_callnumber.CheckLen(m_szAbnField))
				{
					pluginAnaResult result=eLackInfo;
					sprintf(m_sAbnReason,"%d",m_iAbnReason);
					retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
					//reOutputNum(pps,retValue);
					return;
				}
			}
			if (m_szDealType[4]=='1')
			//校验省内2～8号码段号码长度的合法性
			{
				if(m_iAbnReason=m_callnumber.CheckGDLen(m_szAbnField))
				{
					pluginAnaResult result=eLackInfo;
					sprintf(m_sAbnReason,"%d",m_iAbnReason);
					retValue.setAnaResult(result, m_sAbnReason, m_szAbnField);
					//reOutputNum(pps,retValue);
					return;
				}
			}
		}//end of 固话
	}

	//插件执行出口
	getParam();
	sendOutputParams(retValue);
	return;
}


int C_CallingRegular::Init(PacketParser &pps)
{
	/*get some of input params which will not change after first coming*/
	/*
	memset( m_szPipeId, 0, sizeof( m_szPipeId ) );   
	pps.getFieldValue( 1,m_szPipeId );
	DeleteSpace(m_szPipeId ); 

	char szProcessId[FIELD_LEN];
	memset( szProcessId, 0, sizeof( szProcessId ) );   
	pps.getFieldValue( 2,szProcessId );
	DeleteSpace( szProcessId ); 
  	m_iProcessId = atoi( szProcessId );
	*/
	
	memset(m_LastSerCatId,0,sizeof(m_LastSerCatId));
	pps.getFieldValue(1,m_LastSerCatId);
	DeleteSpace( m_LastSerCatId ); 

	memset(m_LastSourceId,0,sizeof(m_LastSourceId));
	strcpy(m_LastSourceId, m_szSourceId);
	DeleteSpace( m_LastSourceId ); 

	strcpy(m_szIniPath, getenv("ZHJS_INI"));
	
	//初始化号码类
	//m_callnumber.Init(m_szServiceId, m_LastSourceId, table);
	m_callnumber.Reset(m_szServiceId, m_LastSourceId);

	initFlag=1;

	return 0;
}

int C_CallingRegular::getInputParams(PacketParser & pps)
{
	//获取插件其他可变参数
	//获取插件对应的业务代码，用去查询实际的物理表名
	memset(m_szServCatId,0,sizeof(m_szServCatId));
	pps.getFieldValue(1,m_szServCatId);
	DeleteSpace( m_szServCatId ); 

	/*
	memset(m_szSourceId,0,sizeof(m_szSourceId));
	pps.getFieldValue(4,m_szSourceId);
	DeleteSpace( m_szSourceId ); 
	*/
	
	//初始化号码类
	if (strcmp(m_LastSerCatId,m_szServCatId)!=0  ||  strcmp(m_LastSourceId,m_szSourceId)!=0  )
	{
		strcpy(m_LastSerCatId, m_szServCatId);
		strcpy(m_LastSourceId, m_szSourceId);
		//m_callnumber.Init(m_szServiceId, m_LastSourceId, table);
		m_callnumber.Reset(m_szServiceId, m_LastSourceId);
	}
	
	memset(m_szCallNbr,0,sizeof(m_szCallNbr));
	pps.getFieldValue(2,m_szCallNbr);
	DeleteSpace( m_szCallNbr ); 
	
	memset(m_szNbrType,0,sizeof(m_szNbrType));
	pps.getFieldValue(3,m_szNbrType);
	DeleteSpace( m_szNbrType ); 
	
	memset(m_szCdrBegin,0,sizeof(m_szCdrBegin));
	pps.getFieldValue(4,m_szCdrBegin);
	DeleteSpace( m_szCdrBegin ); 
	
	memset(m_szDealType,0,sizeof(m_szDealType));
	pps.getFieldValue(5,m_szDealType);
	DeleteSpace( m_szDealType ); 

	memset(m_szDefTollcode,0,sizeof(m_szDefTollcode));
	pps.getFieldValue(6,m_szDefTollcode);
	DeleteSpace( m_szDefTollcode ); 
	
	return 0;
}


int  C_CallingRegular::sendOutputParams(ResParser& retValue)
{
	retValue.setFieldValue( 1, m_szBefTollTel, strlen(m_szBefTollTel) );
	retValue.setFieldValue( 2, m_szTollcode, strlen(m_szTollcode) );
	retValue.setFieldValue( 3, m_szAfterTollTel, strlen(m_szAfterTollTel) );
	retValue.setFieldValue( 4, m_szTollTel, strlen(m_szTollTel) );
	retValue.setFieldValue( 5, m_szCallNumber, strlen(m_szCallNumber) );
	return 0;
}

int C_CallingRegular::setParam()
{
	m_callnumber.Set_Value();
	m_callnumber.Set_ServCatId(m_szServCatId);
	m_callnumber.Set_SourceId(m_szSourceId);
	m_callnumber.Set_CallNbr(m_szCallNbr);
	m_callnumber.Set_NbrType(m_szNbrType);
	m_callnumber.Set_CdrBegin(m_szCdrBegin);
	m_callnumber.Set_DefTollcode(m_szDefTollcode);	
	return 0;
}

int C_CallingRegular::getParam()
{
	m_callnumber.Get_BefTollTel(m_szBefTollTel);
	m_callnumber.Get_Tollcode(m_szTollcode);
	m_callnumber.Get_AfterTollTel(m_szAfterTollTel);

	sprintf(m_szTollTel,"%s%s",m_szTollcode,m_szAfterTollTel);
	sprintf(m_szCallNumber,"%s%s",m_szBefTollTel,m_szTollTel);

	return 0;
}


int C_CallingRegular::reOutputNum(PacketParser & pps,ResParser& retValue)
{
	//重新输出回写到话单中，只在无资料出现时使用
	pps.getFieldValue(2,m_szCallNbr);
	DeleteSpace( m_szCallNbr ); 
	retValue.setFieldValue( 5, m_szCallNbr, strlen(m_szCallNbr) );
	return 0;
}


/*************************************************
* 描述    ：获取调试标志和设置初始化标志
* 入口参数：无
* 出口参数：无
* 返回    ：0
**************************************************/
void C_CallingRegular::init(char *szSourceGroupID, char *szServiceID, int index)
{
	memset( m_szSourceGroupId, 0, sizeof( m_szSourceGroupId ) );   
	strcpy( m_szSourceGroupId, szSourceGroupID );
	DeleteSpace(m_szSourceGroupId ); 

	memset( m_szServiceId, 0, sizeof( m_szServiceId ) );   
	strcpy( m_szServiceId, szServiceID );
	DeleteSpace(m_szServiceId ); 

	table = getMemPoint();
	if(table == NULL)
		throw(ERR_SHM_ERROR, "pAccessMem is null", __FILE__, __LINE__);
	//初始化号码类
	m_callnumber.Init(table);
}

/*************************************************
* 描述    ：处理消息函数
* 入口参数：消息类
* 出口参数：无
* 返回    ：无
**************************************************/
void C_CallingRegular::message(MessageParser&  pMessage)
{
	int message=pMessage.getMessageType();
	switch(message)
	{
		case MESSAGE_NEW_FILE:
			memset(m_szSourceId,0,sizeof(m_szSourceId));
			strcpy(m_szSourceId, pMessage.getSourceId());
			DeleteSpace( m_szSourceId ); 
			break;
		default:
			break;
	}
}

/*************************************************
* 描述    ：打印插件版本号
* 入口参数：无
* 出口参数：无
* 返回    ：无
**************************************************/
void C_CallingRegular::printMe()
{
	printf("\t插件名称:StdChkCallingNbr,版本号：3.0.0 \n");
}

/*************************************************
* 描述    ：打印插件名称
* 入口参数：无
* 出口参数：无
* 返回    ：插件名称
**************************************************/
std::string C_CallingRegular::getPluginName()
{
	return "StdChkCallingNbr";
}

std::string C_CallingRegular::getPluginVersion(){
	return "3.0.0";
}


